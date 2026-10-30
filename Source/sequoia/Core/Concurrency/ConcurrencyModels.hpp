////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Classes with a queue-like behaviour to which tasks can be pushed and results recovered, possibly
           following concurrent execution

           The three concurrency models sequoia::concurrency::serial, sequoia::concurrency::asynchronous
           and sequoia::concurrency::thread_pool have a common interface for pushing tasks via the `push`
           method.
 */

#include "sequoia/Core/Meta/TypeTraits.hpp"

#include <atomic>
#include <concepts>
#include <cstddef>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <stdexcept>
#include <vector>

namespace sequoia::concurrency
{

  /** \brief a task queue designed for use by multiple threads.

      This class supports both aggressive pushing and popping and also speculative versions which
      do not necessarily acquire the underlying mutex and may therefore fail.

      \throws std::logic_error from a `push` which takes the mutex of a finished
               queue.
   */
  template<class R, class Task=std::packaged_task<R()>, class Q=std::queue<Task>>
  class task_queue
  {
  public:
    using task_type = Task;

    task_queue() = default;
    task_queue(const task_queue&) = delete;
    task_queue(task_queue&&)      = delete;

    ~task_queue() = default;

    task_queue& operator=(const task_queue&) = delete;
    task_queue& operator=(task_queue&&)      = delete;

    void finish()
    {
      {
        std::scoped_lock<std::mutex> lock{m_Mutex};
        m_Finished = true;
      }

      m_CV.notify_all();
    }

    void push(task_type&& task)
    {
      {
        std::scoped_lock<std::mutex> lock{m_Mutex};
        throw_if_finished();
        m_Q.push(std::move(task));
      }

      m_CV.notify_one();
    }

    [[nodiscard]]
    bool push(task_type&& task, std::try_to_lock_t t)
    {
      if(std::unique_lock<std::mutex> lock{m_Mutex, t}; lock)
      {
        throw_if_finished();
        m_Q.push(std::move(task));
      }
      else
      {
        return false;
      }

      m_CV.notify_one();

      return true;
    }

    [[nodiscard]]
    task_type pop(std::try_to_lock_t t)
    {
      if(std::unique_lock<std::mutex> lock{m_Mutex, t}; lock)
      {
        return get();
      }

      return Task{};
    }

    [[nodiscard]]
    task_type pop()
    {
      std::unique_lock<std::mutex> lock{m_Mutex};
      while(m_Q.empty() && !m_Finished) m_CV.wait(lock);

      return get();
    }
  private:
    Q m_Q;
    std::mutex m_Mutex;
    std::condition_variable m_CV;
    bool m_Finished{};

    // Call only with the mutex held, so that `finish` cannot come between
    // this check and the push
    void throw_if_finished() const
    {
      if(m_Finished)
        throw std::logic_error{"A task cannot be pushed to a finished task_queue"};
    }

    [[nodiscard]]
    Task get()
    {
      if(!m_Q.empty())
      {
        Task task{std::move(m_Q.front())};
        m_Q.pop();
        return task;
      }

      return {};
    }
  };

  /** \brief The task queues of a `thread_pool`: one queue which every thread
             shares, or a queue for each thread.
   */
  enum class task_queue_arrangement : bool { shared, one_per_thread };

  namespace impl
  {
    template<class R, task_queue_arrangement Arrangement> struct queue_details
    {
      using Q_type     = task_queue<R>;
      using task_type  = Q_type::task_type;
      using queue_type = std::vector<Q_type>;

      std::size_t push_cycles{};
    };

    template<class R> struct queue_details<R, task_queue_arrangement::shared>
    {
      using Q_type     = task_queue<R>;
      using task_type  = Q_type::task_type;
      using queue_type = Q_type;
    };
  }

  //===================================Serial Execution Model===================================//

  /** \brief Tasks may be `push`ed, upon which they are immediately invoked */

  template<class R>  class serial
  {
  public:
    using return_type = R;

    template<class Fn, class... Args>
      requires std::invocable<Fn, Args...> && std::is_convertible_v<R, std::invoke_result_t<Fn, Args...>>
    [[nodiscard]]
    constexpr std::invoke_result_t<Fn, Args...> push(Fn&& fn, Args&&... args)
    {
      return std::forward<Fn>(fn)(std::forward<Args>(args)...);
    }
  };

  //==================================Asynchronous Execution==================================//

  /** \brief Tasks may be `push`ed, upon which they are fed to std::async */

  template<class R>
  class asynchronous
  {
  public:
    using return_type = R;

    asynchronous() = default;
    asynchronous(const asynchronous&)     = delete;
    asynchronous(asynchronous&&) noexcept = default;
    ~asynchronous() = default;

    asynchronous& operator=(const asynchronous&)     = delete;
    asynchronous& operator=(asynchronous&&) noexcept = default;

