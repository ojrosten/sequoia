////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "RegularAllocationTestDiagnostics.hpp"

#include "ContainerDiagnosticsUtilities.hpp"
#include "AllocationTestDiagnosticsUtilities.hpp"

#include <vector>

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path allocation_false_positive_diagnostics::source_file() const noexcept
  {
    return std::source_location::current().file_name();
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
        using beast = perfectly_normal_beast<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;

        auto allocGetter{
          [](const beast& b){ return b.x.get_allocator(); }
        };

        check_semantics(report_line("Broken check invariant"), beast{{1}, allocator{}}, beast{{1}, allocator{}}, mutator, allocation_info{allocGetter, {0_c, {0_c,1_mu}, {0_anp,0_awp}}});

        check_semantics(report_line("Incorrect init allocs"),
                      [](){ return beast{{1,2}}; },
                      [](){ return beast{{5}, allocator{}}; },
                      mutator,
                      allocation_info{allocGetter, {2_c, {2_c,1_mu}, {0_anp,1_awp}}});
      }

      {
        using beast = perfectly_normal_beast<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;

        auto allocGetter{
          [](const beast& b){ return b.x.get_allocator(); }
        };

        check_semantics(report_line("Incorrect copy x allocs"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info{allocGetter, {0_c, {1_c,1_mu}, {1_anp,1_awp}}});

        check_semantics(report_line("Incorrect copy y allocs"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info{allocGetter, {1_c, {0_c,1_mu}, {1_anp,1_awp}}});

        check_semantics(report_line("Incorrect mutation allocs"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info{allocGetter, {1_c, {1_c,0_mu}, {1_anp,1_awp}}});

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

        check_semantics(report_line("Incorrect copy assign y to x allocs"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info{allocGetter, predictions()});
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

        check_semantics(report_line("Incorrect copy x allocs"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, m, allocation_info{getter{}, {0_c, {1_c,0_mu}, {1_anp,1_awp}}});

        check_semantics(report_line("Incorrect copy y allocs"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, m, allocation_info{getter{}, {1_c, {0_c,0_mu}, {1_anp,1_awp}}});

        check_semantics(report_line("Incorrect mutation allocs"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, m, allocation_info{getter{}, {1_c, {1_c,1_mu}, {1_anp,1_awp}}});

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

        check_semantics(report_line("Incorrect copy assign y to x allocs"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, m, allocation_info{getter{}, predictions()});
      }


      {
        using beast = inefficient_serialization<int, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>>;
        using allocator = typename beast::allocator_type;

        auto allocGetter{ [](const beast& b){ return b.x.get_allocator(); } };

        check_semantics(report_line("Inefficient serialization"), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info{allocGetter, {1_c, {1_c,1_mu}, {1_anp,1_awp}}});
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

  [[nodiscard]]
  std::filesystem::path allocation_false_negative_diagnostics::source_file() const noexcept
  {
    return std::source_location::current().file_name();
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
          b.x.reserve(10);
          b.x.push_back(1);
        }
      };

      check_semantics(report_line(""), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info{allocGetter, {1_c, {1_c,1_mu}, {1_anp,1_awp}}});
      check_semantics(report_line(""), beast{{1,2}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info{allocGetter, {1_c, {1_c,1_mu}, {0_anp,1_awp}}});
      check_semantics(report_line(""), beast(allocator{}), beast{{5,6}, allocator{}}, mutator, allocation_info{allocGetter, {0_c, {1_c,1_mu}, {1_anp,1_awp}}});
      check_semantics(report_line(""), beast{{1,2}}, beast(allocator{}), mutator, allocation_info{allocGetter, {1_c, {0_c,1_mu}, {0_anp,0_awp}}});
      check_semantics(report_line(""), beast{{1,2}}, beast{{5}, allocator{}}, mutator, allocation_info{allocGetter, {1_c, {1_c,1_mu}, {0_anp,1_awp}}});

      auto[x,y]{check_semantics(report_line(""),
                      [](){ return beast{{1,2}}; },
                      [](){ return beast{{5}, allocator{}}; },
                      mutator,
                      allocation_info{allocGetter, {1_c, {1_c,1_mu}, {0_anp,1_awp}}})};

      check(equality, report_line("check_semantics return value (x)"), x, beast{{1,2}});
      check(equality, report_line("check_semantics return value (y)"), y, beast{{5}});
    }

    {
      using handle = std::shared_ptr<int>;
      using beast = perfectly_sharing_beast<int, handle, shared_counting_allocator<handle, PropagateCopy, PropagateMove, PropagateSwap>>;
      using allocator = typename beast::allocator_type;
      using getter = typename beast::alloc_acquirer;

      auto mutator{
        [](beast& b) {
          if(!b.x.empty()) *b.x.front() = 9;
          else { b.x.reserve(10); b.x.push_back(std::make_shared<int>(1)); }
        }
      };

      check_semantics(report_line(""), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info{getter{}, {1_c, {1_c,0_mu}, {1_anp, 1_awp}}});
      check_semantics(report_line(""), beast{{1,2}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info{getter{}, {1_c, {1_c,0_mu}, {1_anp,1_awp,0_manp,0_ma}}});
      check_semantics(report_line(""), beast(allocator{}), beast{{5,6}, allocator{}}, mutator, allocation_info{getter{}, {0_c, {1_c,0_mu}, {1_anp, 1_awp}}});
      check_semantics(report_line(""), beast{{1,2}}, beast(allocator{}), mutator, allocation_info{getter{}, {1_c, {0_c,1_mu}, {0_anp,0_awp}}});
      check_semantics(report_line(""), beast{{1,2}}, beast{{5}, allocator{}}, mutator, allocation_info{getter{}, {1_c, {1_c,0_mu}, {1_anp,1_awp,0_manp,0_ma}}});
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

      check_semantics(report_line(""),
                      beast{},
                      beast{"A string which is clearly long enough to evade the small string optimisation"},
                      mutator, allocation_info{allocGetter, {0_c, {1_c,1_mu}, {1_anp,1_awp}}});
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


      check_semantics(report_line(""), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info{allocGetter, {2_c, {2_c,1_mu,1_pc,1_pm}, {1_anp,1_awp}}});
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

      check_semantics(report_line(""), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info{allocGetter, {1_c, {1_c,1_mu,2_pc,1_pm}, {1_anp,1_awp}}});
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


      check_semantics(report_line(""), beast{{1}, allocator{}}, beast{{5,6}, allocator{}}, mutator, allocation_info{allocGetter, {1_c, {1_c,1_mu,1_pc,2_pm}, {1_anp,1_awp}}});
    }


    {
      using beast = doubly_normal_beast<int, double, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>, shared_counting_allocator<double, PropagateCopy, PropagateMove, PropagateSwap>>;
      using xAllocator = typename beast::x_allocator_type;
      using yAllocator = typename beast::y_allocator_type;

      auto xAllocGetter{
        [](const beast& b){ return b.x.get_allocator(); }
      };

      auto yAllocGetter{
        [](const beast& b){
          return b.y.get_allocator();
        }
      };

      auto mutator{
        [](beast& b) {
          b.x.push_back(1);
          b.y.push_back(1);
        }
      };

      check_semantics(report_line(""),
                      beast{{1}, {1}, xAllocator{}, yAllocator{}},
                      beast{{5,6}, {5,6}, xAllocator{}, yAllocator{}},
                      mutator,
                      allocation_info{xAllocGetter, {1_c, {1_c,1_mu}, {1_anp,1_awp}}},
                      allocation_info{yAllocGetter, {1_c, {1_c,1_mu}, {1_anp,1_awp}}});
    }
  }


}
