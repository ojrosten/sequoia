////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "MonotonicSequenceTest.hpp"
#include "MonotonicSequenceTestingUtilities.hpp"

namespace sequoia::unit_testing
{
  void monotonic_sequence_test::run_tests()
  {
    test_decreasing_sequence();
    test_static_decreasing_sequence();
    test_static_increasing_sequence();
  }

  void monotonic_sequence_test::test_decreasing_sequence()
  {
    using namespace maths;

    check_exception_thrown<std::logic_error>([](){ monotonic_sequence<int> s{1,2}; }, LINE("Invariant violated by initialization"));

    monotonic_sequence<int> s{}, t{1};
    // - ; 1
    
    check_equivalence(s, std::initializer_list<int>{}, LINE(""));
    check_equivalence(t, std::initializer_list<int>{1}, LINE(""));
    check_regular_semantics(s, t, LINE("Regular Semantics"));

    check_exception_thrown<std::logic_error>([&t](){ t.push_back(2); }, LINE(""));
    check_equivalence(t, std::initializer_list<int>{1}, LINE("Invariant violated by attempted push_back"));

    check_exception_thrown<std::logic_error>([&t](){ t.insert(t.cbegin(), 0); }, LINE("Invariant violated by attempted insertion at beginning"));
    check_equivalence(t, std::initializer_list<int>{1}, LINE(""));

    t.push_back(1);
    // - ; 1,1

    check_equivalence(t, std::initializer_list<int>{1, 1}, LINE(""));
    check_equality(t, monotonic_sequence<int>{1,1}, LINE(""));

    check_exception_thrown<std::logic_error>([&t](){ t.insert(t.cbegin() + 1, 2); }, LINE("Invariant violated by attempted insertion in middle"));

    t.mutate(t.cbegin(), t.cend() - 1, [](int a){ return a *= 2; });
    // - ; 2,1

    check_equivalence(t, std::initializer_list<int>{2, 1}, LINE(""));
    check_equality(t, monotonic_sequence<int>{2,1}, LINE(""));

    s.mutate(s.begin(), s.end(), [](int a) { return a+1; });
    check_equivalence(s, std::initializer_list<int>{}, LINE(""));

    s.push_back(4);
    // 4 ; 2,1

    check_equivalence(s, std::initializer_list<int>{4}, LINE(""));
    check_regular_semantics(s, t, LINE("Regular Semantics"));

    t.erase(t.begin(), t.end());
    // 4 ; -
    
    check_equivalence(t, std::initializer_list<int>{}, LINE(""));

    s.erase(s.begin());
    // - ; -

    check_equivalence(s, std::initializer_list<int>{}, LINE(""));

    check_equality(t.capacity(), 2ul, LINE("Capacity"));
    t.shrink_to_fit();
    check_equality(t.capacity(), 0ul, LINE("Capacity"));
    t.reserve(2);
    check_equality(t.capacity(), 2ul, LINE("Capacity"));

    t = monotonic_sequence<int>{8, 4, 3};
    // - ; 8,4,3
    check_equivalence(t, std::initializer_list<int>{8, 4, 3}, LINE(""));

    check_exception_thrown<std::logic_error>(
      [&t](){ t.mutate(t.begin(), t.begin()+2, [](const int i) { return i/2; }); }, LINE(""));

    check_equivalence(t, std::initializer_list<int>{8, 4, 3}, LINE(""));

    t.mutate(t.begin(), t.end(), [](const int i) { return i/2; });
    // -; 4, 2, 1
    
    check_equivalence(t, std::initializer_list<int>{4, 2, 1}, LINE(""));
  }

  void monotonic_sequence_test::test_static_decreasing_sequence()
  {
    using namespace maths;

    check_exception_thrown<std::logic_error>([](){ static_monotonic_sequence<double, 2> s{1,2}; }, LINE("Invariant violated by initialization"));

    constexpr static_monotonic_sequence<double, 2> s{5.1, 3.8}, t{-3.4, -4.4};
    check_regular_semantics(s, t, LINE("Regular Semantics"));
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
    check_equivalence(s, std::initializer_list<int>{-2,0,2,2,4,12}, LINE(""));

    constexpr auto s2{make_sequence<true>()};
    check_equivalence(s2, std::initializer_list<int>{-2,0,2,2,4,12}, LINE(""));
    
    check_equality(s, static_monotonic_sequence<int, 6, std::greater<int>>{-2,0,2,2,4,12}, LINE(""));

    static_monotonic_sequence<int, 6, std::greater<int>> t{2,2,2,3,3,3};
    check_exception_thrown<std::logic_error>([&t](){
        t.mutate(t.begin()+1, t.begin()+3,[](const int i){ return i*2;});}, LINE(""));
    
    check_equivalence(t, std::initializer_list<int>{2,2,2,3,3,3}, LINE(""));
    check_regular_semantics(s, t, LINE("Regular Semantics"));

    static_monotonic_sequence<int, 6, std::greater<int>> u{2,3,3,4,4,5};
    check_exception_thrown<std::logic_error>([&u](){
        u.mutate(u.begin()+1, u.begin()+4,[](const int i){ return i*2;});}, LINE(""));

    check_equivalence(u, std::initializer_list<int>{2,3,3,4,4,5}, LINE(""));
  }
}
