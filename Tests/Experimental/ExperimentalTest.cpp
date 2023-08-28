////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "ExperimentalTest.hpp"

#include "sequoia/Core/DataStructures/PartitionedData.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path experimental_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void experimental_test::run_tests()
  {
    using namespace data_structures;

    bucketed_sequence<int> s{};
  }
}
