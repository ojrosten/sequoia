////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "DynamicGraphUnweightedBootstrappedTest.hpp"

#include "sequoia/TestFramework/StateTransitionUtilities.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view unweighted_graph_bootstrapped_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void unweighted_graph_bootstrapped_test::run_tests()
  {
    execute_operations();
  }

  void unweighted_graph_bootstrapped_test::execute_operations()
  {
    using namespace maths;
    using graph_to_test = graph<directed_flavour::directed, null_weight, null_weight>;

    using transition_graph = transition_checker<graph_to_test>::transition_graph;
    using edge_t           = transition_checker<graph_to_test>::edge;

    transition_graph trg{
      {
        { edge_t{0,
                 "",
                 [this](const graph_to_test& g) -> const graph_to_test& {
                   check_exception_thrown<std::out_of_range>(LINE("cbegin_edges throws for empty graph"), [&g]() { return g.cbegin_edges(0); });
                   return g;
                 }
          },
          edge_t{0,
                 "",
                 [this](const graph_to_test& g) -> const graph_to_test& {
                   check_exception_thrown<std::out_of_range>(LINE("cend_edges throws for empty graph"), [&g]() { return g.cend_edges(0); });
                   return g;
                 }
          },
          edge_t{0,
                 "",
                 [this](const graph_to_test& g) -> const graph_to_test& {
                   check_exception_thrown<std::out_of_range>(LINE("swapping nodes throws for empty graph"), [g{g}]() mutable { g.swap_nodes(0,0); });
                   return g;
                 }
          },
          edge_t{1,
                 "Add a node",
                 [this](const graph_to_test& g) -> graph_to_test {
                   auto gr{g};
                   check(equality, LINE("Index of added node is 0"), gr.add_node(), 0_sz);
                   return gr;
                 }
          }
        },
        {}
      },
      {graph_to_test{}, graph_to_test{{}}}
    };

    auto checker{
        [this](std::string_view description, const graph_to_test& obtained, const graph_to_test& prediction) {
          check(equality, description, obtained, prediction);
        }
    };

    transition_checker<graph_to_test>::check(LINE(""), trg, checker);
  }
}