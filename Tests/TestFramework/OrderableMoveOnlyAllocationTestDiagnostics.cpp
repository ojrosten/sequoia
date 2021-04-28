////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "OrderableMoveOnlyAllocationTestDiagnostics.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view orderable_move_only_allocation_false_negative_diagnostics::source_file() const noexcept
  {
    return __FILE__;
  }

  void orderable_move_only_allocation_false_negative_diagnostics::run_tests()
  {
    do_allocation_tests(*this);
  }

  template<bool PropagateMove, bool PropagateSwap>
  void orderable_move_only_allocation_false_negative_diagnostics::test_allocation()
  {
    test_semantics_allocations<PropagateMove, PropagateSwap>();
  }

  template<bool PropagateMove, bool PropagateSwap>
  void orderable_move_only_allocation_false_negative_diagnostics::test_semantics_allocations()
  {

  }
  
  [[nodiscard]]
  std::string_view orderable_move_only_allocation_false_positive_diagnostics::source_file() const noexcept
  {
    return __FILE__;
  }

  void orderable_move_only_allocation_false_positive_diagnostics::run_tests()
  {
    do_allocation_tests(*this);
  }

  template<bool PropagateMove, bool PropagateSwap>
  void orderable_move_only_allocation_false_positive_diagnostics::test_allocation()
  {
    test_semantics_allocations<PropagateMove, PropagateSwap>();
  }

  template<bool PropagateMove, bool PropagateSwap>
  void orderable_move_only_allocation_false_positive_diagnostics::test_semantics_allocations()
  {

  }
}
