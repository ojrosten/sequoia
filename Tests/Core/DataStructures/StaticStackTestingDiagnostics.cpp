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
  std::filesystem::path test_static_stack_false_positives::source_file() const
  {
    return std::source_location::current().file_name();
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
    check(report_line("Empty stack"), !s.empty());
    check(equality, report_line(""), s.size(), 1_sz);
  }

  void test_static_stack_false_positives::check_depth_1()
  {
    using namespace data_structures;

    static_stack<int, 1> s{}, t{};
    t.push(1);

    // check_equality for static_stack is not independently
    // sensitive to empty and size, so one of these needs
    // to be explicitly checked!

    check(report_line("Empty stack"), !s.empty());
    check(report_line("Non-empty stack"), t.empty());

    check(equality, report_line("Empty stack versus populated stack"), s, t);

    s.push(2);
    check(equality, report_line("Differing elements"), s, t);

    s.pop();
    check(equality, report_line("Empty stack versus populated stack"), s, t);
  }
}
