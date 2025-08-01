////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "MoveOnlyAllocationTestDiagnostics.hpp"
#include "../MoveOnlyTestDiagnosticsUtilities.hpp"

#include <vector>

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path move_only_allocation_false_negative_diagnostics::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void move_only_allocation_false_negative_diagnostics::run_tests()
  {
    do_allocation_tests();
  }

  template<bool PropagateMove, bool PropagateSwap>
  void move_only_allocation_false_negative_diagnostics::test_allocation()
  {
    test_move_only_semantics_allocations<PropagateMove, PropagateSwap>();
  }

  template<bool PropagateMove, bool PropagateSwap>
  void move_only_allocation_false_negative_diagnostics::test_move_only_semantics_allocations()
  {
    using equivalent_type = std::vector<int, shared_counting_allocator<int, true, PropagateMove, PropagateSwap>>;

    auto mutator{
      [](auto& b) {
        b.x.reserve(b.x.capacity() + 1);
        b.x.push_back(1);
      }
    };

    {
      using beast = move_only_beast<int, shared_counting_allocator<int, true, PropagateMove, PropagateSwap>>;
      auto allocGetter{
        [](const beast& b){
          return b.x.get_allocator();
        }
      };

      check_semantics("Incorrect para-move allocs", beast{}, beast{2}, beast{}, beast{2}, mutator,
                      allocation_info{allocGetter, {0_pm, {0_pm, 1_mu}, {1_manp}}});

      check_semantics("Incorrect para-move allocs, as unique", beast{}, beast{2}, equivalent_type{}, equivalent_type{2}, mutator,
                      allocation_info{allocGetter, {0_pm, {0_pm, 1_mu}, {1_manp}}});

      check_semantics("Incorrect mutation allocs", beast{}, beast{2}, beast{}, beast{2}, mutator,
                      allocation_info{allocGetter, {0_pm, {1_pm, 0_mu}, {1_manp}}});

      if constexpr(!PropagateMove)
      {
        check_semantics("Incorrect assignment allocs", beast{}, beast{2}, beast{}, beast{2}, mutator,
                        allocation_info{allocGetter, {0_pm, {1_pm, 1_mu}, {0_manp}}});
      }

      const auto[x,y]{check_semantics("Move-only beast",
                                      []() { return beast{}; },
                                      []() { return beast{2}; },
                                      mutator,
                                      allocation_info{allocGetter, {0_pm, {2_pm, 1_mu}, {1_manp}}})};

      check(equality, "check_semantics return value (x)", x, beast{2});
      check(equality, "check_semantics return value (y)", y, beast{});
    }

    {
      using beast = move_only_broken_move<int, shared_counting_allocator<int, true, PropagateMove, PropagateSwap>>;
      auto allocGetter{
        [](const beast& b){
          return b.x.get_allocator();
        }
      };

      check_semantics("Broken move", beast{1}, beast{2}, beast{1}, beast{2}, mutator,
                      allocation_info{allocGetter, {1_pm, {1_pm, 1_mu}, {0_manp}}});
    }

    {
      using beast = move_only_broken_move_assignment<int, shared_counting_allocator<int, true, PropagateMove, PropagateSwap>>;
      auto allocGetter{
        [](const beast& b){
          return b.x.get_allocator();
        }
      };

      check_semantics("Broken move assignment", beast{1}, beast{2}, beast{1}, beast{2}, mutator,
                      allocation_info{allocGetter, {1_pm, {1_pm, 1_mu}, {0_manp}}});
    }


    if constexpr(PropagateSwap)
    {
      using beast = move_only_broken_swap<int, shared_counting_allocator<int, true, PropagateMove, PropagateSwap>>;
      auto allocGetter{
        [](const beast& b){
          return b.x.get_allocator();
        }
      };

      check_semantics("Broken swap", beast{1}, beast{2}, beast{1}, beast{2}, mutator,
                      allocation_info{allocGetter, {1_pm, {1_pm, 1_mu}, {0_manp}}});
    }

    {
      using beast = move_only_inefficient_move<int, shared_counting_allocator<int, true, PropagateMove, PropagateSwap>>;
      auto allocGetter{
        [](const beast& b){
          return b.x.get_allocator();
        }
      };

      check_semantics("Inefficient move", beast{1}, beast{2}, beast{1}, beast{2}, mutator,
                      allocation_info{allocGetter, {1_pm, {1_pm, 1_mu}, {0_manp}}});
    }

    {
      using beast = move_only_inefficient_move_assignment<int, shared_counting_allocator<int, true, PropagateMove, PropagateSwap>>;
      auto allocGetter{
        [](const beast& b){
          return b.x.get_allocator();
        }
      };

      check_semantics("Inefficient move assignment", beast{1}, beast{2}, beast{1}, beast{2}, mutator,
                      allocation_info{allocGetter, {1_pm, {1_pm, 1_mu}, {0_manp}}});
    }

    {
      using beast = move_only_beast<int, shared_counting_allocator<int, true, PropagateMove, PropagateSwap>>;

      auto allocGetter{
        [](const beast& b){
          return b.x.get_allocator();
        }
      };

      check_semantics("Invariant violated: x != xClone", beast{1}, beast{2}, beast{3}, beast{2}, mutator,
                      allocation_info{allocGetter, {1_pm, {1_pm, 1_mu}, {0_manp}}});

      check_semantics("Invariant violated: y != YClone", beast{1}, beast{2}, beast{1}, beast{3}, mutator,
                      allocation_info{allocGetter, {1_pm, {1_pm, 1_mu}, {0_manp}}});

      check_semantics("Invariant violated: x == y", beast{1}, beast{1}, beast{1}, beast{1}, mutator,
                      allocation_info{allocGetter, {1_pm, {1_pm, 1_mu}, {0_manp}}});
    }

    {
      using beast = move_only_specified_moved_from_beast<int, shared_counting_allocator<int, true, PropagateMove, PropagateSwap>>;

      auto allocGetter{
        [](const beast& b) {
          return b.x.get_allocator();
        }
      };

      check_semantics("Incorrect moved-from state post construction", beast{1}, beast{2}, beast{1}, beast{2}, beast{3}, beast{}, mutator,
        allocation_info{allocGetter, {1_pm, {1_pm, 1_mu}, {0_manp}}});

      check_semantics("Incorrect moved-from state post construction", []() { return beast{1}; }, []() { return beast{2}; }, beast{3}, beast{}, mutator,
        allocation_info{allocGetter, {1_pm, {1_pm, 1_mu}, {0_manp}}});

      check_semantics("Incorrect moved-from state post assignment", beast{1}, beast{2}, beast{1}, beast{2}, beast{}, beast{3}, mutator,
        allocation_info{allocGetter, {1_pm, {1_pm, 1_mu}, {0_manp}}});

      check_semantics("Incorrect moved-from state post assignment", []() { return beast{1}; }, []() { return beast{2}; }, beast{}, beast{3}, mutator,
        allocation_info{allocGetter, {1_pm, {1_pm, 1_mu}, {0_manp}}});
    }
  }

  [[nodiscard]]
  std::filesystem::path move_only_allocation_false_positive_diagnostics::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void move_only_allocation_false_positive_diagnostics::run_tests()
  {
    do_allocation_tests();
  }

  template<bool PropagateMove, bool PropagateSwap>
  void move_only_allocation_false_positive_diagnostics::test_allocation()
  {
    test_move_only_semantics_allocations<PropagateMove, PropagateSwap>();
  }

  template<bool PropagateMove, bool PropagateSwap>
  void move_only_allocation_false_positive_diagnostics::test_move_only_semantics_allocations()
  {
    using equivalent_type = std::vector<int, shared_counting_allocator<int, true, PropagateMove, PropagateSwap>>;

    auto mutator{
      [](auto& b) {
        b.x.reserve(b.x.capacity() + 1);
        b.x.push_back(1);
      }
    };

    {
      using beast = move_only_beast<int, shared_counting_allocator<int, true, PropagateMove, PropagateSwap>>;
      auto allocGetter{
        [](const beast& b){
          return b.x.get_allocator();
        }
      };

      check_semantics("Move-only beast", beast{1}, beast{2}, beast{1}, beast{2}, mutator,
                      allocation_info{allocGetter, {1_pm, {1_pm, 1_mu}, {0_manp}}});

      check_semantics("Move-only beast, as unique", beast{1}, beast{2}, equivalent_type{1}, equivalent_type{2}, mutator,
                      allocation_info{allocGetter, {1_pm, {1_pm, 1_mu}, {0_manp}}});
      
      check_semantics("Move-only beast", beast{}, beast{2}, beast{}, beast{2}, mutator,
                      allocation_info{allocGetter, {0_pm, {1_pm, 1_mu}, {1_manp}}});

      const auto[x,y]{check_semantics("Move-only beast",
                                      []() { return beast{}; },
                                      []() { return beast{2}; },
                                      mutator,
                                      allocation_info{allocGetter, {0_pm, {1_pm, 1_mu}, {1_manp}}})};

      check(equality, "check_semantics return value (x)", x, beast{});
      check(equality, "check_semantics return value (y)", y, beast{2});
    }

    {
      using beast = move_only_specified_moved_from_beast<int, shared_counting_allocator<int, true, PropagateMove, PropagateSwap>>;
      auto allocGetter{
        [](const beast& b) {
          return b.x.get_allocator();
        }
      };

      check_semantics("Check moved-from state", beast{1}, beast{2}, beast{1}, beast{2}, beast{}, beast{},  mutator,
        allocation_info{allocGetter, {1_pm, {1_pm, 1_mu}, {0_manp}}});

      check_semantics("Check moved-from state, as unique", beast{1}, beast{2}, equivalent_type{1}, equivalent_type{2}, equivalent_type{}, equivalent_type{},  mutator,
        allocation_info{allocGetter, {1_pm, {1_pm, 1_mu}, {0_manp}}});

      check_semantics("Check moved-from state", []() { return beast{1}; }, []() { return beast{2}; }, beast{}, beast{}, mutator,
        allocation_info{allocGetter, {1_pm, {1_pm, 1_mu}, {0_manp}}});
    }
  }
}
