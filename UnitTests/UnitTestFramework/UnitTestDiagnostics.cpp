////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "UnitTestDiagnostics.hpp"
#include "UnitTestUtilities.hpp"

#include <complex>

namespace sequoia::unit_testing
{
  void false_positive_diagnostics::run_tests()
  {
      
    basic_tests();
    test_container_checks();
    test_relative_performance();
    test_mixed();
    test_regular_semantics();

    do_allocation_tests(*this);
  }

  void false_positive_diagnostics::basic_tests()
  {
    check(LINE(""), false);
      
    check_equality(LINE(""), 5, 4);
    check_equality(LINE(""), 6.5, 5.6);

    check_equality(LINE(""), std::pair<int, double>{5, 7.8}, std::pair<int, double>{5, -7.8});
    check_equality(LINE(""), std::pair<int, double>{5, 7.8}, std::pair<int, double>{-5, 7.8});

    check_equality(LINE(""), std::tuple<int, double, float>{4, 3.4, -9.2f}, std::tuple<int, double, float>{0, 3.4, -9.2f});
    check_equality(LINE(""), std::tuple<int, double, float>{4, 3.4, -9.2f}, std::tuple<int, double, float>{4, 0.0, -9.2f});
    check_equality(LINE(""), std::tuple<int, double, float>{4, 3.4, -9.2f}, std::tuple<int, double, float>{4, 3.4, -0.0f});

    check_equality_within_tolerance(LINE(""), 3.0, 5.0, 1.0);
    check_equality_within_tolerance(LINE(""), 7.0, 5.0, 1.0);

    check_exception_thrown<std::runtime_error>(LINE("Exception expected but nothing thrown"), [](){});
    check_exception_thrown<std::runtime_error>(LINE("Exception thrown but of wrong type"), [](){ throw std::logic_error("Error"); });
  }

  void false_positive_diagnostics::test_container_checks()
  {
    check_equality(LINE("Empty vector check which should fail"), std::vector<double>{}, std::vector<double>{1});
    check_equality(LINE("One element vector check which should fail due to wrong value"), std::vector<double>{1}, std::vector<double>{2});
    check_equality(LINE("One element vector check which should fail due to differing sizes"), std::vector<double>{1}, std::vector<double>{1,2});
    check_equality(LINE("Multi-element vector comparison which should fail due to last element"), std::vector<double>{1,5}, std::vector<double>{1,4});
    check_equality(LINE("Multi-element vector comparison which should fail due to first element"), std::vector<double>{1,5}, std::vector<double>{0,5});
    check_equality(LINE("Multi-element vector comparison which should fail due to middle element"), std::vector<double>{1,5,3.2}, std::vector<double>{1,5,3.3});      
    check_equality(LINE("Multi-element vector comparison which should fail due to different sizes"), std::vector<double>{1,5,3.2}, std::vector<double>{5,3.2});

    std::vector<float> refs{-4.3, 2.8, 6.2, 7.3}, ans{1.1, -4.3, 2.8, 6.2, 8.4, 7.3};

    check_range(LINE("Iterators demarcate differing number of elements"), refs.cbegin(), refs.cend(), ans.cbegin(), ans.cend());
    check_range(LINE("Iterators demarcate differing elements"), refs.cbegin(), refs.cend(), ans.cbegin(), ans.cbegin() + 4);
  }

  void false_positive_diagnostics::test_relative_performance()
  {
    auto wait{[](const size_t millisecs) {
        std::this_thread::sleep_for(std::chrono::milliseconds(millisecs));
      }
    };

    check_relative_performance(LINE("Performance Test for which fast task is too slow, [1, (2.0, 2.0)"),
                               [wait]() { return wait(1); },
                               [wait]() { return wait(1); }, 2.0, 2.0);
      
    check_relative_performance(LINE("Performance Test for which fast task is too slow [1, (2.0, 3.0)"),
                               [wait]() { return wait(1); },
                               [wait]() { return wait(1); }, 2.0, 3.0);
      
    check_relative_performance(LINE("Performance Test for which fast task is too fast [4, (2.0, 2.5)]"),
                               [wait]() { return wait(1); },
                               [wait]() { return wait(4); }, 2.0, 2.5);      
  }

