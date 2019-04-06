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

    check_equality(1ul, s.size(), LINE("Empty sequence should not report non-zero size"));
    check_equality(0ul, t.size(), LINE("Non-Eepty sequence should not report zero size"));

    check_equality(2, t.back(), LINE("Back element equal to 1 should not report as 2"));
    check_equality(2, t.front(), LINE("Front element equal to 1 should not report as 2"));
    check_equality(2, t[0], LINE("Zeroth element equal to 1 should not report as 2"));
    check_equality(2, *t.begin(), LINE("begin() pointing to 1 should not dereference as 2"));
    check_equality(2, *t.cbegin(), LINE("cbegin() pointing to 1 should not dereference as 2"));
    check_equality(2, *t.rbegin(), LINE("rbegin() pointing to 1 should not dereference as 2"));
    check_equality(2, *t.crbegin(), LINE("crbegin() pointing to 1 should not dereference as 2"));

    s.push_back(2);
    // 2 ; 1

    check_equality(s, t, LINE("Sequences of size 1 with different elements should compare different"));

    s.insert(s.cbegin(), 3);
    // 3,2 ; 1

    check_equality(s, t, LINE("Sequences of different sizes should compare different"));

    t.insert(t.end(), 0);
    // 3,2 ; 1, 0

    check_equality(1, t.back(), LINE("Back element equal to 0 should not report as 1"));
    check_equality(0, t.front(), LINE("Front element equal to 0 should not report as 1"));
    check_equality(0, t[0], LINE("Zeroth element equal to 1 should not report as 1"));
    check_equality(1, t[1], LINE("First element equal to 0 should not report as 1"));
    check_equality(0, *t.begin(), LINE("begin() pointing to 1 should not dereference as 0"));
    check_equality(0, *t.cbegin(), LINE("cbegin() pointing to 1 should not dereference as 0"));
    check_equality(1, *t.rbegin(), LINE("rbegin() pointing to 0 should not dereference as 1"));
    check_equality(1, *t.crbegin(), LINE("crbegin() pointing to 0 should not dereference as 1"));
    check_equality(1, *(t.end()-1), LINE("iterator pointing to 0 should not dereference as 1"));
    check_equality(1, *(t.cend()-1), LINE("citerator pointing to 0 should not dereference as 1"));
    check_equality(0, *(t.rend()-1), LINE("riterator pointing to 1 should not dereference as 0"));
    check_equality(0, *(t.crend()-1), LINE("criterator pointing to 1 should not dereference as 0"));

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
