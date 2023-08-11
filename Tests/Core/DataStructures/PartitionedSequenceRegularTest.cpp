////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "PartitionedSequenceRegularTest.hpp"
#include "PartitionedDataGenericTests.hpp"
#include "sequoia/Core/DataStructures/PartitionedData.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path partitioned_sequence_regular_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void partitioned_sequence_regular_test::run_tests()
  {
    using namespace data_structures;
    partition_data_operations<partitioned_sequence<double, object::by_value<double>>>::execute_operations(*this);
  }
}
