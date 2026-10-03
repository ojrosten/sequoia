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
    using namespace partitioned_data;

    // A standard container's size type is std::size_t; these carry only the types the constraint reads.
    template<std::integral SizeType>
    struct fake_container
    {
      using size_type = SizeType;
    };

    template<std::integral IndexType, std::integral SizeType>
    struct fake_partitions
    {
      using value_type = IndexType;
      using size_type  = SizeType;
    };

    template<class Container, class Partitions>
    concept sequence_admits = requires { typename data_structures::partitioned_sequence<int, Container, Partitions>; };

    template<class PartitionedData>
    struct partitioned_operations : partitioned_data_operations<PartitionedData>
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
                   auto i{d.erase_from_partition(d.cbegin_partition(0))};
                   t.check(equality, "Erase from non-existent partition", i, d.begin_partition(0));
                   return d;
                 }
          );

        trg.join(data_description::empty,
          data_description::empty,
          t.report(""),
          [&t](data_type d) -> data_type {
            auto i{d.erase_from_partition(d.cbegin_partition(0), d.cend_partition(0))};
            t.check(equality, "Erase range from non-existent partition", i, d.begin_partition(0));
            return d;
          }
        );

        trg.join(data_description::empty,
                 data_description::empty,
                 t.report(""),
                 [&t](data_type d) -> data_type {
                   auto i{d.erase_from_partition(0, 0)};
                   t.check(equality, "Erase from non-existent partition", i, d.begin_partition(0));
                   return d;
                 }
          );

        trg.join(data_description::empty,
                 data_description::empty,
                 t.report(""),
                 [&t](data_type d) -> data_type {
                   auto i{d.erase_from_partition(1, 0)};
                   t.check(equality, "", i, d.begin_partition(0));
                   return d;
                 }
          );

        trg.join(data_description::empty,
                 data_description::empty,
                 t.report(""),
                 [&t](data_type d) -> data_type {
                   auto i{d.erase_from_partition(0, 1)};
                   t.check(equality, "", i, d.begin_partition(0));
                   return d;
                 }
          );

        trg.join(data_description::empty,
                 data_description::empty,
                 t.report(""),
                 [&t](data_type d) -> data_type {
                   auto i{d.erase_from_partition(1, 1)};
                   t.check(equality, "", i, d.begin_partition(0));
                   return d;
                 }
          );

        trg.join(data_description::empty,
          data_description::empty,
          t.report(""),
          [&t](data_type d) -> data_type {
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
                 [&t](data_type d) -> data_type {
                   auto i{d.erase_from_partition(d.cbegin_partition(1))};
                   t.check(equality, "Erase from non-existent partition", i, d.begin_partition(1));
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
  std::filesystem::path partitioned_sequence_regular_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void partitioned_sequence_regular_test::run_tests()
  {
    using namespace data_structures;
    test_index_type_constraint();
    partitioned_operations<partitioned_sequence<int>>::execute(*this);
  }

  void partitioned_sequence_regular_test::test_index_type_constraint()
  {
    STATIC_CHECK( sequence_admits<std::vector<int>, maths::monotonic_sequence<std::size_t,  std::ranges::greater>>);
    STATIC_CHECK(!sequence_admits<std::vector<int>, maths::monotonic_sequence<std::uint8_t, std::ranges::greater>>);

    STATIC_CHECK( sequence_admits<fake_container<std::uint8_t>,  fake_partitions<std::uint8_t,  std::uint8_t>>);
    STATIC_CHECK( sequence_admits<fake_container<std::uint8_t>,  fake_partitions<std::uint16_t, std::uint8_t>>);
    STATIC_CHECK(!sequence_admits<fake_container<std::uint8_t>,  fake_partitions<std::int8_t,   std::uint8_t>>);
    STATIC_CHECK(!sequence_admits<fake_container<std::uint16_t>, fake_partitions<std::uint8_t,  std::uint8_t>>);
    STATIC_CHECK(!sequence_admits<fake_container<std::uint8_t>,  fake_partitions<std::uint8_t,  std::uint16_t>>);

    // Each index type below counts to both size types, so only its being bool or a character type refuses it.
    STATIC_CHECK(!sequence_admits<fake_container<bool>,        fake_partitions<bool, bool>>);
    STATIC_CHECK(!sequence_admits<fake_container<std::int8_t>, fake_partitions<char, std::int8_t>>);
  }
}
