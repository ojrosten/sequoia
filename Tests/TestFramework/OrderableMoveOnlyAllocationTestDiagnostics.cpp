////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "OrderableMoveOnlyAllocationTestDiagnostics.hpp"
#include "OrderableMoveOnlyTestDiagnosticsUtilities.hpp"

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
    using beast = orderable_move_only_beast<int, shared_counting_allocator<int, true, PropagateMove, PropagateSwap>>;

    auto getter{[](const beast& b){ return b.x.get_allocator(); }};
    auto mutator{[](beast& b) { b.x.shrink_to_fit(); b.x.push_back(3); }};

    check_semantics(LINE(""),
                    [](){ return beast{}; },
                    [](){ return beast{2}; },
                    std::weak_ordering::less,
                    mutator,
                    allocation_info{getter, {0_pm, {1_pm, 1_mu}, {1_manp}}});

    check_semantics(LINE(""),
                    beast{},
                    beast{2},
                    beast{},
                    beast{2},
                    std::weak_ordering::less,
                    mutator,
                    allocation_info{getter, {0_pm, {1_pm, 1_mu}, {1_manp}}});
  }
}
