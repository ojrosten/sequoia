////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "RegularAllocationTestFalsePositiveDiagnosticsInefficientOperations.hpp"

#include "ContainerDiagnosticsUtilities.hpp"
#include "AllocationTestDiagnosticsUtilities.hpp"

#include <vector>

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path allocation_false_positive_diagnostics_inefficient_operations::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void allocation_false_positive_diagnostics_inefficient_operations::run_tests()
  {
    do_allocation_tests(*this);
  }

  template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void allocation_false_positive_diagnostics_inefficient_operations::test_allocation()
  {
    test_regular_semantics<PropagateCopy, PropagateMove, PropagateSwap>();
  }

  template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void allocation_false_positive_diagnostics_inefficient_operations::test_regular_semantics()
  {
      auto mutator{ [](auto& b) { b.x.push_back(1); } };

      {
        using beast = inefficient_equality<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;

        auto allocGetter{
          [](const beast& b){ return b.x.get_allocator(); }
        };

        check_semantics(report_line("Inefficient equality"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info{allocGetter, {1_c, {1_c,1_mu}, {1_anp,1_awp}}});
      }

      {
        using beast = inefficient_inequality<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;

        auto allocGetter{
          [](const beast& b){ return b.x.get_allocator(); }
        };

        check_semantics(report_line("Inefficient inequality"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info{allocGetter, {1_c, {1_c,1_mu}, {1_anp,1_awp}}});
      }

      {
        using beast = inefficient_copy<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;

        auto allocGetter{
          [](const beast& b){ return b.x.get_allocator(); }
        };

        auto mutator2{
          [](beast& b) {
            b.x.reserve(b.x.capacity() + 1);
            b.x.push_back(1);
          }
        };

        check_semantics(report_line("Inefficient copy"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator2, allocation_info{allocGetter, {2_c, {3_c,1_mu,1_pc,1_pm}, {1_anp,1_awp}}});
        check_semantics(report_line("Inefficient copy"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator2, allocation_info{allocGetter, {3_c, {2_c,1_mu,1_pc,1_pm}, {1_anp,1_awp}}});
      }

      {
        using beast = inefficient_move<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;

        auto allocGetter{
          [](const beast& b){ return b.x.get_allocator(); }
        };

        check_semantics(report_line("Inefficient move"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info{allocGetter, {1_c, {1_c,1_mu}, {1_anp,1_awp}}});
      }

      {
        using beast = inefficient_para_copy<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;

        auto allocGetter{
          [](const beast& b){ return b.x.get_allocator(); }
        };

        auto mutator2{
          [](beast& b) {
            b.x.reserve(b.x.capacity() + 1);
            b.x.push_back(1);
          }
        };

        check_semantics(report_line("Inefficient para-copy"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator2, allocation_info{allocGetter, {1_c, {1_c,1_mu,3_pc,1_pm}, {1_anp,1_awp}}});
      }

      {
        using beast = inefficient_para_move<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;

        auto allocGetter{
          [](const beast& b){ return b.x.get_allocator(); }
        };

        auto mutator2{
          [](beast& b) {
            b.x.reserve(b.x.capacity() + 1);
            b.x.push_back(1);
          }
        };

        check_semantics(report_line("Inefficient para-move"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator2, allocation_info{allocGetter, {1_c, {1_c,1_mu,1_pc,3_pm}, {1_anp,1_awp}}});
      }

      {
        using beast = inefficient_serialization<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;

        auto allocGetter{ [](const beast& b){ return b.x.get_allocator(); } };

        check_semantics(report_line("Inefficient serialization"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info{allocGetter, {1_c, {1_c,1_mu}, {1_anp,1_awp}}});
      }
  }
}
