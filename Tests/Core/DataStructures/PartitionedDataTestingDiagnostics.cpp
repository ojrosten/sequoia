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
  std::filesystem::path partitioned_data_false_positive_test::source_file() const
  {
    return std::source_location::current().file_name();
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

    test<bucketed_sequence<T>>();
    test<partitioned_sequence<T>>();
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


    check(equivalence, report("Empty data inequivalent to non-empty data "), d, expected_t{{value_type{1}}});
    check(equivalence, report("Non-empty data inequivalent to empty data "), e, expected_t{{}});
    check(equivalence, report("Single partitions holding different elements compare not equal"), f, expected_t{{value_type{1}}});
    check(equivalence, report("Identical elements divided between different partitions compare notequal"), g, expected_t{{value_type{1}}, {value_type{2}}});
    check(equivalence, report("Identical elements divided between different partitions compare notequal"), h, expected_t{{value_type{1}, value_type{2}}});

    check(equality, report(""), d, e);
    check(equality, report(""), e, f);
    check(equality, report(""), g, h);

    check(equality, report("Size 0 should not compare equal to 1"), d.size(), 1_sz);
    check(equality, report("Size 1 should not compare equal to 0"), e.size(), 0_sz);

    check(equality, report(""), d.num_partitions(), 1_sz);
    check(equality, report(""), e.num_partitions(), 0_sz);
    check(equality, report(""), g.num_partitions(), 2_sz);
    check(equality, report(""), h.num_partitions(), 1_sz);

    check(equality, report(""), *e.begin_partition(0), value_type{2});
    check(equality, report(""), *e.cbegin_partition(0), value_type{2});
    check(equality, report(""), *e.rbegin_partition(0), value_type{2});
    check(equality, report(""), *e.crbegin_partition(0), value_type{2});
    check(equality, report(""), e[0][0], value_type{2});

    check(equality, report(""), *(g.end_partition(0) -1), value_type{1});
    check(equality, report(""), *(g.cend_partition(0) -1), value_type{1});
    check(equality, report(""), *(g.rend_partition(0) - 1), value_type{2});
    check(equality, report(""), *(g.crend_partition(0) - 1), value_type{2});
    check(equality, report(""), g[0][1], value_type{1});

    check(equality, report(""), *h.begin_partition(0), value_type{2});
    check(equality, report(""), *h.cbegin_partition(0), value_type{2});
    check(equality, report(""), *h.rbegin_partition(0), value_type{2});
    check(equality, report(""), *h.crbegin_partition(0), value_type{2});
    check(equality, report(""), h[0][0], value_type{2});

    check(equality, report(""), *h.begin_partition(1), value_type{1});
    check(equality, report(""), *h.cbegin_partition(1), value_type{1});
    check(equality, report(""), *h.rbegin_partition(1), value_type{1});
    check(equality, report(""), *h.crbegin_partition(1), value_type{1});
    check(equality, report(""), h[1][0], value_type{1});


    check(equality, report(""), *ce.begin_partition(0), value_type{2});
    check(equality, report(""), *ce.rbegin_partition(0), value_type{2});
    check(equality, report(""), ce[0][0], value_type{2});

    check(equality, report(""), *(cg.end_partition(0) -1), value_type{1});
    check(equality, report(""), *(cg.rend_partition(0) - 1), value_type{2});
    check(equality, report(""), cg[0][1], value_type{1});

    check(equality, report(""), *ch.begin_partition(0), value_type{2});
    check(equality, report(""), *ch.rbegin_partition(0), value_type{2});
    check(equality, report(""), ch[0][0], value_type{2});

    check(equality, report(""), *ch.begin_partition(1), value_type{1});
    check(equality, report(""), *h.crbegin_partition(1), value_type{1});
    check(equality, report(""), ch[1][0], value_type{1});
  }
}
