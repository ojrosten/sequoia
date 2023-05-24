////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "StaticLinearlyPartitionedSequenceTest.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path static_linearly_partitioned_sequence_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void static_linearly_partitioned_sequence_test::run_tests()
  {
    using namespace data_structures;

    {
      using sequence = static_linearly_partitioned_sequence<int, 1, 1>;
      static_assert(sizeof(sequence) == sizeof(int));

      constexpr sequence a{};
      sequence b{{1}};
      check_semantics(report_line(""), a, b);

      b[0][0] = 2;
      check(equality, report_line(""), b, sequence{{2}});
    }

    {
      using sequence = static_linearly_partitioned_sequence<int, 2, 3>;
      static_assert(sizeof(sequence) == 6 * sizeof(int));

      constexpr sequence a{{1,2,-1}, {3,-2,1}};
      sequence b{};

      check_semantics(report_line(""), a, b);

      b[1][2] = 1;
      check(equality, report_line(""), b, {{0, 0, 0}, {0, 0, 1}});
    }
  }
}
