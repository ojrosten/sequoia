////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "ConcurrencyModelsTest.hpp"
#include "ConcurrencyModels.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view threading_models_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void threading_models_test::run_tests()
  {
    using namespace concurrency;

    test_task_queue();
    
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
      check(LINE(""), q.push(task_t{[&a](){ a+= 1; }}, std::try_to_lock));
      auto t{q.pop(std::try_to_lock)};

      t();
      check_equality(LINE(""), a, 1);
      
      q.push(task_t{[&a](){ a+= 2; }});
      t = q.pop();

      t();
      check_equality(LINE(""), a, 3);

      q.finish();
    }
    
    {
      using q_t = task_queue<int>;
      using task_t = q_t::task_t;

      q_t q{};

      check(LINE(""), q.push(task_t{[](){ return 1;}}, std::try_to_lock));
      auto t{q.pop(std::try_to_lock)};

      auto fut{t.get_future()};
      t();
      
      check_equality(LINE(""), fut.get(), 1);
      
      q.push(task_t{[](){ return 2;}});
      t = q.pop();

      fut = t.get_future();
      t();
      
      check_equality(LINE(""), fut.get(), 2);

      q.finish();
    }
  }

  template<class ThreadModel, class Exception, class... Args>
  void threading_models_test::test_exceptions(std::string_view message, Args&&... args)
  {
    ThreadModel threadModel{std::forward<Args>(args)...};;
    using R = typename ThreadModel::return_type;
    
    threadModel.push([]() -> R { throw Exception{"Error!"}; });

    check_exception_thrown<Exception>(std::string{message}, [&threadModel]() { return threadModel.get(); });
  }

  template<class Model>
  void threading_models_test::test_functor_update()
  {
    Model threadModel;

    updatable u;

    threadModel.push(u, 0);

    threadModel.get();
	
    check_equality(LINE(""), u.get_data(), std::vector<int>{0});
  }    
}
