#pragma once

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>

#include <iostream>

#include "Utilities.hpp"

namespace sequoia
{
  namespace concurrency
  {
    //===================================Null Threading Model===================================//

    template<class R=void>  class serial
    {
    public:
      using return_type = R;

      serial() {}

      template<class Fn, class... Args> void push(Fn&& fn, Args... args)
      {
        static_assert(std::is_same<R, std::result_of_t<Fn(Args...)>>::value, "Function return type inconsistent!");
        m_Results.push_back(std::forward<Fn>(fn)(args...));
      }

      auto get() const { return m_Results; }
    private:
      std::vector<R> m_Results;
    };

    template<> class serial<void>
    {
    public:
      using return_type = void;

      serial() {}

      template<class Fn, class... Args> void push(Fn&& fn, Args... args)
      {
        static_assert(std::is_same<void, typename std::result_of_t<Fn(Args...)>>::value, "Fn must return void!");
        fn(args...);
      }

      void get() {}
    };

    //==================================Asynchronous Processing==================================// 

    template<class R>
    class asynchronous
    {
    public:
      using return_type = R;
      asynchronous() {}

      template<class Fn, class... Args>
      void push(Fn&& fn, Args... args)
      {
        m_Futures.emplace_back(std::async(std::launch::async | std::launch::deferred, std::forward<Fn>(fn), args...));
      }

      auto get()
      {
        utilities::ReturnValues<R> values;
        for(auto&& fut : m_Futures)
        {
          values.emplace_back(fut);
        }

        return values.get();
      }
    private:
      std::vector<std::future<R>> m_Futures;
    };

    //=======================================Thread Pool========================================//

    template<class R, template<class, class> class Q>
    class thread_pool
    {
    public:
      using return_type = R;

      thread_pool(const std::size_t numThreads)
      {
        if(!numThreads)
          throw std::out_of_range("Cannot initialize thread pool with zero threads!");

        try
        {
          for(std::size_t i = 0; i < numThreads; ++i)
          {
            auto loop = [this]()
            {
              for(;;)
              {
                Task task;
                {
                  std::unique_lock<std::mutex> lock(m_Mutex);
                  if(m_Queue.empty())
                    m_CV.wait(lock, [this](){ return (!m_Queue.empty() || m_Execution != CONTINUE); });

                  if(m_Execution == TERMINATE) break;

                  if(!m_Queue.empty())
                  {
                    task = std::move(m_Queue.front());
                    m_Queue.pop();
                  }
                }

                if(m_Execution != TERMINATE)
                {
                  if(task.valid())
                  {
                    task();
                  }
                  else if(m_Execution == FINISH)
                  {
                    break;
                  }
                }
                else
                {
                  break;
                }
              }
            };

            m_Threads.emplace_back(std::thread(loop));
          }
        }
        catch(std::system_error& e)
        {
          if(m_Threads.empty())
            throw e;

          // else do nothing
        }
      }

      thread_pool(const thread_pool&) = delete;

      template<class Fn, class... Args>
      void push(Fn&& fn, Args... args)
      {
        static_assert(std::is_same<R, std::decay_t<std::result_of_t<Fn(Args...)>>>::value, "Function return type inconsistent!");

        std::unique_lock<std::mutex> lck(m_Mutex);
        if(m_Execution == CONTINUE)
        {
          try
          {
            {
              Task task(std::bind(std::forward<Fn>(fn), args...));
              m_Futures.emplace_back(task.get_future());
              m_Queue.push(std::move(task));
            }
            m_CV.notify_one();
          }
          catch(const std::exception& e)
          {
            std::unique_lock<std::mutex> lck(m_Mutex);
            m_Execution = TERMINATE;
            join();
            throw e;
          }
        }
      }

      void join()
      {
        {
          std::unique_lock<std::mutex> lck(m_Mutex);
          if(m_Execution == CONTINUE) m_Execution = FINISH;
        }

        m_CV.notify_all();
        for(auto&& worker : m_Threads)
        {
          worker.join();
        }
      }

      auto get()
      {
        join();
        utilities::ReturnValues<R> values;
        for(auto&& fut : m_Futures)
        {
          values.emplace_back(fut);
        }

        return values.get();
      }
    private:
      using Task = std::packaged_task<R()>;

      std::queue<Task, Q<Task, std::allocator<Task>>> m_Queue;
      std::vector<std::thread> m_Threads;
      std::mutex m_Mutex;
      std::condition_variable m_CV;

      std::vector<std::future<R>> m_Futures;

      std::string m_ExceptionMessage;

      enum Execution : char { CONTINUE, FINISH, TERMINATE };
      Execution m_Execution{CONTINUE};
    };
  }
}
