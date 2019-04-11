////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "MonotonicSequenceTestingDiagnostics.hpp"

namespace sequoia::unit_testing
{
  void monotonic_sequence_false_positive_test::run_tests()
  {
    using namespace sequoia::maths;

    monotonic_sequence<int> s{}, t{1};
    // - ; 1

    check_equivalence(s, std::initializer_list<int>{1}, LINE("Empty sequence inequivalent to a single value"));
    check_equivalence(t, std::initializer_list<int>{}, LINE("Non-empty sequence inequivalent to empty list"));
    check_equivalence(t, std::initializer_list<int>{2}, LINE("Sequence of size one inequivalent to list with different value"));
    check_equality(s, t, LINE("Empty/non-empty sequences should compare different"));

    check(!s.empty(), LINE("Empty sequence should not report as non-empty"));
    check(t.empty(), LINE("Non-empty sequence should not report as empty"));

    check_equality(s.size(), 1ul, LINE("Empty sequence should not report non-zero size"));
    check_equality(t.size(), 0ul, LINE("Non-Eepty sequence should not report zero size"));

    check_equality(t.back(), 2, LINE("Back element equal to 1 should not report as 2"));
    check_equality(t.front(), 2, LINE("Front element equal to 1 should not report as 2"));
    check_equality(t[0], 2, LINE("Zeroth element equal to 1 should not report as 2"));
    check_equality(*t.begin(), 2, LINE("begin() pointing to 1 should not dereference as 2"));
    check_equality(*t.cbegin(), 2, LINE("cbegin() pointing to 1 should not dereference as 2"));
    check_equality(*t.rbegin(), 2, LINE("rbegin() pointing to 1 should not dereference as 2"));
    check_equality(*t.crbegin(), 2, LINE("crbegin() pointing to 1 should not dereference as 2"));

    s.push_back(2);
    // 2 ; 1

    check_equality(s, t, LINE("Sequences of size 1 with different elements should compare different"));

    s.insert(s.cbegin(), 3);
    // 3,2 ; 1

    check_equality(s, t, LINE("Sequences of different sizes should compare different"));

    t.insert(t.end(), 0);
    // 3,2 ; 1, 0

    check_equality(t.back(), 1, LINE("Back element equal to 0 should not report as 1"));
    check_equality(t.front(), 0, LINE("Front element equal to 0 should not report as 1"));
    check_equality(t[0], 0, LINE("Zeroth element equal to 1 should not report as 1"));
    check_equality(t[1], 1, LINE("First element equal to 0 should not report as 1"));
    check_equality(*t.begin(), 0, LINE("begin() pointing to 1 should not dereference as 0"));
    check_equality(*t.cbegin(), 0, LINE("cbegin() pointing to 1 should not dereference as 0"));
    check_equality(*t.rbegin(), 1, LINE("rbegin() pointing to 0 should not dereference as 1"));
    check_equality(*t.crbegin(), 1, LINE("crbegin() pointing to 0 should not dereference as 1"));
    check_equality(*(t.end()-1), 1, LINE("iterator pointing to 0 should not dereference as 1"));
    check_equality(*(t.cend()-1), 1, LINE("citerator pointing to 0 should not dereference as 1"));
    check_equality(*(t.rend()-1), 0, LINE("riterator pointing to 1 should not dereference as 0"));
    check_equality(*(t.crend()-1), 0, LINE("criterator pointing to 1 should not dereference as 0"));

    check_equivalence(s, std::initializer_list<int>{3, 1}, LINE("Inequivalent sequences of size two"));
    check_equivalence(s, std::initializer_list<int>{1, 2}, LINE("Inequivalent sequences of size two"));
    check_equality(s, t, LINE("Sequences of size 2 with different elements should compare different"));

    t.mutate(t.begin(), t.end(), [](const int& i){ return i * 3; });
    // 3, 2 ; 3, 0

    check_equality(s, t, LINE("Sequences of size 2 with different final elements should compare different"));

    t.mutate(t.begin(), t.end(), [](const int& i){ return i + 2; });
    // 3, 2 ; 5, 2

    check_equality(s, t, LINE("Sequences of size 2 with different first elements should compare different"));
  }
}
