////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "PartitionedDataTestingDiagnostics.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view partitioned_data_false_positive_test::source_file() const noexcept
  {
    return __FILE__;
  }

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

    test<partitioned_sequence<T, data_sharing::independent<T>>>();
    test<partitioned_sequence<T, data_sharing::shared<T>>>();
  }
  
  template<class PartitionedData> void partitioned_data_false_positive_test::test()
  {
    using value_type = typename PartitionedData::value_type;
    using expected_t = std::initializer_list<std::initializer_list<value_type>>;
    
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
      

    check_equivalence(LINE("Empty data inequivalent to non-empty data "), d, expected_t{{value_type{1}}});
    check_equivalence(LINE("Non-empty data inequivalent to empty data "), e, expected_t{{}});
    check_equivalence(LINE("Single partitions holding different elements compare not equal"), f, expected_t{{value_type{1}}});
    check_equivalence(LINE("Identical elements divided between different partitions compare notequal"), g, expected_t{{value_type{1}}, {value_type{2}}});
    check_equivalence(LINE("Identical elements divided between different partitions compare notequal"), h, expected_t{{value_type{1}, value_type{2}}});

    check_equality(LINE(""), d, e);
    check_equality(LINE(""), e, f);
    check_equality(LINE(""), g, h);
       
    check_equality(LINE("Size 0 should not compare equal to 1"), d.size(), 1ul);
    check_equality(LINE("Size 1 should not compare equal to 0"), e.size(), 0ul);

    check_equality(LINE(""), d.num_partitions(), 1ul);
    check_equality(LINE(""), e.num_partitions(), 0ul);
    check_equality(LINE(""), g.num_partitions(), 2ul);
    check_equality(LINE(""), h.num_partitions(), 1ul);

    check_equality(LINE(""), *e.begin_partition(0), value_type{2});
    check_equality(LINE(""), *e.cbegin_partition(0), value_type{2});
    check_equality(LINE(""), *e.rbegin_partition(0), value_type{2});
    check_equality(LINE(""), *e.crbegin_partition(0), value_type{2});
    check_equality(LINE(""), e[0][0], value_type{2});

    check_equality(LINE(""), *(g.end_partition(0) -1), value_type{1});
    check_equality(LINE(""), *(g.cend_partition(0) -1), value_type{1});
    check_equality(LINE(""), *(g.rend_partition(0) - 1), value_type{2});
    check_equality(LINE(""), *(g.crend_partition(0) - 1), value_type{2});
    check_equality(LINE(""), g[0][1], value_type{1});

    check_equality(LINE(""), *h.begin_partition(0), value_type{2});
    check_equality(LINE(""), *h.cbegin_partition(0), value_type{2});
    check_equality(LINE(""), *h.rbegin_partition(0), value_type{2});
    check_equality(LINE(""), *h.crbegin_partition(0), value_type{2});
    check_equality(LINE(""), h[0][0], value_type{2});

    check_equality(LINE(""), *h.begin_partition(1), value_type{1});
    check_equality(LINE(""), *h.cbegin_partition(1), value_type{1});
    check_equality(LINE(""), *h.rbegin_partition(1), value_type{1});
    check_equality(LINE(""), *h.crbegin_partition(1), value_type{1});
    check_equality(LINE(""), h[1][0], value_type{1});

    
    check_equality(LINE(""), *ce.begin_partition(0), value_type{2});
    check_equality(LINE(""), *ce.rbegin_partition(0), value_type{2});
    check_equality(LINE(""), ce[0][0], value_type{2});

    check_equality(LINE(""), *(cg.end_partition(0) -1), value_type{1});
    check_equality(LINE(""), *(cg.rend_partition(0) - 1), value_type{2});
    check_equality(LINE(""), cg[0][1], value_type{1});

    check_equality(LINE(""), *ch.begin_partition(0), value_type{2});
    check_equality(LINE(""), *ch.rbegin_partition(0), value_type{2});
    check_equality(LINE(""), ch[0][0], value_type{2});

    check_equality(LINE(""), *ch.begin_partition(1), value_type{1});
    check_equality(LINE(""), *h.crbegin_partition(1), value_type{1});
    check_equality(LINE(""), ch[1][0], value_type{1});
  }
}
