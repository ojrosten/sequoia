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
    test<bucketed_storage<int, data_sharing::shared<int>>>();

    test<contiguous_storage<int, data_sharing::independent<int>>>();
    test<contiguous_storage<int, data_sharing::shared<int>>>();
  }

  template<class PartitionedData> void partitioned_data_false_positive_test::test()
  {
    using value_type = typename PartitionedData::value_type;
    using expected_t = std::initializer_list<std::initializer_list<value_type>>;

    this->failure_message_prefix(type_to_string<PartitionedData>::str());
    
    PartitionedData
      d{},
      e{{value_type{1}}},
      f{{value_type{2}}},
      g{{value_type{1}, value_type{2}}},
      h{{value_type{1}}, {value_type{2}}};

    const PartitionedData
      ce{{value_type{1}}},
      cg{{value_type{1}, value_type{2}}},
      ch{{value_type{1}}, {value_type{2}}};
      

    check_equivalence(d, expected_t{{value_type{1}}}, LINE("Empty data inequivalent to non-empty data "));
    check_equivalence(e, expected_t{{}}, LINE("Non-empty data inequivalent to empty data "));
    check_equivalence(f, expected_t{{value_type{1}}}, LINE("Single partitions holding different elements compare not equal"));
    check_equivalence(g, expected_t{{value_type{1}}, {value_type{2}}}, LINE("Identical elements divided between different partitions compare notequal"));
    check_equivalence(h, expected_t{{value_type{1}, value_type{2}}}, LINE("Identical elements divided between different partitions compare notequal"));
       
    check_equality(1ul, d.size(), LINE("Size 0 should not compare equal to 1"));
    check_equality(0ul, e.size(), LINE("Size 1 should not compare equal to 0"));

    check_equality(1ul, d.num_partitions(), LINE(""));
    check_equality(0ul, e.num_partitions(), LINE(""));
    check_equality(2ul, g.num_partitions(), LINE(""));
    check_equality(1ul, h.num_partitions(), LINE(""));

    check_equality(value_type{2}, *e.begin_partition(0), LINE(""));
    check_equality(value_type{2}, *e.cbegin_partition(0), LINE(""));
    check_equality(value_type{2}, *e.rbegin_partition(0), LINE(""));
    check_equality(value_type{2}, *e.crbegin_partition(0), LINE(""));
    check_equality(value_type{2}, e[0][0], LINE(""));

    check_equality(value_type{1}, *(g.end_partition(0) -1), LINE(""));
    check_equality(value_type{1}, *(g.cend_partition(0) -1), LINE(""));
    check_equality(value_type{2}, *(g.rend_partition(0) - 1), LINE(""));
    check_equality(value_type{2}, *(g.crend_partition(0) - 1), LINE(""));
    check_equality(value_type{1}, g[0][1], LINE(""));

    check_equality(value_type{2}, *h.begin_partition(0), LINE(""));
    check_equality(value_type{2}, *h.cbegin_partition(0), LINE(""));
    check_equality(value_type{2}, *h.rbegin_partition(0), LINE(""));
    check_equality(value_type{2}, *h.crbegin_partition(0), LINE(""));
    check_equality(value_type{2}, h[0][0], LINE(""));

    check_equality(value_type{1}, *h.begin_partition(1), LINE(""));
    check_equality(value_type{1}, *h.cbegin_partition(1), LINE(""));
    check_equality(value_type{1}, *h.rbegin_partition(1), LINE(""));
    check_equality(value_type{1}, *h.crbegin_partition(1), LINE(""));
    check_equality(value_type{1}, h[1][0], LINE(""));

    
    check_equality(value_type{2}, *ce.begin_partition(0), LINE(""));
    check_equality(value_type{2}, *ce.rbegin_partition(0), LINE(""));
    check_equality(value_type{2}, ce[0][0], LINE(""));

    check_equality(value_type{1}, *(cg.end_partition(0) -1), LINE(""));
    check_equality(value_type{2}, *(cg.rend_partition(0) - 1), LINE(""));
    check_equality(value_type{1}, cg[0][1], LINE(""));

    check_equality(value_type{2}, *ch.begin_partition(0), LINE(""));
    check_equality(value_type{2}, *ch.rbegin_partition(0), LINE(""));
    check_equality(value_type{2}, ch[0][0], LINE(""));

    check_equality(value_type{1}, *ch.begin_partition(1), LINE(""));
    check_equality(value_type{1}, *h.crbegin_partition(1), LINE(""));
    check_equality(value_type{1}, ch[1][0], LINE(""));
  }
}