  void false_positive_diagnostics::test_mixed()
  {
    using t_0 = std::vector<std::pair<int, float>>;
    using t_1 = std::set<double>;
    using t_2 = std::complex<double>;
    using type = std::tuple<t_0, t_1, t_2>;
      
    type a{t_0{{1, 2.1f}, {2, 2.8f}}, {3.3, -9.6, 3.2}, {1.1, 0.2}};

    {
      type b{t_0{{2, 2.1f}, {2, 2.8f}}, {3.3, -9.6, 3.2}, {1.1, 0.2}};
      check_equality(LINE(""), a, b);
    }

    {
      type b{t_0{{1, 2.1f}, {2, 2.8f}}, {3.4, -9.6, 3.2}, {1.1, 0.2}};
      check_equality(LINE(""), a, b);
    }
  }

  void false_positive_diagnostics::test_regular_semantics()
  {
    check_regular_semantics(LINE("Broken equality"), broken_equality{1}, broken_equality{2});
    check_regular_semantics(LINE("Broken inequality"), broken_inequality{1}, broken_inequality{2});
    check_regular_semantics(LINE("Broken copy"), broken_copy{1}, broken_copy{2});
    check_regular_semantics(LINE("Broken move"), broken_move{1}, broken_move{2});
    check_regular_semantics(LINE("Broken copy assignment"), broken_copy_assignment{1}, broken_copy_assignment{2});
    check_regular_semantics(LINE("Broken move assignment"), broken_move_assignment{1}, broken_move_assignment{2});
    check_regular_semantics(LINE("Broken swap"), broken_swap{1}, broken_swap{2});
    check_regular_semantics(LINE("Broken copy value semantics"), broken_copy_value_semantics{1}, broken_copy_value_semantics{2}, [](auto& b){ *b.x.front() = 3; });
    check_regular_semantics(LINE("Broken copy assignment value semantics"), broken_copy_assignment_value_semantics{1}, broken_copy_assignment_value_semantics{2}, [](auto& b){ *b.x.front() = 3; });
    check_regular_semantics(LINE("Broken check invariant"), perfectly_normal_beast{1}, perfectly_normal_beast{1});

    check_regular_semantics(LINE("Broken equality"), broken_equality{1}, broken_equality{2}, broken_equality{1}, broken_equality{2});
    check_regular_semantics(LINE("Broken inequality"), broken_inequality{1}, broken_inequality{2}, broken_inequality{1}, broken_inequality{2});
    check_regular_semantics(LINE("Broken move"), broken_move{1}, broken_move{2}, broken_move{1}, broken_move{2});
    check_regular_semantics(LINE("Broken move"), broken_swap{1}, broken_swap{2}, broken_swap{1}, broken_swap{2});
    check_regular_semantics(LINE("Broken move assignment"), broken_move_assignment{1}, broken_move_assignment{2}, broken_move_assignment{1}, broken_move_assignment{2});
    check_regular_semantics(LINE("Broken check invariant"), perfectly_normal_beast{1}, perfectly_normal_beast{1}, perfectly_normal_beast{1}, perfectly_normal_beast{1});
    check_regular_semantics(LINE("Broken check invariant"), perfectly_normal_beast{1}, perfectly_normal_beast{3}, perfectly_normal_beast{2}, perfectly_normal_beast{3});
    check_regular_semantics(LINE("Broken check invariant"), perfectly_normal_beast{1}, perfectly_normal_beast{2}, perfectly_normal_beast{3}, perfectly_normal_beast{2});
  }

  template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void false_positive_diagnostics::test_allocation()
  {
    test_regular_semantics_allocations<PropagateCopy, PropagateMove, PropagateSwap>();
  }

  template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void false_positive_diagnostics::test_regular_semantics_allocations()
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
    
