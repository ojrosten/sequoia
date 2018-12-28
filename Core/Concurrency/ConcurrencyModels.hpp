////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file ConcurrencyModels.hpp
    \brief Classes with a queue-like interface to which tasks can be pushed and results popped, possibly
           following concurrent execution

 */

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>

#include <iostream>

#include "Utilities.hpp"

namespace sequoia::concurrency
{
  template<class R, class Task=std::packaged_task<R()>, class Q=std::queue<Task>>
  class task_queue
  {
  public:
    using task_t = Task;
    
    task_queue() = default;
    task_queue(const task_queue&) = delete;
    task_queue(task_queue&&)      = delete;

    ~task_queue() = default;
    
    task_queue& operator=(const task_queue&) = delete;
    task_queue& operator=(task_queue&&)      = delete;

    void finish()
    {
      {
        std::unique_lock<std::mutex> lock{m_Mutex};
        m_Finished = true;
      }

      m_CV.notify_all();
    }

    void push(task_t&& task)
    {
      {
        std::unique_lock<std::mutex> lock{m_Mutex};
        m_Q.push(std::move(task));
      }
      m_CV.notify_one();
    }

    [[nodiscard]]
    bool push(task_t&& task, std::try_to_lock_t t)
    {
      std::unique_lock<std::mutex> lock{m_Mutex, t};
      if(!lock) return false;

      m_Q.push(std::move(task));
      m_CV.notify_one();
      
      return true;
    }

    [[nodiscard]]
    task_t pop(std::try_to_lock_t t)
    {
      std::unique_lock<std::mutex> lock{m_Mutex, t};
      if(!lock || m_Q.empty()) return Task{};

      return get();
    }

    [[nodiscard]]
    task_t pop()
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

    Task get()
    {
      Task task{};
      if(!m_Q.empty())
        {        
          task = std::move(m_Q.front());
          m_Q.pop();
        }
        
      return std::move(task);
    }
  };

  namespace impl
  {
    template<class R>
    auto get_results(std::vector<std::future<R>>& futures)
    {
      if constexpr(std::is_same_v<R, void>)
      {
        std::for_each(futures.begin(), futures.end(), [](std::future<R>& fut) { fut.get(); });
      }
      else
      {
        std::vector<R> values;
        values.reserve(futures.size());
        std::transform(futures.begin(), futures.end(), std::back_inserter(values), [](std::future<R>& fut) { return fut.get(); });
        return values;
      }
    }

    template<class R, bool MultiChannel> struct queue_details
    {
      using Q_t = task_queue<R>;
      using task_t = typename Q_t::task_t;
      using queue_type = std::vector<Q_t>;

      std::size_t m_PushCycles;
    };

    template<class R> struct queue_details<R, false>
    {
      using Q_t = task_queue<R>;
      using task_t = typename Q_t::task_t;
      using queue_type = Q_t;
    };
  }
  
  //===================================Serial Execution Model===================================//

  template<class R=void>  class serial
  {
  public:
    using return_type = R;

    template<class Fn, class... Args> void push(Fn&& fn, Args&&... args)
    {
      static_assert(std::is_convertible_v<R, std::invoke_result_t<std::decay_t<Fn>, std::decay_t<Args>...>>,
                    "Function return type inconsistent!");
      m_Results.push_back(fn(std::forward<Args>(args)...));
    }

    const auto& get() const noexcept{ return m_Results; }
  private:
    std::vector<R> m_Results;
  };

  template<> class serial<void>
  {
  public:
    using return_type = void;

    template<class Fn, class... Args> constexpr void push(Fn&& fn, Args&&... args)
    {
      fn(std::forward<Args>(args)...);
    }

    constexpr void get() const noexcept {}
  };

  //==================================Asynchronous Execution==================================// 

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
    void push(Fn&& fn, Args&&... args)
    {
      m_Futures.emplace_back(std::async(std::launch::async | std::launch::deferred, std::forward<Fn>(fn), std::forward<Args>(args)...));
    }

    auto get()
    {
      return impl::get_results(m_Futures);
    }
  private:
    std::vector<std::future<R>> m_Futures;
  };

  //=======================================Thread Pool========================================//

  template<class R, bool MultiChannel=true>
  class thread_pool : private impl::queue_details<R, MultiChannel>
  {
  public:
    using return_type = R;
      
    template<bool B=MultiChannel, class=std::enable_if_t<!B>>
    explicit thread_pool(const std::size_t numThreads)        
    {
      make_pool(numThreads);
    }

    template<bool B=MultiChannel, class=std::enable_if_t<B>>
    thread_pool(const std::size_t numThreads, const std::size_t pushCycles = 46)
      : impl::queue_details<R, MultiChannel>{pushCycles}
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

    template<class Fn, class... Args>
    void push(Fn&& fn, Args&&... args)
    {
      static_assert(std::is_convertible_v<R, std::invoke_result_t<std::decay_t<Fn>, std::decay_t<Args>...>>,
                    "Function return type inconsistent!");

      task_t task{[=](){ return fn(args...); }};
      m_Futures.push_back(task.get_future());

                  
      if constexpr(MultiChannel)
      {
        const auto qIndex{m_QueueIndex++};        
        const auto N{m_Threads.size()};

        if(qIndex >= N)
        {
          for(std::size_t i{}; i < N * this->m_PushCycles; ++i)
          {
            if(m_Queues[(qIndex + i) % N].push(std::move(task), std::try_to_lock))
              return;
          }
        }

        m_Queues[qIndex % N].push(std::move(task));
      }
      else
      {
        m_Queues.push(std::move(task));
      }
    }

    void join()
    {
      join_all();
      joined = true;        
    }

    auto get()
    {
      join();
      return impl::get_results(m_Futures);
    }
  private:
    using task_t   = typename impl::queue_details<R, MultiChannel>::task_t;
    using Queues_t = typename impl::queue_details<R, MultiChannel>::queue_type;
      
    Queues_t m_Queues;
    std::vector<std::thread> m_Threads;
    std::vector<std::future<R>> m_Futures;
    bool joined{};

    std::size_t m_QueueIndex{};

    void make_pool(const std::size_t numThreads)
    {
      m_Threads.reserve(numThreads);

      for(std::size_t q{}; q<numThreads; ++q)
      {
        auto loop{[=]() {
            if constexpr(MultiChannel)
            {
              task_t task{m_Queues[q].pop()};
              if(task.valid())
                task();
              else
                return;
            }
              
            while(true)
            {
              task_t task{};
                
              if constexpr(MultiChannel)
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
      if constexpr(MultiChannel)
        for(auto& q : m_Queues) q.finish();
      else
        m_Queues.finish();
               
      for(auto& t : m_Threads) t.join();
    }
  };
}
