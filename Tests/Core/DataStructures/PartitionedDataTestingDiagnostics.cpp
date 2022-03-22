////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

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
    using namespace object;

    test<bucketed_sequence<T, object::independent<T>>>();
    test<bucketed_sequence<T, object::shared<T>>>();

    test<partitioned_sequence<T, object::independent<T>>>();
    test<partitioned_sequence<T, object::shared<T>>>();
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


    check(equivalence, LINE("Empty data inequivalent to non-empty data "), d, expected_t{{value_type{1}}});
    check(equivalence, LINE("Non-empty data inequivalent to empty data "), e, expected_t{{}});
    check(equivalence, LINE("Single partitions holding different elements compare not equal"), f, expected_t{{value_type{1}}});
    check(equivalence, LINE("Identical elements divided between different partitions compare notequal"), g, expected_t{{value_type{1}}, {value_type{2}}});
    check(equivalence, LINE("Identical elements divided between different partitions compare notequal"), h, expected_t{{value_type{1}, value_type{2}}});

    check(equality, LINE(""), d, e);
    check(equality, LINE(""), e, f);
    check(equality, LINE(""), g, h);

    check(equality, LINE("Size 0 should not compare equal to 1"), d.size(), 1_sz);
    check(equality, LINE("Size 1 should not compare equal to 0"), e.size(), 0_sz);

    check(equality, LINE(""), d.num_partitions(), 1_sz);
    check(equality, LINE(""), e.num_partitions(), 0_sz);
    check(equality, LINE(""), g.num_partitions(), 2_sz);
    check(equality, LINE(""), h.num_partitions(), 1_sz);

    check(equality, LINE(""), *e.begin_partition(0), value_type{2});
    check(equality, LINE(""), *e.cbegin_partition(0), value_type{2});
    check(equality, LINE(""), *e.rbegin_partition(0), value_type{2});
    check(equality, LINE(""), *e.crbegin_partition(0), value_type{2});
    check(equality, LINE(""), e[0][0], value_type{2});

    check(equality, LINE(""), *(g.end_partition(0) -1), value_type{1});
    check(equality, LINE(""), *(g.cend_partition(0) -1), value_type{1});
    check(equality, LINE(""), *(g.rend_partition(0) - 1), value_type{2});
    check(equality, LINE(""), *(g.crend_partition(0) - 1), value_type{2});
    check(equality, LINE(""), g[0][1], value_type{1});

    check(equality, LINE(""), *h.begin_partition(0), value_type{2});
    check(equality, LINE(""), *h.cbegin_partition(0), value_type{2});
    check(equality, LINE(""), *h.rbegin_partition(0), value_type{2});
    check(equality, LINE(""), *h.crbegin_partition(0), value_type{2});
    check(equality, LINE(""), h[0][0], value_type{2});

    check(equality, LINE(""), *h.begin_partition(1), value_type{1});
    check(equality, LINE(""), *h.cbegin_partition(1), value_type{1});
    check(equality, LINE(""), *h.rbegin_partition(1), value_type{1});
    check(equality, LINE(""), *h.crbegin_partition(1), value_type{1});
    check(equality, LINE(""), h[1][0], value_type{1});


    check(equality, LINE(""), *ce.begin_partition(0), value_type{2});
    check(equality, LINE(""), *ce.rbegin_partition(0), value_type{2});
    check(equality, LINE(""), ce[0][0], value_type{2});

    check(equality, LINE(""), *(cg.end_partition(0) -1), value_type{1});
    check(equality, LINE(""), *(cg.rend_partition(0) - 1), value_type{2});
    check(equality, LINE(""), cg[0][1], value_type{1});

    check(equality, LINE(""), *ch.begin_partition(0), value_type{2});
    check(equality, LINE(""), *ch.rbegin_partition(0), value_type{2});
    check(equality, LINE(""), ch[0][0], value_type{2});

    check(equality, LINE(""), *ch.begin_partition(1), value_type{1});
    check(equality, LINE(""), *h.crbegin_partition(1), value_type{1});
    check(equality, LINE(""), ch[1][0], value_type{1});
  }
}
