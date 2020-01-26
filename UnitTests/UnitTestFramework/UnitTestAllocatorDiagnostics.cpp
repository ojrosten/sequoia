////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "UnitTestDiagnosticsUtilities.hpp"
#include "UnitTestAllocatorDiagnostics.hpp"

namespace sequoia::unit_testing
{
  void allocator_false_positive_diagnostics::run_tests()
  {
    do_allocation_tests(*this);
    do_move_only_allocation_tests(*this);
  }

  template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void allocator_false_positive_diagnostics::test_allocation()
  {
    test_regular_semantics<PropagateCopy, PropagateMove, PropagateSwap>();
  }

  template<bool PropagateMove, bool PropagateSwap>
  void allocator_false_positive_diagnostics::test_move_only_allocation()
  {
    test_move_only_semantics_allocations<PropagateMove, PropagateSwap>();
  }

  template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void allocator_false_positive_diagnostics::test_regular_semantics()
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
        
        
        check_regular_semantics(LINE("Broken equality"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info<beast, allocator>{allocGetter, {1, {1,1}, {1,1}}});
      }

      {
        using beast = broken_inequality<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;
        
        
        check_regular_semantics(LINE("Broken inequality"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info<beast, allocator>{allocGetter, {1, {1,1}, {1,1}}});
      }

      {
        using beast = inefficient_equality<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;
        
        
        check_regular_semantics(LINE("Broken equality"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info<beast, allocator>{allocGetter, {1, {1,1}, {1,1}}});
      }
      

      {
        using beast = broken_copy<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;
        
        
        check_regular_semantics(LINE("Broken copy"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info<beast, allocator>{allocGetter, {1, {1,1}, {1,1}}});
      }

      {
        using beast = broken_copy_alloc<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;
        
        
        check_regular_semantics(LINE("Broken copy alloc"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info<beast, allocator>{allocGetter, {1, {1,1}, {1,1}}});
      }

      {
        using beast = broken_move<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;
        
        
        check_regular_semantics(LINE("Broken move"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info<beast, allocator>{allocGetter, {1, {1,1}, {1,1}}});
      }

      {
        using beast = broken_move_alloc<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;
        
        
        check_regular_semantics(LINE("Broken move alloc"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info<beast, allocator>{allocGetter, {1, {1,1}, {1,1}}});
      }

      {
        using beast = inefficient_move<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;
        
        
        check_regular_semantics(LINE("Inefficient move"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info<beast, allocator>{allocGetter, {1, {1,1}, {1,1}}});
      }

      {
        using beast = broken_copy_assignment<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;
        
        
        check_regular_semantics(LINE("Broken copy assignment"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info<beast, allocator>{allocGetter, {1, {1,1}, {1,1}}});
      }

      {
        using beast = broken_move_assignment<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;
        
        
        check_regular_semantics(LINE("Broken move assignment"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info<beast, allocator>{allocGetter, {1, {1,1}, {1,1}}});
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
        
        
        check_regular_semantics(LINE("Broken copy value semantics"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, m, allocation_info<beast, allocator>{allocGetter, {1, {1,0}, {1,1}}});
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
        
        
        check_regular_semantics(LINE("Broken copy assignment value semantics"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, m, allocation_info<beast, allocator>{allocGetter, {1, {1,0}, {1,1}}});
      }

      {
        using beast = inefficient_copy<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;

        auto mutator{
          [](beast& b) {
            b.x.reserve(3);
            b.x.push_back(1);
          }
        };

        
        check_regular_semantics(LINE(""), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info<beast, allocator>{allocGetter, {2, {3,1,1,1}, {1,1}}});
        check_regular_semantics(LINE(""), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info<beast, allocator>{allocGetter, {3, {2,1,1,1}, {1,1}}});
      }

      {
        using beast = inefficient_copy_alloc<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;

        auto mutator{
          [](beast& b) {
            b.x.reserve(3);
            b.x.push_back(1);
          }
        };

        
        check_regular_semantics(LINE(""), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info<beast, allocator>{allocGetter, {1, {1,1,3,1}, {1,1}}});
      }

      {
        using beast = perfectly_normal_beast<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;


        
        check_regular_semantics(LINE("Broken check invariant"), beast{{1}, allocator{}}, beast{{1}, allocator{}}, mutator, allocation_info<beast, allocator>{allocGetter, {0, {0,1}, {0,0}}});
      }

      {
        using beast = perfectly_normal_beast<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;

        {
          
          check_regular_semantics(LINE("Incorrect copy x allocs"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info<beast, allocator>{allocGetter, {0, {1,1}, {1,1}}});
        }
        {
          
          check_regular_semantics(LINE("Incorrect copy y allocs"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info<beast, allocator>{allocGetter, {1, {0,1}, {1,1}}});
        }
        {
          allocation_predictions predictions{1, {1,1}, {0,1}};
          if constexpr(!std::allocator_traits<shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>::propagate_on_container_copy_assignment::value)
          {
            predictions = allocation_predictions{1, {1,1}, {1,0}};
          }

          
          check_regular_semantics(LINE("Incorrect copy assign y to x allocs"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info<beast, allocator>{allocGetter, predictions});
        }
        {
          
          check_regular_semantics(LINE("Incorrect mutation allocs"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info<beast, allocator>{allocGetter, {1, {1,0}, {1,1}}});
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
          
          check_regular_semantics(LINE("Incorrect copy x allocs"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, m, allocation_info<beast, allocator>{allocGetter, {0, {1,0}, {1,1}}});
        }
        {
          
          check_regular_semantics(LINE("Incorrect copy y allocs"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, m, allocation_info<beast, allocator>{allocGetter, {1, {0,0}, {1,1}}});
        }
        {
          allocation_predictions predictions{1, {1,0}, {0,1}};
          if constexpr(!std::allocator_traits<shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>::propagate_on_container_copy_assignment::value)
          {
            predictions = allocation_predictions{1, {1,0}, {1,0}};
          }

          
          check_regular_semantics(LINE("Incorrect copy assign y to x allocs"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, m, allocation_info<beast, allocator>{allocGetter, predictions});
        }
        {
          
          check_regular_semantics(LINE("Incorrect mutation allocs"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, m, allocation_info<beast, allocator>{allocGetter, {1, {1,1}, {1,1}}});
        }
      }
    }    
  }

  template<bool PropagateMove, bool PropagateSwap>
  void allocator_false_positive_diagnostics::test_move_only_semantics_allocations()
  {
    check_regular_semantics(LINE(""), broken_equality{1}, broken_equality{2}, broken_equality{1}, broken_equality{2});

    check_regular_semantics(LINE(""), broken_inequality{1}, broken_inequality{2}, broken_inequality{1}, broken_inequality{2});

    check_regular_semantics(LINE(""), broken_move{1}, broken_move{2}, broken_move{1}, broken_move{2});

    check_regular_semantics(LINE(""), broken_move_assignment{1}, broken_move_assignment{2}, broken_move_assignment{1}, broken_move_assignment{2});

    check_regular_semantics(LINE(""), perfectly_normal_beast{1}, perfectly_normal_beast{1}, perfectly_normal_beast{1}, perfectly_normal_beast{1});

    check_regular_semantics(LINE(""), perfectly_normal_beast{1}, perfectly_normal_beast{3}, perfectly_normal_beast{2}, perfectly_normal_beast{3});

    check_regular_semantics(LINE(""), perfectly_normal_beast{1}, perfectly_normal_beast{2}, perfectly_normal_beast{3}, perfectly_normal_beast{2});

    {
      using beast = perfectly_normal_beast<int, shared_counting_allocator<int, true, PropagateMove, PropagateSwap>>;
      auto allocGetter{
        [](const beast& beast){
          return beast.x.get_allocator();
        }
      };
      
      check_regular_semantics(LINE(""), beast{1}, beast{2}, beast{1}, beast{2}, move_only_allocation_info{allocGetter, move_only_allocation_predictions{0}});
    }
  }

  void allocator_false_negative_diagnostics::run_tests()
  {
    do_allocation_tests(*this);
    do_move_only_allocation_tests(*this);
  }

  template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void allocator_false_negative_diagnostics::test_allocation()
  {
    test_regular_semantics<PropagateCopy, PropagateMove, PropagateSwap>();
  }

  template<bool PropagateMove, bool PropagateSwap>
  void allocator_false_negative_diagnostics::test_move_only_allocation()
  {
    test_move_only_semantics_allocations<PropagateMove, PropagateSwap>();
  }

  template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void allocator_false_negative_diagnostics::test_regular_semantics()
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
     
      check_regular_semantics(LINE(""), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info<beast, allocator>{allocGetter, {1, {1,1}, {1,1}}});

      check_regular_semantics(LINE(""), beast(allocator{}), beast{{5,6}, allocator{}}, mutator, allocation_info<beast, allocator>{allocGetter, {0, {1,1}, {1,1}}});
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
        
      
      check_regular_semantics(LINE(""), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, m, allocation_info<beast, allocator>{allocGetter, {1, {1,0}, {1, 1}}});

      check_regular_semantics(LINE(""), beast(allocator{}), beast{{5,6}, allocator{}}, m, allocation_info<beast, allocator>{allocGetter, {0, {1,0}, {1, 1}}});
    }

    {
      using beast = inefficient_copy<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
      using allocator = typename beast::allocator_type;

      auto mutator{
        [](beast& b) {
          b.x.reserve(3);
          b.x.push_back(1);
        }
      };

      
      check_regular_semantics(LINE(""), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info<beast, allocator>{allocGetter, {2, {2,1,1,1}, {1,1}}});
    }

    {
      using beast = inefficient_copy_alloc<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
      using allocator = typename beast::allocator_type;

      auto mutator{
        [](beast& b) {
          b.x.shrink_to_fit();
          b.x.reserve(3);
          b.x.push_back(1);
        }
      };

      check_regular_semantics(LINE(""), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info<beast, allocator>{allocGetter, {1, {1,1,2,1}, {1,1}}});
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
      
      check_regular_semantics(LINE("")
                              , beast{{1}, {1}, xAllocator{}, yAllocator{}}
                              , beast{{5,6}, {5,6}, xAllocator{}, yAllocator{}}
                              , mutator
                              , allocation_info<beast, xAllocator>{allocGetter, {1, {1,1}, {1,1}}}
                              , allocation_info{yAllocGetter, {1, {1,1}, {1,1}}});
    }
  }

  template<bool PropagateMove, bool PropagateSwap>
  void allocator_false_negative_diagnostics::test_move_only_semantics_allocations()
  {
    {
      using beast = perfectly_normal_beast<int, shared_counting_allocator<int, true, PropagateMove, PropagateSwap>>;
      auto allocGetter{
        [](const beast& beast){
          return beast.x.get_allocator();
        }
      };

      check_regular_semantics(LINE(""), beast{1}, beast{2}, beast{1}, beast{2}, move_only_allocation_info{allocGetter, move_only_allocation_predictions{1}});
    }
  }
}
