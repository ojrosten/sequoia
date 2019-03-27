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

    monotonic_sequence<int> s{}, t{1};
    check_equivalence(s, std::initializer_list<int>{}, LINE(""));
    check_equivalence(t, std::initializer_list<int>{1}, LINE(""));
    check_regular_semantics(s, t, LINE("Regular Semantics"));
  }

  void monotonic_sequence_test::test_static_decreasing_sequence()
  {
  }
}
