////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "OrderableRegularAllocationTestDiagnostics.hpp"
#include "../OrderableRegularTestDiagnosticsUtilities.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path orderable_regular_allocation_false_positive_diagnostics::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void orderable_regular_allocation_false_positive_diagnostics::run_tests()
  {
    do_allocation_tests();
  }

  template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void orderable_regular_allocation_false_positive_diagnostics::test_allocation()
  {
    test_semantics_allocations<PropagateCopy, PropagateMove, PropagateSwap>();
  }

  template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void orderable_regular_allocation_false_positive_diagnostics::test_semantics_allocations()
  {
    using beast = orderable_regular_beast<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;

    auto getter{[](const beast& b){ return b.x.get_allocator(); }};
    auto mutator{[](beast& b) { b.x.shrink_to_fit(); b.x.push_back(3); }};

    check_semantics("",
                    [](){ return beast{}; },
                    [](){ return beast{2}; },
                    std::weak_ordering::less,
                    mutator,
                    allocation_info{getter, {0_c, {1_c, 1_mu}, {1_anp, 1_awp}}});

    check_semantics("",
                    beast{},
                    beast{2},
                    std::weak_ordering::less,
                    mutator,
                    allocation_info{getter, {0_c, {1_c, 1_mu}, {1_anp, 1_awp}}});
  }


  [[nodiscard]]
  std::filesystem::path orderable_regular_allocation_false_negative_diagnostics::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void orderable_regular_allocation_false_negative_diagnostics::run_tests()
  {
    do_allocation_tests();
  }

  template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void orderable_regular_allocation_false_negative_diagnostics::test_allocation()
  {
    test_semantics_allocations<PropagateCopy, PropagateMove, PropagateSwap>();
  }

  template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void orderable_regular_allocation_false_negative_diagnostics::test_semantics_allocations()
  {
    using beast = orderable_regular_inefficient_comparisons<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;

    auto getter{[](const beast& b){ return b.x.get_allocator(); }};
    auto mutator{[](beast& b) { b.x.shrink_to_fit(); b.x.push_back(3); }};

    check_semantics("",
                    [](){ return beast{}; },
                    [](){ return beast{2}; },
                    std::weak_ordering::less,
                    mutator,
                    allocation_info{getter, {0_c, {1_c, 1_mu}, {1_anp, 0_awp}}});

    check_semantics("",
                    beast{},
                    beast{2},
                    std::weak_ordering::less,
                    mutator,
                    allocation_info{getter, {0_c, {1_c, 1_mu}, {1_anp, 0_awp}}});
  }
}
