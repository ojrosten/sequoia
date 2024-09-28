////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "PartitionedSequenceAllocationTest.hpp"
#include "PartitionedDataAllocationTestingUtilities.hpp"

namespace sequoia::testing
{
  namespace
  {
    template<class S, bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
    using allocator_template = shared_counting_allocator<S, PropagateCopy, PropagateMove, PropagateSwap>;

    template<class S, bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
    using allocator_type
      = std::scoped_allocator_adaptor<
        allocator_template<S, PropagateCopy, PropagateMove, PropagateSwap>,
        allocator_template<typename S::value_type, PropagateCopy, PropagateMove, PropagateSwap>
      >;
  }

  [[nodiscard]]
  std::filesystem::path partitioned_sequence_allocation_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void partitioned_sequence_allocation_test::run_tests()
  {
    do_allocation_tests(*this);
  }


  template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void partitioned_sequence_allocation_test::test_allocation()
  {
    test_contiguous_allocation<int, PropagateCopy, PropagateMove, PropagateSwap>();
  }

  template<class T, bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void partitioned_sequence_allocation_test::test_contiguous_allocation()
  {
    using namespace data_structures;

    using storage = typename custom_partitioned_sequence_generator<T, PropagateCopy, PropagateMove, PropagateSwap>::storage_type;

    using allocator = typename storage::allocator_type;
    using partitions_allocator = typename storage::partitions_allocator_type;
    using prediction = std::initializer_list<std::initializer_list<int>>;

    auto makeMessage{
      [](std::string_view message) {
        return add_type_info<storage>(message);
      }
    };

    auto partitionMaker{ [](storage& s) { s.add_slot(); } };
    // null; [0,2][1]
    auto[s,t]{check_semantics(report(add_type_info<storage>("")),
                              [](){ return storage{allocator{}, partitions_allocator{}}; },
                              [](){ return storage{{{0,2}, {1}}, allocator{}, partitions_allocator{}}; },
                              partitionMaker,
                              allocation_info{contiguous_alloc_getter<storage>{}, {0_c, {1_c,0_mu}, {1_anp, 1_awp}}},
                              allocation_info{partitions_alloc_getter<storage>{}, {0_c, {1_c,1_mu}, {1_anp, 1_awp}}})};

    check(equivalence, report(makeMessage("")), s, prediction{});
    check(equivalence, report(makeMessage("")), t, prediction{{0,2}, {1}});

    s.add_slot();
    // []

    check(equality, report(makeMessage("")), s, storage{{{}}, allocator{}, partitions_allocator{}});
  }
}
