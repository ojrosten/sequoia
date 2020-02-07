////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "StaticStackTest.hpp"
#include "StaticStackTestingUtilities.hpp"

#include "StaticStack.hpp"

namespace sequoia::unit_testing
{
  [[nodiscard]]
  std::string_view test_static_stack::source_file_name() const noexcept
  {
    return __FILE__;
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

    check_equality(LINE("Equality of null queues"), t, s);

    check_exception_thrown<std::logic_error>(LINE("Can't push to null stack"), [&t]() { t.push(1); });
    check_exception_thrown<std::logic_error>(LINE("Can't construct non-null null stack"), []() { static_stack<int, 0>{1}; });
  }

  void test_static_stack::check_depth_1()
  {
    using namespace data_structures;
    using stack_t = static_stack<int, 1>;

    constexpr stack_t s{1};
    stack_t t{};
    t.push(2);

    check_regular_semantics(LINE("Standard Semantics"), s, t);

    check_exception_thrown<std::logic_error>(LINE("Trying to push two elements to stack of depth 1"), [&t]() { t.push(1); });
    check_exception_thrown<std::logic_error>(LINE("Can't construct stack of depth 1 with 2 elements"), []() { static_stack<int, 1>{1, 2}; });

    t.pop();
    check_equality(LINE(""), t, stack_t{});
    
    t.push(1);
    check_equality(LINE(""), t, stack_t{1});
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

    check_equality(LINE(""), t, stack_t{11, 12});

    t.pop();
    check_equality(LINE(""), t, stack_t{11});

    t.pop();
    check_equality(LINE(""), t, stack_t{});
  }
}
