#pragma once

#include "UnitTestUtils.hpp"

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <chrono>

namespace sequoia
{
  namespace experimental
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

      bool push(task_t&& task)
      {
        {
          std::unique_lock<std::mutex> lock{m_Mutex};
          m_Q.push(std::move(task));
        }
        m_CV.notify_one();

        return true;
      }

      bool push(task_t&& task, std::try_to_lock_t t)
      {
        std::unique_lock<std::mutex> lock{m_Mutex, t};
        if(!lock) return false;

        m_Q.push(std::move(task));
        m_CV.notify_one();
      
        return true;
      }

      task_t pop(std::try_to_lock_t t)
      {
        std::unique_lock<std::mutex> lock{m_Mutex, t};
        if(!lock || m_Q.empty()) return Task{};

        return get();
      }

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
      class future_store
      {
      public:
        void stash(std::future<R>&& f)
        {
          m_Futures.push_back(f);
        }

        std::vector<std::future<R>> acquire()
        {
          return std::move(m_Futures);
        }
      private:
        std::vector<std::future<R>> m_Futures;
      };

      template<>
      class future_store<void>
      {        
      };
    }

    template<class R, bool Speculative=true>
    class thread_pool : private impl::future_store<R>
    {
    public:
      thread_pool(const std::size_t numThreads, const std::size_t pushCycles = 46)
        : m_Queues(numThreads)
        , m_PushCycles{pushCycles}
      {
        m_Threads.reserve(numThreads);

        for(std::size_t q{}; q<numThreads; ++q)
        {
          auto loop{[this, q]() {
              while(true)
              {
                task_t task{};

                if constexpr(Speculative)
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
                  task = m_Queues[q].pop();
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

        task_t task{[=](){ fn(args...); }};
        if constexpr(!std::is_void_v<R>) this->stash(task->get_future());
        
        const auto qIndex{m_QueueIndex++};        
        const auto N{m_Threads.size()};
          
        if constexpr(Speculative)
        {     
          for(std::size_t i{}; i < N * m_PushCycles; ++i)
          {
            if(m_Queues[(qIndex + i) % N].push(std::move(task), std::try_to_lock)) return;          
          }
        }

        m_Queues[qIndex % N].push(std::move(task));
      }

      void join()
      {
        join_all();
        joined = true;        
      }

      auto get()
      {
        join();
        if constexpr(!std::is_void_v<R>) return this->acquire();
      }
    private:
      using Q_t = task_queue<R>;
      using task_t = typename Q_t::task_t;
      std::vector<Q_t> m_Queues;
      std::vector<std::thread> m_Threads;
      bool joined{};

      std::size_t m_QueueIndex{}, m_PushCycles{46};

      void join_all()
      {
        for(auto& q : m_Queues) q.finish();
        for(auto& t : m_Threads) t.join();
      }
    };
  }
  
  namespace unit_testing
  {    
    class test_experimental : public unit_test
    {      
    public:
      using unit_test::unit_test;
    private:
      void run_tests();

      void waiting_task(const std::chrono::milliseconds millisecs);

      template<class ThreadModel, class... Args>
      void waiting_task(const std::size_t nTasks, const std::chrono::milliseconds millisecs, Args&&... args);
    };

    class Wait
    {
      std::chrono::milliseconds m_Wait;
    public:      
      Wait(const std::chrono::milliseconds millisecs) : m_Wait{millisecs} {}
      
      void operator()() const
      {
        std::this_thread::sleep_for(m_Wait);
      }
    };
  }
}
