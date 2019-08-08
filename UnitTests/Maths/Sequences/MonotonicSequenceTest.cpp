////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "MonotonicSequenceTest.hpp"
#include "MonotonicSequenceTestingUtilities.hpp"

#include "UnitTestUtilities.hpp"

namespace sequoia::unit_testing
{
  void monotonic_sequence_test::run_tests()
  {
    test_decreasing_sequence();
    test_static_decreasing_sequence();
    test_static_increasing_sequence();
    test_allocator();
  }

  void monotonic_sequence_test::test_decreasing_sequence()
  {
    using namespace maths;

    check_exception_thrown<std::logic_error>(LINE("Invariant violated by initialization"), [](){ monotonic_sequence<int> s{1,2}; });

    monotonic_sequence<int> s{}, t{1};
    // - ; 1
    
    check_equivalence(LINE(""), s, std::initializer_list<int>{});
    check_equivalence(LINE(""), t, std::initializer_list<int>{1});
    check_regular_semantics(LINE("Regular Semantics"), s, t);

    check_exception_thrown<std::logic_error>(LINE(""), [&t](){ t.push_back(2); });
    check_equivalence(LINE("Invariant violated by attempted push_back"), t, std::initializer_list<int>{1});

    check_exception_thrown<std::logic_error>(LINE("Invariant violated by attempted insertion at beginning"), [&t](){ t.insert(t.cbegin(), 0); });
    check_equivalence(LINE(""), t, std::initializer_list<int>{1});

    t.push_back(1);
    // - ; 1,1

    check_equivalence(LINE(""), t, std::initializer_list<int>{1, 1});
    check_equality(LINE(""), t, monotonic_sequence<int>{1,1});

    check_exception_thrown<std::logic_error>(LINE("Invariant violated by attempted insertion in middle"), [&t](){ t.insert(t.cbegin() + 1, 2); });

    t.mutate(t.cbegin(), t.cend() - 1, [](int a){ return a *= 2; });
    // - ; 2,1

    check_equivalence(LINE(""), t, std::initializer_list<int>{2, 1});
    check_equality(LINE(""), t, monotonic_sequence<int>{2,1});

    s.mutate(s.begin(), s.end(), [](int a) { return a+1; });
    check_equivalence(LINE(""), s, std::initializer_list<int>{});

    s.push_back(4);
    // 4 ; 2,1

    check_equivalence(LINE(""), s, std::initializer_list<int>{4});
    check_regular_semantics(LINE("Regular Semantics"), s, t);

    t.erase(t.begin(), t.end());
    // 4 ; -
    
    check_equivalence(LINE(""), t, std::initializer_list<int>{});

    s.erase(s.begin());
    // - ; -

    check_equivalence(LINE(""), s, std::initializer_list<int>{});

    check_equality(LINE("Capacity"), t.capacity(), 2ul);
    t.shrink_to_fit();
    check_equality(LINE("Capacity"), t.capacity(), 0ul);
    t.reserve(2);
    check_equality(LINE("Capacity"), t.capacity(), 2ul);

    t = monotonic_sequence<int>{8, 4, 3};
    // - ; 8,4,3
    check_equivalence(LINE(""), t, std::initializer_list<int>{8, 4, 3});

    check_exception_thrown<std::logic_error>(LINE(""),
      [&t](){ t.mutate(t.begin(), t.begin()+2, [](const int i) { return i/2; }); });

    check_equivalence(LINE(""), t, std::initializer_list<int>{8, 4, 3});

    t.mutate(t.begin(), t.end(), [](const int i) { return i/2; });
    // -; 4, 2, 1
    
    check_equivalence(LINE(""), t, std::initializer_list<int>{4, 2, 1});
  }

  void monotonic_sequence_test::test_static_decreasing_sequence()
  {
    using namespace maths;

    check_exception_thrown<std::logic_error>(LINE("Invariant violated by initialization"), [](){ static_monotonic_sequence<double, 2> s{1,2}; });

    constexpr static_monotonic_sequence<double, 2> s{5.1, 3.8}, t{-3.4, -4.4};
    check_regular_semantics(LINE("Regular Semantics"), s, t);
  }

  template<bool Check>
  constexpr maths::static_monotonic_sequence<int,6, std::greater<int>>
  monotonic_sequence_test::make_sequence()
  {
    using namespace maths;

    static_monotonic_sequence<int, 6, std::greater<int>> s{-1,0,1,1,2,6};

    s.mutate<Check>(s.begin(), s.end(), [](const int i) { return i * 2; });
    
    return s;
  }
  
  void monotonic_sequence_test::test_static_increasing_sequence()
  {
    using namespace maths;

    constexpr auto s{make_sequence<false>()};
    check_equivalence(LINE(""), s, std::initializer_list<int>{-2,0,2,2,4,12});

    constexpr auto s2{make_sequence<true>()};
    check_equivalence(LINE(""), s2, std::initializer_list<int>{-2,0,2,2,4,12});
    
    check_equality(LINE(""), s, static_monotonic_sequence<int, 6, std::greater<int>>{-2,0,2,2,4,12});

    static_monotonic_sequence<int, 6, std::greater<int>> t{2,2,2,3,3,3};
    check_exception_thrown<std::logic_error>(LINE(""), [&t](){
        t.mutate(t.begin()+1, t.begin()+3,[](const int i){ return i*2;});});
    
    check_equivalence(LINE(""), t, std::initializer_list<int>{2,2,2,3,3,3});
    check_regular_semantics(LINE("Regular Semantics"), s, t);

    static_monotonic_sequence<int, 6, std::greater<int>> u{2,3,3,4,4,5};
    check_exception_thrown<std::logic_error>(LINE(""), [&u](){
        u.mutate(u.begin()+1, u.begin()+4,[](const int i){ return i*2;});});

    check_equivalence(LINE(""), u, std::initializer_list<int>{2,3,3,4,4,5});
  }

  void monotonic_sequence_test::test_allocator()
  {
    using namespace maths;

    using allocator = custom_allocator<int>;
    using sequence = monotonic_sequence<int, std::less<int>, std::vector<int, allocator>>;

    int
      sAllocCount{}, sDeallocCount{},
      tAllocCount{}, tDeallocCount{};    

    {
      allocator sAlloc{sAllocCount, sDeallocCount};
      
      sequence s(sAlloc);
      check_equivalence(LINE(""), s, std::initializer_list<int>{});
      check_equality(LINE(""), sAllocCount, 0);

      allocator tAlloc{tAllocCount, tDeallocCount};
      sequence t{{4, 3}, tAlloc};
      check_equivalence(LINE(""), t, std::initializer_list<int>{4, 3});
      check_equality(LINE(""), tAllocCount, 1);

      check_regular_semantics(LINE("Regular Semantics"), s, t, allocator{});

      check_equality(LINE(""), sAllocCount, 0);
      check_equality(LINE(""), tAllocCount, 2);

      auto mutator{
        [](sequence& seq){
          const auto val{seq.empty() ? 0 : seq.back() - 1};
          seq.push_back(val);
        }
      };
      check_allocations(LINE(""), s, t, mutator, allocation_info<allocator>{sAlloc, tAlloc, {0, 1, 1, 1}});
    }

    check_equality(LINE(""), sDeallocCount, 0);
    check_equality(LINE(""), tDeallocCount, 5);
  }
}
