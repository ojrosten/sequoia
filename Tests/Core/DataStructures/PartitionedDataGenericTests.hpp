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

      // [][]
      two_empty_partitions,

      // [2][]
      two_2__,

      // [][2]
      two__2,

      // [3][]
      two_3__,

      // [2][3]
      two_2__3,

      // [3][2]
      two_3__2
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
            t.check(equality, description, obtained, prediction);
            if(host != target) t.check_semantics(description, prediction, parent);
          }
      };

      transition_checker<data_t>::check(report_line(""), trg, checker);
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
                t.check_exception_thrown<std::out_of_range>(t.report_line("Pushing back to non-existent partition throws"), [&d]() { return d.push_back_to_partition(0, 8); });
                return d;
              }
            },
            {
              data_description::empty,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                t.check_exception_thrown<std::out_of_range>(t.report_line("Inserting to non-existent partition throws"), [&d]() { return d.insert_to_partition(d.cbegin_partition(0), 8); });
                return d;
              }
            },
            {
              data_description::empty,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                t.check_exception_thrown<std::out_of_range>(t.report_line("Inserting to non-existent partition throws"), [&d]() { return d.insert_to_partition(0, 0, 8); });
                return d;
              }
            },
            {
              data_description::empty,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                t.check_exception_thrown<std::out_of_range>(t.report_line("Swapping non-existent partition throws"), [&d]() { return d.swap_partitions(0, 0); });
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
                t.check_exception_thrown<std::out_of_range>(t.report_line("Pushing back to non-existent partition throws"), [&d]() { return d.push_back_to_partition(1, 8); });
                return d;
              }
            },
            {
              data_description::empty_partition,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                t.check_exception_thrown<std::out_of_range>(t.report_line("Inserting to non-existent partition throws"), [&d]() { return d.insert_to_partition(d.cbegin_partition(1), 8); });
                return d;
              }
            },
            {
              data_description::empty_partition,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                t.check_exception_thrown<std::out_of_range>(t.report_line("Inserting to non-existent partition throws"), [&d]() { return d.insert_to_partition(1, 0, 8); });
                return d;
              }
            },
            {
              data_description::empty_partition,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                t.check_exception_thrown<std::out_of_range>(t.report_line("Swapping non-existent partition throws"), [&d]() { return d.swap_partitions(0, 1); });
                return d;
              }
            },
            {
              data_description::empty_partition,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                t.check_exception_thrown<std::out_of_range>(t.report_line("Swapping non-existent partition throws"), [&d]() { return d.swap_partitions(1, 0); });
                return d;
              }
            },
            {
              data_description::empty_partition,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                auto i{d.erase_from_partition(d.cbegin_partition(0))};
                t.check(equality, report_line("Erase from partition with nothing in it"), i, d.begin_partition(0));
                return d;
              }
            },
            {
              data_description::empty_partition,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                auto i{d.erase_from_partition(0, 0)};
                t.check(equality, report_line("Erase from partition with nothing in it"), i, d.begin_partition(0));
                return d;
              }
            },
            {
              data_description::empty_partition,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                auto i{d.erase_from_partition(0, 1)};
                t.check(equality, report_line("Erase from partition with nothing in it"), i, d.begin_partition(0));
                return d;
              }
            },
            {
              data_description::empty_partition,
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
                t.check_exception_thrown<std::out_of_range>(t.report_line(""), [&d]() { return d.swap_partitions(1, 0); });
                return d;
              }
            },
            {
              data_description::empty_partition,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                t.check_exception_thrown<std::out_of_range>(t.report_line(""), [&d]() { return d.swap_partitions(0, 1); });
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
            }
          }, // end 'one_3'
          {  // begin 'two_empty_partitions'
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
            }
          }  // end 'two_empty_partitions'
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

          // 'two_empty_partitions'
          make_and_check(t, t.report_line(""), {{}, {}})
        }
      };
    }
  };
}
