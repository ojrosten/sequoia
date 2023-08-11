////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "BucketedSequenceRegularTest.hpp"
#include "PartitionedDataGenericTests.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path bucketed_sequence_regular_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void bucketed_sequence_regular_test::run_tests()
  {
    using namespace data_structures;
    partition_data_operations<bucketed_sequence<double, object::by_value<double>>>::execute_operations(*this);
  }
}
