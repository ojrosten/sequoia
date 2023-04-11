////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "PartitionedDataAllocationTest.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view partitioned_data_allocation_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void partitioned_data_allocation_test::run_tests()
  {
    do_allocation_tests(*this);
  }


  template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void partitioned_data_allocation_test::test_allocation()
  {
    using namespace object;

    test_bucketed_allocation<int, by_value<int>, PropagateCopy, PropagateMove, PropagateSwap>();
    test_bucketed_allocation<int, shared<int>, PropagateCopy, PropagateMove, PropagateSwap>();

    test_contiguous_allocation<int, by_value<int>, PropagateCopy, PropagateMove, PropagateSwap>();
    test_contiguous_allocation<int, shared<int>, PropagateCopy, PropagateMove, PropagateSwap>();
  }

  template<class T, class Handler, bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void partitioned_data_allocation_test::test_bucketed_allocation()
  {
    using namespace data_structures;

    using storage = bucketed_sequence<T, Handler, custom_bucketed_storage_traits<T, Handler, PropagateCopy, PropagateMove, PropagateSwap>>;
    using allocator = typename storage::allocator_type;
    using prediction = std::initializer_list<std::initializer_list<int>>;

    auto makeMessage{
      [](std::string_view message) {
        return add_type_info<storage>(message);
      }
    };

    auto partitionMaker{ [](storage& s) { s.add_slot(); } };

    // null; [0,2][1]
    auto[s,t]{check_semantics(report_line(""),
                    [](){ return storage(allocator{}); },
                    [](){ return storage{{{0,2}, {1}}, allocator{}}; },
                    partitionMaker,
                    allocation_info{bucket_alloc_getter<storage>{},
                                    {0_c, {1_c,1_mu}, {1_anp,1_awp}},
                                    { {0_c, {2_c,0_mu}, {2_anp,2_awp}, {0_containers, 2_containers, 3_postmutation}} }
                    })};

    check(equivalence, report_line(makeMessage("")), s, prediction{});
    check(equivalence, report_line(makeMessage("")), t, prediction{{0,2}, {1}});

    s.add_slot();
    // []

    check(equality, report_line(makeMessage("")), s, storage{{{}}, allocator{}});

    auto mutator{
      [](storage& s) {
        s.add_slot();
        s.push_back_to_partition(s.num_partitions() - 1, 3);
      }
    };

    check_semantics(report_line(""),
                    s,
                    t,
                    mutator,
                    allocation_info{bucket_alloc_getter<storage>{},
                                    {1_c, {1_c,1_mu}, {1_anp,1_awp}},
                                    { {0_c, {2_c,1_mu}, {2_anp,2_awp}, {1_containers, 2_containers, 3_postmutation}} }
                    });
  }

  template<class T, class Handler, bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void partitioned_data_allocation_test::test_contiguous_allocation()
  {
    using namespace data_structures;

    using storage = partitioned_sequence<T, Handler, custom_partitioned_sequence_traits<T, Handler, PropagateCopy, PropagateMove, PropagateSwap>>;
    using allocator = typename storage::allocator_type;
    using partitions_allocator = typename storage::traits_type::partitions_allocator_type;
    using prediction = std::initializer_list<std::initializer_list<int>>;

    auto makeMessage{
      [](std::string_view message) {
        return add_type_info<storage>(message);
      }
    };

    auto partitionMaker{ [](storage& s) { s.add_slot(); } };
    // null; [0,2][1]
    auto[s,t]{check_semantics(report_line(add_type_info<storage>("")),
                              [](){ return storage{allocator{}, partitions_allocator{}}; },
                              [](){ return storage{{{0,2}, {1}}, allocator{}, partitions_allocator{}}; },
                              partitionMaker,
                              allocation_info{contiguous_alloc_getter<storage>{}, {0_c, {1_c,0_mu}, {1_anp, 1_awp}}},
                              allocation_info{partitions_alloc_getter<storage>{}, {0_c, {1_c,1_mu}, {1_anp, 1_awp}}})};

    check(equivalence, report_line(makeMessage("")), s, prediction{});
    check(equivalence, report_line(makeMessage("")), t, prediction{{0,2}, {1}});

    s.add_slot();
    // []

    check(equality, report_line(makeMessage("")), s, storage{{{}}, allocator{}, partitions_allocator{}});
  }
}
