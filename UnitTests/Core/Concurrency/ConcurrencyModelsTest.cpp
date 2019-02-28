////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "ConcurrencyModelsTest.hpp"
#include "ConcurrencyModels.hpp"

namespace sequoia::unit_testing
{
  void threading_models_test::run_tests()
  {
    using namespace concurrency;

    test_task_queue();

    test_waiting_task(std::chrono::milliseconds{10});
    test_waiting_task_return(std::chrono::milliseconds{10});
    
    test_exceptions<thread_pool<void>, std::runtime_error>(LINE("pool_2"), 2u);
    test_exceptions<thread_pool<void, false>, std::runtime_error>(LINE("pool_2M"), 2u);
    test_exceptions<asynchronous<void>, std::runtime_error>(LINE("async"));

    test_exceptions<thread_pool<int>, std::runtime_error>(LINE("pool_2"), 2u);
    test_exceptions<thread_pool<int, false>, std::runtime_error>(LINE("pool_2M"), 2u);
    test_exceptions<asynchronous<int>, std::runtime_error>(LINE("async"));

    // Both the thread pool and asynchronous processing models
    // ultimately take copies of the functor, so ony for
    // serial does a modifiable functor make sense
    //update_vector_tests<thread_pool<void, std::deque>>();
    //update_vector_tests<asynchronous<void>>();
  
    test_functor_update<serial<void>>();
  }

  void threading_models_test::test_task_queue()
  {
    using namespace concurrency;

    {
      using q_t = task_queue<void>;
      using task_t = q_t::task_t;

      q_t q{};

      int a{};
      check(q.push(task_t{[&a](){ a+= 1; }}, std::try_to_lock), LINE(""));
      auto t{q.pop(std::try_to_lock)};

      t();
      check_equality(1, a, LINE(""));
      
      q.push(task_t{[&a](){ a+= 2; }});
      t = q.pop();

      t();
      check_equality(3, a, LINE(""));

      q.finish();
    }
    
    {
      using q_t = task_queue<int>;
      using task_t = q_t::task_t;

      q_t q{};

      check(q.push(task_t{[](){ return 1;}}, std::try_to_lock), LINE(""));
      auto t{q.pop(std::try_to_lock)};

      auto fut{t.get_future()};
      t();
      
      check_equality(1, fut.get(), LINE(""));
      
      q.push(task_t{[](){ return 2;}});
      t = q.pop();

      fut = t.get_future();
      t();
      
      check_equality(2, fut.get(), LINE(""));

      q.finish();
    }
  }

  void threading_models_test::test_waiting_task(const std::chrono::milliseconds millisecs)
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

      check_relative_performance(threadPoolFn, nullThreadFn, 1.9, 2.1, LINE("Two Waiting tasks; pool_2/null"));
      check_relative_performance(threadPoolMonoFn, nullThreadFn, 1.9, 2.1, LINE("Two Waiting tasks; pool_2M/null"));
      check_relative_performance(asyncFn, nullThreadFn, 1.9, 2.1, LINE("Two Waiting tasks; async/null"));
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

      check_relative_performance(threadPoolFn, nullThreadFn, 1.9, 2.1, LINE("Four Waiting tasks; pool_2/null"));
      check_relative_performance(threadPoolMonoFn, nullThreadFn, 1.9, 2.1, LINE("Four Waiting tasks; pool_2M/null"));
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

      check_relative_performance(threadPoolFn, nullThreadFn, 3.9, 4.1, LINE("Four Waiting tasks; pool_4/null"));
      check_relative_performance(threadPoolMonoFn, nullThreadFn, 3.9, 4.1, LINE("Four Waiting tasks; pool_4M/null"));
    }
  }

  void threading_models_test::test_waiting_task_return(const std::chrono::milliseconds millisecs)
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

      auto futures{check_relative_performance(threadPoolFn, nullThreadFn, 1.9, 2.1, LINE("Two Waiting tasks; pool_2/null"))};
      check_return_values(std::move(futures), LINE("pool_2"));

          
      futures = check_relative_performance(threadPoolMonoFn, nullThreadFn, 1.9, 2.1, LINE("Two Waiting tasks; pool_2M/null"));
      check_return_values(std::move(futures), LINE("pool_2M"));

      futures = check_relative_performance(asyncFn, nullThreadFn, 1.9, 2.1, LINE("Two Waiting tasks; async/null"));
      check_return_values(std::move(futures), LINE("async"));
    }
  }

  void threading_models_test::check_return_values(performance_results<std::vector<int>>&& futures, std::string_view message)
  {
    for(auto& f : futures.fast_futures)
    {
      auto results{f.get()};
      if(check_equality(2ul, results.size(), LINE(message)))
      {
        check_equality(0, results[0], LINE(message));
        check_equality(1, results[1], LINE(message));
      }
    }

    for(auto& f : futures.slow_futures)
    {
      auto results{f.get()};
      if(check_equality(2ul, results.size(), LINE(message)))
      {
        check_equality(0, results[0], LINE(message));
        check_equality(1, results[1], LINE(message));
      }
    }
  }

  template<class ThreadModel, class... Args>
  void threading_models_test::waiting_task(const std::size_t nTasks, const std::chrono::milliseconds millisecs, Args&&... args)
  {
    ThreadModel model{std::forward<Args>(args)...};
    
    for(int i{}; i < nTasks; ++i)
    {
      model.push(Wait{millisecs});
    }

    model.get();
  }

  template<class ThreadModel, class... Args>
  std::vector<int> threading_models_test::waiting_task_return(const std::size_t nTasks, const std::chrono::milliseconds millisecs, Args&&... args)
  {
    ThreadModel model{std::forward<Args>(args)...};
    
    for(int i{}; i < nTasks; ++i)
    {
      model.push([millisecs, i]() {
          Wait wait{millisecs};
          wait();
          return i;
        }
      );
    }

    return model.get();
  }

  template<class ThreadModel, class Exception, class... Args>
  void threading_models_test::test_exceptions(std::string_view message, Args&&... args)
  {
    ThreadModel threadModel{std::forward<Args>(args)...};;
    using R = typename ThreadModel::return_type;
    
    threadModel.push([]() -> R { throw Exception{"Error!"}; });

    check_exception_thrown<Exception>([&threadModel]() { return threadModel.get(); }, std::string{message});
  }

  template<class Model>
  void threading_models_test::test_functor_update()
  {
    Model threadModel;

    UpdatableFunctor functor;

    threadModel.push(functor, 0);

    threadModel.get();
	
    check_equality(std::vector<int>{0}, functor.get_data());
  }    
}
