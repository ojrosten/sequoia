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
      node,
      node_loop,
      node_two_loops,
      node_node,
      node_link_node,
      node_reverse_link_node,
      node_loop_link_node,
      node_loop_node,
      node_node_node,
      node_link_node_node,
      node_node_reverse_link_node,
      node_link_node_link_node,
      node_link_node_reverse_link_node
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
            [this](graph_to_test g) -> graph_to_test {
              check(equality, report_line("Index of added node is 0"), g.add_node(), 0_sz);
              return g;
            }
          }
        }, // end 'empty' edges
        {
          {
            graph_description::empty,
            "Erase node to give empty graph",
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node,
            "Attempt to erase edge past the end",
            [](graph_to_test g) -> const graph_to_test {
              g.erase_edge(g.cend_edges(0));
              return g;
            }
          },
          {
            graph_description::node_loop,
            "Add loop",
            [](graph_to_test g) -> graph_to_test {
              g.join(0, 0);
              return g;
            }
          },
          {
            graph_description::node_node,
            "Add second node",
            [this](graph_to_test g) -> graph_to_test {
              check(equality, report_line("Index of added node is 1"), g.add_node(), 1_sz);
              return g;
            }
          },
          {
            graph_description::node,
            "Swap nodes",
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(0,0);
              return g;
            }
          }
        }, // end 'node' edges
        {
          {
            graph_description::node,
            "Remove loop",
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(0));
              return g;
            }
          },
          {
            graph_description::node_two_loops,
            "Add a second loop",
            [](graph_to_test g) -> graph_to_test {
              g.join(0, 0);
              return g;
            }
          },
          {
            graph_description::node_loop,
            "Swap nodes",
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(0,0);
              return g;
            }
          }
        }, // end 'node_loop' edges
        {
          {
            graph_description::node_loop,
            "Remove first loop",
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(0));
              return g;
            }
          },
          {
            graph_description::node_loop,
            "Remove second loop",
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(std::ranges::next(g.cbegin_edges(0)));
              return g;
            }
          },
          {
            graph_description::node_two_loops,
            "Swap loops",
            [](graph_to_test g) -> graph_to_test {
              g.swap_edges(0, 0, 1);
              return g;
            }
          },
          {
            graph_description::node_two_loops,
            "Swap loops",
            [](graph_to_test g) -> graph_to_test {
              g.swap_edges(0, 1, 0);
              return g;
            }
          }
        }, // end 'node_two_loops' edges
        {
          {
            graph_description::node,
            "Erase node 0",
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node,
            "Erase node 1",
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_link_node,
            "Join nodes 0,1",
            [](graph_to_test g) -> graph_to_test {
              g.join(0, 1);
              return g;
            }
          },
          {
            graph_description::node,
            "Erase node 0",
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node,
            "Erase node 1",
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(1);
              return g;
            }
          }
        }, // end 'node_node' edges
        {
          {
            graph_description::node,
            "Erase node 0",
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node,
            "Erase node 1",
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_loop_link_node,
            "Add loop to node 0 and then swap it with link",
            [](graph_to_test g) -> graph_to_test {
              g.join(0, 0);
              g.swap_edges(0, 0, 1);
              return g;
            }
          },
          {
            graph_description::node_reverse_link_node,
            "Swap nodes {0,1}",
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(0,1);
              return g;
            }
          },
          {
            graph_description::node_reverse_link_node,
            "Swap nodes {1,0}",
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(1,0);
              return g;
            }
          }
        }, // end 'node_link_node' edges
        {
          {
            graph_description::node,
            "Erase node 0",
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node,
            "Erase node 1",
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_link_node,
            "Swap nodes {0,1}",
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(0,1);
              return g;
            }
          }
          ,
          {
            graph_description::node_link_node,
            "Swap nodes {1,0}",
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(1,0);
              return g;
            }
          }
        }, // end 'node_reverse_link_node' edges
        {
          {
            graph_description::node,
            "Erase node 0",
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_loop,
            "Erase node 1",
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_link_node,
            "Remove loop",
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(0));
              return g;
            }
          },
          {
            graph_description::node_loop_node,
            "Remove link",
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(std::ranges::next(g.cbegin_edges(0)));
              return g;
            }
          }
        }, // end 'node_loop_link_node' edges
        {
          {
            graph_description::node_loop_node,
            "Remove link",
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(std::ranges::next(g.cbegin_edges(0)));
              return g;
            }
          }
        }, // end 'node_loop_node' edges
        {
          {
            graph_description::node_node,
            "Erase node 0",
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_node,
            "Erase node 1",
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_node,
            "Erase node 2",
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(2);
              return g;
            }
          },
          {
            graph_description::node_link_node_node,
            "Join {0,1}",
            [](graph_to_test g) -> graph_to_test {
              g.join(0,1);
              return g;
            }
          },
          {
            graph_description::node_node_reverse_link_node,
            "Join {2,1}",
            [](graph_to_test g) -> graph_to_test {
              g.join(2,1);
              return g;
            }
          }
        }, // end 'node_node_node' edges
        {
          {
            graph_description::node_node,
            "Erase node 0",
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_node,
            "Erase node 1",
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_link_node,
            "Erase node 2",
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(2);
              return g;
            }
          },
          {
            graph_description::node_node_node,
            "Remove link {0,1}",
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(0));
              return g;
            }
          },
          {
            graph_description::node_link_node_link_node,
            "Join {2,1}",
            [](graph_to_test g) -> graph_to_test {
              g.join(1,2);
              return g;
            }
          },
          {
            graph_description::node_link_node_reverse_link_node,
            "Join {2,1}",
            [](graph_to_test g) -> graph_to_test {
              g.join(2,1);
              return g;
            }
          },
          {
            graph_description::node_node_reverse_link_node,
            "Swap nodes {0,2}",
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(0,2);
              return g;
            }
          },
          {
            graph_description::node_node_reverse_link_node,
            "Swap nodes {2,0}",
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(2,0);
              return g;
            }
          }
        }, // end 'node_link_node_node' edges
        {
          {
            graph_description::node_reverse_link_node,
            "Erase node 0",
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_node,
            "Erase node 1",
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_node,
            "Erase node 2",
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(2);
              return g;
            }
          },
          {
            graph_description::node_node_node,
            "Remove link {2,1}",
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(2));
              return g;
            }
          },
          {
            graph_description::node_link_node_reverse_link_node,
            "Join {0,1}",
            [](graph_to_test g) -> graph_to_test {
              g.join(0,1);
              return g;
            }
          },
          {
            graph_description::node_link_node_node,
            "Swap nodes {0,2}",
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(0,2);
              return g;
            }
          },
          {
            graph_description::node_link_node_node,
            "Swap nodes {2,0}",
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(2,0);
              return g;
            }
          }
        }, // end 'node_node_reverse_link_node' edges
        {
          {
            graph_description::node_link_node,
            "Erase node 0",
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_node,
            "Erase node 1",
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_link_node,
            "Erase node 2",
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(2);
              return g;
            }
          },
          {
            graph_description::node_link_node_node,
            "Remove link {1,2}",
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(1));
              return g;
            }
          }
        }, // end 'node_link_node_link_node' edges
        {
          {
            graph_description::node_reverse_link_node,
            "Erase node 0",
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_node,
            "Erase node 1",
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_link_node,
            "Erase node 2",
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(2);
              return g;
            }
          },
          {
            graph_description::node_node_reverse_link_node,
            "Remove link {0,1}",
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(0));
              return g;
            }
          },
          {
            graph_description::node_link_node_node,
            "Remove link {2,1}",
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(2));
              return g;
            }
          }
        }, // end 'node_link_node_reverse_link_node' edges
      },
      {
        //  'empty'
        [this](){
          graph_to_test g{};
          check(equivalence, report_line(""), g, edges_equivalent_t{});

          return g;
        }(),

        //  'node'
        //  x
        [this](){
          graph_to_test g{{}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{}});

          return g;
        }(),

        //  'node_loop'
        //  /\
        //  \/
        //  x
        [this](){
          graph_to_test g{{edge_t{0}}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{edge_t{0}}});

          return g;
        }(),

        //  'node_two_loops'
        //  /\ /\
        //  \/ \/
        //    x
        [this](){
          graph_to_test g{{edge_t{0}, edge_t{0}}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{edge_t{0}, edge_t{0}}});

          return g;
        }(),

        //  'node_node'
        //  x    x
        [this](){
          graph_to_test g{{}, {}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{}, {}});

          return g;
        }(),

        //  'node_link_node'
        //  x ---> x
        [this](){
          graph_to_test g{{edge_t{1}}, {}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{edge_t{1}}, {}});

          return g;
        }(),

        //  'node_reverse_link_node'
        //  x <--- x
        [this](){
          graph_to_test g{{}, {edge_t{0}}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{}, {edge_t{0}}});

          return g;
        }(),

        //  'node_loop_link_node'
        //   /\
        //   \/
        //   x ---> x
        [this](){
          graph_to_test g{{edge_t{0}, edge_t{1}}, {}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{edge_t{0}, edge_t{1}}, {}});

          return g;
        }(),

        //  node_loop_node
        //  /\
        //  \/
        //  x      x
        [this](){
          graph_to_test g{{edge_t{0}}, {}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{edge_t{0}}, {}});

          return g;
        }(),

        //  'node_node_node'
        //  x    x    x
        [this](){
          graph_to_test g{{}, {}, {}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{}, {}, {}});

          return g;
        }(),

        //  'node_link_node_node'
        //   x ---> x    x
        [this](){
          graph_to_test g{{edge_t{1}}, {}, {}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{edge_t{1}}, {}, {}});

          return g;
        }(),

        //  'node_node_reverse_link_node'
        //  x    x <--- x
        [this](){
          graph_to_test g{{}, {}, {edge_t{1}}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{}, {}, {edge_t{1}}});

          return g;
        }(),

        // 'node_link_node_link_node'
        //  x ---> x ---> x
        [this](){
          graph_to_test g{{edge_t{1}}, {edge_t{2}}, {}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{edge_t{1}}, {edge_t{2}}, {}});

          return g;
        }(),

        // 'node_link_node_reverse_link_node'
        //  x ---> x <--- x
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