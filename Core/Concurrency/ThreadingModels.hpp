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

    //==================================Asynchronous Processing==================================// 

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
        if constexpr(std::is_same_v<R, void>)
        {
          std::for_each(m_Futures.begin(), m_Futures.end(), [](std::future<R>& fut) { fut.get(); });
        }
        else
        {
          std::vector<R> values;
          values.reserve(m_Futures.size());
          std::transform(m_Futures.begin(), m_Futures.end(), std::back_inserter(values), [](std::future<R>& fut) { return fut.get(); });
          return values;
        }
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
                    m_CV.wait(lock, [this](){ return (!m_Queue.empty() || m_Status != status::running); });

                  if(m_Status == status::terminated) break;

                  if(!m_Queue.empty())
                  {
                    task = std::move(m_Queue.front());
                    m_Queue.pop();
                  }
                }

                if(m_Status != status::terminated)
                {
                  if(task.valid())
                  {
                    task();
                  }
                  else if(m_Status == status::finished)
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
        catch(std::system_error&)
        {
          if(m_Threads.empty())
            throw;

          // else do nothing
        }
      }

      thread_pool(const thread_pool&) = delete;

      template<class Fn, class... Args>
      void push(Fn&& fn, Args... args)
      {
        static_assert(std::is_same<R, std::decay_t<std::result_of_t<Fn(Args...)>>>::value, "Function return type inconsistent!");

        std::unique_lock<std::mutex> lck(m_Mutex);
        if(m_Status == status::running)
        {
          try
          {
            {
              //            Task task(std::bind(std::forward<Fn>(fn), args...));
              Task task{[=]() { return fn(args...);}};

              // This bit must be linearly synchronized
              m_Futures.emplace_back(task.get_future());

              // This bit can be distributed
              m_Queue.push(std::move(task));
            }
            m_CV.notify_one();
          }
          catch(...)
          {
            std::unique_lock<std::mutex> lck(m_Mutex);
            m_Status = status::terminated;
            join();
            throw;
          }
        }
      }

      void join()
      {
        {
          std::unique_lock<std::mutex> lck(m_Mutex);
          if(m_Status == status::running) m_Status = status::finished;
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
      std::vector<std::future<R>> m_Futures;

      std::vector<std::thread> m_Threads;
      std::mutex m_Mutex;
      std::condition_variable m_CV;

      enum class status { running, finished, terminated };
      status m_Status{status::running};
    };
  }
}
