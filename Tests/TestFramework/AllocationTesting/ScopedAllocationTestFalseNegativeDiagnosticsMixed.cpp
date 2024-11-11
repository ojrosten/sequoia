////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "ScopedAllocationTestFalseNegativeDiagnosticsMixed.hpp"
#include "ScopedAllocationTestDiagnosticsUtilities.hpp"
#include "AllocationTestDiagnosticsUtilities.hpp"

namespace sequoia::testing
{
  namespace
  {
    template<alloc InnerAllocator>
    using perfectly_mixed_beast
      = typename scoped_beast_builder<perfectly_normal_beast, perfectly_sharing_beast<int, std::shared_ptr<int>, InnerAllocator>>::beast;

    template<alloc InnerAllocator>
    using weirdly_mixed_beast
      = typename scoped_beast_builder<perfectly_normal_beast, inefficient_para_copy<int, InnerAllocator>>::beast;
  }

  [[nodiscard]]
  std::filesystem::path scoped_allocation_false_positive_diagnostics_mixed::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void scoped_allocation_false_positive_diagnostics_mixed::run_tests()
  {
    do_allocation_tests();
  }

  template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void scoped_allocation_false_positive_diagnostics_mixed::test_allocation()
  {
    test_perfectly_mixed<PropagateCopy, PropagateMove, PropagateSwap>();
    test_weirdly_mixed<PropagateCopy, PropagateMove, PropagateSwap>();
  }

  template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void scoped_allocation_false_positive_diagnostics_mixed::test_perfectly_mixed()
  {
    using inner_allocator = shared_counting_allocator<std::shared_ptr<int>, PropagateCopy, PropagateMove, PropagateSwap>;
    using beast = perfectly_mixed_beast<inner_allocator>;

    auto getter{[](const beast& b) { return b.x.get_allocator(); }};

    auto mutator{
      [](beast& b) {
        b.x.reserve(10);
        b.x.push_back({});
      }
    };

    check_semantics("",
      beast{},
      beast{{1}},
      mutator,
      allocation_info{
        getter,
        {0_c, {1_c,1_mu}, {1_anp,1_awp}},
        { {0_c, {1_c,0_mu}, {1_anp,1_awp}, {0_containers, 1_containers, 2_postmutation}} }
      }
    );

    check_semantics("",
      beast{},
      beast{{1}, {2, 3}},
      mutator,
      allocation_info{
        getter,
        {0_c, {1_c,1_mu}, {1_anp,1_awp}},
        { {0_c, {2_c,0_mu}, {2_anp,2_awp}, {0_containers, 2_containers, 3_postmutation}} }
      }
    );

    check_semantics("",
      beast{{1}},
      beast{},
      mutator,
      allocation_info{
        getter,
        {1_c, {0_c,1_mu}, {0_anp,0_awp}},
        { {1_c, {0_c,0_mu}, {0_anp,0_awp}, {1_containers, 0_containers, 1_postmutation}} }
      }
    );

    check_semantics("",
      beast{{2}},
      beast{{1}},
      mutator,
      allocation_info{
        getter,
        {1_c, {1_c,1_mu}, {0_anp,1_awp}},
        { {1_c, {1_c,0_mu}, {1_anp,1_awp,0_manp,0_ma}, {1_containers, 1_containers, 2_postmutation}} }
      }
    );

    check_semantics("",
      beast{{2}},
      beast{{1}, {5,6}},
      mutator,
      allocation_info{
        getter,
        {1_c, {1_c,1_mu}, {1_anp,1_awp}},
        { {1_c, {2_c,0_mu}, {2_anp,2_awp}, {1_containers, 2_containers, 3_postmutation}} }
      }
    );

    check_semantics("",
      beast{{2}, {7,8}},
      beast{{1}},
      mutator,
      allocation_info{
        getter,
        {1_c, {1_c,1_mu}, {0_anp,1_awp}},
        { {2_c, {1_c,0_mu}, {1_anp,1_awp,0_manp,0_ma}, {2_containers, 1_containers, 2_postmutation}} }
      }
    );

    check_semantics("",
      beast{{2}, {7,8}},
      beast{{1}, {5,6}},
      mutator,
      allocation_info{
        getter,
        {1_c, {1_c,1_mu}, {0_anp,1_awp}},
        { {2_c, {2_c,0_mu}, {2_anp,2_awp,0_manp,0_ma}, {2_containers, 2_containers, 3_postmutation}} }
      }
    );
  }

  template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void scoped_allocation_false_positive_diagnostics_mixed::test_weirdly_mixed()
  {
    using inner_allocator = shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>;
    using beast = weirdly_mixed_beast<inner_allocator>;

    auto getter{[](const beast& b) { return b.x.get_allocator(); }};

    auto mutator{
      [](beast& b) {
        b.x.reserve(10);
        b.x.push_back({});
      }
    };

    check_semantics("",
      beast{},
      beast{{1}},
      mutator,
      allocation_info{
        getter,
        {0_c, {1_c,1_mu}, {1_anp,1_awp}},
        { {0_c, {2_c,0_mu,2_pc,1_pm}, {2_anp,2_awp,1_manp,0_ma}, {0_containers, 1_containers, 2_postmutation}} }
      }
    );
  }
}
