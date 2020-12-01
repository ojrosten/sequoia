////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "StaticLinearlyPartitionedSequenceTest.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view static_linearly_partitioned_sequence_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void static_linearly_partitioned_sequence_test::run_tests()
  {
    using namespace data_structures;

    using sequence = static_linearly_partitioned_sequence<int, 1, 1>;

    constexpr sequence a{}, b{{1}};
    check_semantics("", a, b);    
  }
}
