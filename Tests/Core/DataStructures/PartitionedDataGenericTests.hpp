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
    using data_t           = PartitionedData;
    using value_type       = typename PartitionedData::value_type;
    using equiv_t          = std::initializer_list<std::initializer_list<value_type>>;
    using transition_graph = typename transition_checker<data_t>::transition_graph;

    static void execute_operations(regular_test& t)
    {
      auto trg{make_transition_graph(t)};

      auto checker{
          [&t](std::string_view description, const data_t& obtained, const data_t& prediction, const data_t& parent, std::size_t host, std::size_t target) {
            t.check(equality, {description, no_source_location}, obtained, prediction);
            if(host != target) t.check_semantics({description, no_source_location}, prediction, parent);
          }
      };

      transition_checker<data_t>::check(t.report_line(""), trg, checker);
    }

    [[nodiscard]]
    static data_t make_and_check(regular_test& t, std::string_view description, equiv_t init)
    {
      data_t d{init};
      t.check(equivalence, description, d, init);
      return d;
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
              t.report_line(""),
              [&t](data_t d) -> data_t {
                t.check_exception_thrown<std::out_of_range>("Pushing back to non-existent partition throws", [&d]() { return d.push_back_to_partition(0, 8); });
                return d;
              }
            },
            {
              data_description::empty,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                t.check_exception_thrown<std::out_of_range>("Inserting to non-existent partition throws", [&d]() { return d.insert_to_partition(d.cbegin_partition(0), 8); });
                return d;
              }
            },
            {
              data_description::empty,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                t.check_exception_thrown<std::out_of_range>("Inserting to non-existent partition throws", [&d]() { return d.insert_to_partition(0, 0, 8); });
                return d;
              }
            },
            {
              data_description::empty,
              t.report_line("Swapping non-existent partition"),
              [&t](data_t d) -> data_t {
                d.swap_partitions(0, 0);
                return d;
              }
            },
            {
              data_description::empty,
              t.report_line("Clear empty container"),
              [&t](data_t d) -> data_t {
                d.clear();
                return d;
              }
            },
            {
              data_description::empty_partition,
              t.report_line("Add slot to empty container"),
              [&t](data_t d) -> data_t {
                d.add_slot();
                return d;
              }
            },
            {
              data_description::empty_partition,
              t.report_line("Insert slot to empty container"),
              [&t](data_t d) -> data_t {
                d.insert_slot(0);
                return d;
              }
            }
          }, // end 'empty'
          {  // begin 'empty_partition'
            {
              data_description::empty_partition,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                t.check_exception_thrown<std::out_of_range>("Pushing back to non-existent partition throws", [&d]() { return d.push_back_to_partition(1, 8); });
                return d;
              }
            },
            {
              data_description::empty_partition,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                t.check_exception_thrown<std::out_of_range>("Inserting to non-existent partition throws", [&d]() { return d.insert_to_partition(d.cbegin_partition(1), 8); });
                return d;
              }
            },
            {
              data_description::empty_partition,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                t.check_exception_thrown<std::out_of_range>("Inserting to non-existent partition throws", [&d]() { return d.insert_to_partition(1, 0, 8); });
                return d;
              }
            },
            {
              data_description::empty_partition,
              t.report_line("Swapping non-existent partition"),
              [&t](data_t d) -> data_t {
                d.swap_partitions(0, 1);
                return d;
              }
            },
            {
              data_description::empty_partition,
              t.report_line("Swapping non-existent partition"),
              [&t](data_t d) -> data_t {
                d.swap_partitions(1, 0);
                return d;
              }
            },
            {
              data_description::empty_partition,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                auto i{d.erase_from_partition(d.cbegin_partition(0))};
                t.check(equality, t.report_line("Erase from partition with nothing in it"), i, d.begin_partition(0));
                return d;
              }
            },
            {
              data_description::empty_partition,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                auto i{d.erase_from_partition(0, 0)};
                t.check(equality, t.report_line("Erase from partition with nothing in it"), i, d.begin_partition(0));
                return d;
              }
            },
            {
              data_description::empty_partition,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                auto i{d.erase_from_partition(0, 1)};
                t.check(equality, t.report_line("Erase from partition with nothing in it"), i, d.begin_partition(0));
                return d;
              }
            },
            {
              data_description::empty_partition,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                auto i{d.erase_from_partition(d.cbegin_partition(0), d.cend_partition(0))};
                t.check(equality, t.report_line("Erase empty range"), i, d.begin_partition(0));
                return d;
              }
            },
            {
              data_description::empty_partition,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                t.check_exception_thrown<std::domain_error>("", [&d](){ d.erase_from_partition(d.cbegin_partition(0), d.cend_partition(1)); });
                return d;
              }
            },
            {
              data_description::empty_partition,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                t.check_exception_thrown<std::domain_error>("", [&d](){ d.erase_from_partition(d.cbegin_partition(1), d.cend_partition(0)); });
                return d;
              }
            },
            {
              data_description::empty_partition,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                auto i{d.erase_from_partition(d.cbegin_partition(1), d.cend_partition(1))};
                t.check(equality, t.report_line("Erase fictional range"), i, d.end_partition(1));
                return d;
              }
            },
            {
              data_description::empty_partition,
              t.report_line("Swapping non-existent partition"),
              [&t](data_t d) -> data_t {
                d.swap_partitions(0, 0);
                return d;
              }
            },
            {
              data_description::empty_partition,
              t.report_line("Swapping non-existent partition"),
              [&t](data_t d) -> data_t {
                d.swap_partitions(1, 0);
                return d;
              }
            },
            {
              data_description::empty_partition,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.swap_partitions(0, 1);
                return d;
              }
            },
            {
              data_description::empty,
              t.report_line("Clear empty container"),
              [&t](data_t d) -> data_t {
                d.clear();
                return d;
              }
            },
            {
              data_description::empty,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.erase_slot(0);
                return d;
              }
            },
            {
              data_description::one_2,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.push_back_to_partition(0, 2);
                return d;
              }
            },
            {
              data_description::one_2,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.insert_to_partition(d.cbegin_partition(0), 2);
                return d;
              }
            },
            {
              data_description::two_empty_partitions,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.add_slot();
                return d;
              }
            },
            {
              data_description::two_empty_partitions,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.insert_slot(0);
                return d;
              }
            },
            {
              data_description::two_empty_partitions,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.insert_slot(1);
                return d;
              }
            }
          }, // end 'empty_partition'
          {  // begin 'one_2'
            {
              data_description::empty_partition,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.erase_from_partition(d.cbegin_partition(0));
                return d;
              }
            },
            {
              data_description::empty_partition,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.erase_from_partition(d.cbegin_partition(0), d.cend_partition(0));
                return d;
              }
            },
            {
              data_description::empty,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.erase_slot(0);
                return d;
              }
            },
            {
              data_description::empty,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.clear();
                return d;
              }
            },
            {
              data_description::one_2,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.swap_partitions(0, 0);
                return d;
              }
            },
            {
              data_description::one_3,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                *d.begin_partition(0) = 3;
                return d;
              }
            },
            {
              data_description::one_3,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                *d.rbegin_partition(0) = 3;
                return d;
              }
            },
            {
              data_description::one_2_3,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.push_back_to_partition(0, 3);
                return d;
              }
            },
            {
              data_description::one_2_3,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.insert_to_partition(d.cbegin_partition(0)+1, 3);
                return d;
              }
            },
            {
              data_description::two_2__,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.add_slot();
                return d;
              }
            },
            {
              data_description::two__2,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.insert_slot(0);
                return d;
              }
            }
          }, // end 'one_2'
          {  // begin 'one_3'
            {
              data_description::one_2,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                *d.begin_partition(0) = 2;
                return d;
              }
            },
            {
              data_description::one_3_2,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.push_back_to_partition(0, 2);
                return d;
              }
            },
            {
              data_description::one_2_3,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.insert_to_partition(d.cbegin_partition(0), 2);
                return d;
              }
            }
          }, // end 'one_3'
          {  // begin 'one_2_3'
            {
              data_description::one_3,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.erase_from_partition(d.cbegin_partition(0));
                return d;
              }
            },
            {
              data_description::one_3,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.erase_from_partition(d.cbegin_partition(0), d.cbegin_partition(0) + 1);
                return d;
              }
            },
            {
              data_description::one_2,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.erase_from_partition(d.cbegin_partition(0)+1);
                return d;
              }
            },
            {
              data_description::one_2,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.erase_from_partition(d.cbegin_partition(0) + 1, d.cend_partition(0));
                return d;
              }
            },
            {
              data_description::empty_partition,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.erase_from_partition(d.cbegin_partition(0), d.cend_partition(0));
                return d;
              }
            },
            {
              data_description::empty,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.erase_slot(0);
                return d;
              }
            },
            {
              data_description::empty,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.clear();
                return d;
              }
            },
            {
              data_description::one_3_2,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                std::ranges::sort(d.begin_partition(0), d.end_partition(0), std::greater{});
                return d;
              }
            },
            {
              data_description::two__2_3,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.insert_slot(0);
                return d;
              }
            }
          }, // end 'one_2_3'
          {  // begin 'one_3_2'
            {
              data_description::one_2,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.erase_from_partition(d.cbegin_partition(0));
                return d;
              }
            },
            {
              data_description::one_3,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.erase_from_partition(d.cbegin_partition(0) + 1);
                return d;
              }
            },
            {
              data_description::one_2_3,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                std::ranges::sort(d.begin_partition(0), d.end_partition(0));
                return d;
              }
            },
            {
              data_description::one_3_4_2,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.insert_to_partition(d.cbegin_partition(0) + 1, 4);
                return d;
              }
            },
          }, // end 'one_3_2'
          {  // begin 'one_3_4_2'
            {
              data_description::one_3_2,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.erase_from_partition(d.cbegin_partition(0) + 1);
                return d;
              }
            },
            {
              data_description::one_3_2,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.erase_from_partition(d.cbegin_partition(0) + 1, d.cbegin_partition(0) + 2);
                return d;
              }
            },
            {
              data_description::one_3,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.erase_from_partition(d.cbegin_partition(0) + 1, d.cend_partition(0));
                return d;
              }
            },
            {
              data_description::one_2,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.erase_from_partition(d.cbegin_partition(0), d.cbegin_partition(0)+2);
                return d;
              }
            }
          }, // end 'one_3_4_2'
          {  // begin 'two_empty_partitions'
            {
              data_description::two_empty_partitions,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                t.check_exception_thrown<std::domain_error>("", [&d](){ d.erase_from_partition(d.cbegin_partition(0), d.cend_partition(1)); });
                return d;
              }
            },
            {
              data_description::two_empty_partitions,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.swap_partitions(0, 0);
                return d;
              }
            },
            {
              data_description::empty_partition,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.erase_slot(0);
                return d;
              }
            },
            {
              data_description::empty_partition,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.erase_slot(1);
                return d;
              }
            },
            {
              data_description::empty,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.clear();
                return d;
              }
            },
            {
              data_description::two_2__,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.push_back_to_partition(0, 2);
                return d;
              }
            },
            {
              data_description::two_2__,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.insert_to_partition(d.cbegin_partition(0), 2);
                return d;
              }
            },
            {
              data_description::two__2,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.insert_to_partition(d.cbegin_partition(1), 2);
                return d;
              }
            }
          }, // end 'two_empty_partitions'
          {  // begin 'two_2__'
            {
              data_description::empty_partition,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.erase_slot(0);
                return d;
              }
            },
            {
              data_description::one_2,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.erase_slot(1);
                return d;
              }
            },
            {
              data_description::two__2,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.swap_partitions(0, 1);
                return d;
              }
            },
            {
              data_description::two__2,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.swap_partitions(1, 0);
                return d;
              }
            }
          }, // end 'two_2__'
          {  // begin 'two_3__'
            {
              data_description::two_2_3__,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.insert_to_partition(d.cbegin_partition(0), 2);
                return d;
              }
            }
          }, // end 'two_3__'
          {  // begin 'two_2_3__'
            {
              data_description::two__2_3,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.swap_partitions(0, 1);
                return d;
              }
            },
            {
              data_description::two_3__,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.erase_from_partition(d.cbegin_partition(0));
                return d;
              }
            },
          }, // end 'two_2_3__'
          {  // begin 'two__2'
            {
              data_description::two_2__,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.swap_partitions(1, 0);
                return d;
              }
            }
          }, // end 'two__2'
          {  // begin 'two__2_3'
            {
              data_description::two__2,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.erase_from_partition(d.cbegin_partition(1) + 1);
                return d;
              }
            },
            {
              data_description::two_2_3__,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.swap_partitions(0, 1);
                return d;
              }
            }
          }, // end 'two__2_3'
          {  // begin 'two_2__3'
            {
              data_description::two_3__2,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.swap_partitions(0, 1);
                return d;
              }
            },
            {
              data_description::two_2__,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.erase_from_partition(d.cbegin_partition(1), d.cend_partition(1));
                return d;
              }
            },
            {
              data_description::one_2,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.erase_slot(1);
                return d;
              }
            },
            {
              data_description::one_3,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.erase_slot(0);
                return d;
              }
            },
            {
              data_description::three_2____3,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.insert_slot(1);
                return d;
              }
            },
            {
              data_description::three_2__3__,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.insert_slot(2);
                return d;
              }
            }
          }, // end 'two_2__3'
          {  // begin 'two_3__2'
            {
              data_description::two_2__3,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.swap_partitions(1, 0);
                return d;
              }
            },
            {
              data_description::three_3____2,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.insert_slot(1);
                return d;
              }
            }
          }, // end 'two_3__2'
          {  // begin 'three_2____3'
            {
              data_description::three_3____2,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.swap_partitions(0,2);
                return d;
              }
            },
            {
              data_description::three_2__3__,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.swap_partitions(1,2);
                return d;
              }
            },
            {
              data_description::two_2__3,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.erase_slot(1);
                return d;
              }
            }
          }, // end 'three_2____3'
          {  // begin 'three_3____2'
            {
              data_description::three_2____3,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.swap_partitions(2,0);
                return d;
              }
            },
            {
              data_description::two__2,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.erase_slot(0);
                return d;
              }
            }
          }, // end 'three_3____2'
          {  // begin 'three_2__3__'
            {
              data_description::three_2____3,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                d.swap_partitions(2,1);
                return d;
              }
            }
          }, // end 'three_2__3__'
        },
        {
          //  'empty'
          make_and_check(t, t.report_line(""), {}),

          // 'empty_partition'
          make_and_check(t, t.report_line(""), {{}}),

          // 'one_2'
          make_and_check(t, t.report_line(""), {{2}}),

          // 'one_3'
          make_and_check(t, t.report_line(""), {{3}}),

          // 'one_2_3'
          make_and_check(t, t.report_line(""), {{2, 3}}),

          // 'one_3_2'
          make_and_check(t, t.report_line(""), {{3, 2}}),

          // 'one_3_4_2'
          make_and_check(t, t.report_line(""), {{3, 4, 2}}),

          // 'two_empty_partitions'
          make_and_check(t, t.report_line(""), {{}, {}}),

          // 'two_2__'
          make_and_check(t, t.report_line(""), {{2}, {}}),

          // 'two_3__'
          make_and_check(t, t.report_line(""), {{3}, {}}),

          // 'two_2_3__'
          make_and_check(t, t.report_line(""), {{2,3}, {}}),

          // 'two__2'
          make_and_check(t, t.report_line(""), {{}, {2}}),

          // 'two__2_3'
          make_and_check(t, t.report_line(""), {{}, {2, 3}}),

          // 'two_2__3'
          make_and_check(t, t.report_line(""), {{2}, {3}}),

          // 'two_3__2'
          make_and_check(t, t.report_line(""), {{3}, {2}}),

          // 'three_2____3'
          make_and_check(t, t.report_line(""), {{2}, {}, {3}}),

          // 'three_3____2'
          make_and_check(t, t.report_line(""), {{3}, {}, {2}}),

          // 'three_2__3__'
          make_and_check(t, t.report_line(""), {{2}, {3}, {}}),
        }
      };
    }
  };
}
