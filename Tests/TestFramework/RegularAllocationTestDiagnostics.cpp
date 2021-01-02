////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "RegularAllocationTestDiagnostics.hpp"

#include "CoreDiagnosticsUtilities.hpp"
#include "AllocationTestDiagnosticsUtilities.hpp"

#include <vector>

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view allocation_false_positive_diagnostics::source_file() const noexcept
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
      auto mutator{
        [](auto& b) {
          b.x.push_back(1);
        }
      };

      {
        using beast = broken_equality<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;

        auto allocGetter{
          [](const beast& b){ return b.x.get_allocator(); }
        };

        check_semantics(LINE("Broken equality"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info{allocGetter, {1_c, {1_c,1_mu}, {1_awp,1_anp}}});
      }

      {
        using beast = broken_inequality<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;

        auto allocGetter{
          [](const beast& b){ return b.x.get_allocator(); }
        };

        check_semantics(LINE("Broken inequality"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info{allocGetter, {1_c, {1_c,1_mu}, {1_awp,1_anp}}});
      }

      {
        using beast = inefficient_equality<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;

        auto allocGetter{
          [](const beast& b){ return b.x.get_allocator(); }
        };
        // TO DO: add advice reminding the since comparison consistency uses x == x, y allocations are unaffected
        check_semantics(LINE("Inefficient equality"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info{allocGetter, {1_c, {1_c,1_mu}, {1_awp,1_anp}}});
      }

      {
        using beast = inefficient_inequality<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;

        auto allocGetter{
          [](const beast& b){ return b.x.get_allocator(); }
        };
        
        check_semantics(LINE("Inefficient inequality"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info{allocGetter, {1_c, {1_c,1_mu}, {1_awp,1_anp}}});
      }

      {
        using beast = broken_copy<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;

        auto allocGetter{
         [](const beast& b){ return b.x.get_allocator(); }
        };

        check_semantics(LINE("Broken copy"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info{allocGetter, {1_c, {1_c,1_mu}, {1_awp,1_anp}}});
      }

      {
        using beast = broken_para_copy<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;

        auto allocGetter{
          [](const beast& b){ return b.x.get_allocator(); }
        };

        check_semantics(LINE("Broken para-copy"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info{allocGetter, {1_c, {1_c,1_mu}, {1_awp,1_anp}}});
      }

      {
        using beast = broken_move<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;

        auto allocGetter{
          [](const beast& b){ return b.x.get_allocator(); }
        };

        check_semantics(LINE("Broken move"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info{allocGetter, {1_c, {1_c,1_mu}, {1_awp,1_anp}}});
      }

      {
        using beast = broken_para_move<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;

        auto allocGetter{
          [](const beast& b){ return b.x.get_allocator(); }
        };
        
        check_semantics(LINE("Broken para-move"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info{allocGetter, {1_c, {1_c,1_mu}, {1_awp,1_anp}}});
      }

      {
        using beast = broken_copy_assignment<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;

        auto allocGetter{
          [](const beast& b){ return b.x.get_allocator(); }
        };

        check_semantics(LINE("Broken copy assignment"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info{allocGetter, {1_c, {1_c,1_mu}, {1_awp,1_anp}}});
      }

      {
        using beast = broken_move_assignment<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;

        auto allocGetter{
          [](const beast& b){ return b.x.get_allocator(); }
        };

        check_semantics(LINE("Broken move assignment"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info{allocGetter, {1_c, {1_c,1_mu}, {1_awp,1_anp}}});
      }

      {
        using handle = std::shared_ptr<int>;
        using beast = broken_copy_value_semantics<int, handle, shared_counting_allocator<handle, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;
        using getter = typename beast::alloc_acquirer;

        auto m{
          [](beast& b) {
            *b.x.front() = 9;
          }
        };

        check_semantics(LINE("Broken copy value semantics"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, m, allocation_info{getter{}, {1_c, {1_c,0_mu}, {1_awp,1_anp}}});
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

        check_semantics(LINE("Broken copy assignment value semantics"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, m, allocation_info{allocGetter, {1_c, {1_c,0_mu}, {1_awp,1_anp}}});
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

        check_semantics(LINE("Inefficient copy"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator2, allocation_info{allocGetter, {2_c, {3_c,1_mu,1_pc,1_pm}, {1_awp,1_anp}}});
        check_semantics(LINE("Inefficient copy"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator2, allocation_info{allocGetter, {3_c, {2_c,1_mu,1_pc,1_pm}, {1_awp,1_anp}}});
      }

      {
        using beast = inefficient_move<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;

        auto allocGetter{
          [](const beast& b){ return b.x.get_allocator(); }
        };
                
        check_semantics(LINE("Inefficient move"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info{allocGetter, {1_c, {1_c,1_mu}, {1_awp,1_anp}}});
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
       
        check_semantics(LINE("Inefficient para-copy"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator2, allocation_info{allocGetter, {1_c, {1_c,1_mu,3_pc,1_pm}, {1_awp,1_anp}}});
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
        
        check_semantics(LINE("Inefficient para-move"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator2, allocation_info{allocGetter, {1_c, {1_c,1_mu,1_pc,3_pm}, {1_awp,1_anp}}});
      }
      
      {
        using beast = perfectly_normal_beast<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;

        auto allocGetter{
          [](const beast& b){ return b.x.get_allocator(); }
        };
      
        check_semantics(LINE("Broken check invariant"), beast{{1}, allocator{}}, beast{{1}, allocator{}}, mutator, allocation_info{allocGetter, {0_c, {0_c,1_mu}, {0_awp,0_anp}}});
      }

      {
        using beast = perfectly_normal_beast<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;

        auto allocGetter{
          [](const beast& b){ return b.x.get_allocator(); }
        };

        check_semantics(LINE("Incorrect copy x allocs"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info{allocGetter, {0_c, {1_c,1_mu}, {1_awp,1_anp}}});

        check_semantics(LINE("Incorrect copy y allocs"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info{allocGetter, {1_c, {0_c,1_mu}, {1_awp,1_anp}}});

        check_semantics(LINE("Incorrect mutation allocs"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info{allocGetter, {1_c, {1_c,0_mu}, {1_awp,1_anp}}});

        auto predictions{
          []() -> allocation_predictions {
            if constexpr(!std::allocator_traits<shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>::propagate_on_container_copy_assignment::value)
            {
              return {1_c, {1_c,1_mu}, {1_awp,0_anp}};
            }
            else
            {
              return {1_c, {1_c,1_mu}, {0_awp,1_anp}};
            }
          }
        };

        check_semantics(LINE("Incorrect copy assign y to x allocs"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info{allocGetter, predictions()});
      }

      {
        using handle = std::shared_ptr<int>;
        using beast = perfectly_sharing_beast<int, handle, shared_counting_allocator<handle, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;
        using getter = typename beast::alloc_acquirer;

        auto m{
          [](beast& b) {
            *b.x.front() = 9;
          }
        };

        check_semantics(LINE("Incorrect copy x allocs"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, m, allocation_info{getter{}, {0_c, {1_c,0_mu}, {1_awp,1_anp}}});

        check_semantics(LINE("Incorrect copy y allocs"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, m, allocation_info{getter{}, {1_c, {0_c,0_mu}, {1_awp,1_anp}}});

        check_semantics(LINE("Incorrect mutation allocs"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, m, allocation_info{getter{}, {1_c, {1_c,1_mu}, {1_awp,1_anp}}});

        auto predictions{
          []() -> allocation_predictions {
            if constexpr(!std::allocator_traits<shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>::propagate_on_container_copy_assignment::value)
            {
              return {1_c, {1_c,0_mu}, {1_awp,0_anp}};
            }
            else
            {
              return {1_c, {1_c,0_mu}, {0_awp,1_anp}};
            }
          }
        };

        check_semantics(LINE("Incorrect copy assign y to x allocs"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, m, allocation_info{getter{}, predictions()});
      }


      {
        using beast = inefficient_serialization<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;

        auto allocGetter{ [](const beast& b){ return b.x.get_allocator(); } };

        check_semantics(LINE("Inefficient serialization"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info{allocGetter, {1_c, {1_c,1_mu}, {1_awp,1_anp}}});
      }

      {
        if constexpr(PropagateCopy != PropagateMove)
        {
          using beast = broken_copy_assignment_propagation<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
          using allocator = typename beast::allocator_type;

          auto allocGetter{[](const beast& b) { return b.x.get_allocator(); }};

          check_semantics(LINE("Broken copy assignment propagation"), beast{{1,2}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info{allocGetter, {1_c, {1_c,1_mu}, {1_awp,0_anp}}});
          check_semantics(LINE("Broken copy assignment propagation; 'negative' allocation counts"),
                          beast{{1,2}, allocator{}}, beast{}, mutator, allocation_info{allocGetter, {1_c, {0_c,1_mu}, {0_awp,0_anp}}});
        }
      }
  }

  [[nodiscard]]
  std::string_view allocation_false_negative_diagnostics::source_file() const noexcept
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
    {
      using beast = perfectly_normal_beast<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
      using allocator = typename beast::allocator_type;

      auto allocGetter{
        [](const beast& b){ return b.x.get_allocator(); }
      };

      auto mutator{
        [](beast& b) {
          b.x.push_back(1);
        }
      };

      check_semantics(LINE(""), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info{allocGetter, {1_c, {1_c,1_mu}, {1_awp,1_anp}}});
      check_semantics(LINE(""), beast{{1,2}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info{allocGetter, {1_c, {1_c,1_mu}, {1_awp,0_anp}}});
      check_semantics(LINE(""), beast(allocator{}), beast{{5,6}, allocator{}}, mutator, allocation_info{allocGetter, {0_c, {1_c,1_mu}, {1_awp,1_anp}}});
    }

    {
      using handle = std::shared_ptr<int>;
      using beast = perfectly_sharing_beast<int, handle, shared_counting_allocator<handle, PropagateCopy, PropagateMove, PropagateSwap>>;
      using allocator = typename beast::allocator_type;
      using getter = typename beast::alloc_acquirer;

      auto mutator{
        [](beast& b) {
          *b.x.front() = 9;
        }
      };

      check_semantics(LINE(""), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info{getter{}, {1_c, {1_c,0_mu}, {1_awp, 1_anp}}});

      check_semantics(LINE(""), beast(allocator{}), beast{{5,6}, allocator{}}, mutator, allocation_info{getter{}, {0_c, {1_c,0_mu}, {1_awp, 1_anp}}});
    }

    /* TO DO: enable test once there's a fix for this libc++ bug https://bugs.llvm.org/show_bug.cgi?id=48439 {
      using beast = perfectly_stringy_beast<char, shared_counting_allocator<char, PropagateCopy, PropagateMove, PropagateSwap>>;

      auto allocGetter{
        [](const beast& b){ return b.x.get_allocator(); }
      };

      auto mutator{
        [](beast& b) {
          b.x.append("\nSome profound addition to the already remarkable string");
        }
      };

      check_semantics(LINE(""),
                      beast{},
                      beast{"A string which is clearly long enough to evade the small string optimisation"},
                      mutator, allocation_info{allocGetter, {0_c, {1_c,1_mu}, {1_awp,1_anp}}});
    }*/

    {
      using beast = inefficient_copy<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
      using allocator = typename beast::allocator_type;

      auto allocGetter{
        [](const beast& b){ return b.x.get_allocator(); }
      };

      auto mutator{
        [](beast& b) {
          b.x.reserve(b.x.capacity() + 1);
          b.x.push_back(1);
        }
      };

      
      check_semantics(LINE(""), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info{allocGetter, {2_c, {2_c,1_mu,1_pc,1_pm}, {1_awp,1_anp}}});
    }

    {
      using beast = inefficient_para_copy<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
      using allocator = typename beast::allocator_type;

      auto allocGetter{
        [](const beast& b){ return b.x.get_allocator(); }
      };

      auto mutator{
        [](beast& b) {
          b.x.shrink_to_fit();
          b.x.reserve(b.x.capacity() + 1);
          b.x.push_back(1);
        }
      };

      check_semantics(LINE(""), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info{allocGetter, {1_c, {1_c,1_mu,2_pc,1_pm}, {1_awp,1_anp}}});
    }

    {
      using beast = inefficient_para_move<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
      using allocator = typename beast::allocator_type;

      auto allocGetter{
        [](const beast& b){ return b.x.get_allocator(); }
      };

      auto mutator{
        [](beast& b) {
          b.x.reserve(b.x.capacity() + 1);
          b.x.push_back(1);
        }
      };

        
      check_semantics(LINE(""), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info{allocGetter, {1_c, {1_c,1_mu,1_pc,2_pm}, {1_awp,1_anp}}});
    }


    {
      using beast = doubly_normal_beast<int, double, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>, shared_counting_allocator<double, PropagateCopy, PropagateMove, PropagateSwap>>;
      using xAllocator = typename beast::x_allocator_type;
      using yAllocator = typename beast::y_allocator_type;

      auto xAllocGetter{
        [](const beast& b){ return b.x.get_allocator(); }
      };

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
                              , allocation_info{xAllocGetter, {1_c, {1_c,1_mu}, {1_awp,1_anp}}}
                              , allocation_info{yAllocGetter, {1_c, {1_c,1_mu}, {1_awp,1_anp}}});
    }
  }

  
}
