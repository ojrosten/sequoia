////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "MonotonicSequenceTest.hpp"
#include "MonotonicSequenceTestingUtilities.hpp"

#include "MonotonicSequence.hpp"

namespace sequoia::unit_testing
{
  void monotonic_sequence_test::run_tests()
  {
    test_decreasing_sequence();
    test_static_decreasing_sequence();
  }

  void monotonic_sequence_test::test_decreasing_sequence()
  {
    using namespace sequoia::maths;

    check_exception_thrown<std::logic_error>([](){ monotonic_sequence<int> s{1,2}; }, LINE("Invariant violated by initialization"));

    monotonic_sequence<int> s{}, t{1};
    // - ; 1
    
    check_equivalence(s, std::initializer_list<int>{}, LINE(""));
    check_equivalence(t, std::initializer_list<int>{1}, LINE(""));
    check_regular_semantics(s, t, LINE("Regular Semantics"));

    check_exception_thrown<std::logic_error>([&t](){ t.push_back(2); });
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

    check_equality(2ul, t.capacity(), LINE("Capacity"));
    t.shrink_to_fit();
    check_equality(0ul, t.capacity(), LINE("Capacity"));
    t.reserve(2);
    check_equality(2ul, t.capacity(), LINE("Capacity"));
  }

  void monotonic_sequence_test::test_static_decreasing_sequence()
  {
    using namespace sequoia::maths;

    check_exception_thrown<std::logic_error>([](){ static_monotonic_sequence<double, 2> s{1,2}; }, LINE("Invariant violated by initialization"));

    constexpr static_monotonic_sequence<double, 2> s{5.1, 3.8}, t{-3.4, -4.4};
    check_regular_semantics(s, t, LINE("Regular Semantics"));
  }
}
