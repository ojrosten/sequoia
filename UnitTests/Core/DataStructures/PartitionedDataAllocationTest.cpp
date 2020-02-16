////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "PartitionedDataAllocationTest.hpp"

namespace sequoia::unit_testing
{
  [[nodiscard]]
  std::string_view partitioned_data_allocation_test::source_file_name() const noexcept
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
    using namespace data_sharing;

    test_bucketed_allocation<int, independent<int>, PropagateCopy, PropagateMove, PropagateSwap>();
    test_bucketed_allocation<int, shared<int>, PropagateCopy, PropagateMove, PropagateSwap>();

    test_contiguous_allocation<int, independent<int>, PropagateCopy, PropagateMove, PropagateSwap>();
    test_contiguous_allocation<int, shared<int>, PropagateCopy, PropagateMove, PropagateSwap>();
  }

  template<class T, class SharingPolicy, bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void partitioned_data_allocation_test::test_bucketed_allocation()
  {
    using namespace data_structures;

    using storage = bucketed_storage<T, SharingPolicy, custom_bucketed_storage_traits<T, SharingPolicy, PropagateCopy, PropagateMove, PropagateSwap>>;
    using allocator = typename storage::allocator_type;
    using prediction = std::initializer_list<std::initializer_list<int>>;

    auto makeMessage{
      [](std::string_view message) {
        return add_type_info<storage>(message);
      }
    };

    // null; [0,2][1]
      
    storage
      s(allocator{}),
      t{{{0,2}, {1}}, allocator{}};

    auto allocGetter{
      [](const storage& s) {
        return s.get_allocator();
      }
    };

    auto innerAllocGetter{
      [](const storage& s) {
        return s.get_allocator().inner_allocator();
      }
    };

    check_equivalence(LINE(makeMessage("")), s, prediction{});
    check_equivalence(LINE(makeMessage("")), t, prediction{{0,2}, {1}});
    check_equality(LINE(makeMessage("")), allocGetter(s).allocs(), 0);
    check_equality(LINE(makeMessage("Only a single allocation necessary due to reservation")), allocGetter(t).allocs(), 1);
    check_equality(LINE(makeMessage("Only a single allocation per bucket due to reservation")), innerAllocGetter(t).allocs(), 2);
 
    auto partitionMaker{
      [](storage& s) {
        s.add_slot();
      }
    };

    using info = allocation_info<storage, allocator>;

    check_regular_semantics(LINE(""), s, t, partitionMaker,
                            info{allocGetter, {{0, {1,1}, {1,1}}, {0, {2,0}, {2,2}}}});
      
    s.add_slot();
    // []
        
    check_equality(LINE(makeMessage("")), s, storage{{{}}, allocator{}});

    auto mutator{
      [](storage& s) {
        s.add_slot();
        s.push_back_to_partition(s.num_partitions() - 1, 3);
      }
    };

    check_regular_semantics(LINE(""), s, t, mutator,
                            info{allocGetter, {{1, {1,1}, {1,1}}, {0, {2,1}, {2,2}}}});
  }
  
  template<class T, class SharingPolicy, bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void partitioned_data_allocation_test::test_contiguous_allocation()
  {
    using namespace data_structures;
      
    using storage = partitioned_sequence<T, SharingPolicy, custom_partitioned_sequence_traits<T, SharingPolicy, PropagateCopy, PropagateMove, PropagateSwap>>;
    using allocator = typename storage::allocator_type;
    using partitions_allocator = typename storage::traits_type::partitions_allocator_type;
    using prediction = std::initializer_list<std::initializer_list<int>>;

    auto makeMessage{
      [](std::string_view message) {
        return add_type_info<storage>(message);
      }
    };

    auto partitionsAllocGetter{
      [](const storage& s) {
        return s.get_partitions_allocator();
      }        
    };

    auto allocGetter{
      [](const storage& s){
        return s.get_allocator();          
      }
    };

    // null; [0,2][1]
    storage
      s{allocator{}, partitions_allocator{}},
      t{{{0,2}, {1}}, allocator{}, partitions_allocator{}};

      check_equivalence(LINE(makeMessage("")), s, prediction{});
      check_equivalence(LINE(makeMessage("")), t, prediction{{0,2}, {1}});
      check_equality(LINE(makeMessage("")), partitionsAllocGetter(s).allocs(), 0);
      check_equality(LINE(makeMessage("")), allocGetter(s).allocs(), 0);
      check_equality(LINE(makeMessage("Only a single allocation necessary due to reservation")), partitionsAllocGetter(t).allocs(), 1);
      check_equality(LINE(makeMessage("Only a single allocation necessary due to reservation")), allocGetter(t).allocs(), 1);

      auto partitionMaker{
        [](storage& s) {
          s.add_slot();
        }
      };
        
      check_regular_semantics(LINE(add_type_info<storage>("")), s, t, partitionMaker,
                              allocation_info<storage, allocator>{allocGetter, {0, {1,0}, {1, 1}}},
                              allocation_info<storage, partitions_allocator>{partitionsAllocGetter, {0, {1,1}, {1, 1}}});

      s.add_slot();
      // []

      check_equality(LINE(makeMessage("")), s, storage{{{}}, allocator{}, partitions_allocator{}});
  }
}
