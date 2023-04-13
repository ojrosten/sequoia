////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "MoveOnlyTestDiagnostics.hpp"
#include "MoveOnlyTestDiagnosticsUtilities.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path move_only_false_positive_diagnostics::source_file() const noexcept
  {
    return std::source_location::current().file_name();
  }

  void move_only_false_positive_diagnostics::run_tests()
  {
    test_regular_semantics();
  }

  void move_only_false_positive_diagnostics::test_regular_semantics()
  {
    check_semantics(report_line("Broken equality"),   move_only_broken_equality{1},    move_only_broken_equality{2},    move_only_broken_equality{1},    move_only_broken_equality{2});
    check_semantics(report_line("Broken inequality"), move_only_broken_inequality{1},  move_only_broken_inequality{2},  move_only_broken_inequality{1},  move_only_broken_inequality{2});
    check_semantics(report_line("Broken move"),  move_only_broken_move{1},  move_only_broken_move{2},  move_only_broken_move{1},  move_only_broken_move{2});
    check_semantics(report_line("Broken swap"),  move_only_broken_swap{1},  move_only_broken_swap{2},  move_only_broken_swap{1},  move_only_broken_swap{2});
    check_semantics(report_line("Broken move assignment"), move_only_broken_move_assignment{1}, move_only_broken_move_assignment{2}, move_only_broken_move_assignment{1}, move_only_broken_move_assignment{2});
    check_semantics(report_line("Broken check invariant"), move_only_beast{1}, move_only_beast{1}, move_only_beast{1}, move_only_beast{1});
    check_semantics(report_line("Broken check invariant"), move_only_beast{1}, move_only_beast{3}, move_only_beast{2}, move_only_beast{3});
    check_semantics(report_line("Broken check invariant"), move_only_beast{2}, move_only_beast{1}, move_only_beast{2}, move_only_beast{3});

    check_semantics(report_line("Incorrect moved-from state"), resource_binder{1}, resource_binder{2}, resource_binder{1}, resource_binder{2}, resource_binder{3});
    check_semantics(report_line("Incorrect moved-from state"), []() { return resource_binder{1}; }, []() {return resource_binder{2}; }, resource_binder{3});
  }


  [[nodiscard]]
  std::filesystem::path move_only_false_negative_diagnostics::source_file() const noexcept
  {
    return std::source_location::current().file_name();
  }

  void move_only_false_negative_diagnostics::run_tests()
  {
    test_regular_semantics();
  }

  void move_only_false_negative_diagnostics::test_regular_semantics()
  {
    using beast = move_only_beast<int>;
    check_semantics(report_line(""), beast{1}, beast{2}, beast{1}, beast{2});
    check_semantics(report_line("Function object syntax"), [](){ return beast{1}; }, [](){ return beast{2}; });

    check_semantics(report_line("Check moved-from state"), resource_binder{1}, resource_binder{2}, resource_binder{1}, resource_binder{2}, resource_binder{0});
    check_semantics(report_line("Check moved-from state"), []() { return resource_binder{1}; }, []() {return resource_binder{2}; }, resource_binder{0});
  }
}
