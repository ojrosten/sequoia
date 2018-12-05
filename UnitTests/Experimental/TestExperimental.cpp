#include "TestExperimental.hpp"

#include "ThreadingModels.hpp"

namespace sequoia::unit_testing
{
  
  void test_experimental::run_tests()
  {
    using namespace experimental;

    using q_t = task_queue<int>;
    using task_t = q_t::task_t;

    q_t q{};

    q.push(task_t{[](){ return 1;}}, std::try_to_lock);
    auto t{q.pop(std::try_to_lock)};

    q.push(task_t{[](){ return 1;}});
    t = q.pop();

    q.finish();

    test_experimental::waiting_task(std::chrono::milliseconds{10});
  }

  void test_experimental::waiting_task(const std::chrono::milliseconds millisecs)
  {
    using namespace concurrency;

    {
      auto threadPoolFn{[this, millisecs](){
          waiting_task<experimental::thread_pool<void>>(2u, millisecs, 2u);
        }
      };

      auto threadPoolMonoChannelFn{[this, millisecs](){
          waiting_task<experimental::thread_pool<void, false>>(2u, millisecs, 2u);
        }
      };

      auto nullThreadFn{[this, millisecs]() { waiting_task<serial<void>>(2u, millisecs); }};

      check_relative_performance(threadPoolFn, nullThreadFn, 1.9, true, LINE("Two Waiting tasks; pool_2/null"));

      check_relative_performance(threadPoolMonoChannelFn, nullThreadFn, 1.9, true, LINE("Two Waiting tasks; pool_2M/null"));
    }

    {
      auto threadPoolFn{[this, millisecs](){
          waiting_task<experimental::thread_pool<void>>(4u, millisecs, 2u);
        }
      };

      auto nullThreadFn{[this, millisecs]() { waiting_task<serial<void>>(4u, millisecs); }};

      check_relative_performance(threadPoolFn, nullThreadFn, 1.9, true, "Four Waiting tasks; pool_2/null");
    }

    {
      auto threadPoolFn{[this, millisecs](){
          waiting_task<experimental::thread_pool<void>>(4u, millisecs, 4u);
        }
      };

      auto nullThreadFn{[this, millisecs]() { waiting_task<serial<void>>(4u, millisecs); }};

      check_relative_performance(threadPoolFn, nullThreadFn, 3.75, true, "Four Waiting tasks; pool_4/null");
    }

    {
      auto threadPoolFn{[this, millisecs](){
          waiting_task<experimental::thread_pool<void>>(12u, millisecs, 3u);
        }
      };

      auto nullThreadFn{[this, millisecs]() { waiting_task<serial<void>>(12u, millisecs); }};

      check_relative_performance(threadPoolFn, nullThreadFn, 2.9, true, "12 Waiting tasks; pool_3/null");
    }

    /*
    {
      auto threadPoolFn{[this, millisecs](){
          waiting_task<experimental::thread_pool<void>>(1000u, millisecs, 4u);
        }
      };

      auto nullThreadFn{[this, millisecs]() { waiting_task<serial<void>>(1000u, millisecs); }};

      check_relative_performance(threadPoolFn, nullThreadFn, 3.9, true, "1000 Waiting tasks; pool_3/null");
    }



    {
      auto threadPoolFn{[this, millisecs](){
          waiting_task<experimental::thread_pool<void, false>>(1000u, millisecs, 4u);
        }
      };

      auto nullThreadFn{[this, millisecs]() { waiting_task<serial<void>>(1000u, millisecs); }};

      check_relative_performance(threadPoolFn, nullThreadFn, 3.9, true, "1000 Waiting tasks; pool_3/null Single");
    }
    */
  }

  template<class ThreadModel, class... Args>
  void test_experimental::waiting_task(const std::size_t nTasks, const std::chrono::milliseconds millisecs, Args&&... args)
  {
    ThreadModel model{std::forward<Args>(args)...};
    
    for(std::size_t i{}; i < nTasks; ++i)
    {
      model.push(Wait{millisecs});
    }

    model.get();
  }
}
