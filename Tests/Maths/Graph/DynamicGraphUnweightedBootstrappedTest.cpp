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
  namespace
  {
    enum graph_description : std::size_t {
      empty = 0,
      node = 1,
      node_loop = 2,
      node_two_loops = 3,
      node_node = 4,
      node_link_node = 5,
      node_reverse_link_node = 6,
      node_loop_link_node = 7,
      node_loop_node = 8,
      node_node_node = 9,
      node_link_node_node = 10,
      node_node_reverse_link_node = 11,
      node_link_node_link_node = 12,
      node_link_node_reverse_link_node = 13
    };
  }

  [[nodiscard]]
  std::filesystem::path unweighted_graph_bootstrapped_test::source_file() const
  {
    return std::source_location::current().file_name();
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
    using edges_equivalent_t = std::initializer_list<std::initializer_list<edge_t>>;

    using transition_graph = transition_checker<graph_to_test>::transition_graph;

    transition_graph trg{
      {
        { {
            graph_description::empty,
            "",
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("cbegin_edges throws for empty graph"), [&g]() { return g.cbegin_edges(0); });
              return g;
            }
          },
          {
            graph_description::empty,
            "",
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("cend_edges throws for empty graph"), [&g]() { return g.cend_edges(0); });
              return g;
            }
          },
          {
            graph_description::empty,
            "",
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("crbegin_edges throws for empty graph"), [&g]() { return g.crbegin_edges(0); });
              return g;
            }
          },
          {
            graph_description::empty,
            "",
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("crend_edges throws for empty graph"), [&g]() { return g.crend_edges(0); });
              return g;
            }
          },
          {
            graph_description::empty,
            "",
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("swapping nodes throws for empty graph"), [g{g}]() mutable { g.swap_nodes(0,0); });
              return g;
            }
          },
          {
            graph_description::node,
            "Add node to empty graph",
            [this](const graph_to_test& g) -> graph_to_test {
              auto gr{g};
              check(equality, report_line("Index of added node is 0"), gr.add_node(), 0_sz);
              return gr;
            }
          }
        }, // end node 0 edges
        {
          {
            graph_description::empty,
            "Erase node to give empty graph",
            [](const graph_to_test& g) -> graph_to_test {
              auto gr{g};
              gr.erase_node(0);
              return gr;
            }
          },
          {
            graph_description::node,
            "Attempt to erase edge past the end",
            [](const graph_to_test& g) -> const graph_to_test {
              auto gr{g};
              gr.erase_edge(gr.cend_edges(0));
              return gr;
            }
          },
          {
            graph_description::node_loop,
            "Add loop",
            [](const graph_to_test & g) -> graph_to_test {
              auto gr{g};
              gr.join(0, 0);
              return gr;
            }
          },
          {
            graph_description::node_node,
            "Add second node",
            [this](const graph_to_test& g) -> graph_to_test {
              auto gr{g};
              check(equality, report_line("Index of added node is 1"), gr.add_node(), 1_sz);
              return gr;
            }
          }
        }, // end node 1 edges
        {
          {
            graph_description::node,
            "Remove loop",
            [](const graph_to_test& g) -> graph_to_test {
              auto gr{g};
              gr.erase_edge(gr.cbegin_edges(0));
              return gr;
            }
          },
          {
            graph_description::node_two_loops,
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
            graph_description::node_loop,
            "Remove first loop",
            [](const graph_to_test& g) -> graph_to_test {
              auto gr{g};
              gr.erase_edge(gr.cbegin_edges(0));
              return gr;
            }
          },
          {
            graph_description::node_loop,
            "Remove second loop",
            [](const graph_to_test& g) -> graph_to_test {
              auto gr{g};
              gr.erase_edge(std::ranges::next(gr.cbegin_edges(0)));
              return gr;
            }
          },
          {
            graph_description::node_two_loops,
            "Swap loops",
            [](const graph_to_test& g) -> graph_to_test {
              auto gr{g};
              gr.swap_edges(0, 0, 1);
              return gr;
            }
          },
          {
            graph_description::node_two_loops,
            "Swap loops",
            [](const graph_to_test& g) -> graph_to_test {
              auto gr{g};
              gr.swap_edges(0, 1, 0);
              return gr;
            }
          }
        }, // end node 3 edges
        {
          {
            graph_description::node,
            "Erase node 0",
            [](const graph_to_test& g) -> graph_to_test {
              auto gr{g};
              gr.erase_node(0);
              return gr;
            }
          },
          {
            graph_description::node,
            "Erase node 1",
            [](const graph_to_test& g) -> graph_to_test {
              auto gr{g};
              gr.erase_node(1);
              return gr;
            }
          },
          {
            graph_description::node_link_node,
            "Join nodes 0,1",
            [](const graph_to_test& g) -> graph_to_test {
              auto gr{g};
              gr.join(0, 1);
              return gr;
            }
          },
          {
            graph_description::node,
            "Erase node 0",
            [](const graph_to_test& g) -> graph_to_test {
              auto gr{g};
              gr.erase_node(0);
              return gr;
            }
          },
          {
            graph_description::node,
            "Erase node 1",
            [](const graph_to_test& g) -> graph_to_test {
              auto gr{g};
              gr.erase_node(1);
              return gr;
            }
          }
        }, // end node 4 edges
        {
          {
            graph_description::node,
            "Erase node 0",
            [](const graph_to_test& g) -> graph_to_test {
              auto gr{g};
              gr.erase_node(0);
              return gr;
            }
          },
          {
            graph_description::node,
            "Erase node 1",
            [](const graph_to_test& g) -> graph_to_test {
              auto gr{g};
              gr.erase_node(1);
              return gr;
            }
          },
          {
            graph_description::node_loop_link_node,
            "Add loop to node 0 and then swap it with link",
            [](const graph_to_test& g) -> graph_to_test {
              auto gr{g};
              gr.join(0, 0);
              gr.swap_edges(0, 0, 1);
              return gr;
            }
          }
        }, // end node 5 edges
        {
          {
            graph_description::node,
            "Erase node 0",
            [](const graph_to_test& g) -> graph_to_test {
              auto gr{g};
              gr.erase_node(0);
              return gr;
            }
          },
          {
            graph_description::node,
            "Erase node 1",
            [](const graph_to_test& g) -> graph_to_test {
              auto gr{g};
              gr.erase_node(1);
              return gr;
            }
          }
        }, // end node 6 edges
        {
          {
            graph_description::node,
            "Erase node 0",
            [](const graph_to_test& g) -> graph_to_test {
              auto gr{g};
              gr.erase_node(0);
              return gr;
            }
          },
          {
            graph_description::node_loop,
            "Erase node 1",
            [](const graph_to_test& g) -> graph_to_test {
              auto gr{g};
              gr.erase_node(1);
              return gr;
            }
          },
          {
            graph_description::node_link_node,
            "Remove loop",
            [](const graph_to_test& g) -> graph_to_test {
              auto gr{g};
              gr.erase_edge(gr.cbegin_edges(0));
              return gr;
            }
          },
          {
            graph_description::node_loop_node,
            "Remove link",
            [](const graph_to_test& g) -> graph_to_test {
              auto gr{g};
              gr.erase_edge(std::ranges::next(gr.cbegin_edges(0)));
              return gr;
            }
          }
        }, // end node 7 edges
        {
          {
            graph_description::node_loop_node,
            "Remove link",
            [](const graph_to_test& g) -> graph_to_test {
              auto gr{g};
              gr.erase_edge(std::ranges::next(gr.cbegin_edges(0)));
              return gr;
            }
          }
        }, // end node 8 edges
        {
          {
            graph_description::node_node,
            "Erase node 0",
            [](const graph_to_test& g) -> graph_to_test {
              auto gr{g};
              gr.erase_node(0);
              return gr;
            }
          },
          {
            graph_description::node_node,
            "Erase node 1",
            [](const graph_to_test& g) -> graph_to_test {
              auto gr{g};
              gr.erase_node(1);
              return gr;
            }
          },
          {
            graph_description::node_node,
            "Erase node 2",
            [](const graph_to_test& g) -> graph_to_test {
              auto gr{g};
              gr.erase_node(2);
              return gr;
            }
          },
          {
            graph_description::node_link_node_node,
            "Join {0,1}",
            [](const graph_to_test& g) -> graph_to_test {
              auto gr{g};
              gr.join(0,1);
              return gr;
            }
          },
          {
            graph_description::node_node_reverse_link_node,
            "Join {2,1}",
            [](const graph_to_test& g) -> graph_to_test {
              auto gr{g};
              gr.join(2,1);
              return gr;
            }
          }
        }, // end node 9 edges
        {
          {
            graph_description::node_node,
            "Erase node 0",
            [](const graph_to_test& g) -> graph_to_test {
              auto gr{g};
              gr.erase_node(0);
              return gr;
            }
          },
          {
            graph_description::node_node,
            "Erase node 1",
            [](const graph_to_test& g) -> graph_to_test {
              auto gr{g};
              gr.erase_node(1);
              return gr;
            }
          },
          {
            graph_description::node_link_node,
            "Erase node 2",
            [](const graph_to_test& g) -> graph_to_test {
              auto gr{g};
              gr.erase_node(2);
              return gr;
            }
          },
          {
            graph_description::node_node_node,
            "Remove link",
            [](const graph_to_test& g) -> graph_to_test {
              auto gr{g};
              gr.erase_edge(gr.cbegin_edges(0));
              return gr;
            }
          },
          {
            graph_description::node_link_node_link_node,
            "Join {2,1}",
            [](const graph_to_test& g) -> graph_to_test {
              auto gr{g};
              gr.join(1,2);
              return gr;
            }
          },
          {
            graph_description::node_link_node_reverse_link_node,
            "Join {2,1}",
            [](const graph_to_test& g) -> graph_to_test {
              auto gr{g};
              gr.join(2,1);
              return gr;
            }
          }
        }, // end node 10 edges
        {
          {
            graph_description::node_reverse_link_node,
            "Erase node 0",
            [](const graph_to_test& g) -> graph_to_test {
              auto gr{g};
              gr.erase_node(0);
              return gr;
            }
          },
          {
            graph_description::node_node,
            "Erase node 1",
            [](const graph_to_test& g) -> graph_to_test {
              auto gr{g};
              gr.erase_node(1);
              return gr;
            }
          },
          {
            graph_description::node_node,
            "Erase node 2",
            [](const graph_to_test& g) -> graph_to_test {
              auto gr{g};
              gr.erase_node(2);
              return gr;
            }
          },
          {
            graph_description::node_node_node,
            "Remove link",
            [](const graph_to_test& g) -> graph_to_test {
              auto gr{g};
              gr.erase_edge(gr.cbegin_edges(2));
              return gr;
            }
          }
        }, // end node 11 edges
        {

        }, // end node 12 edges
        {

        }, // end node 13 edges
      },
      {
        // [0] empty
        [this](){
          graph_to_test g{};
          check(equivalence, report_line(""), g, edges_equivalent_t{});

          return g;
        }(),

        // [1] x
        [this](){
          graph_to_test g{{}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{}});

          return g;
        }(),

        // [2] /\
        //     \/
        //     x
        [this](){
          graph_to_test g{{edge_t{0}}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{edge_t{0}}});

          return g;
        }(),

        // [3] /\ /\
        //     \/ \/
        //       x
        [this](){
          graph_to_test g{{edge_t{0}, edge_t{0}}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{edge_t{0}, edge_t{0}}});

          return g;
        }(),

        // [4] x    x
        [this](){
          graph_to_test g{{}, {}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{}, {}});

          return g;
        }(),

        // [5] x ---> x
        [this](){
          graph_to_test g{{edge_t{1}}, {}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{edge_t{1}}, {}});

          return g;
        }(),

        // [6] x <--- x
        [this](){
          graph_to_test g{{}, {edge_t{0}}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{}, {edge_t{0}}});

          return g;
        }(),

        // [7] /\
        //     \/
        //     x ---> x
        [this](){
          graph_to_test g{{edge_t{0}, edge_t{1}}, {}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{edge_t{0}, edge_t{1}}, {}});

          return g;
        }(),

        // [8] /\
        //     \/
        //     x      x
        [this](){
          graph_to_test g{{edge_t{0}}, {}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{edge_t{0}}, {}});

          return g;
        }(),

        // [9] x    x    x
        [this](){
          graph_to_test g{{}, {}, {}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{}, {}, {}});

          return g;
        }(),

        // [10] x ---> x    x
        [this](){
          graph_to_test g{{edge_t{1}}, {}, {}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{edge_t{1}}, {}, {}});

          return g;
        }(),

        // [11] x    x <--- x
        [this](){
          graph_to_test g{{}, {}, {edge_t{1}}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{}, {}, {edge_t{1}}});

          return g;
        }(),

        // [12] x ---> x ---> x
        [this](){
          graph_to_test g{{edge_t{1}}, {edge_t{2}}, {}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{edge_t{1}}, {edge_t{2}}, {}});

          return g;
        }(),

        // [13] x ---> x <--- x
        [this](){
          graph_to_test g{{edge_t{1}}, {}, {edge_t{1}}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{edge_t{1}}, {}, {edge_t{1}}});

          return g;
        }(),
      }
    };

    auto checker{
        [this](std::string_view description, const graph_to_test& obtained, const graph_to_test& prediction) {
          check(equality, description, obtained, prediction);
        }
    };

    transition_checker<graph_to_test>::check(report_line(""), trg, checker);
  }
}