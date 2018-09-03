#include "TestThreadingModels.hpp"
#include "ThreadingModels.hpp"

namespace sequoia
{
  namespace unit_testing
  {
    void test_threading_models::run_tests()
    {
      void_return_type_tests();
      return_value_tests();
      functor_update_tests();
    }

    void test_threading_models::functor_update_tests()
    {
      using namespace concurrency;

      update_vector_tests<serial<void>>();

      // Both the thread pool and asynchronous processing models
      // ultimately take copies of the functor, so ony for
      // serial does a modifiable functor make sense
      //update_vector_tests<thread_pool<void, std::deque>>();
      //update_vector_tests<asynchronous<void>>();
    }
    
    void test_threading_models::void_return_type_tests()
    {
      using namespace concurrency;

      {
        auto nullThreadFn   = [this]() { testWaitingTask<serial<void>, 2>(); };
        auto threadPoolFn_2 = [this]() { testWaitingTask<thread_pool<void, std::deque>, 2>(2u); };
        auto asyncFn        = [this]() { testWaitingTask<asynchronous<void>, 2>(); };
        check_relative_performance(threadPoolFn_2, nullThreadFn, 1.9, true, "Two Waiting tasks; pool_2/null");
        check_relative_performance(asyncFn, nullThreadFn, 1.9, true, "Two Waiting tasks; async/null");
      }

      {
        auto nullThreadFn   = [this]() { testWaitingTask<serial<void>, 4>(); };
        auto threadPoolFn_2 = [this]() { testWaitingTask<thread_pool<void, std::deque>, 4>(2u); };
        auto asyncFn        = [this]() { testWaitingTask<asynchronous<void>, 4>(); };
        check_relative_performance(threadPoolFn_2, nullThreadFn, 1.9, true, "Four Waiting tasks; pool_2/null");
        check_relative_performance(asyncFn, nullThreadFn, 1.9, true, "Four Waiting tasks; async/null");
      }

      {
        auto nullThreadFn   = [this]() { testWaitingTask<serial<void>, 4>(); };
        auto threadPoolFn_4 = [this]() { testWaitingTask<thread_pool<void, std::deque>, 4>(4u); };
        check_relative_performance(threadPoolFn_4, nullThreadFn, 3.75, true, "Four Waiting tasks; pool_4/null");
      }
    }

    void test_threading_models::return_value_tests()
    {
      using namespace concurrency;
      
      {
        const size_t upper{static_cast<size_t>(1e7)};

        TriangularNumbers
          sillyTask(upper),
          sillyTask2(upper + 1);

        auto nullThreadFn = [this, sillyTask, sillyTask2]()
        { return test_lval_task<serial<size_t>>(sillyTask, sillyTask2); };
        auto threadPoolFn_2 = [this, sillyTask, sillyTask2]()
          { return test_lval_task<thread_pool<size_t, std::deque>>(sillyTask, sillyTask2, 2u); };
        auto asyncFn = [this, sillyTask, sillyTask2]()
        { return test_lval_task<asynchronous<size_t>>(sillyTask, sillyTask2); };

        auto futures = check_relative_performance(threadPoolFn_2, nullThreadFn, 1.4, true, "Two silly lval tasks; pool_2/null");
        for(auto&& fut : futures.fast_futures)
        {
          auto results = fut.get();
          check_equality<size_t>(2, results.size());
          if(results.size() == 2)
          {
            auto iter = results.cbegin();
            check_equality<size_t>(upper*(upper + 1) / 2, *iter++);
            check_equality<size_t>((upper + 1)*(upper + 2) / 2, *iter);
          }
        }
        for(auto&& fut : futures.slow_futures)
        {
          auto results = fut.get();
          check_equality<size_t>(2, results.size());
          if(results.size() == 2)
          {
            auto iter = results.cbegin();
            check_equality<size_t>(upper*(upper + 1) / 2, *iter++);
            check_equality<size_t>((upper + 1)*(upper + 2) / 2, *iter);
          }
        }

        futures = check_relative_performance(asyncFn, nullThreadFn, 1.4, true, "Two silly lval tasks; async/null");
        for(auto&& fut : futures.fast_futures)
        {
          auto results = fut.get();
          check_equality<size_t>(2, results.size());
          if(results.size() == 2)
          {
            auto iter = results.cbegin();
            check_equality<size_t>(upper*(upper + 1) / 2, *iter++);
            check_equality<size_t>((upper + 1)*(upper + 2) / 2, *iter);
          }
        }
        for(auto&& fut : futures.slow_futures)
        {
          auto results = fut.get();
          check_equality<size_t>(2, results.size());
          if(results.size() == 2)
          {
            auto iter = results.cbegin();
            check_equality<size_t>(upper*(upper + 1) / 2, *iter++);
            check_equality<size_t>((upper + 1)*(upper + 2) / 2, *iter);
          }
        }
      }

      {
        const size_t upper  = static_cast<size_t>(1e7);
        auto nullThreadFn   = [this]() { return test_rval_task<serial<size_t>, 2>(upper); };
        auto threadPoolFn_2 = [this]() { return test_rval_task<thread_pool<size_t, std::deque>, 2>(upper, 2u); };
        auto asyncFn        = [this]() { return test_rval_task<asynchronous<size_t>, 2>(upper); };

        auto futures = check_relative_performance(threadPoolFn_2, nullThreadFn, 1.4, true, "Two silly rval tasks; pool_2/null");
        for(auto&& fut : futures.fast_futures)
        {
          auto results = fut.get();
          check_equality<size_t>(2, results.size());
          if(results.size() == 2)
          {
            auto iter = results.cbegin();
            check_equality<size_t>(upper*(upper + 1) / 2, *iter++, "test thread_pool_2 rval first task");
            check_equality<size_t>(upper*(upper - 1) / 2, *iter, "test thread_pool_2 rval second task");
          }
        }
        for(auto&& fut : futures.slow_futures)
        {
          auto results = fut.get();
          check_equality<size_t>(2, results.size());
          if(results.size() == 2)
          {
            auto iter = results.cbegin();
            check_equality<size_t>(upper*(upper + 1) / 2, *iter++);
            check_equality<size_t>(upper*(upper - 1) / 2, *iter);
          }
        }
        futures = check_relative_performance(asyncFn, nullThreadFn, 1.4, true, "Two silly rval tasks; async/null");
        for(auto&& fut : futures.fast_futures)
        {
          auto results = fut.get();
          check_equality<size_t>(2, results.size());
          if(results.size() == 2)
          {
            auto iter = results.cbegin();
            check_equality<size_t>(upper*(upper + 1) / 2, *iter++);
            check_equality<size_t>(upper*(upper - 1) / 2, *iter);
          }
        }
        for(auto&& fut : futures.slow_futures)
        {
          auto results = fut.get();
          check_equality<size_t>(2, results.size());
          if(results.size() == 2)
          {
            auto iter = results.cbegin();
            check_equality<size_t>(upper*(upper + 1) / 2, *iter++);
            check_equality<size_t>(upper*(upper - 1) / 2, *iter);
          }
        }
      }

      test_exceptions<thread_pool<void, std::deque>, std::runtime_error>(2u);
      test_exceptions<asynchronous<void>, std::runtime_error>();
    }
  }
}
