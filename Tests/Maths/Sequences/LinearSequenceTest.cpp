////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "LinearSequenceTest.hpp"

#include "LinearSequenceTestingUtilities.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path linear_sequence_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void linear_sequence_test::run_tests()
  {
    test_linear_sequence();
    test_static_linear_sequence();
  }

  void linear_sequence_test::test_linear_sequence()
  {
    using namespace maths;

    linear_sequence<int, int> s{1, 3}, t{0,2};
    check(equivalence, "", s, std::pair{1, 3});
    check(equivalence, "", t, std::pair{0, 2});

    check_semantics("", s, t);

    check(equality, "", s[0], 1);
    check(equality, "", s[-1], -2);
    check(equality, "", s[1], 4);

    check(equality, "", t[0], 0);
    check(equality, "", t[-1], -2);
    check(equality, "", t[1], 2);
  }

  void linear_sequence_test::test_static_linear_sequence()
  {
    using namespace maths;

    constexpr static_linear_sequence<std::size_t, 1, 3, 2, std::size_t> s{};
    static_assert(s.start() == 1);
    static_assert(s.step() == 3);
    static_assert(s.size() == 2);


    constexpr auto val{s[2]};
    check(equality, "", val, 7_sz);
  }
}
