////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "RegularAllocationTestFalsePositiveDiagnosticsBrokenSemantics.hpp"

#include "ContainerDiagnosticsUtilities.hpp"
#include "AllocationTestDiagnosticsUtilities.hpp"

#include <vector>

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path allocation_false_positive_diagnostics_broken_semantics::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void allocation_false_positive_diagnostics_broken_semantics::run_tests()
  {
    do_allocation_tests(*this);
  }

  template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void allocation_false_positive_diagnostics_broken_semantics::test_allocation()
  {
    test_regular_semantics<PropagateCopy, PropagateMove, PropagateSwap>();
  }

  template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void allocation_false_positive_diagnostics_broken_semantics::test_regular_semantics()
  {
      auto mutator{ [](auto& b) { b.x.push_back(1); } };

      {
        using beast = broken_equality<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;

        auto allocGetter{
          [](const beast& b){ return b.x.get_allocator(); }
        };

        check_semantics(report_line("Broken equality"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info{allocGetter, {1_c, {1_c,1_mu}, {1_anp,1_awp}}});
      }

      {
        using beast = broken_inequality<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;

        auto allocGetter{
          [](const beast& b){ return b.x.get_allocator(); }
        };

        check_semantics(report_line("Broken inequality"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info{allocGetter, {1_c, {1_c,1_mu}, {1_anp,1_awp}}});
      }

      {
        using beast = broken_copy<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;

        auto allocGetter{
         [](const beast& b){ return b.x.get_allocator(); }
        };

        check_semantics(report_line("Broken copy"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info{allocGetter, {1_c, {1_c,1_mu}, {1_anp,1_awp}}});
      }

      {
        using beast = broken_para_copy<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;

        auto allocGetter{
          [](const beast& b){ return b.x.get_allocator(); }
        };

        check_semantics(report_line("Broken para-copy"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info{allocGetter, {1_c, {1_c,1_mu}, {1_anp,1_awp}}});
      }

      {
        using beast = broken_move<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;

        auto allocGetter{
          [](const beast& b){ return b.x.get_allocator(); }
        };

        check_semantics(report_line("Broken move"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info{allocGetter, {1_c, {1_c,1_mu}, {1_anp,1_awp}}});
      }

      {
        using beast = broken_para_move<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;

        auto allocGetter{
          [](const beast& b){ return b.x.get_allocator(); }
        };

        check_semantics(report_line("Broken para-move"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info{allocGetter, {1_c, {1_c,1_mu}, {1_anp,1_awp}}});
      }

      {
        using beast = broken_copy_assignment<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;

        auto allocGetter{
          [](const beast& b){ return b.x.get_allocator(); }
        };

        check_semantics(report_line("Broken copy assignment"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info{allocGetter, {1_c, {1_c,1_mu}, {1_anp,1_awp}}});
      }

      {
        using beast = broken_move_assignment<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;

        auto allocGetter{
          [](const beast& b){ return b.x.get_allocator(); }
        };

        check_semantics(report_line("Broken move assignment"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info{allocGetter, {1_c, {1_c,1_mu}, {1_anp,1_awp}}});
      }
  }
}
