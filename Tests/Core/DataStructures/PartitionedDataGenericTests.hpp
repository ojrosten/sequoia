////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "PartitionedDataTestingUtilities.hpp"

#include "sequoia/TestFramework/RegularTestCore.hpp"
#include "sequoia/TestFramework/StateTransitionUtilities.hpp"

namespace sequoia::testing
{
  namespace partitioned_data
  {
    enum data_description : std::size_t {
      empty = 0,

      // []
      empty_partition,

      // [2]
      one_2,

      // [3]
      one_3,

      // [2,3]
      one_2_3,

      // [3,2]
      one_3_2,

      // [3, 2, 4]
      one_3_4_2,

      // [][]
      two_empty_partitions,

      // [2][]
      two_2__,

      // [3][]
      two_3__,

      // [2,3][]
      two_2_3__,

      // [][2]
      two__2,

      // [][2,3]
      two__2_3,

      // [2][3]
      two_2__3,

      // [3][2]
      two_3__2,

      // [2][][3]
      three_2____3,

      // [3][][2]
      three_3____2,

      // [2][3][]
      three_2__3__
    };
  }

  template<class PartitionedData>
  class partitioned_data_operations
  {
  public:
    using data_type        = PartitionedData;
    using value_type       = PartitionedData::value_type;
    using equiv_type       = std::initializer_list<std::initializer_list<value_type>>;
    using transition_graph = transition_checker<data_type>::transition_graph;

    static void execute_operations(regular_test& t)
    {
      auto trg{make_transition_graph(t)};

      auto checker{
          [&t](std::string_view description, const data_type& obtained, const data_type& prediction, const data_type& parent, std::size_t host, std::size_t target) {
            t.check(equality, {description, no_source_location}, obtained, prediction);
            if(host != target) t.check_semantics({description, no_source_location}, prediction, parent);
          }
      };

      transition_checker<data_type>::check(t.report(""), trg, checker);
    }

    [[nodiscard]]
    static data_type make_and_check(regular_test& t, std::string_view description, equiv_type init)
    {
      data_type d{init};
      t.check(equivalence, description, d, init);
      return d;
    }

    static void check_partition_is_empty(regular_test& t, data_type& d, const std::size_t i)
    {
      const data_type& c{d};

      t.check("begin_partition equals end_partition",           d.begin_partition(i)   == d.end_partition(i));
      t.check("begin_partition equals end_partition (const)",   c.begin_partition(i)   == c.end_partition(i));
      t.check("cbegin_partition equals cend_partition",         d.cbegin_partition(i)  == d.cend_partition(i));
      t.check("rbegin_partition equals rend_partition",         d.rbegin_partition(i)  == d.rend_partition(i));
      t.check("rbegin_partition equals rend_partition (const)", c.rbegin_partition(i)  == c.rend_partition(i));
      t.check("crbegin_partition equals crend_partition",       d.crbegin_partition(i) == d.crend_partition(i));
      t.check("operator[] equals end_partition",                d[i]                   == d.end_partition(i));
      t.check("operator[] equals end_partition (const)",        c[i]                   == c.end_partition(i));
      t.check("partition is empty",                             d.partition(i).empty());
      t.check("partition is empty (const)",                     c.partition(i).empty());
      t.check("cpartition is empty",                            d.cpartition(i).empty());
      t.check(equality, "size_of_partition is zero", d.size_of_partition(i), 0uz);
    }

    /** The contract does not say which partition index the iterators of a missing partition carry.
        These checks pin the implementation's choice, and change with it.
     */
    static void check_iterator_partition_indices_are_npos(regular_test& t, data_type& d, const std::size_t i)
    {
      constexpr auto npos{data_type::partition_iterator::npos};
      const data_type& c{d};

      t.check(equality, "Partition index of begin_partition",          d.begin_partition(i).partition_index(),  npos);
      t.check(equality, "Partition index of end_partition",            d.end_partition(i).partition_index(),    npos);
      t.check(equality, "Partition index of rbegin_partition",         d.rbegin_partition(i).partition_index(), npos);
      t.check(equality, "Partition index of rend_partition",           d.rend_partition(i).partition_index(),   npos);
      t.check(equality, "Partition index of begin_partition (const)",  c.begin_partition(i).partition_index(),  npos);
      t.check(equality, "Partition index of end_partition (const)",    c.end_partition(i).partition_index(),    npos);
      t.check(equality, "Partition index of rbegin_partition (const)", c.rbegin_partition(i).partition_index(), npos);
      t.check(equality, "Partition index of rend_partition (const)",   c.rend_partition(i).partition_index(),   npos);
    }

