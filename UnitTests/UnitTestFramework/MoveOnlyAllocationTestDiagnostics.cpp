////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "MoveOnlyAllocationTestDiagnostics.hpp"
#include "MoveOnlyTestDiagnosticUtilities.hpp"

#include <vector>

namespace sequoia::unit_testing
{
  [[nodiscard]]
  std::string_view move_only_allocation_false_positive_diagnostics::source_file_name() const noexcept
  {
    return __FILE__;
  }

  void move_only_allocation_false_positive_diagnostics::run_tests()
  {
    do_allocation_tests(*this);
  }
  
  template<bool PropagateMove, bool PropagateSwap>
  void move_only_allocation_false_positive_diagnostics::test_allocation()
  {
    test_move_only_semantics_allocations<PropagateMove, PropagateSwap>();
  }

  template<bool PropagateMove, bool PropagateSwap>
  void move_only_allocation_false_positive_diagnostics::test_move_only_semantics_allocations()
  {
    {
      using beast = move_only_beast<int, shared_counting_allocator<int, true, PropagateMove, PropagateSwap>>;
      auto allocGetter{
        [](const beast& beast){
          return beast.x.get_allocator();
        }
      };

      check_regular_semantics(LINE(""), beast{1}, beast{2}, beast{1}, beast{2},
                              move_only_allocation_info{allocGetter, move_only_allocation_predictions{0, {0, 1}}});
    }

    {
      using beast = move_only_broken_move<int, shared_counting_allocator<int, true, PropagateMove, PropagateSwap>>;
      auto allocGetter{
        [](const beast& beast){
          return beast.x.get_allocator();
        }
      };
      
      check_regular_semantics(LINE(""), beast{1}, beast{2}, beast{1}, beast{2},
                              move_only_allocation_info{allocGetter, move_only_allocation_predictions{1, {0, 1}}});
    }

    {
      using beast = move_only_broken_move_assignment<int, shared_counting_allocator<int, true, PropagateMove, PropagateSwap>>;
      auto allocGetter{
        [](const beast& beast){
          return beast.x.get_allocator();
        }
      };
      
      check_regular_semantics(LINE(""), beast{1}, beast{2}, beast{1}, beast{2},
                              move_only_allocation_info{allocGetter, move_only_allocation_predictions{1, {0, 1}}});
    }

    
    if constexpr(PropagateSwap)
    {
      using beast = move_only_broken_swap<int, shared_counting_allocator<int, true, PropagateMove, PropagateSwap>>;
      auto allocGetter{
        [](const beast& beast){
          return beast.x.get_allocator();
        }
      };
      
      check_regular_semantics(LINE(""), beast{1}, beast{2}, beast{1}, beast{2},
                              move_only_allocation_info{allocGetter, move_only_allocation_predictions{1, {0, 1}}});
    }

    {
      using beast = move_only_inefficient_move<int, shared_counting_allocator<int, true, PropagateMove, PropagateSwap>>;
      auto allocGetter{
        [](const beast& beast){
          return beast.x.get_allocator();
        }
      };
      
      check_regular_semantics(LINE(""), beast{1}, beast{2}, beast{1}, beast{2},
                              move_only_allocation_info{allocGetter, move_only_allocation_predictions{1, {0, 1}}});
    }

    {
      using beast = move_only_inefficient_move_assignment<int, shared_counting_allocator<int, true, PropagateMove, PropagateSwap>>;
      auto allocGetter{
        [](const beast& beast){
          return beast.x.get_allocator();
        }
      };
      
      check_regular_semantics(LINE(""), beast{1}, beast{2}, beast{1}, beast{2},
                              move_only_allocation_info{allocGetter, move_only_allocation_predictions{1, {0, 1}}});
    }
  }

  [[nodiscard]]
  std::string_view move_only_allocation_false_negative_diagnostics::source_file_name() const noexcept
  {
    return __FILE__;
  }

  void move_only_allocation_false_negative_diagnostics::run_tests()
  {
    do_allocation_tests(*this);
  }

  template<bool PropagateMove, bool PropagateSwap>
  void move_only_allocation_false_negative_diagnostics::test_allocation()
  {
    test_move_only_semantics_allocations<PropagateMove, PropagateSwap>();
  }

  template<bool PropagateMove, bool PropagateSwap>
  void move_only_allocation_false_negative_diagnostics::test_move_only_semantics_allocations()
  {
    {
      using beast = move_only_beast<int, shared_counting_allocator<int, true, PropagateMove, PropagateSwap>>;
      auto allocGetter{
        [](const beast& beast){
          return beast.x.get_allocator();
        }
      };

      check_regular_semantics(LINE(""), beast{1}, beast{2}, beast{1}, beast{2},
                              move_only_allocation_info{allocGetter, move_only_allocation_predictions{1, {0, 1}}});
    }
  }
}
