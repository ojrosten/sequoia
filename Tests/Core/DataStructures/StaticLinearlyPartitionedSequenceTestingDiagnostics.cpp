////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "StaticLinearlyPartitionedSequenceTestingDiagnostics.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view static_linearly_partitioned_sequence_false_positive_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void static_linearly_partitioned_sequence_false_positive_test::run_tests()
  {
    using namespace data_structures;

    using sequence = static_linearly_partitioned_sequence<int, 1, 1>;
    using prediction = std::array<std::array<int, 1>, 1>;

    sequence a{};
    check_equivalence(LINE(""), a, prediction{{1}});

    sequence b{{1}};
    check_equivalence(LINE(""), b, prediction{{2}});

    check_equality(LINE(""), a, b);
  }
}
