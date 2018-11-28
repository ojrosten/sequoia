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
      auto threadPoolFn_2{[this, millisecs](){
          waiting_task<experimental::thread_pool<void>>(2u, millisecs, 2u);
        }
      };

      auto nullThreadFn{[this, millisecs]() { waiting_task<serial<void>>(2u, millisecs); }};

      check_relative_performance(threadPoolFn_2, nullThreadFn, 1.9, true, "Two Waiting tasks; pool_2/null");
    }

    {
      auto threadPoolFn_2{[this, millisecs](){
          waiting_task<experimental::thread_pool<void>>(4u, millisecs, 2u);
        }
      };

      auto nullThreadFn{[this, millisecs]() { waiting_task<serial<void>>(4u, millisecs); }};

      check_relative_performance(threadPoolFn_2, nullThreadFn, 1.9, true, "Four Waiting tasks; pool_2/null");
    }

    {
      auto threadPoolFn_4{[this, millisecs](){
          waiting_task<experimental::thread_pool<void>>(4u, millisecs, 4u);
        }
      };

      auto nullThreadFn{[this, millisecs]() { waiting_task<serial<void>>(4u, millisecs); }};

      check_relative_performance(threadPoolFn_4, nullThreadFn, 3.75, true, "Four Waiting tasks; pool_4/null");
    }
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
