////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "MoveOnlyTestDiagnostics.hpp"
#include "MoveOnlyTestDiagnosticUtilities.hpp"

namespace sequoia::unit_testing
{
  [[nodiscard]]
  std::string_view move_only_false_positive_diagnostics::source_file_name() const noexcept
  {
    return __FILE__;
  }

  void move_only_false_positive_diagnostics::run_tests()
  {
    test_regular_semantics();
  }

  void move_only_false_positive_diagnostics::test_regular_semantics()
  {
    check_regular_semantics(LINE("Broken equality"),  move_only_broken_equality{1},  move_only_broken_equality{2},  move_only_broken_equality{1},  move_only_broken_equality{2});
    check_regular_semantics(LINE("Broken inequality"),  move_only_broken_inequality{1},  move_only_broken_inequality{2},  move_only_broken_inequality{1},  move_only_broken_inequality{2});
    check_regular_semantics(LINE("Broken move"),  move_only_broken_move{1},  move_only_broken_move{2},  move_only_broken_move{1},  move_only_broken_move{2});
    check_regular_semantics(LINE("Broken move"),  move_only_broken_swap{1},  move_only_broken_swap{2},  move_only_broken_swap{1},  move_only_broken_swap{2});
    check_regular_semantics(LINE("Broken move assignment"), move_only_broken_move_assignment{1}, move_only_broken_move_assignment{2}, move_only_broken_move_assignment{1}, move_only_broken_move_assignment{2});
    check_regular_semantics(LINE("Broken check invariant"), move_only_beast{1}, move_only_beast{1}, move_only_beast{1}, move_only_beast{1});
    check_regular_semantics(LINE("Broken check invariant"), move_only_beast{1}, move_only_beast{3}, move_only_beast{2}, move_only_beast{3});
    check_regular_semantics(LINE("Broken check invariant"), move_only_beast{1}, move_only_beast{2}, move_only_beast{3}, move_only_beast{2});
  }


  [[nodiscard]]
  std::string_view move_only_false_negative_diagnostics::source_file_name() const noexcept
  {
    return __FILE__;
  }

  void move_only_false_negative_diagnostics::run_tests()
  {
    test_regular_semantics();
  }

  void move_only_false_negative_diagnostics::test_regular_semantics()
  {
    check_regular_semantics(LINE(""), move_only_beast{1}, move_only_beast{2}, move_only_beast{1}, move_only_beast{2});
  }
}
