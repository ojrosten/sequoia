////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "RegularAllocationTestFalsePositiveDiagnostics.hpp"

#include "../ContainerDiagnosticsUtilities.hpp"
#include "AllocationTestDiagnosticsUtilities.hpp"

#include <vector>

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path allocation_false_positive_diagnostics::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void allocation_false_positive_diagnostics::run_tests()
  {
    do_allocation_tests();
  }

  template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void allocation_false_positive_diagnostics::test_allocation()
  {
    test_regular_semantics<PropagateCopy, PropagateMove, PropagateSwap>();
  }

  template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void allocation_false_positive_diagnostics::test_regular_semantics()
  {
    {
      using beast = perfectly_normal_beast<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
      using allocator = typename beast::allocator_type;

      auto allocGetter{ [](const beast& b){ return b.x.get_allocator(); } };
      auto mutator{[](auto& b) { b.x.push_back(1); }};

      check_semantics("Broken check invariant", beast{{1}, allocator{}}, beast{{1}, allocator{}}, mutator, allocation_info{allocGetter, {0_c, {0_c,1_mu}, {0_anp,0_awp}}});

      check_semantics("Incorrect init allocs",
                    [](){ return beast{{1,2}}; },
                    [](){ return beast{{5}, allocator{}}; },
                    mutator,
                    allocation_info{allocGetter, {2_c, {2_c,1_mu}, {0_anp,1_awp}}});
    }

    {
      using beast = perfectly_normal_beast<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
      using allocator = typename beast::allocator_type;

      auto allocGetter{ [](const beast& b){ return b.x.get_allocator(); } };
      auto mutator{[](auto& b) { b.x.push_back(1); }};

      check_semantics("Incorrect copy x allocs", beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info{allocGetter, {0_c, {1_c,1_mu}, {1_anp,1_awp}}});

      check_semantics("Incorrect copy y allocs", beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info{allocGetter, {1_c, {0_c,1_mu}, {1_anp,1_awp}}});

      check_semantics("Incorrect mutation allocs", beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info{allocGetter, {1_c, {1_c,0_mu}, {1_anp,1_awp}}});

      auto predictions{
        []() -> allocation_predictions {
          if constexpr(!std::allocator_traits<shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>::propagate_on_container_copy_assignment::value)
          {
            return {1_c, {1_c,1_mu}, {0_anp,1_awp}};
          }
          else
          {
            return {1_c, {1_c,1_mu}, {1_anp,0_awp}};
          }
        }
      };

      check_semantics("Incorrect copy assign y to x allocs", beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info{allocGetter, predictions()});
    }

    {
      using handle = std::shared_ptr<int>;
      using beast = perfectly_sharing_beast<int, handle, shared_counting_allocator<handle, PropagateCopy, PropagateMove, PropagateSwap>>;
      using allocator = typename beast::allocator_type;
      using getter = typename beast::alloc_acquirer;

      auto m{ [](beast& b) { *b.x.front() = 9; } };

      check_semantics("Incorrect copy x allocs", beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, m, allocation_info{getter{}, {0_c, {1_c,0_mu}, {1_anp,1_awp}}});

      check_semantics("Incorrect copy y allocs", beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, m, allocation_info{getter{}, {1_c, {0_c,0_mu}, {1_anp,1_awp}}});

      check_semantics("Incorrect mutation allocs", beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, m, allocation_info{getter{}, {1_c, {1_c,1_mu}, {1_anp,1_awp}}});

      auto predictions{
        []() -> allocation_predictions {
          if constexpr(!std::allocator_traits<shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>::propagate_on_container_copy_assignment::value)
          {
            return {1_c, {1_c,0_mu}, {0_anp,1_awp}};
          }
          else
          {
            return {1_c, {1_c,0_mu}, {1_anp,0_awp}};
          }
        }
      };

      check_semantics("Incorrect copy assign y to x allocs", beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, m, allocation_info{getter{}, predictions()});
    }
  }
}
