////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "StaticStackTest.hpp"
#include "StaticStackTestingUtilities.hpp"

#include "sequoia/Core/DataStructures/StaticStack.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path test_static_stack::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void test_static_stack::run_tests()
  {
    check_depth_0();
    check_depth_1();
    check_depth_2();
  }

  void test_static_stack::check_depth_0()
  {
    using namespace data_structures;

    constexpr static_stack<int, 0> s{};
    static_stack<int, 0> t{};

    check(equality, report_line("Equality of null queues"), t, s);

    check_exception_thrown<std::logic_error>(report_line("Can't push to null stack"), [&t]() { t.push(1); });
    check_exception_thrown<std::logic_error>(report_line("Can't construct non-null null stack"), []() { static_stack<int, 0>{1}; });
  }

  void test_static_stack::check_depth_1()
  {
    using namespace data_structures;
    using stack_t = static_stack<int, 1>;

    constexpr stack_t s{1};
    stack_t t{};
    t.push(2);

    check_semantics(report_line("Standard Semantics"), s, t);

    check_exception_thrown<std::logic_error>(report_line("Trying to push two elements to stack of depth 1"), [&t]() { t.push(1); });
    check_exception_thrown<std::logic_error>(report_line("Can't construct stack of depth 1 with 2 elements"), []() { static_stack<int, 1>{1, 2}; });

    t.pop();
    check(equality, report_line(""), t, stack_t{});

    t.push(1);
    check(equality, report_line(""), t, stack_t{1});
  }

  constexpr auto test_static_stack::make_static_stack_2()
  {
    using namespace data_structures;

    static_stack<int, 2> s{};

    s.push(10);
    s.pop();
    s.push(11);
    s.push(12);

    return s;
  }

  void test_static_stack::check_depth_2()
  {
    using namespace data_structures;
    using stack_t = static_stack<int, 2>;

    constexpr stack_t s{make_static_stack_2()};
    auto t{s};

    check(equality, report_line(""), t, stack_t{11, 12});

    t.pop();
    check(equality, report_line(""), t, stack_t{11});

    t.pop();
    check(equality, report_line(""), t, stack_t{});
  }
}
