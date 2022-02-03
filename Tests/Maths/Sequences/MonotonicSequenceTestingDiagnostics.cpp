////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

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

    check(equivalence, LINE("Empty sequence inequivalent to a single value"), s, std::initializer_list<int>{1});
    check(equivalence, LINE("Non-empty sequence inequivalent to empty list"), t, std::initializer_list<int>{});
    check(equivalence, LINE("Sequence of size one inequivalent to list with different value"), t, std::initializer_list<int>{2});
    check(equality, LINE("Empty/non-empty sequences should compare different"), s, t);

    s.push_back(2);
    // 2 ; 1

    check(equality, LINE("Sequences of size 1 with different elements should compare different"), s, t);

    s.insert(s.cbegin(), 3);
    // 3,2 ; 1

    check(equality, LINE("Sequences of different sizes should compare different"), s, t);

    t.insert(t.end(), 0);
    // 3,2 ; 1, 0

    check(equality, LINE("Sequences of size 2 with different elements should compare different"), s, t);

    t.mutate(t.begin(), t.end(), [](const int& i){ return i * 3; });
    // 3, 2 ; 3, 0

    check(equality, LINE("Sequences of size 2 with different final elements should compare different"), s, t);

    t.mutate(t.begin(), t.end(), [](const int& i){ return i + 2; });
    // 3, 2 ; 5, 2

    check(equality, LINE("Sequences of size 2 with different first elements should compare different"), s, t);
  }
}
