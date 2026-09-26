////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "PartitionedSequenceRegularTest.hpp"
#include "PartitionedDataGenericTests.hpp"
#include "sequoia/Core/DataStructures/PartitionedData.hpp"

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

    using namespace partitioned_data;

    template<class PartitionedData>
    struct partitioned_operations : partitioned_data_operations<PartitionedData>
    {
      using data_t = partitioned_data_operations<PartitionedData>::data_t;

      static void execute(regular_test& t)
      {
        auto trg{partitioned_data_operations<PartitionedData>::make_transition_graph(t)};

        // begin 'empty'
        trg.join(data_description::empty,
                 data_description::empty,
                 t.report(""),
                 [&t](data_t d) -> data_t {
                   auto i{d.erase_from_partition(d.cbegin_partition(0))};
                   t.check(equality, "Erase from non-existent partition", i, d.begin_partition(0));
                   return d;
                 }
          );

        trg.join(data_description::empty,
          data_description::empty,
          t.report(""),
          [&t](data_t d) -> data_t {
            auto i{d.erase_from_partition(d.cbegin_partition(0), d.cend_partition(0))};
            t.check(equality, "Erase range from non-existent partition", i, d.begin_partition(0));
            return d;
          }
        );

        trg.join(data_description::empty,
                 data_description::empty,
                 t.report(""),
                 [&t](data_t d) -> data_t {
                   auto i{d.erase_from_partition(0, 0)};
                   t.check(equality, "Erase from non-existent partition", i, d.begin_partition(0));
                   return d;
                 }
          );

        trg.join(data_description::empty,
                 data_description::empty,
                 t.report(""),
                 [&t](data_t d) -> data_t {
                   auto i{d.erase_from_partition(1, 0)};
                   t.check(equality, "", i, d.begin_partition(0));
                   return d;
                 }
          );

        trg.join(data_description::empty,
                 data_description::empty,
                 t.report(""),
                 [&t](data_t d) -> data_t {
                   auto i{d.erase_from_partition(0, 1)};
                   t.check(equality, "", i, d.begin_partition(0));
                   return d;
                 }
          );

        trg.join(data_description::empty,
                 data_description::empty,
                 t.report(""),
                 [&t](data_t d) -> data_t {
                   auto i{d.erase_from_partition(1, 1)};
                   t.check(equality, "", i, d.begin_partition(0));
                   return d;
                 }
          );

        trg.join(data_description::empty,
          data_description::empty,
          t.report(""),
          [&t](data_t d) -> data_t {
            t.check(equality, "", d.capacity(), 0uz);
            t.check(equality, "", d.num_partitions_capacity(), 0uz);

            d.reserve(4);
            t.check(equality, "", d.capacity(), 4uz);
            t.check(equality, "", d.num_partitions_capacity(), 0uz);

            d.reserve_partitions(8);
            t.check(equality, "", d.capacity(), 4uz);
            t.check(equality, "", d.num_partitions_capacity(), 8uz);

            d.shrink_to_fit();
            t.check(equality, "May fail if shrink to fit impl does not reduce capacity", d.capacity(), 0uz);
            t.check(equality, "May fail if shrink to fit impl does not reduce capacity", d.num_partitions_capacity(), 0uz);

            return d;
          }
        );

        // end 'empty'
        // begin 'empty_partition'

        trg.join(data_description::empty_partition,
                 data_description::empty_partition,
                 t.report(""),
                 [&t](data_t d) -> data_t {
                   auto i{d.erase_from_partition(d.cbegin_partition(1))};
                   t.check(equality, "Erase from non-existent partition", i, d.begin_partition(1));
                   return d;
                 }
          );

        // end 'empty_partition'

        auto checker{
            [&t](std::string_view description, const data_t& obtained, const data_t& prediction, const data_t& parent, std::size_t host, std::size_t target) {
              t.check(equality, {description, no_source_location}, obtained, prediction);
              if(host != target) t.check_semantics({description, no_source_location}, prediction, parent);
            }
        };

        transition_checker<data_t>::check(t.report(""), trg, checker);
      }
    };
  }

  [[nodiscard]]
  std::filesystem::path partitioned_sequence_regular_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void partitioned_sequence_regular_test::run_tests()
  {
    using namespace data_structures;
    test_copyability();
    partitioned_operations<partitioned_sequence<int>>::execute(*this);
    test_index_type_limit();
  }

  void partitioned_sequence_regular_test::test_copyability()
  {
    using namespace data_structures;
    using move_only_sequence = partitioned_sequence<move_only_element>;
    using copyable_sequence  = partitioned_sequence<int>;

    STATIC_CHECK(!std::is_copy_constructible_v<move_only_sequence>);
    STATIC_CHECK(!std::is_copy_assignable_v<move_only_sequence>);
    STATIC_CHECK(!std::is_constructible_v<move_only_sequence,
                                          const move_only_sequence&,
                                          move_only_sequence::allocator_type,
                                          move_only_sequence::partitions_allocator_type>);
    STATIC_CHECK( std::is_nothrow_move_constructible_v<move_only_sequence>);

    STATIC_CHECK( std::is_copy_constructible_v<copyable_sequence>);
    STATIC_CHECK( std::is_copy_assignable_v<copyable_sequence>);

    STATIC_CHECK( std::is_copy_constructible_v<partitioned_sequence<non_assignable_element>>);
    STATIC_CHECK(!std::is_copy_assignable_v<partitioned_sequence<non_assignable_element>>);
    STATIC_CHECK( std::is_constructible_v<copyable_sequence,
                                          const copyable_sequence&,
                                          copyable_sequence::allocator_type,
                                          copyable_sequence::partitions_allocator_type>);
  }

  void partitioned_sequence_regular_test::test_index_type_limit()
  {
    using index_type      = std::uint8_t;
    using partitions_type = maths::monotonic_sequence<index_type, std::ranges::greater>;
    using sequence_type   = data_structures::partitioned_sequence<int, std::vector<int>, partitions_type>;

    constexpr std::size_t limit{std::numeric_limits<index_type>::max()};

    sequence_type sequence{};
    sequence.add_slot();
    for(std::size_t i{}; i < limit; ++i)
    {
      sequence.push_back_to_partition(0, static_cast<int>(i));
    }

    check(equality, "A partition as long as the index type counts is admitted", sequence.size_of_partition(0), limit);

    check_exception_thrown<std::out_of_range>(
      "Pushing back beyond what the index type counts throws",
      [&sequence]() { sequence.push_back_to_partition(0, 0); }
    );

    check_exception_thrown<std::out_of_range>(
      "Inserting beyond what the index type counts throws",
      [&sequence]() { sequence.insert_to_partition(sequence.cbegin_partition(0), 0); }
    );

    check(equality, "Unchanged by the refused growth", sequence.size_of_partition(0), limit);

    sequence_type manyPartitions{};
    for(std::size_t i{}; i < limit; ++i)
    {
      manyPartitions.add_slot();
    }

    check(equality, "As many partitions as the index type counts are admitted", manyPartitions.num_partitions(), limit);

    check_exception_thrown<std::out_of_range>(
      "Adding a partition beyond what the index type counts throws",
      [&manyPartitions]() { manyPartitions.add_slot(); }
    );

    check_exception_thrown<std::out_of_range>(
      "Inserting a partition beyond what the index type counts throws",
      [refused{manyPartitions}]() mutable { refused.insert_slot(0); }
    );

    check_exception_thrown<std::out_of_range>(
      "Inserting a partition at the end beyond what the index type counts throws",
      [refused{manyPartitions}]() mutable { refused.insert_slot(limit); }
    );

    check(equality, "Unchanged by the refused partitions", manyPartitions.num_partitions(), limit);
  }
}
