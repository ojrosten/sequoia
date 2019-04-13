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
    test_set<int>();
    test_set<std::vector<double>>();
  }

  template<class T> void partitioned_data_false_positive_test::test_set()
  {
    using namespace data_structures;
    using namespace data_sharing;
    
    test<bucketed_storage<T, data_sharing::independent<T>>>();
    test<bucketed_storage<T, data_sharing::shared<T>>>();

    test<contiguous_storage<T, data_sharing::independent<T>>>();
    test<contiguous_storage<T, data_sharing::shared<T>>>();
  }
  
  template<class PartitionedData> void partitioned_data_false_positive_test::test()
  {
    using value_type = typename PartitionedData::value_type;
    using expected_t = std::initializer_list<std::initializer_list<value_type>>;

    this->failure_message_prefix(demangle<PartitionedData>());
    
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

    check_equality(d, e, LINE(""));
    check_equality(e, f, LINE(""));
    check_equality(g, h, LINE(""));
       
    check_equality(d.size(), 1ul, LINE("Size 0 should not compare equal to 1"));
    check_equality(e.size(), 0ul, LINE("Size 1 should not compare equal to 0"));

    check_equality(d.num_partitions(), 1ul, LINE(""));
    check_equality(e.num_partitions(), 0ul, LINE(""));
    check_equality(g.num_partitions(), 2ul, LINE(""));
    check_equality(h.num_partitions(), 1ul, LINE(""));

    check_equality(*e.begin_partition(0), value_type{2}, LINE(""));
    check_equality(*e.cbegin_partition(0), value_type{2}, LINE(""));
    check_equality(*e.rbegin_partition(0), value_type{2}, LINE(""));
    check_equality(*e.crbegin_partition(0), value_type{2}, LINE(""));
    check_equality(e[0][0], value_type{2}, LINE(""));

    check_equality(*(g.end_partition(0) -1), value_type{1}, LINE(""));
    check_equality(*(g.cend_partition(0) -1), value_type{1}, LINE(""));
    check_equality(*(g.rend_partition(0) - 1), value_type{2}, LINE(""));
    check_equality(*(g.crend_partition(0) - 1), value_type{2}, LINE(""));
    check_equality(g[0][1], value_type{1}, LINE(""));

    check_equality(*h.begin_partition(0), value_type{2}, LINE(""));
    check_equality(*h.cbegin_partition(0), value_type{2}, LINE(""));
    check_equality(*h.rbegin_partition(0), value_type{2}, LINE(""));
    check_equality(*h.crbegin_partition(0), value_type{2}, LINE(""));
    check_equality(h[0][0], value_type{2}, LINE(""));

    check_equality(*h.begin_partition(1), value_type{1}, LINE(""));
    check_equality(*h.cbegin_partition(1), value_type{1}, LINE(""));
    check_equality(*h.rbegin_partition(1), value_type{1}, LINE(""));
    check_equality(*h.crbegin_partition(1), value_type{1}, LINE(""));
    check_equality(h[1][0], value_type{1}, LINE(""));

    
    check_equality(*ce.begin_partition(0), value_type{2}, LINE(""));
    check_equality(*ce.rbegin_partition(0), value_type{2}, LINE(""));
    check_equality(ce[0][0], value_type{2}, LINE(""));

    check_equality(*(cg.end_partition(0) -1), value_type{1}, LINE(""));
    check_equality(*(cg.rend_partition(0) - 1), value_type{2}, LINE(""));
    check_equality(cg[0][1], value_type{1}, LINE(""));

    check_equality(*ch.begin_partition(0), value_type{2}, LINE(""));
    check_equality(*ch.rbegin_partition(0), value_type{2}, LINE(""));
    check_equality(ch[0][0], value_type{2}, LINE(""));

    check_equality(*ch.begin_partition(1), value_type{1}, LINE(""));
    check_equality(*h.crbegin_partition(1), value_type{1}, LINE(""));
    check_equality(ch[1][0], value_type{1}, LINE(""));
  }
}
