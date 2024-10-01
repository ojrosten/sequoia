////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "PartitionedSequenceRegularTest.hpp"
#include "PartitionedDataGenericTests.hpp"
#include "sequoia/Core/DataStructures/PartitionedData.hpp"

namespace sequoia::testing
{
  namespace
  {
    using namespace partitioned_data;

    template<class PartitionedData>
    struct partitioned_operations : partitioned_data_operations<PartitionedData>
    {
      using data_t = typename partitioned_data_operations<PartitionedData>::data_t;

      static void execute(regular_test& t)
      {
        auto trg{partitioned_data_operations<PartitionedData>::make_transition_graph(t)};

        // begin 'empty'
        trg.join(data_description::empty,
                 data_description::empty,
                 t.report_line(""),
                 [&t](data_t d) -> data_t {
                   auto i{d.erase_from_partition(d.cbegin_partition(0))};
                   t.check(equality, "Erase from non-existent partition", i, d.begin_partition(0));
                   return d;
                 }
          );

        trg.join(data_description::empty,
          data_description::empty,
          t.report_line(""),
          [&t](data_t d) -> data_t {
            auto i{d.erase_from_partition(d.cbegin_partition(0), d.cend_partition(0))};
            t.check(equality, "Erase range from non-existent partition", i, d.begin_partition(0));
            return d;
          }
        );

        trg.join(data_description::empty,
                 data_description::empty,
                 t.report_line(""),
                 [&t](data_t d) -> data_t {
                   auto i{d.erase_from_partition(0, 0)};
                   t.check(equality, "Erase from non-existent partition", i, d.begin_partition(0));
                   return d;
                 }
          );

        trg.join(data_description::empty,
                 data_description::empty,
                 t.report_line(""),
                 [&t](data_t d) -> data_t {
                   auto i{d.erase_from_partition(1, 0)};
                   t.check(equality, "", i, d.begin_partition(0));
                   return d;
                 }
          );

        trg.join(data_description::empty,
                 data_description::empty,
                 t.report_line(""),
                 [&t](data_t d) -> data_t {
                   auto i{d.erase_from_partition(0, 1)};
                   t.check(equality, "", i, d.begin_partition(0));
                   return d;
                 }
          );

        trg.join(data_description::empty,
                 data_description::empty,
                 t.report_line(""),
                 [&t](data_t d) -> data_t {
                   auto i{d.erase_from_partition(1, 1)};
                   t.check(equality, "", i, d.begin_partition(0));
                   return d;
                 }
          );

        trg.join(data_description::empty,
          data_description::empty,
          t.report_line(""),
          [&t](data_t d) -> data_t {
            t.check(equality, "", d.capacity(), 0_sz);
            t.check(equality, "", d.num_partitions_capacity(), 0_sz);

            d.reserve(4);
            t.check(equality, "", d.capacity(), 4_sz);
            t.check(equality, "", d.num_partitions_capacity(), 0_sz);

            d.reserve_partitions(8);
            t.check(equality, "", d.capacity(), 4_sz);
            t.check(equality, "", d.num_partitions_capacity(), 8_sz);

            d.shrink_to_fit();
            t.check(equality, "May fail if shrink to fit impl does not reduce capacity", d.capacity(), 0_sz);
            t.check(equality, "May fail if shrink to fit impl does not reduce capacity", d.num_partitions_capacity(), 0_sz);

            return d;
          }
        );

        // end 'empty'
        // begin 'empty_partition'

        trg.join(data_description::empty_partition,
                 data_description::empty_partition,
                 t.report_line(""),
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

        transition_checker<data_t>::check(t.report_line(""), trg, checker);
      }
    };
  }

  [[nodiscard]]
  std::filesystem::path partitioned_sequence_regular_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void partitioned_sequence_regular_test::run_tests()
  {
    using namespace data_structures;
    partitioned_operations<partitioned_sequence<int>>::execute(*this);
  }
}
