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
    using edge_t = graph_to_test::edge_init_type;

    using transition_graph = transition_checker<graph_to_test>::transition_graph;

    transition_graph trg{
      {
        { {
            0,
            "",
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(LINE("cbegin_edges throws for empty graph"), [&g]() { return g.cbegin_edges(0); });
              return g;
            }
          },
          {
            0,
            "",
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(LINE("cend_edges throws for empty graph"), [&g]() { return g.cend_edges(0); });
              return g;
            }
          },
          {
            0,
            "",
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(LINE("crbegin_edges throws for empty graph"), [&g]() { return g.crbegin_edges(0); });
              return g;
            }
          },
          {
            0,
            "",
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(LINE("crend_edges throws for empty graph"), [&g]() { return g.crend_edges(0); });
              return g;
            }
          },
          {
            0,
            "",
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(LINE("swapping nodes throws for empty graph"), [g{g}]() mutable { g.swap_nodes(0,0); });
              return g;
            }
          },
          {
            1,
            "Add node to empty graph",
            [this](const graph_to_test& g) -> graph_to_test {
              auto gr{g};
              check(equality, LINE("Index of added node is 0"), gr.add_node(), 0_sz);
              return gr;
            }
          }
        }, // end node 0 edges
        {
          {
            0,
            "Erase node to give empty graph",
            [](const graph_to_test& g) -> graph_to_test {
              auto gr{g};
              gr.erase_node(0);
              return gr;
            }
          },
          {
            2,
            "Add loop",
            [](const graph_to_test & g) -> graph_to_test {
              auto gr{g};
              gr.join(0, 0);
              return gr;
            }
          },
          {
            4,
            "Add second node",
            [this](const graph_to_test& g) -> graph_to_test {
              auto gr{g};
              check(equality, LINE("Index of added node is 0"), gr.add_node(), 1_sz);
              return gr;
            }
          }
        }, // end node 1 edges
        {
          {
            1,
            "Remove loop",
            [](const graph_to_test& g) -> graph_to_test {
              auto gr{g};
              gr.erase_edge(gr.cbegin_edges(0));
              return gr;
            }
          },
          {
            3,
            "Add a second loop",
            [](const graph_to_test& g) -> graph_to_test {
              auto gr{g};
              gr.join(0, 0);
              return gr;
            }
          }
        }, // end node 2 edges
        {
          {
            2,
            "Remove first loop",
            [](const graph_to_test& g) -> graph_to_test {
              auto gr{g};
              gr.erase_edge(gr.cbegin_edges(0));
              return gr;
            }
          },
          {
            2,
            "Remove second loop",
            [](const graph_to_test& g) -> graph_to_test {
              auto gr{g};
              gr.erase_edge(std::next(gr.cbegin_edges(0)));
              return gr;
            }
          }
        }, // end node 3 edges
        {
          {
            1,
            "Erase the first node",
            [](const graph_to_test& g) -> graph_to_test {
              auto gr{g};
              gr.erase_node(0);
              return gr;
            }
          },
          {
            1,
            "Erase the first node",
            [](const graph_to_test& g) -> graph_to_test {
              auto gr{g};
              gr.erase_node(1);
              return gr;
            }
          }
        } // end node 4 edges
      },
      {
        graph_to_test{},
        // [0] empty

        graph_to_test{{}},
        // [1] x

        graph_to_test{{edge_t{0}}},
        // [2] /\
        //     \/
        //     x

        graph_to_test{{edge_t{0}, edge_t{0}}},
        // [3] /\ /\
        //     \/ \/
        //       x

        graph_to_test{{}, {}}
        // [4] x    x
      }
    };

    auto checker{
        [this](std::string_view description, const graph_to_test& obtained, const graph_to_test& prediction) {
          check(equality, description, obtained, prediction);
        }
    };

    transition_checker<graph_to_test>::check(LINE(""), trg, checker);
  }
}