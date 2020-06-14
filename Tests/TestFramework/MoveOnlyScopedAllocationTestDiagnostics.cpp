////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "MoveOnlyScopedAllocationTestDiagnostics.hpp"
#include "MoveOnlyScopedAllocationTestDiagnosticUtilities.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view move_only_scoped_allocation_false_negative_diagnostics::source_file_name() const noexcept
  {
    return __FILE__;
  }

  void move_only_scoped_allocation_false_negative_diagnostics::run_tests()
  {
    do_allocation_tests(*this);
  }

  template<bool PropagateMove, bool PropagateSwap>
  void move_only_scoped_allocation_false_negative_diagnostics::test_allocation()
  {
    test_regular_semantics<PropagateMove, PropagateSwap>();
  }

  template<bool PropagateMove, bool PropagateSwap>
  void move_only_scoped_allocation_false_negative_diagnostics::test_regular_semantics()
  {
  }


  [[nodiscard]]
  std::string_view move_only_scoped_allocation_false_positive_diagnostics::source_file_name() const noexcept
  {
    return __FILE__;
  }

  void move_only_scoped_allocation_false_positive_diagnostics::run_tests()
  {
    do_allocation_tests(*this);
  }

  template<bool PropagateMove, bool PropagateSwap>
  void move_only_scoped_allocation_false_positive_diagnostics::test_allocation()
  {
    test_regular_semantics<PropagateMove, PropagateSwap>();
  }

  template<bool PropagateMove, bool PropagateSwap>
  void move_only_scoped_allocation_false_positive_diagnostics::test_regular_semantics()
  {
  }
}
