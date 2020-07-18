////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "MonotonicSequenceTestingDiagnostics.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view monotonic_sequence_false_positive_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void monotonic_sequence_false_positive_test::run_tests()
  {
    using namespace sequoia::maths;

    monotonic_sequence<int> s{}, t{1};
    // - ; 1

    check_equivalence(LINE("Empty sequence inequivalent to a single value"), s, std::initializer_list<int>{1});
    check_equivalence(LINE("Non-empty sequence inequivalent to empty list"), t, std::initializer_list<int>{});
    check_equivalence(LINE("Sequence of size one inequivalent to list with different value"), t, std::initializer_list<int>{2});
    check_equality(LINE("Empty/non-empty sequences should compare different"), s, t);

    check(LINE("Empty sequence should not report as non-empty"), !s.empty());
    check(LINE("Non-empty sequence should not report as empty"), t.empty());

    check_equality(LINE("Empty sequence should not report non-zero size"), s.size(), 1ul);
    check_equality(LINE("Non-Eepty sequence should not report zero size"), t.size(), 0ul);

    check_equality(LINE("Back element equal to 1 should not report as 2"), t.back(), 2);
    check_equality(LINE("Front element equal to 1 should not report as 2"), t.front(), 2);
    check_equality(LINE("Zeroth element equal to 1 should not report as 2"), t[0], 2);
    check_equality(LINE("begin() pointing to 1 should not dereference as 2"), *t.begin(), 2);
    check_equality(LINE("cbegin() pointing to 1 should not dereference as 2"), *t.cbegin(), 2);
    check_equality(LINE("rbegin() pointing to 1 should not dereference as 2"), *t.rbegin(), 2);
    check_equality(LINE("crbegin() pointing to 1 should not dereference as 2"), *t.crbegin(), 2);

    s.push_back(2);
    // 2 ; 1

    check_equality(LINE("Sequences of size 1 with different elements should compare different"), s, t);

    s.insert(s.cbegin(), 3);
    // 3,2 ; 1

    check_equality(LINE("Sequences of different sizes should compare different"), s, t);

    t.insert(t.end(), 0);
    // 3,2 ; 1, 0

    check_equality(LINE("Back element equal to 0 should not report as 1"), t.back(), 1);
    check_equality(LINE("Front element equal to 0 should not report as 1"), t.front(), 0);
    check_equality(LINE("Zeroth element equal to 1 should not report as 1"), t[0], 0);
    check_equality(LINE("First element equal to 0 should not report as 1"), t[1], 1);
    check_equality(LINE("begin() pointing to 1 should not dereference as 0"), *t.begin(), 0);
    check_equality(LINE("cbegin() pointing to 1 should not dereference as 0"), *t.cbegin(), 0);
    check_equality(LINE("rbegin() pointing to 0 should not dereference as 1"), *t.rbegin(), 1);
    check_equality(LINE("crbegin() pointing to 0 should not dereference as 1"), *t.crbegin(), 1);
    check_equality(LINE("iterator pointing to 0 should not dereference as 1"), *(t.end()-1), 1);
    check_equality(LINE("citerator pointing to 0 should not dereference as 1"), *(t.cend()-1), 1);
    check_equality(LINE("riterator pointing to 1 should not dereference as 0"), *(t.rend()-1), 0);
    check_equality(LINE("criterator pointing to 1 should not dereference as 0"), *(t.crend()-1), 0);

    check_equivalence(LINE("Inequivalent sequences of size two"), s, std::initializer_list<int>{3, 1});
    check_equivalence(LINE("Inequivalent sequences of size two"), s, std::initializer_list<int>{1, 2});
    check_equality(LINE("Sequences of size 2 with different elements should compare different"), s, t);

    t.mutate(t.begin(), t.end(), [](const int& i){ return i * 3; });
    // 3, 2 ; 3, 0

    check_equality(LINE("Sequences of size 2 with different final elements should compare different"), s, t);

    t.mutate(t.begin(), t.end(), [](const int& i){ return i + 2; });
    // 3, 2 ; 5, 2

    check_equality(LINE("Sequences of size 2 with different first elements should compare different"), s, t);
  }
}
