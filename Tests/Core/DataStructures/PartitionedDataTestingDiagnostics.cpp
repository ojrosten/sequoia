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
  std::filesystem::path partitioned_data_false_negative_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void partitioned_data_false_negative_test::run_tests()
  {
    test_set<int>();
    test_set<std::vector<double>>();
  }

  template<class T> void partitioned_data_false_negative_test::test_set()
  {
    using namespace data_structures;
    using namespace object;

    test<bucketed_sequence<T>>();
    test<partitioned_sequence<T>>();
  }

  template<class PartitionedData> void partitioned_data_false_negative_test::test()
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


    check(equivalence, "Empty data inequivalent to non-empty data ", d, expected_t{{value_type{1}}});
    check(equivalence, "Non-empty data inequivalent to empty data ", e, expected_t{{}});
    check(equivalence, "Single partitions holding different elements compare not equal", f, expected_t{{value_type{1}}});
    check(equivalence, "Identical elements divided between different partitions compare notequal", g, expected_t{{value_type{1}}, {value_type{2}}});
    check(equivalence, "Identical elements divided between different partitions compare notequal", h, expected_t{{value_type{1}, value_type{2}}});

    check(equality, "", d, e);
    check(equality, "", e, f);
    check(equality, "", g, h);

    check(equality, "Size 0 should not compare equal to 1", d.size(), 1uz);
    check(equality, "Size 1 should not compare equal to 0", e.size(), 0uz);

    check(equality, "", d.num_partitions(), 1uz);
    check(equality, "", e.num_partitions(), 0uz);
    check(equality, "", g.num_partitions(), 2uz);
    check(equality, "", h.num_partitions(), 1uz);

    check(equality, "", *e.begin_partition(0), value_type{2});
    check(equality, "", *e.cbegin_partition(0), value_type{2});
    check(equality, "", *e.rbegin_partition(0), value_type{2});
    check(equality, "", *e.crbegin_partition(0), value_type{2});
    check(equality, "", e[0][0], value_type{2});

    check(equality, "", *(g.end_partition(0) -1), value_type{1});
    check(equality, "", *(g.cend_partition(0) -1), value_type{1});
    check(equality, "", *(g.rend_partition(0) - 1), value_type{2});
    check(equality, "", *(g.crend_partition(0) - 1), value_type{2});
    check(equality, "", g[0][1], value_type{1});

    check(equality, "", *h.begin_partition(0), value_type{2});
    check(equality, "", *h.cbegin_partition(0), value_type{2});
    check(equality, "", *h.rbegin_partition(0), value_type{2});
    check(equality, "", *h.crbegin_partition(0), value_type{2});
    check(equality, "", h[0][0], value_type{2});

    check(equality, "", *h.begin_partition(1), value_type{1});
    check(equality, "", *h.cbegin_partition(1), value_type{1});
    check(equality, "", *h.rbegin_partition(1), value_type{1});
    check(equality, "", *h.crbegin_partition(1), value_type{1});
    check(equality, "", h[1][0], value_type{1});


    check(equality, "", *ce.begin_partition(0), value_type{2});
    check(equality, "", *ce.rbegin_partition(0), value_type{2});
    check(equality, "", ce[0][0], value_type{2});

    check(equality, "", *(cg.end_partition(0) -1), value_type{1});
    check(equality, "", *(cg.rend_partition(0) - 1), value_type{2});
    check(equality, "", cg[0][1], value_type{1});

    check(equality, "", *ch.begin_partition(0), value_type{2});
    check(equality, "", *ch.rbegin_partition(0), value_type{2});
    check(equality, "", ch[0][0], value_type{2});

    check(equality, "", *ch.begin_partition(1), value_type{1});
    check(equality, "", *h.crbegin_partition(1), value_type{1});
    check(equality, "", ch[1][0], value_type{1});
  }
}