    [[nodiscard]]
    static transition_graph make_transition_graph(regular_test& t)
    {
      using namespace partitioned_data;
      return transition_graph{
        {
          { // begin 'empty'
            {
              data_description::empty,
              t.report(""),
              [&t](data_type d) -> data_type {
                t.check_exception_thrown<std::out_of_range>("Pushing back to non-existent partition throws", [&d]() { return d.push_back_to_partition(0, 8); });
                return d;
              }
            },
            {
              data_description::empty,
              t.report(""),
              [&t](data_type d) -> data_type {
                t.check_exception_thrown<std::out_of_range>("Inserting to non-existent partition throws", [&d]() { return d.insert_to_partition(d.cbegin_partition(0), 8); });
                return d;
              }
            },
            {
              data_description::empty,
              t.report(""),
              [&t](data_type d) -> data_type {
                t.check_exception_thrown<std::out_of_range>("Inserting to non-existent partition throws", [&d]() { return d.insert_to_partition(0, 0, 8); });
                return d;
              }
            },
            {
              data_description::empty,
              t.report("Swapping non-existent partition"),
              [](data_type d) -> data_type {
                d.swap_partitions(0, 0);
                return d;
              }
            },
            {
              data_description::empty,
              t.report("Clear empty container"),
              [](data_type d) -> data_type {
                d.clear();
                return d;
              }
            },
            {
              data_description::empty_partition,
              t.report("Add slot to empty container"),
              [](data_type d) -> data_type {
                d.add_slot();
                return d;
              }
            },
            {
              data_description::empty_partition,
              t.report("Insert slot to empty container"),
              [](data_type d) -> data_type {
                d.insert_slot(0);
                return d;
              }
            },
            {
              data_description::empty,
              t.report("An empty sequence treats index 0 as the index of an empty partition"),
              [&t](data_type d) -> data_type {
                check_partition_is_empty(t, d, 0);
                check_iterator_partition_indices_are_npos(t, d, 0);
                return d;
              }
            },
            {
              data_description::empty,
              t.report("A default-constructed sequence treats index 0 as the index of an empty partition"),
              [&t](data_type) -> data_type {
                data_type d{};
                check_partition_is_empty(t, d, 0);
                check_iterator_partition_indices_are_npos(t, d, 0);
                return d;
              }
            },
            {
              data_description::empty,
              t.report(""),
              [&t](data_type d) -> data_type {
                auto i{d.erase_from_partition(d.cbegin_partition(0))};
                t.check(equality, "Erase from non-existent partition", i, d.begin_partition(0));
                return d;
              }
            },
            {
              data_description::empty,
              t.report(""),
              [&t](data_type d) -> data_type {
                auto i{d.erase_from_partition(d.cbegin_partition(0), d.cend_partition(0))};
                t.check(equality, "Erase range from non-existent partition", i, d.begin_partition(0));
                return d;
              }
            },
            {
              data_description::empty,
              t.report(""),
              [&t](data_type d) -> data_type {
                auto i{d.erase_from_partition(0, 0)};
                t.check(equality, "Erase from non-existent partition", i, d.begin_partition(0));
                return d;
              }
            },
            {
              data_description::empty,
              t.report(""),
              [&t](data_type d) -> data_type {
                auto i{d.erase_from_partition(1, 0)};
                t.check(equality, "", i, d.begin_partition(0));
                return d;
              }
            },
            {
              data_description::empty,
              t.report(""),
              [&t](data_type d) -> data_type {
                auto i{d.erase_from_partition(0, 1)};
                t.check(equality, "", i, d.begin_partition(0));
                return d;
              }
            },
            {
              data_description::empty,
              t.report(""),
              [&t](data_type d) -> data_type {
                auto i{d.erase_from_partition(1, 1)};
                t.check(equality, "", i, d.begin_partition(0));
                return d;
              }
            }
          }, // end 'empty'
          {  // begin 'empty_partition'
            {
              data_description::empty_partition,
              t.report(""),
              [&t](data_type d) -> data_type {
                t.check_exception_thrown<std::out_of_range>("Pushing back to non-existent partition throws", [&d]() { return d.push_back_to_partition(1, 8); });
                return d;
              }
            },
            {
              data_description::empty_partition,
              t.report(""),
              [&t](data_type d) -> data_type {
                t.check_exception_thrown<std::out_of_range>("Inserting to non-existent partition throws", [&d]() { return d.insert_to_partition(d.cbegin_partition(1), 8); });
                return d;
              }
            },
            {
              data_description::empty_partition,
              t.report(""),
              [&t](data_type d) -> data_type {
                t.check_exception_thrown<std::out_of_range>("Inserting to non-existent partition throws", [&d]() { return d.insert_to_partition(1, 0, 8); });
                return d;
              }
            },
            {
              data_description::empty_partition,
              t.report("Swapping non-existent partition"),
              [](data_type d) -> data_type {
                d.swap_partitions(0, 1);
                return d;
              }
            },
            {
              data_description::empty_partition,
              t.report("Swapping non-existent partition"),
              [](data_type d) -> data_type {
                d.swap_partitions(1, 0);
                return d;
              }
            },
            {
              data_description::empty_partition,
              t.report(""),
              [&t](data_type d) -> data_type {
                auto i{d.erase_from_partition(d.cbegin_partition(0))};
                t.check(equality, "Erase from partition with nothing in it", i, d.begin_partition(0));
                return d;
              }
            },
            {
              data_description::empty_partition,
              t.report(""),
              [&t](data_type d) -> data_type {
                auto i{d.erase_from_partition(0, 0)};
                t.check(equality, "Erase from partition with nothing in it", i, d.begin_partition(0));
                return d;
              }
            },
            {
              data_description::empty_partition,
              t.report(""),
              [&t](data_type d) -> data_type {
                auto i{d.erase_from_partition(0, 1)};
                t.check(equality, "Erase from partition with nothing in it", i, d.begin_partition(0));
                return d;
              }
            },
            {
              data_description::empty_partition,
              t.report(""),
              [&t](data_type d) -> data_type {
                auto i{d.erase_from_partition(d.cbegin_partition(0), d.cend_partition(0))};
                t.check(equality, "Erase empty range", i, d.begin_partition(0));
                return d;
              }
            },
            {
              data_description::empty_partition,
              t.report(""),
              [&t](data_type d) -> data_type {
                t.check_exception_thrown<std::domain_error>("", [&d](){ d.erase_from_partition(d.cbegin_partition(0), d.cend_partition(1)); });
                return d;
              }
            },
            {
              data_description::empty_partition,
              t.report(""),
              [&t](data_type d) -> data_type {
                t.check_exception_thrown<std::domain_error>("", [&d](){ d.erase_from_partition(d.cbegin_partition(1), d.cend_partition(0)); });
                return d;
              }
            },
            {
              data_description::empty_partition,
              t.report(""),
              [&t](data_type d) -> data_type {
                auto i{d.erase_from_partition(d.cbegin_partition(1), d.cend_partition(1))};
                t.check(equality, "Erase fictional range", i, d.end_partition(1));
                return d;
              }
            },
            {
              data_description::empty_partition,
              t.report("Swapping non-existent partition"),
              [](data_type d) -> data_type {
                d.swap_partitions(0, 0);
                return d;
              }
            },
            {
              data_description::empty_partition,
              t.report("Swapping non-existent partition"),
              [](data_type d) -> data_type {
                d.swap_partitions(1, 0);
                return d;
              }
            },
            {
              data_description::empty_partition,
              t.report(""),
              [](data_type d) -> data_type {
                d.swap_partitions(0, 1);
                return d;
              }
            },
            {
              data_description::empty,
              t.report("Clear empty container"),
              [](data_type d) -> data_type {
                d.clear();
                return d;
              }
            },
            {
              data_description::empty,
              t.report(""),
              [](data_type d) -> data_type {
                d.erase_slot(0);
                return d;
              }
            },
            {
              data_description::one_2,
              t.report(""),
              [](data_type d) -> data_type {
                d.push_back_to_partition(0, 2);
                return d;
              }
            },
            {
              data_description::one_2,
              t.report(""),
              [](data_type d) -> data_type {
                d.insert_to_partition(d.cbegin_partition(0), 2);
                return d;
              }
            },
            {
              data_description::one_2,
              t.report(""),
              [](data_type d) -> data_type {
                d.insert_to_partition(0, 0, 2);
                return d;
              }
            },
            {
              data_description::two_empty_partitions,
              t.report(""),
              [](data_type d) -> data_type {
                d.add_slot();
                return d;
              }
            },
            {
              data_description::two_empty_partitions,
              t.report(""),
              [](data_type d) -> data_type {
                d.insert_slot(0);
                return d;
              }
            },
            {
              data_description::two_empty_partitions,
              t.report(""),
              [](data_type d) -> data_type {
                d.insert_slot(1);
                return d;
              }
            }
          }, // end 'empty_partition'
          {  // begin 'one_2'
            {
              data_description::empty_partition,
              t.report(""),
              [](data_type d) -> data_type {
                d.erase_from_partition(d.cbegin_partition(0));
                return d;
              }
            },
            {
              data_description::empty_partition,
              t.report(""),
              [](data_type d) -> data_type {
                d.erase_from_partition(d.cbegin_partition(0), d.cend_partition(0));
                return d;
              }
            },
            {
              data_description::empty,
              t.report("A sequence whose only slot is erased treats index 0 as the index of an empty partition"),
              [&t](data_type d) -> data_type {
                d.erase_slot(0);
                check_partition_is_empty(t, d, 0);
                check_iterator_partition_indices_are_npos(t, d, 0);
                return d;
              }
            },
            {
              data_description::empty,
              t.report(""),
              [](data_type d) -> data_type {
                d.clear();
                return d;
              }
            },
            {
              data_description::one_2,
              t.report(""),
              [](data_type d) -> data_type {
                d.swap_partitions(0, 0);
                return d;
              }
            },
            {
              data_description::one_3,
              t.report(""),
              [](data_type d) -> data_type {
                *d.begin_partition(0) = 3;
                return d;
              }
            },
            {
              data_description::one_3,
              t.report(""),
              [](data_type d) -> data_type {
                *d.rbegin_partition(0) = 3;
                return d;
              }
            },
            {
              data_description::one_2_3,
              t.report(""),
              [](data_type d) -> data_type {
                d.push_back_to_partition(0, 3);
                return d;
              }
            },
            {
              data_description::one_2_3,
              t.report(""),
              [](data_type d) -> data_type {
                d.insert_to_partition(d.cbegin_partition(0)+1, 3);
                return d;
              }
            },
            {
              data_description::one_2_3,
              t.report(""),
              [](data_type d) -> data_type {
                d.insert_to_partition(0, 1, 3);
                return d;
              }
            },
            {
              data_description::one_2,
              t.report(""),
              [&t](data_type d) -> data_type {
                t.check_exception_thrown<std::out_of_range>(
                  "Inserting beyond the end of a partition throws",
                  [&d]() { return d.insert_to_partition(0, 2, 3); }
                );
                return d;
              }
            },
            {
              data_description::two_2__,
              t.report(""),
              [](data_type d) -> data_type {
                d.add_slot();
                return d;
              }
            },
            {
              data_description::two__2,
              t.report(""),
              [](data_type d) -> data_type {
                d.insert_slot(0);
                return d;
              }
            }
          }, // end 'one_2'
          {  // begin 'one_3'
            {
              data_description::one_2,
              t.report(""),
              [](data_type d) -> data_type {
                *d.begin_partition(0) = 2;
                return d;
              }
            },
            {
              data_description::one_3_2,
              t.report(""),
              [](data_type d) -> data_type {
                d.push_back_to_partition(0, 2);
                return d;
              }
            },
            {
              data_description::one_2_3,
              t.report(""),
              [](data_type d) -> data_type {
                d.insert_to_partition(d.cbegin_partition(0), 2);
                return d;
              }
            },
            {
              data_description::one_2_3,
              t.report(""),
              [](data_type d) -> data_type {
                d.insert_to_partition(0, 0, 2);
                return d;
              }
            }
          }, // end 'one_3'
          {  // begin 'one_2_3'
            {
              data_description::one_3,
              t.report(""),
              [](data_type d) -> data_type {
                d.erase_from_partition(d.cbegin_partition(0));
                return d;
              }
            },
            {
              data_description::one_3,
              t.report(""),
              [](data_type d) -> data_type {
                d.erase_from_partition(d.cbegin_partition(0), d.cbegin_partition(0) + 1);
                return d;
              }
            },
            {
              data_description::one_2,
              t.report(""),
              [](data_type d) -> data_type {
                d.erase_from_partition(d.cbegin_partition(0)+1);
                return d;
              }
            },
            {
              data_description::one_2,
              t.report(""),
              [](data_type d) -> data_type {
                d.erase_from_partition(d.cbegin_partition(0) + 1, d.cend_partition(0));
                return d;
              }
            },
            {
              data_description::empty_partition,
              t.report(""),
              [](data_type d) -> data_type {
                d.erase_from_partition(d.cbegin_partition(0), d.cend_partition(0));
                return d;
              }
            },
            {
              data_description::empty,
              t.report(""),
              [](data_type d) -> data_type {
                d.erase_slot(0);
                return d;
              }
            },
            {
              data_description::empty,
              t.report(""),
              [](data_type d) -> data_type {
                d.clear();
                return d;
              }
            },
            {
              data_description::one_3_2,
              t.report(""),
              [](data_type d) -> data_type {
                std::ranges::sort(d.begin_partition(0), d.end_partition(0), std::greater{});
                return d;
              }
            },
            {
              data_description::two__2_3,
              t.report(""),
              [](data_type d) -> data_type {
                d.insert_slot(0);
                return d;
              }
            }
          }, // end 'one_2_3'
          {  // begin 'one_3_2'
            {
              data_description::one_2,
              t.report(""),
              [](data_type d) -> data_type {
                d.erase_from_partition(d.cbegin_partition(0));
                return d;
              }
            },
            {
              data_description::one_3,
              t.report(""),
              [](data_type d) -> data_type {
                d.erase_from_partition(d.cbegin_partition(0) + 1);
                return d;
              }
            },
            {
              data_description::one_2_3,
              t.report(""),
              [](data_type d) -> data_type {
                std::ranges::sort(d.begin_partition(0), d.end_partition(0));
                return d;
              }
            },
            {
              data_description::one_3_4_2,
              t.report(""),
              [](data_type d) -> data_type {
                d.insert_to_partition(d.cbegin_partition(0) + 1, 4);
                return d;
              }
            },
          }, // end 'one_3_2'
          {  // begin 'one_3_4_2'
            {
              data_description::one_3_2,
              t.report(""),
              [](data_type d) -> data_type {
                d.erase_from_partition(d.cbegin_partition(0) + 1);
                return d;
              }
            },
            {
              data_description::one_3_2,
              t.report(""),
              [](data_type d) -> data_type {
                d.erase_from_partition(d.cbegin_partition(0) + 1, d.cbegin_partition(0) + 2);
                return d;
              }
            },
            {
              data_description::one_3,
              t.report(""),
              [](data_type d) -> data_type {
                d.erase_from_partition(d.cbegin_partition(0) + 1, d.cend_partition(0));
                return d;
              }
            },
            {
              data_description::one_2,
              t.report(""),
              [](data_type d) -> data_type {
                d.erase_from_partition(d.cbegin_partition(0), d.cbegin_partition(0)+2);
                return d;
              }
            }
          }, // end 'one_3_4_2'
          {  // begin 'two_empty_partitions'
            {
              data_description::two_empty_partitions,
              t.report(""),
              [&t](data_type d) -> data_type {
                t.check_exception_thrown<std::domain_error>("", [&d](){ d.erase_from_partition(d.cbegin_partition(0), d.cend_partition(1)); });
                return d;
              }
            },
            {
              data_description::two_empty_partitions,
              t.report(""),
              [](data_type d) -> data_type {
                d.swap_partitions(0, 0);
                return d;
              }
            },
            {
              data_description::empty_partition,
              t.report(""),
              [](data_type d) -> data_type {
                d.erase_slot(0);
                return d;
              }
            },
            {
              data_description::empty_partition,
              t.report(""),
              [](data_type d) -> data_type {
                d.erase_slot(1);
                return d;
              }
            },
            {
              data_description::empty,
              t.report(""),
              [](data_type d) -> data_type {
                d.clear();
                return d;
              }
            },
            {
              data_description::two_2__,
              t.report(""),
              [](data_type d) -> data_type {
                d.push_back_to_partition(0, 2);
                return d;
              }
            },
            {
              data_description::two_2__,
              t.report(""),
              [](data_type d) -> data_type {
                d.insert_to_partition(d.cbegin_partition(0), 2);
                return d;
              }
            },
            {
              data_description::two__2,
              t.report(""),
              [](data_type d) -> data_type {
                d.insert_to_partition(d.cbegin_partition(1), 2);
                return d;
              }
            }
          }, // end 'two_empty_partitions'
          {  // begin 'two_2__'
            {
              data_description::empty_partition,
              t.report(""),
              [](data_type d) -> data_type {
                d.erase_slot(0);
                return d;
              }
            },
            {
              data_description::one_2,
              t.report(""),
              [](data_type d) -> data_type {
                d.erase_slot(1);
                return d;
              }
            },
            {
              data_description::two__2,
              t.report(""),
              [](data_type d) -> data_type {
                d.swap_partitions(0, 1);
                return d;
              }
            },
            {
              data_description::two__2,
              t.report(""),
              [](data_type d) -> data_type {
                d.swap_partitions(1, 0);
                return d;
              }
            },
            {
              data_description::two_2__3,
              t.report(""),
              [](data_type d) -> data_type {
                d.insert_to_partition(1, 0, 3);
                return d;
              }
            }
          }, // end 'two_2__'
          {  // begin 'two_3__'
            {
              data_description::two_2_3__,
              t.report(""),
              [](data_type d) -> data_type {
                d.insert_to_partition(d.cbegin_partition(0), 2);
                return d;
              }
            }
          }, // end 'two_3__'
          {  // begin 'two_2_3__'
            {
              data_description::two__2_3,
              t.report(""),
              [](data_type d) -> data_type {
                d.swap_partitions(0, 1);
                return d;
              }
            },
            {
              data_description::two_3__,
              t.report(""),
              [](data_type d) -> data_type {
                d.erase_from_partition(d.cbegin_partition(0));
                return d;
              }
            },
          }, // end 'two_2_3__'
          {  // begin 'two__2'
            {
              data_description::two_2__,
              t.report(""),
              [](data_type d) -> data_type {
                d.swap_partitions(1, 0);
                return d;
              }
            }
          }, // end 'two__2'
          {  // begin 'two__2_3'
            {
              data_description::two__2,
              t.report(""),
              [](data_type d) -> data_type {
                d.erase_from_partition(d.cbegin_partition(1) + 1);
                return d;
              }
            },
            {
              data_description::two_2_3__,
              t.report(""),
              [](data_type d) -> data_type {
                d.swap_partitions(0, 1);
                return d;
              }
            }
          }, // end 'two__2_3'
          {  // begin 'two_2__3'
            {
              data_description::two_2__3,
              t.report("A sequence of two partitions treats index 2 as the index of an empty partition"),
              [&t](data_type d) -> data_type {
                check_partition_is_empty(t, d, 2);
                check_iterator_partition_indices_are_npos(t, d, 2);
                return d;
              }
            },
            {
              data_description::two_2__3,
              t.report("A sequence of two partitions treats index 7 as the index of an empty partition"),
              [&t](data_type d) -> data_type {
                check_partition_is_empty(t, d, 7);
                check_iterator_partition_indices_are_npos(t, d, 7);
                return d;
              }
            },
            {
              data_description::two_3__2,
              t.report(""),
              [](data_type d) -> data_type {
                d.swap_partitions(0, 1);
                return d;
              }
            },
            {
              data_description::two_2__,
              t.report(""),
              [](data_type d) -> data_type {
                d.erase_from_partition(d.cbegin_partition(1), d.cend_partition(1));
                return d;
              }
            },
            {
              data_description::one_2,
              t.report(""),
              [](data_type d) -> data_type {
                d.erase_slot(1);
                return d;
              }
            },
            {
              data_description::one_3,
              t.report(""),
              [](data_type d) -> data_type {
                d.erase_slot(0);
                return d;
              }
            },
            {
              data_description::three_2____3,
              t.report(""),
              [](data_type d) -> data_type {
                d.insert_slot(1);
                return d;
              }
            },
            {
              data_description::three_2__3__,
              t.report(""),
              [](data_type d) -> data_type {
                d.insert_slot(2);
                return d;
              }
            },
            {
              data_description::two_2__3,
              t.report(""),
              [&t](data_type d) -> data_type {
                t.check_exception_thrown<std::out_of_range>(
                  "Inserting beyond the end of a partition other than the last throws",
                  [&d]() { return d.insert_to_partition(0, 2, 4); }
                );
                return d;
              }
            },
            {
              data_description::two_2__3,
              t.report(""),
              [&t](data_type d) -> data_type {
                t.check_exception_thrown<std::out_of_range>(
                  "Inserting beyond the end of a partition other than the first throws",
                  [&d]() { return d.insert_to_partition(1, 2, 4); }
                );
                return d;
              }
            }
          }, // end 'two_2__3'
          {  // begin 'two_3__2'
            {
              data_description::two_2__3,
              t.report(""),
              [](data_type d) -> data_type {
                d.swap_partitions(1, 0);
                return d;
              }
            },
            {
              data_description::three_3____2,
              t.report(""),
              [](data_type d) -> data_type {
                d.insert_slot(1);
                return d;
              }
            }
          }, // end 'two_3__2'
          {  // begin 'three_2____3'
            {
              data_description::three_3____2,
              t.report(""),
              [](data_type d) -> data_type {
                d.swap_partitions(0,2);
                return d;
              }
            },
            {
              data_description::three_2__3__,
              t.report(""),
              [](data_type d) -> data_type {
                d.swap_partitions(1,2);
                return d;
              }
            },
            {
              data_description::two_2__3,
              t.report(""),
              [](data_type d) -> data_type {
                d.erase_slot(1);
                return d;
              }
            }
          }, // end 'three_2____3'
          {  // begin 'three_3____2'
            {
              data_description::three_2____3,
              t.report(""),
              [](data_type d) -> data_type {
                d.swap_partitions(2,0);
                return d;
              }
            },
            {
              data_description::two__2,
              t.report(""),
              [](data_type d) -> data_type {
                d.erase_slot(0);
                return d;
              }
            }
          }, // end 'three_3____2'
          {  // begin 'three_2__3__'
            {
              data_description::three_2____3,
              t.report(""),
              [](data_type d) -> data_type {
                d.swap_partitions(2,1);
                return d;
              }
            }
          }, // end 'three_2__3__'
        },
        {
          //  'empty'
          make_and_check(t, t.report(""), {}),

          // 'empty_partition'
          make_and_check(t, t.report(""), {{}}),

          // 'one_2'
          make_and_check(t, t.report(""), {{2}}),

          // 'one_3'
          make_and_check(t, t.report(""), {{3}}),

          // 'one_2_3'
          make_and_check(t, t.report(""), {{2, 3}}),

          // 'one_3_2'
          make_and_check(t, t.report(""), {{3, 2}}),

          // 'one_3_4_2'
          make_and_check(t, t.report(""), {{3, 4, 2}}),

          // 'two_empty_partitions'
          make_and_check(t, t.report(""), {{}, {}}),

          // 'two_2__'
          make_and_check(t, t.report(""), {{2}, {}}),

          // 'two_3__'
          make_and_check(t, t.report(""), {{3}, {}}),

          // 'two_2_3__'
          make_and_check(t, t.report(""), {{2,3}, {}}),

          // 'two__2'
          make_and_check(t, t.report(""), {{}, {2}}),

          // 'two__2_3'
          make_and_check(t, t.report(""), {{}, {2, 3}}),

          // 'two_2__3'
          make_and_check(t, t.report(""), {{2}, {3}}),

          // 'two_3__2'
          make_and_check(t, t.report(""), {{3}, {2}}),

          // 'three_2____3'
          make_and_check(t, t.report(""), {{2}, {}, {3}}),

          // 'three_3____2'
          make_and_check(t, t.report(""), {{3}, {}, {2}}),

          // 'three_2__3__'
          make_and_check(t, t.report(""), {{2}, {3}, {}}),
        }
      };
    }
  };
}
