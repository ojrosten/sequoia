////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "RegularAllocationTestFalsePositiveDiagnosticsBrokenValueSemantics.hpp"

#include "../ContainerDiagnosticsUtilities.hpp"
#include "AllocationTestDiagnosticsUtilities.hpp"

#include <vector>

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path allocation_false_positive_diagnostics_broken_value_semantics::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void allocation_false_positive_diagnostics_broken_value_semantics::run_tests()
  {
    do_allocation_tests(*this);
  }

  template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void allocation_false_positive_diagnostics_broken_value_semantics::test_allocation()
  {
    test_regular_semantics<PropagateCopy, PropagateMove, PropagateSwap>();
  }

  template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void allocation_false_positive_diagnostics_broken_value_semantics::test_regular_semantics()
  {
      auto mutator{ [](auto& b) { b.x.push_back(1); } };

      {
        using handle = std::shared_ptr<int>;
        using beast = broken_copy_value_semantics<int, handle, shared_counting_allocator<handle, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;
        using getter = typename beast::alloc_acquirer;

        static_assert(std::is_same_v<alloc_equivalence_class_generator_t<beast, getter>, allocation_equivalence_classes::container_of_pointers<allocator>>);

        auto m{
          [](beast& b) {
            *b.x.front() = 9;
          }
        };

        check_semantics(report_line("Broken copy value semantics"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, m, allocation_info{getter{}, {1_c, {1_c,0_mu}, {1_anp,1_awp}}});
      }

      {
        using handle = std::shared_ptr<int>;
        using beast = broken_copy_assignment_value_semantics<int, handle, shared_counting_allocator<handle, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;

        auto allocGetter{
          [](const beast& b){ return b.x.get_allocator(); }
        };

        auto m{
          [](beast& b) {
            *b.x.front() = 9;
          }
        };

        check_semantics(report_line("Broken copy assignment value semantics"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, m, allocation_info{allocGetter, {1_c, {1_c,0_mu}, {1_anp,1_awp}}});
      }

      {
        if constexpr(PropagateCopy != PropagateMove)
        {
          using beast = broken_copy_assignment_propagation<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
          using allocator = typename beast::allocator_type;

          auto allocGetter{[](const beast& b) { return b.x.get_allocator(); }};

          check_semantics(report_line("Broken copy assignment propagation"), beast{{1,2}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info{allocGetter, {1_c, {1_c,1_mu}, {0_anp,1_awp}}});
          check_semantics(report_line("Broken copy assignment propagation; 'negative' allocation counts"),
                          beast{{1,2}, allocator{}}, beast{}, mutator, allocation_info{allocGetter, {1_c, {0_c,1_mu}, {0_anp,0_awp}}});
        }
      }
  }
}