    using allocator = std::vector<int>::allocator_type;
    check_regular_semantics(LINE(""), broken_equality{1}, broken_equality{2}, broken_equality{1}, broken_equality{2}, allocator{});
    check_regular_semantics(LINE(""), broken_inequality{1}, broken_inequality{2}, broken_inequality{1}, broken_inequality{2}, allocator{});
    check_regular_semantics(LINE(""), broken_move{1}, broken_move{2}, broken_move{1}, broken_move{2});
    check_regular_semantics(LINE(""), broken_move_assignment{1}, broken_move_assignment{2}, broken_move_assignment{1}, broken_move_assignment{2}, allocator{});
    check_regular_semantics(LINE(""), perfectly_normal_beast{1}, perfectly_normal_beast{1}, perfectly_normal_beast{1}, perfectly_normal_beast{1}, allocator{});
    check_regular_semantics(LINE(""), perfectly_normal_beast{1}, perfectly_normal_beast{3}, perfectly_normal_beast{2}, perfectly_normal_beast{3}, allocator{});
    check_regular_semantics(LINE(""), perfectly_normal_beast{1}, perfectly_normal_beast{2}, perfectly_normal_beast{3}, perfectly_normal_beast{2}, allocator{});
  }

  void false_negative_diagnostics::run_tests()
  {
    basic_tests();
    test_container_checks();
    test_relative_performance();
    test_mixed();
    test_regular_semantics();

    do_allocation_tests(*this);
  }

  void false_negative_diagnostics::basic_tests()
  {
    check(LINE(""), true);

    check_equality(LINE(""), 5, 5);
    check_equality(LINE(""), 5.0, 5.0);

    check_equality_within_tolerance(LINE(""), 4.5, 5.0, 1.0);
    check_equality_within_tolerance(LINE(""), 5.5, 5.0, 1.0);

    check_equality(LINE(""), std::pair<int, double>{5, 7.8}, std::pair<int, double>{5, 7.8});

    check_equality(LINE(""), std::tuple<int, double, float>{4, 3.4, -9.2f}, std::tuple<int, double, float>{4, 3.4, -9.2f});

    check_exception_thrown<std::runtime_error>(LINE(""), [](){ throw std::runtime_error("Error");});

      
  }

  void false_negative_diagnostics::test_container_checks()
  {
    check_equality(LINE("Empty vector check which should pass"), std::vector<double>{}, std::vector<double>{});
    check_equality(LINE("One element vector check which should pass"), std::vector<double>{2}, std::vector<double>{2});
    check_equality(LINE("Multi-element vector comparison which should pass"), std::vector<double>{1,5}, std::vector<double>{1,5});

    std::vector<float> refs{-4.3, 2.8, 6.2, 7.3}, ans{1.1, -4.3, 2.8, 6.2, 8.4, 7.3};
      
    check_range(LINE("Iterators demarcate identical elements"), refs.cbegin(), refs.cbegin()+3, ans.cbegin()+1, ans.cbegin()+4);
  }
    
  void false_negative_diagnostics::test_relative_performance()
  {
      
    auto wait{[](const size_t millisecs) {
        std::this_thread::sleep_for(std::chrono::milliseconds(millisecs));
      }
    };

    auto fast{[wait]() {
        return wait(1);
      }
    };      

    check_relative_performance(LINE("Performance Test which should pass"), fast, [wait]() { return wait(2); }, 1.9, 2.1, 5);
    check_relative_performance(LINE("Performance Test which should pass"), fast, [wait]() { return wait(4); }, 3.9, 4.1);
  }

  void false_negative_diagnostics::test_mixed()
  {
    using t_0 = std::vector<std::pair<int, float>>;
    using t_1 = std::set<double>;
    using t_2 = std::complex<double>;
    using type = std::tuple<t_0, t_1, t_2>;
      
    type a{t_0{{1, 2.1f}, {2, 2.8f}}, {3.3, -9.6, 3.2}, {1.1, 0.2}};
    type b{t_0{{1, 2.1f}, {2, 2.8f}}, {3.3, -9.6, 3.2}, {1.1, 0.2}};

    check_equality(LINE(""), a, b);
  }

  void false_negative_diagnostics::test_regular_semantics()
  {
    {
      check_regular_semantics(LINE(""), perfectly_normal_beast{1}, perfectly_normal_beast{2});
      check_regular_semantics(LINE(""), perfectly_normal_beast{1}, perfectly_normal_beast{2}, perfectly_normal_beast{1}, perfectly_normal_beast{2});
    }
  }

  template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void false_negative_diagnostics::test_allocation()
  {
    test_regular_semantics_allocations<PropagateCopy, PropagateMove, PropagateSwap>();
  }

  template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void false_negative_diagnostics::test_regular_semantics_allocations()
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

    using allocator = std::vector<int>::allocator_type;
    check_regular_semantics(LINE(""), perfectly_normal_beast{1}, perfectly_normal_beast{2}, perfectly_normal_beast{1}, perfectly_normal_beast{2}, allocator{});

    {
      using beast = doubly_normal_beast<int, double, shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>, shared_counting_allocator<double, PropagateCopy, PropagateMove, PropagateSwap>>;
      using xAllocator = typename beast::x_allocator_type;
      using yAllocator = typename beast::y_allocator_type;

      auto yAllocGetter{
        [](const auto& beast){
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
                              , allocation_info<beast, yAllocator>{yAllocGetter, {1, {1,1}, {1,1}}});
    }
  }  
}
