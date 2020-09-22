////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "OrderableMoveOnlyTestDiagnostics.hpp"
#include "OrderableMoveOnlyTestDiagnosticsUtilities.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view orderable_move_only_false_positive_diagnostics::source_file() const noexcept
  {
    return __FILE__;
  }

  void orderable_move_only_false_positive_diagnostics::run_tests()
  {
    test_regular_semantics();
  }

  void orderable_move_only_false_positive_diagnostics::test_regular_semantics()
  {
  }

  [[nodiscard]]
  std::string_view orderable_move_only_false_negative_diagnostics::source_file() const noexcept
  {
    return __FILE__;
  }

  void orderable_move_only_false_negative_diagnostics::run_tests()
  {
    test_regular_semantics();
  }

  void orderable_move_only_false_negative_diagnostics::test_regular_semantics()
  {
    check_semantics(LINE(""), orderable_move_only_beast{1}, orderable_move_only_beast{2}, orderable_move_only_beast{1}, orderable_move_only_beast{2}, std::weak_ordering::less);
  }
}
