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
  std::filesystem::path partitioned_data_false_positive_test::source_file() const noexcept
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

    test<bucketed_sequence<T, object::by_value<T>>>();
    test<bucketed_sequence<T, object::shared<T>>>();

    test<partitioned_sequence<T, object::by_value<T>>>();
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


    check(equivalence, report_line("Empty data inequivalent to non-empty data "), d, expected_t{{value_type{1}}});
    check(equivalence, report_line("Non-empty data inequivalent to empty data "), e, expected_t{{}});
    check(equivalence, report_line("Single partitions holding different elements compare not equal"), f, expected_t{{value_type{1}}});
    check(equivalence, report_line("Identical elements divided between different partitions compare notequal"), g, expected_t{{value_type{1}}, {value_type{2}}});
    check(equivalence, report_line("Identical elements divided between different partitions compare notequal"), h, expected_t{{value_type{1}, value_type{2}}});

    check(equality, report_line(""), d, e);
    check(equality, report_line(""), e, f);
    check(equality, report_line(""), g, h);

    check(equality, report_line("Size 0 should not compare equal to 1"), d.size(), 1_sz);
    check(equality, report_line("Size 1 should not compare equal to 0"), e.size(), 0_sz);

    check(equality, report_line(""), d.num_partitions(), 1_sz);
    check(equality, report_line(""), e.num_partitions(), 0_sz);
    check(equality, report_line(""), g.num_partitions(), 2_sz);
    check(equality, report_line(""), h.num_partitions(), 1_sz);

    check(equality, report_line(""), *e.begin_partition(0), value_type{2});
    check(equality, report_line(""), *e.cbegin_partition(0), value_type{2});
    check(equality, report_line(""), *e.rbegin_partition(0), value_type{2});
    check(equality, report_line(""), *e.crbegin_partition(0), value_type{2});
    check(equality, report_line(""), e[0][0], value_type{2});

    check(equality, report_line(""), *(g.end_partition(0) -1), value_type{1});
    check(equality, report_line(""), *(g.cend_partition(0) -1), value_type{1});
    check(equality, report_line(""), *(g.rend_partition(0) - 1), value_type{2});
    check(equality, report_line(""), *(g.crend_partition(0) - 1), value_type{2});
    check(equality, report_line(""), g[0][1], value_type{1});

    check(equality, report_line(""), *h.begin_partition(0), value_type{2});
    check(equality, report_line(""), *h.cbegin_partition(0), value_type{2});
    check(equality, report_line(""), *h.rbegin_partition(0), value_type{2});
    check(equality, report_line(""), *h.crbegin_partition(0), value_type{2});
    check(equality, report_line(""), h[0][0], value_type{2});

    check(equality, report_line(""), *h.begin_partition(1), value_type{1});
    check(equality, report_line(""), *h.cbegin_partition(1), value_type{1});
    check(equality, report_line(""), *h.rbegin_partition(1), value_type{1});
    check(equality, report_line(""), *h.crbegin_partition(1), value_type{1});
    check(equality, report_line(""), h[1][0], value_type{1});


    check(equality, report_line(""), *ce.begin_partition(0), value_type{2});
    check(equality, report_line(""), *ce.rbegin_partition(0), value_type{2});
    check(equality, report_line(""), ce[0][0], value_type{2});

    check(equality, report_line(""), *(cg.end_partition(0) -1), value_type{1});
    check(equality, report_line(""), *(cg.rend_partition(0) - 1), value_type{2});
    check(equality, report_line(""), cg[0][1], value_type{1});

    check(equality, report_line(""), *ch.begin_partition(0), value_type{2});
    check(equality, report_line(""), *ch.rbegin_partition(0), value_type{2});
    check(equality, report_line(""), ch[0][0], value_type{2});

    check(equality, report_line(""), *ch.begin_partition(1), value_type{1});
    check(equality, report_line(""), *h.crbegin_partition(1), value_type{1});
    check(equality, report_line(""), ch[1][0], value_type{1});
  }
}
