////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "StaticLinearlyPartitionedSequenceTestingDiagnostics.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path static_linearly_partitioned_sequence_false_negative_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void static_linearly_partitioned_sequence_false_negative_test::run_tests()
  {
    using namespace data_structures;

    using sequence = static_linearly_partitioned_sequence<int, 1, 1>;
    using prediction = std::array<std::array<int, 1>, 1>;

    sequence a{};
    check(equivalence, "", a, prediction{{1}});

    sequence b{{1}};
    check(equivalence, "", b, prediction{{2}});

    check(equality, "", a, b);
  }
}
