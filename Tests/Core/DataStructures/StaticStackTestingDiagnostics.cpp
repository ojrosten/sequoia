////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "StaticStackTestingDiagnostics.hpp"
#include "StaticStackTestingUtilities.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view test_static_stack_false_positives::source_file() const noexcept
  {
    return __FILE__;
  }

  void test_static_stack_false_positives::run_tests()
  {
    check_depth_0();
    check_depth_1();
  }

  void test_static_stack_false_positives::check_depth_0()
  {
    using namespace data_structures;

    constexpr static_stack<int, 0> s{};
    check(LINE("Empty stack"), !s.empty());
    check(equality, LINE(""), s.size(), 1_sz);
  }

  void test_static_stack_false_positives::check_depth_1()
  {
    using namespace data_structures;

    static_stack<int, 1> s{}, t{};
    t.push(1);

    // check_equality for static_stack is not independently
    // sensitive to empty and size, so one of these needs
    // to be explicitly checked!

    check(LINE("Empty stack"), !s.empty());
    check(LINE("Non-empty stack"), t.empty());

    check(equality, LINE("Empty stack versus populated stack"), s, t);

    s.push(2);
    check(equality, LINE("Differing elements"), s, t);

    s.pop();
    check(equality, LINE("Empty stack versus populated stack"), s, t);
  }
}