    template<class Fn, class... Args>
    [[nodiscard]]
    std::future<R> push(Fn&& fn, Args&&... args)
    {
      return std::async(std::launch::async | std::launch::deferred, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }
  };

  //=======================================Thread Pool========================================//

  /** \brief A pool of threads that runs the tasks pushed to it.

      Tasks reach the threads through either one queue, which every thread
      shares, or a queue for each thread. With a queue for each thread, a
      thread may take a task from another thread's queue (task stealing).

      Several threads may call `push` concurrently, with one another and with
      `join`. Every task that `push` accepts runs before `join`, or the
      destructor, returns.

      \throws std::invalid_argument if constructed with zero threads.
      \throws std::logic_error from `push` after `join` has returned, and
               possibly from a `push` concurrent with `join`.
   */

  template<class R, task_queue_arrangement Arrangement=task_queue_arrangement::one_per_thread>
  class thread_pool : private impl::queue_details<R, Arrangement>
  {
  public:
    using return_type = R;

    explicit thread_pool(const std::size_t numThreads)
      requires(Arrangement == task_queue_arrangement::shared)
    {
      make_pool(numThreads);
    }

    thread_pool(const std::size_t numThreads, const std::size_t pushCycles = 46)
      requires(Arrangement == task_queue_arrangement::one_per_thread)
      : impl::queue_details<R, Arrangement>{pushCycles}
      , m_Queues(numThreads)
    {
      make_pool(numThreads);
    }

    thread_pool(const thread_pool&)= delete;
    thread_pool(thread_pool&&)     = delete;

    ~thread_pool()
    {
      if(!joined) join_all();
    }

    thread_pool& operator=(const thread_pool&) = delete;
    thread_pool& operator=(thread_pool&&)      = delete;

    template<std::invocable Fn>
      requires std::is_convertible_v<std::invoke_result_t<Fn>, R> && std::move_constructible<Fn>
    [[nodiscard]]
    std::future<R> push(Fn fn)
    {
      task_type task{std::move(fn)};
      std::future<R> f{task.get_future()};

      if constexpr(Arrangement == task_queue_arrangement::one_per_thread)
      {
        const auto qIndex{m_QueueIndex.fetch_add(1, std::memory_order_relaxed)};
        const auto N{m_Threads.size()};

        if(qIndex >= N)
        {
          for(std::size_t i{}; i < N * this->push_cycles; ++i)
          {
            if(m_Queues[(qIndex + i) % N].push(std::move(task), std::try_to_lock))
              return f;
          }
        }

        m_Queues[qIndex % N].push(std::move(task));
      }
      else
      {
        m_Queues.push(std::move(task));
      }

      return f;
    }

    template<class Fn, class... Args>
      requires    std::invocable<Fn, Args...>
               && std::is_convertible_v<std::invoke_result_t<Fn, Args...>, R>
               && std::move_constructible<Fn>
               && (std::move_constructible<Args> && ...)
    [[nodiscard]]
    std::future<R> push(Fn fn, Args... args)
    {
      return push([fn = std::move(fn), ...args = std::move(args)](){ return fn(args...); });
    }

    void join()
    {
      join_all();
      joined = true;
    }
  private:
    using task_type   = impl::queue_details<R, Arrangement>::task_type;
    using Queues_type = impl::queue_details<R, Arrangement>::queue_type;

    Queues_type m_Queues;
    std::vector<std::thread> m_Threads;
    bool joined{};

    std::atomic<std::size_t> m_QueueIndex{};

    void make_pool(const std::size_t numThreads)
    {
      if(!numThreads)
        throw std::invalid_argument{"A thread pool requires at least one thread"};

      m_Threads.reserve(numThreads);

      for(std::size_t q{}; q<numThreads; ++q)
      {
        auto loop{[=,this]() {
            if constexpr(Arrangement == task_queue_arrangement::one_per_thread)
            {
              task_type task{m_Queues[q].pop()};
              if(task.valid())
                task();
              else
                return;
            }

            while(true)
            {
              task_type task{};

              if constexpr(Arrangement == task_queue_arrangement::one_per_thread)
              {
                const auto N{m_Threads.size()};
                for(std::size_t i{}; i<N; ++i)
                {
                  task = m_Queues[(q+i) % N].pop(std::try_to_lock);
                  if(task.valid()) break;
                }

                if(!task.valid())
                  task = m_Queues[q].pop();
              }
              else
              {
                task = m_Queues.pop();
              }

              if(task.valid())
                task();
              else
                break;

            }
          }
        };

        m_Threads.emplace_back(loop);
      }
    }

    void join_all()
    {
      if constexpr(Arrangement == task_queue_arrangement::one_per_thread)
        for(auto& q : m_Queues) q.finish();
      else
        m_Queues.finish();

      for(auto& t : m_Threads) t.join();
    }
  };

  /** \brief A `thread_pool` whose threads share one queue. */
  template<class R>
  using thread_pool_with_shared_queue = thread_pool<R, task_queue_arrangement::shared>;
}
