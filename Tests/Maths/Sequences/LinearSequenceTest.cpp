////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "LinearSequenceTest.hpp"

#include "sequoia/Maths/Sequences/LinearSequence.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path linear_sequence_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void linear_sequence_test::run_tests()
  {
    test_static_linear_sequence();
  }

  void linear_sequence_test::test_static_linear_sequence()
  {
    using namespace maths;

    constexpr static_linear_sequence<std::size_t, 1, 3, 2, std::size_t> s{};
    STATIC_CHECK(s.start() == 1);
    STATIC_CHECK(s.step() == 3);
    STATIC_CHECK(s.size() == 2);

    constexpr auto val{s[2]};
    check(equality, "", val, 7uz);
  }
}
