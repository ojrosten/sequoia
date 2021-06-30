////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "ConcurrencyModelsPerformanceTest.hpp"
#include "sequoia/Core/Concurrency/ConcurrencyModels.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view threading_models_performance_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void threading_models_performance_test::run_tests()
  {
    test_waiting_task(std::chrono::milliseconds{15});
    test_waiting_task_return(std::chrono::milliseconds{15});
  }

  void threading_models_performance_test::test_waiting_task(const std::chrono::milliseconds millisecs)
  {
    using namespace concurrency;

    {
      auto threadPoolFn{[this, millisecs](){
          waiting_task<thread_pool<void>>(2u, millisecs, 2u);
        }
      };

      auto threadPoolMonoFn{[this, millisecs](){
          waiting_task<thread_pool<void, false>>(2u, millisecs, 2u);
        }
      };

      auto nullThreadFn{[this, millisecs]() { waiting_task<serial<void>>(2u, millisecs); }};

      auto asyncFn{[this, millisecs]() { waiting_task<asynchronous<void>>(2u, millisecs); }};

      check_relative_performance(LINE("Two Waiting tasks; pool_2/null"), threadPoolFn, nullThreadFn, 1.9, 2.1);
      check_relative_performance(LINE("Two Waiting tasks; pool_2M/null"), threadPoolMonoFn, nullThreadFn, 1.9, 2.1);
      check_relative_performance(LINE("Two Waiting tasks; async/null"), asyncFn, nullThreadFn, 1.9, 2.1);
    }

    {
      auto threadPoolFn{[this, millisecs](){
          waiting_task<thread_pool<void>>(4u, millisecs, 2u);
        }
      };

      auto threadPoolMonoFn{[this, millisecs](){
          waiting_task<thread_pool<void, false>>(4u, millisecs, 2u);
        }
      };

      auto nullThreadFn{[this, millisecs]() { waiting_task<serial<void>>(4u, millisecs); }};

      check_relative_performance(LINE("Four Waiting tasks; pool_2/null"), threadPoolFn, nullThreadFn, 1.9, 2.1);
      check_relative_performance(LINE("Four Waiting tasks; pool_2M/null"), threadPoolMonoFn, nullThreadFn, 1.9, 2.1);
    }

    {
      auto threadPoolFn{[this, millisecs](){
          waiting_task<thread_pool<void>>(4u, millisecs, 4u);
        }
      };

       auto threadPoolMonoFn{[this, millisecs](){
           waiting_task<thread_pool<void, false>>(4u, millisecs, 4u);
        }
      };

      auto nullThreadFn{[this, millisecs]() { waiting_task<serial<void>>(4u, millisecs); }};

      check_relative_performance(LINE("Four Waiting tasks; pool_4/null"), threadPoolFn, nullThreadFn, 3.9, 4.1);
      check_relative_performance(LINE("Four Waiting tasks; pool_4M/null"), threadPoolMonoFn, nullThreadFn, 3.9, 4.1);
    }
  }

  void threading_models_performance_test::test_waiting_task_return(const std::chrono::milliseconds millisecs)
  {
    using namespace concurrency;

    {
      auto threadPoolFn{[this, millisecs](){
          return waiting_task_return<thread_pool<int>>(2u, millisecs, 2u);
        }
      };

      auto threadPoolMonoFn{[this, millisecs](){
          return waiting_task_return<thread_pool<int, false>>(2u, millisecs, 2u);
        }
      };

      auto nullThreadFn{[this, millisecs]() { return waiting_task_return<serial<int>>(2u, millisecs); }};

      auto asyncFn{[this, millisecs]() { return waiting_task_return<asynchronous<int>>(2u, millisecs); }};

      check_relative_performance(LINE("Two Waiting tasks; pool_2/null"), threadPoolFn, nullThreadFn, 1.9, 2.1);
      check_relative_performance(LINE("Two Waiting tasks; pool_2M/null"), threadPoolMonoFn, nullThreadFn, 1.9, 2.1);
      check_relative_performance(LINE("Two Waiting tasks; async/null"), asyncFn, nullThreadFn, 1.9, 2.1);
    }
  }

  template<class ThreadModel, class... Args>
  void threading_models_performance_test::waiting_task(const std::size_t nTasks, const std::chrono::milliseconds millisecs, Args&&... args)
  {
    ThreadModel model{std::forward<Args>(args)...};

    for(std::size_t i{}; i < nTasks; ++i)
    {
      model.push(wait{millisecs});
    }

    model.get();
  }

  template<class ThreadModel, class... Args>
  std::vector<int> threading_models_performance_test::waiting_task_return(const std::size_t nTasks, const std::chrono::milliseconds millisecs, Args&&... args)
  {
    ThreadModel model{std::forward<Args>(args)...};

    for(std::size_t i{}; i < nTasks; ++i)
    {
      model.push([millisecs, i{static_cast<int>(i)}]() {
          wait{millisecs}();
          return i;
        }
      );
    }

    return model.get();
  }
}
