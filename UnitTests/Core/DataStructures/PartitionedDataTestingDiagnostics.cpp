////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "PartitionedDataTestingDiagnostics.hpp"

namespace sequoia::unit_testing
{
  void partitioned_data_false_positive_test::run_tests()
  {
    using namespace data_structures;
    using namespace data_sharing;
    
    test<bucketed_storage<int, data_sharing::independent<int>>>();
  }

  template<class PartitionedData> void partitioned_data_false_positive_test::test()
  {
    using value_type = typename PartitionedData::value_type;
    using expected_t = std::initializer_list<std::initializer_list<value_type>>;
    
    PartitionedData d{}, e{{value_type{1}}};

    check_equivalence(d, expected_t{{value_type{1}}}, LINE("Empty data inequivalent to non-empty data "));
  }
}
