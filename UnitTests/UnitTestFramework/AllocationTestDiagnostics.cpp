////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "UnitTestDiagnosticsUtilities.hpp"
#include "AllocationTestDiagnostics.hpp"

#include <vector>

namespace sequoia::unit_testing
{
  [[nodiscard]]
  std::string_view allocation_false_positive_diagnostics::source_file_name() const noexcept
  {
    return __FILE__;
  }

  void allocation_false_positive_diagnostics::run_tests()
  {
    do_allocation_tests(*this);
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
      auto allocGetter{
        [](const auto& beast){
          return beast.x.get_allocator();
        }
      };
      
      auto mutator{
        [](auto& b) {
          b.x.push_back(1);
        }
      };

      {
        using beast = broken_equality<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;
        
        check_semantics(LINE("Broken equality"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info<beast, allocator>{allocGetter, {1_c, {1_c,1_mu}, {1_awp,1_anp}}});
      }

      {
        using beast = broken_inequality<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;
        
        
        check_semantics(LINE("Broken inequality"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info<beast, allocator>{allocGetter, {1_c, {1_c,1_mu}, {1_awp,1_anp}}});
      }

      {
        using beast = inefficient_equality<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;
        
        
        check_semantics(LINE("Broken equality"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info<beast, allocator>{allocGetter, {1_c, {1_c,1_mu}, {1_awp,1_anp}}});
      }
      

      {
        using beast = broken_copy<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;
        
        
        check_semantics(LINE("Broken copy"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info<beast, allocator>{allocGetter, {1_c, {1_c,1_mu}, {1_awp,1_anp}}});
      }

      {
        using beast = broken_copy_alloc<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;
        
        
        check_semantics(LINE("Broken copy alloc"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info<beast, allocator>{allocGetter, {1_c, {1_c,1_mu}, {1_awp,1_anp}}});
      }

      {
        using beast = broken_move<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;
        
        
        check_semantics(LINE("Broken move"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info<beast, allocator>{allocGetter, {1_c, {1_c,1_mu}, {1_awp,1_anp}}});
      }

      {
        using beast = broken_move_alloc<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;
        
        
        check_semantics(LINE("Broken move alloc"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info<beast, allocator>{allocGetter, {1_c, {1_c,1_mu}, {1_awp,1_anp}}});
      }

      {
        using beast = inefficient_move<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;
        
        
        check_semantics(LINE("Inefficient move"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info<beast, allocator>{allocGetter, {1_c, {1_c,1_mu}, {1_awp,1_anp}}});
      }

      {
        using beast = broken_copy_assignment<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;
        
        
        check_semantics(LINE("Broken copy assignment"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info<beast, allocator>{allocGetter, {1_c, {1_c,1_mu}, {1_awp,1_anp}}});
      }

      {
        using beast = broken_move_assignment<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;
        
        
        check_semantics(LINE("Broken move assignment"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info<beast, allocator>{allocGetter, {1_c, {1_c,1_mu}, {1_awp,1_anp}}});
      }

      {
        using handle = std::shared_ptr<int>;
        using beast = broken_copy_value_semantics<int, handle, shared_counting_allocator<handle, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;

        auto m{
          [](beast& b) {
            *b.x.front() = 9;
          }
        };
        
        check_semantics(LINE("Broken copy value semantics"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, m, allocation_info<beast, allocator>{allocGetter, {1_c, {1_c,0_mu}, {1_awp,1_anp}}});
      }

      {
        using handle = std::shared_ptr<int>;
        using beast = broken_copy_assignment_value_semantics<int, handle, shared_counting_allocator<handle, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;

        auto m{
          [](beast& b) {
            *b.x.front() = 9;
          }
        };
        
        
        check_semantics(LINE("Broken copy assignment value semantics"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, m, allocation_info<beast, allocator>{allocGetter, {1_c, {1_c,0_mu}, {1_awp,1_anp}}});
      }

      {
        using beast = inefficient_copy<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;

        auto mutator{
          [](beast& b) {
            b.x.reserve(b.x.capacity() + 1);
            b.x.push_back(1);
          }
        };

        
        check_semantics(LINE(""), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info<beast, allocator>{allocGetter, {2_c, {3_c,1_mu,1_pc,1_pm}, {1_awp,1_anp}}});
        check_semantics(LINE(""), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info<beast, allocator>{allocGetter, {3_c, {2_c,1_mu,1_pc,1_pm}, {1_awp,1_anp}}});
      }

      {
        using beast = inefficient_copy_alloc<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;

        auto mutator{
          [](beast& b) {
            b.x.reserve(b.x.capacity() + 1);
            b.x.push_back(1);
          }
        };

        
        check_semantics(LINE(""), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info<beast, allocator>{allocGetter, {1_c, {1_c,1_mu,3_pc,1_pm}, {1_awp,1_anp}}});
      }

      {
        using beast = perfectly_normal_beast<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;


        
        check_semantics(LINE("Broken check invariant"), beast{{1}, allocator{}}, beast{{1}, allocator{}}, mutator, allocation_info<beast, allocator>{allocGetter, {0_c, {0_c,1_mu}, {0_awp,0_anp}}});
      }

      {
        using beast = perfectly_normal_beast<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;

        {
          
          check_semantics(LINE("Incorrect copy x allocs"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info<beast, allocator>{allocGetter, {0_c, {1_c,1_mu}, {1_awp,1_anp}}});
        }
        {
          
          check_semantics(LINE("Incorrect copy y allocs"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info<beast, allocator>{allocGetter, {1_c, {0_c,1_mu}, {1_awp,1_anp}}});
        }
        {
          allocation_predictions predictions{1_c, {1_c,1_mu}, {0_awp,1_anp}};
          if constexpr(!std::allocator_traits<shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>::propagate_on_container_copy_assignment::value)
          {
            predictions = allocation_predictions{1_c, {1_c,1_mu}, {1_awp,0_anp}};
          }

          
          check_semantics(LINE("Incorrect copy assign y to x allocs"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info<beast, allocator>{allocGetter, predictions});
        }
        {
          
          check_semantics(LINE("Incorrect mutation allocs"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info<beast, allocator>{allocGetter, {1_c, {1_c,0_mu}, {1_awp,1_anp}}});
        }
      }

      {
        using handle = std::shared_ptr<int>;
        using beast = perfectly_sharing_beast<int, handle, shared_counting_allocator<handle, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;

        auto m{
          [](beast& b) {
            *b.x.front() = 9;
          }
        };
        
        {
          
          check_semantics(LINE("Incorrect copy x allocs"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, m, allocation_info<beast, allocator>{allocGetter, {0_c, {1_c,0_mu}, {1_awp,1_anp}}});
        }
        {
          
          check_semantics(LINE("Incorrect copy y allocs"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, m, allocation_info<beast, allocator>{allocGetter, {1_c, {0_c,0_mu}, {1_awp,1_anp}}});
        }
        {
          allocation_predictions predictions{1_c, {1_c,0_mu}, {0_awp,1_anp}};
          if constexpr(!std::allocator_traits<shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>::propagate_on_container_copy_assignment::value)
          {
            predictions = allocation_predictions{1_c, {1_c,0_mu}, {1_awp,0_anp}};
          }

          
          check_semantics(LINE("Incorrect copy assign y to x allocs"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, m, allocation_info<beast, allocator>{allocGetter, predictions});
        }
        {
          
          check_semantics(LINE("Incorrect mutation allocs"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, m, allocation_info<beast, allocator>{allocGetter, {1_c, {1_c,1_mu}, {1_awp,1_anp}}});
        }
      }
    }    
  }

  [[nodiscard]]
  std::string_view allocation_false_negative_diagnostics::source_file_name() const noexcept
  {
    return __FILE__;
  }

  void allocation_false_negative_diagnostics::run_tests()
  {
    do_allocation_tests(*this);
  }

  template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void allocation_false_negative_diagnostics::test_allocation()
  {
    test_regular_semantics<PropagateCopy, PropagateMove, PropagateSwap>();
  }

  template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void allocation_false_negative_diagnostics::test_regular_semantics()
  {
    auto allocGetter{
      [](const auto& beast){
        return beast.x.get_allocator();
      }
    };
    
    {
      using beast = perfectly_normal_beast<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
      using allocator = typename beast::allocator_type;

      auto mutator{
        [](beast& b) {
          b.x.push_back(1);
        }
      };
     
      check_semantics(LINE(""), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info<beast, allocator>{allocGetter, {1_c, {1_c,1_mu}, {1_awp,1_anp}}});

      check_semantics(LINE(""), beast(allocator{}), beast{{5,6}, allocator{}}, mutator, allocation_info<beast, allocator>{allocGetter, {0_c, {1_c,1_mu}, {1_awp,1_anp}}});
    }

    {
      using handle = std::shared_ptr<int>;
      using beast = perfectly_sharing_beast<int, handle, shared_counting_allocator<handle, PropagateCopy, PropagateMove, PropagateSwap>>;
      using allocator = typename beast::allocator_type;

      auto m{
        [](beast& b) {
          *b.x.front() = 9;
        }
      };
        
      
      check_semantics(LINE(""), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, m, allocation_info<beast, allocator>{allocGetter, {1_c, {1_c,0_mu}, {1_awp, 1_anp}}});

      check_semantics(LINE(""), beast(allocator{}), beast{{5,6}, allocator{}}, m, allocation_info<beast, allocator>{allocGetter, {0_c, {1_c,0_mu}, {1_awp, 1_anp}}});
    }

    {
      using beast = inefficient_copy<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
      using allocator = typename beast::allocator_type;

      auto mutator{
        [](beast& b) {
          b.x.reserve(b.x.capacity() + 1);
          b.x.push_back(1);
        }
      };

      
      check_semantics(LINE(""), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info<beast, allocator>{allocGetter, {2_c, {2_c,1_mu,1_pc,1_pm}, {1_awp,1_anp}}});
    }

    {
      using beast = inefficient_copy_alloc<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
      using allocator = typename beast::allocator_type;

      auto mutator{
        [](beast& b) {
          b.x.shrink_to_fit();
          b.x.reserve(b.x.capacity() + 1);
          b.x.push_back(1);
        }
      };

      check_semantics(LINE(""), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info<beast, allocator>{allocGetter, {1_c, {1_c,1_mu,2_pc,1_pm}, {1_awp,1_anp}}});
    }

    
    {
      using beast = doubly_normal_beast<int, double, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>, shared_counting_allocator<double, PropagateCopy, PropagateMove, PropagateSwap>>;
      using xAllocator = typename beast::x_allocator_type;
      using yAllocator = typename beast::y_allocator_type;

      auto yAllocGetter{
        [](const beast& beast){
          return beast.y.get_allocator();
        }
      };
      
      auto mutator{
        [](beast& b) {
          b.x.push_back(1);
          b.y.push_back(1);
        }
      };
      
      check_semantics(LINE("")
                              , beast{{1}, {1}, xAllocator{}, yAllocator{}}
                              , beast{{5,6}, {5,6}, xAllocator{}, yAllocator{}}
                              , mutator
                              , allocation_info<beast, xAllocator>{allocGetter, {1_c, {1_c,1_mu}, {1_awp,1_anp}}}
                              , allocation_info{yAllocGetter, {1_c, {1_c,1_mu}, {1_awp,1_anp}}});
    }
  }

  
}
