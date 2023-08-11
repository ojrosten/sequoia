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
    };
  }

  template<class PartitionedData>
  class partition_data_operations
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
                t.check_exception_thrown<std::out_of_range>(t.report_line("cbegin_edges throws for empty graph"), [&d]() { return d.push_back_to_partition(0, 8.0); });
                return d;
              }
            }
          } // end 'empty'
        },
        {
          //  'empty'
          make_and_check(t, t.report_line(""), {}),
        }
      };
    }
  };
}
