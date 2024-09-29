////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "ConcurrencyModelsTest.hpp"
#include "sequoia/Core/Concurrency/ConcurrencyModels.hpp"

namespace sequoia::testing
{
  using namespace concurrency;

  [[nodiscard]]
  std::filesystem::path threading_models_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void threading_models_test::run_tests()
  {
    test_task_queue();

    test_exceptions<thread_pool<void>>("pool_2M", 2u);
    test_exceptions<thread_pool<void, false>>("pool_2", 2u);
    test_exceptions<asynchronous<void>>("async");

    test_exceptions<thread_pool<int>>("pool_2M", 2u);
    test_exceptions<thread_pool<int, false>>("pool_2", 2u);
    test_exceptions<asynchronous<int>>("async");

    test_execution<thread_pool<int>>("pool_2M", 2u);
    test_execution<thread_pool<int, false>>("pool_2", 2u);
    test_execution<asynchronous<int>>("async");

    test_serial_exceptions();
    test_serial_execution();
  }

  void threading_models_test::test_task_queue()
  {
    {
      using q_t = task_queue<void>;
      using task_t = q_t::task_t;

      q_t q{};

      int a{};
      check("", q.push(task_t{[&a](){ a+= 1; }}, std::try_to_lock));
      auto t{q.pop(std::try_to_lock)};

      t();
      check(equality, "", a, 1);

      q.push(task_t{[&a](){ a+= 2; }});
      t = q.pop();

      t();
      check(equality, "", a, 3);

      q.finish();
    }

    {
      using q_t = task_queue<int>;
      using task_t = q_t::task_t;

      q_t q{};

      check("", q.push(task_t{[](){ return 1;}}, std::try_to_lock));
      auto t{q.pop(std::try_to_lock)};

      auto fut{t.get_future()};
      t();

      check(equality, "", fut.get(), 1);

      q.push(task_t{[](){ return 2;}});
      t = q.pop();

      fut = t.get_future();
      t();

      check(equality, "", fut.get(), 2);

      q.finish();
    }
  }

  template<class ThreadModel, class... Args>
  void threading_models_test::test_exceptions(std::string_view message, Args&&... args)
  {
    ThreadModel model{std::forward<Args>(args)...};
    using R = typename ThreadModel::return_type;

    auto fut{model.push([]() -> R { throw std::runtime_error{"Error!"}; })};

    check_exception_thrown<std::runtime_error>(report(message), [&fut]() { return fut.get(); });
  }

  template<class ThreadModel, class... Args>
  void threading_models_test::test_execution(std::string_view message, Args&&... args)
  {
    using R = typename ThreadModel::return_type;
    ThreadModel model{std::forward<Args>(args)...};

    if constexpr(std::is_void_v<R>)
    {
      int x{};
      auto fut{model.push([&x](){ return ++x; })};
      check(equality, report(message), fut.get(), 1);
    }
    else
    {
      auto fut{model.push([](){ return 42; })};
      check(equality, report(message), fut.get(), 42);
    }
  }

  void threading_models_test::test_serial_exceptions()
  {
    check_exception_thrown<std::runtime_error>("", [](){ serial<void>{}.push([]() { throw std::runtime_error{"Error!"}; }); });
    check_exception_thrown<std::runtime_error>("", [](){ return serial<int>{}.push([]() -> int { throw std::runtime_error{"Error!"}; }); });
  }

  void threading_models_test::test_serial_execution()
  {
    {
      serial<int> model{};
      check(equality, "", model.push([](){ return 42; }), 42);
      check(equality, "", model.push([](){ return 43; }), 43);
    }

    {
      serial<void> model{};
      int x{};
      model.push([&x]() { ++x; });
      check(equality, "", x, 1);
    }
  }

}
