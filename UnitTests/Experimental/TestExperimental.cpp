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
    test_experimental::waiting_task_return(std::chrono::milliseconds{10});
  }

  void test_experimental::waiting_task(const std::chrono::milliseconds millisecs)
  {
    using namespace concurrency;

    {
      auto threadPoolFn{[this, millisecs](){
          waiting_task<experimental::thread_pool<void>>(2u, millisecs, 2u);
        }
      };

      auto threadPoolMonoFn{[this, millisecs](){
          waiting_task<experimental::thread_pool<void, false>>(2u, millisecs, 2u);
        }
      };

      auto nullThreadFn{[this, millisecs]() { waiting_task<serial<void>>(2u, millisecs); }};

      auto asyncFn{[this, millisecs]() { waiting_task<asynchronous<void>>(2u, millisecs); }};

      check_relative_performance(threadPoolFn, nullThreadFn, 1.9, true, LINE("Two Waiting tasks; pool_2/null"));
      check_relative_performance(threadPoolMonoFn, nullThreadFn, 1.9, true, LINE("Two Waiting tasks; pool_2M/null"));
      check_relative_performance(asyncFn, nullThreadFn, 1.9, true, LINE("Two Waiting tasks; async/null"));
    }

    {
      auto threadPoolFn{[this, millisecs](){
          waiting_task<experimental::thread_pool<void>>(4u, millisecs, 2u);
        }
      };

      auto threadPoolMonoFn{[this, millisecs](){
          waiting_task<experimental::thread_pool<void, false>>(4u, millisecs, 2u);
        }
      };

      auto nullThreadFn{[this, millisecs]() { waiting_task<serial<void>>(4u, millisecs); }};

      check_relative_performance(threadPoolFn, nullThreadFn, 1.9, true, LINE("Four Waiting tasks; pool_2/null"));
      check_relative_performance(threadPoolMonoFn, nullThreadFn, 1.9, true, LINE("Four Waiting tasks; pool_2M/null"));
    }

    {
      auto threadPoolFn{[this, millisecs](){
          waiting_task<experimental::thread_pool<void>>(4u, millisecs, 4u);
        }
      };

       auto threadPoolMonoFn{[this, millisecs](){
           waiting_task<experimental::thread_pool<void, false>>(4u, millisecs, 4u);
        }
      };

      auto nullThreadFn{[this, millisecs]() { waiting_task<serial<void>>(4u, millisecs); }};

      check_relative_performance(threadPoolFn, nullThreadFn, 3.9, true, LINE("Four Waiting tasks; pool_4/null"));
      check_relative_performance(threadPoolMonoFn, nullThreadFn, 3.9, true, LINE("Four Waiting tasks; pool_4M/null"));
    }
  }

  void test_experimental::waiting_task_return(const std::chrono::milliseconds millisecs)
  {
    using namespace concurrency;

    {
      auto threadPoolFn{[this, millisecs](){
          waiting_task_return<experimental::thread_pool<int>>(2u, millisecs, 2u);
        }
      };

      auto threadPoolMonoFn{[this, millisecs](){
          waiting_task_return<experimental::thread_pool<int, false>>(2u, millisecs, 2u);
        }
      };

      auto nullThreadFn{[this, millisecs]() { waiting_task_return<serial<int>>(2u, millisecs); }};

      auto asyncFn{[this, millisecs]() { waiting_task_return<asynchronous<int>>(2u, millisecs); }};

      check_relative_performance(threadPoolFn, nullThreadFn, 1.9, true, LINE("Two Waiting tasks; pool_2/null"));
      check_relative_performance(threadPoolMonoFn, nullThreadFn, 1.9, true, LINE("Two Waiting tasks; pool_2M/null"));
      check_relative_performance(asyncFn, nullThreadFn, 1.9, true, LINE("Two Waiting tasks; async/null"));
    }
  }

  template<class ThreadModel, class... Args>
  void test_experimental::waiting_task(const std::size_t nTasks, const std::chrono::milliseconds millisecs, Args&&... args)
  {
    ThreadModel model{std::forward<Args>(args)...};
    
    for(int i{}; i < nTasks; ++i)
    {
      model.push(Wait{millisecs});
    }

    model.get();
  }

  template<class ThreadModel, class... Args>
  std::vector<int> test_experimental::waiting_task_return(const std::size_t nTasks, const std::chrono::milliseconds millisecs, Args&&... args)
  {
    ThreadModel model{std::forward<Args>(args)...};
    
    for(int i{}; i < nTasks; ++i)
    {
      model.push([millisecs, i]() {
          Wait{millisecs};
          return i;
        }
      );
    }

    return model.get();
  }
}
