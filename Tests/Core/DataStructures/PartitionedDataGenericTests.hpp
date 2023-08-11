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

      // [][]
      two_empty_partitions,

      // [2][]
      two_2__,

      // [][2]
      two__2,

      // [3][]
      two_3__,

      // [3][4]
      two_3__4,

      // [4][3]
      two_4__3,

      // [3][4][9,2]
      three_3__4__9_2,

      // [3][9,2][4]
      three_3__9_2_4,
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
            /*{
              data_description::empty_partition,
              t.report_line(""),
              [&t](data_t d) -> data_t {
                t.check_exception_thrown<std::out_of_range>(t.report_line("Inserting to non-existent partition throws"), [&d]() { return d.insert_to_partition(d.cbegin_partition(1), 8); });
                return d;
              }
            },*/
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
              data_description::empty,
              t.report_line("Clear empty container"),
              [&t](data_t d) -> data_t {
                d.clear();
                return d;
              }
            }
          }  // end 'empty_partition'
        },
        {
          //  'empty'
          make_and_check(t, t.report_line(""), {}),

          // 'empty_partition'
          make_and_check(t, t.report_line(""), {{}}),
        }
      };
    }
  };
}
