////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "BucketedSequenceRegularTest.hpp"
#include "PartitionedDataGenericTests.hpp"

namespace sequoia::testing
{
  namespace
  {
    struct non_assignable_element
    {
      const int value{};
    };

    struct move_only_element
    {
      int value{};

      move_only_element() = default;

      move_only_element(move_only_element&&) noexcept = default;

      move_only_element& operator=(move_only_element&&) noexcept = default;
    };

    struct assign_only_element
    {
      int value{};

      assign_only_element() = default;

      assign_only_element(const assign_only_element&) = delete;

      assign_only_element& operator=(const assign_only_element&) = default;
    };

    using namespace partitioned_data;

    template<class PartitionedData>
    struct bucketed_operations : partitioned_data_operations<PartitionedData>
    {
      using data_type = partitioned_data_operations<PartitionedData>::data_type;

      static void execute(regular_test& t)
      {
        auto trg{partitioned_data_operations<PartitionedData>::make_transition_graph(t)};

        // begin 'empty'
        trg.join(data_description::empty,
          data_description::empty,
          t.report(""),
          [&t](data_type d) -> data_type {
            t.check(equality, "", d.num_partitions_capacity(), 0uz);
            t.check(equality, "", d.partition_capacity(0), 0uz);
            return d;
          }
        );

        trg.join(data_description::empty,
          data_description::empty,
          t.report(""),
          [&t](data_type d) -> data_type {
            d.reserve_partitions(4);
            t.check(equality, "", d.num_partitions_capacity(), 4uz);
            t.check(equality, "", d.partition_capacity(0), 0uz);

            d.shrink_num_partitions_to_fit();
            t.check(equality, "May fail if shrink to fit impl does not reduce capacity", d.num_partitions_capacity(), 0uz);
            t.check(equality, "", d.partition_capacity(0), 0uz);

            return d;
          }
        );

        // end 'empty'
        // begin 'empty_partition'

        trg.join(data_description::empty_partition,
                 data_description::empty_partition,
                 t.report(""),
                 [&t](data_type d) -> data_type {
                   auto i{d.begin_partition(1)};
                   t.check(equality, "", i, {d.end_partition(0).base_iterator(), PartitionedData::npos});
                   return d;
                 }
          );

        trg.join(data_description::empty_partition,
                 data_description::empty_partition,
                 t.report(""),
                 [&t](data_type d) -> data_type {
                   auto i{d.erase_from_partition(d.cbegin_partition(1))};
                   t.check(equality, "", i, {d.end_partition(0).base_iterator(), PartitionedData::npos});
                   return d;
                 }
          );

        trg.join(data_description::empty_partition,
                 data_description::empty_partition,
                 t.report(""),
                 [&t](data_type d) -> data_type {
                   t.check(equality, "", d.partition_capacity(0), 0uz);
                   t.check(equality, "", d.partition_capacity(1), 0uz);
                   
                   d.reserve_partition(0, 4);
                   t.check(equality, "", d.partition_capacity(0), 4uz);
                   t.check(equality, "", d.partition_capacity(1), 0uz);
                   
                   d.shrink_to_fit(0);
                   t.check(equality, "May fail if shrink to fit impl does not reduce capacity", d.partition_capacity(0), 0uz);
                 
                   return d;
                 }
        );

        // end 'empty_partition'

        auto checker{
            [&t](std::string_view description, const data_type& obtained, const data_type& prediction, const data_type& parent, std::size_t host, std::size_t target) {
              t.check(equality, {description, no_source_location}, obtained, prediction);
              if(host != target) t.check_semantics({description, no_source_location}, prediction, parent);
            }
        };

        transition_checker<data_type>::check(t.report(""), trg, checker);
      }
    };
  }

  [[nodiscard]]
  std::filesystem::path bucketed_sequence_regular_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void bucketed_sequence_regular_test::run_tests()
  {
    using namespace data_structures;
    test_copyability();
    bucketed_operations<bucketed_sequence<int>>::execute(*this);
  }

  void bucketed_sequence_regular_test::test_copyability()
  {
    using namespace data_structures;
    using move_only_sequence = bucketed_sequence<move_only_element>;
    using copyable_sequence  = bucketed_sequence<int>;
    using move_only_vector_sequence = bucketed_sequence<std::vector<move_only_element>>;

    STATIC_CHECK(!std::is_copy_constructible_v<move_only_sequence>);
    STATIC_CHECK(!std::is_copy_assignable_v<move_only_sequence>);
    STATIC_CHECK(!std::is_constructible_v<move_only_sequence,
                                          const move_only_sequence&,
                                          move_only_sequence::allocator_type>);
    STATIC_CHECK( std::is_nothrow_move_constructible_v<move_only_sequence>);

    STATIC_CHECK( std::is_copy_constructible_v<copyable_sequence>);
    STATIC_CHECK( std::is_copy_assignable_v<copyable_sequence>);

    STATIC_CHECK( std::is_copy_constructible_v<bucketed_sequence<non_assignable_element>>);
    STATIC_CHECK(!std::is_copy_assignable_v<bucketed_sequence<non_assignable_element>>);
    STATIC_CHECK( std::is_constructible_v<copyable_sequence,
                                          const copyable_sequence&,
                                          copyable_sequence::allocator_type>);

    STATIC_CHECK(!std::is_copy_constructible_v<bucketed_sequence<assign_only_element>>);
    STATIC_CHECK(!std::is_copy_assignable_v<bucketed_sequence<assign_only_element>>);

    STATIC_CHECK(!std::is_copy_constructible_v<move_only_vector_sequence>);
    STATIC_CHECK(!std::is_copy_assignable_v<move_only_vector_sequence>);
    STATIC_CHECK(!std::is_constructible_v<move_only_vector_sequence,
                                          const move_only_vector_sequence&,
                                          move_only_vector_sequence::allocator_type>);

    STATIC_CHECK( std::is_copy_constructible_v<bucketed_sequence<std::vector<int>>>);
    STATIC_CHECK( std::is_copy_assignable_v<bucketed_sequence<std::vector<int>>>);
    STATIC_CHECK( std::is_copy_assignable_v<bucketed_sequence<std::map<int, int>>>);
  }
}
