////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "DynamicDirectedGraphUnweightedTest.hpp"

#include "sequoia/TestFramework/StateTransitionUtilities.hpp"

namespace sequoia::testing
{
  namespace
  {
    /// Convention: the indices following 'node' - separated by underscores - give the target node of the associated edges
    enum graph_description : std::size_t {
      empty = 0,

      // x
      node,

      //  /\
      //  \/
      //  x
      node_0,

      //  /\ /\
      //  \/ \/
      //    x
      node_0_0,

      //  x    x
      node_node,

      //  x ---> x
      node_1_node,

      //  x <--- x
      node_node_0,

      //   /\
      //   \/
      //   x ---> x
      node_0_1_node,

      //  /\
      //  \/
      //  x      x
      node_0_node,

      //      /\
      //      \/
      //  x    x
      node_node_1,

      //  x ===> x
      node_1_1_node,

      // x ---> x
      //   <---
      node_1_node_0,

      //  x    x    x
      node_node_node,

      //   x ---> x    x
      node_1_node_node,

      //  x    x <--- x
      node_node_node_1,

      //  x ---> x ---> x
      node_1_node_2_node,

      //  x ---> x <--- x
      node_1_node_node_1,

      // -> x ---> x ---> x --
      node_1_node_2_node_0,

      //      /\
      //      \/
      //  x    x    x
      node_node_1_node,

      //        /\
      //        \/
      //  x ---> x    x
      node_1_node_1_node,


      //        /\
      //        \/
      //  x ---> x    x
      //    <---
      node_1_node_1_0_node,

      //        /\
      //        \/
      //  x ---> x ---> x
      //    <---
      node_1_node_1_0_2_node,

      //        /\
      //        \/
      //  x ---> x ---> x
      node_1_node_1_2_node,

      //             /\
      //             \/
      // -- x    x    x <-
      node_2_node_node_2,

      //     [2]
      //      x
      //    ^^  ^
      //   //    \
      //  //      \
      //  x  ===>  x
      // [0]      [1]
      node_1_1_2_2_node_2_node,

      //      [2]
      //       x
      //    ^^  \^
      //   //    \\
      //  //     `'\
      //  x  ===>  x
      // [0]      [1]
      node_1_1_2_2_node_2_node_1,

      //      [2]
      //       x
      //    ^^  \^
      //   //    \\
      //  //     `'\
      //  x  ===>  x
      // [0]      [1]
      node_2_2_1_1_node_2_node_1,

      //  x [3]
      //  ^
      //  |
      //  |
      //  x ---> x ---> x
      // [0]    [1]    [2]
      node_3_1_node_2_node_node,

      // x ---> x    x <--- x
      node_1_node_node_node_2,

      // x --   x <-   -> x  -- x
      node_2_node_node_node_1
    };
  }

  [[nodiscard]]
  std::filesystem::path dynamic_directed_graph_unweighted_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void dynamic_directed_graph_unweighted_test::run_tests()
  {
    using namespace maths;
    graph_test_helper<null_weight, null_weight, dynamic_directed_graph_unweighted_test> helper{*this};

    helper.run_tests<graph_flavour::directed>();
  }

  template
  <
    maths::graph_flavour GraphFlavour,
    class EdgeWeight,
    class NodeWeight,
    class EdgeWeightCreator,
    class NodeWeightCreator,
    class EdgeStorageTraits,
    class NodeWeightStorageTraits
  >
  void dynamic_directed_graph_unweighted_test::execute_operations()
  {
    using namespace maths;
    using namespace object;
    using graph_to_test = graph_type_generator_t<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightCreator, NodeWeightCreator, EdgeStorageTraits, NodeWeightStorageTraits>;
    using edge_t = typename graph_to_test::edge_init_type;
    using edges_equivalent_t = std::initializer_list<std::initializer_list<edge_t>>;

    using transition_graph = transition_checker<graph_to_test>::transition_graph;

    transition_graph trg{
      {
        { // begin, 'empty'
          {
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
              check_exception_thrown<std::out_of_range>(report_line("cedges throws for empty graph"), [&g]() { return g.cedges(0); });
              return g;
            }
          },
          {
            graph_description::empty,
            "",
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("swapping nodes throws for empty graph"), [g{g}]() mutable { g.swap_nodes(0, 0); });
              return g;
            }
          },
          {
            graph_description::empty,
            "",
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("swapping edges throws for empty graph"), [g{g}]() mutable { g.swap_edges(0, 0, 0); });
              return g;
            }
          },
          {
            graph_description::empty,
            "Clear empty graph",
            [](graph_to_test g) -> graph_to_test {
              g.clear();
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
          },
          {
            graph_description::node,
            "insert node into empty graph",
            [this](graph_to_test g) -> graph_to_test {
              check(equality, report_line("Index of added node is 0"), g.insert_node(0), 0_sz);
              return g;
            }
          }
        }, // end 'empty'
        {  // begin 'node'
          {
            graph_description::node,
            "",
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("cbegin_edges throws when index is out of range"), [&g]() { return g.cbegin_edges(1); });
              return g;
            }
          },
          {
            graph_description::node,
            "",
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("cend_edges throws when index is out of range"), [&g]() { return g.cend_edges(1); });
              return g;
            }
          },
          {
            graph_description::node,
            "",
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("crbegin_edges throws when index is out of range"), [&g]() { return g.crbegin_edges(1); });
              return g;
            }
          },
          {
            graph_description::node,
            "",
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("crend_edges throws when index is out of range"), [&g]() { return g.crend_edges(1); });
              return g;
            }
          },
          {
            graph_description::node,
            "",
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("cedges throws when index is out of range"), [&g]() { return g.cedges(1); });
              return g;
            }
          },
          {
            graph_description::empty,
            "Clear graph",
            [](graph_to_test g) -> graph_to_test {
              g.clear();
              return g;
            }
          },
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
            graph_description::node_0,
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
            graph_description::node_node,
            "Insert second node",
            [this](graph_to_test g) -> graph_to_test {
              check(equality, report_line("Index of added node is 0"), g.insert_node(0), 0_sz);
              return g;
            }
          },
          {
            graph_description::node_node,
            "Insert second node at end",
            [this](graph_to_test g) -> graph_to_test {
              check(equality, report_line("Index of added node is 1"), g.insert_node(1), 1_sz);
              return g;
            }
          },
          {
            graph_description::node,
            "Swap node with self",
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(0,0);
              return g;
            }
          },
          {
            graph_description::node,
            "",
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("swapping nodes throws if first index is out of range"), [g{g}]() mutable { g.swap_nodes(1, 0); });
              return g;
            }
          },
          {
            graph_description::node,
            "",
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("swapping nodes throws if second index is out of range"), [g{g}]() mutable { g.swap_nodes(0, 1); });
              return g;
            }
          },
        }, // end 'node'
        {  // begin 'node_0'
          {
            graph_description::empty,
            "Clear graph",
            [](graph_to_test g) -> graph_to_test {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node,
            "Remove loop",
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(0));
              return g;
            }
          },
          {
            graph_description::node_0_0,
            "Add a second loop",
            [](graph_to_test g) -> graph_to_test {
              g.join(0, 0);
              return g;
            }
          },
          {
            graph_description::node_node_1,
            "Insert node",
            [this](graph_to_test g) -> graph_to_test {
              check(equality, report_line("Index of added node is 0"), g.insert_node(0), 0_sz);
              return g;
            }
          },
          {
            graph_description::node_0_node,
            "Insert node at end",
            [this](graph_to_test g) -> graph_to_test {
              check(equality, report_line("Index of added node is 1"), g.insert_node(1), 1_sz);
              return g;
            }
          },
          {
            graph_description::node_0,
            "Swap node with self",
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(0,0);
              return g;
            }
          },
          {
            graph_description::node_0,
            "Swap edge with self",
            [](graph_to_test g) -> graph_to_test {
              g.swap_edges(0, 0, 0);
              return g;
            }
          },
          {
            graph_description::node_0,
            "",
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("swapping edges throws for first edge index out of range"), [g{g}]() mutable { g.swap_edges(0, 1, 0); });
              return g;
            }
          },
          {
            graph_description::node_0,
            "",
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("swapping edges throws for second edge index out of range"), [g{g}]() mutable { g.swap_edges(0, 0, 1); });
              return g;
            }
          },
          {
            graph_description::node_0,
            "",
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("swapping edges throws for node index out of range"), [g{g}]() mutable { g.swap_edges(1, 0, 0); });
              return g;
            }
          }
        }, // end 'node_0'
        {  // begin 'node_0_0'
          {
            graph_description::empty,
            "Clear graph",
            [](graph_to_test g) -> graph_to_test {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node_0,
            "Remove first loop",
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(0));
              return g;
            }
          },
          {
            graph_description::node_0,
            "Remove second loop",
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(std::ranges::next(g.cbegin_edges(0)));
              return g;
            }
          },
          {
            graph_description::node_0_0,
            "Swap loops",
            [](graph_to_test g) -> graph_to_test {
              g.swap_edges(0, 0, 1);
              return g;
            }
          },
          {
            graph_description::node_0_0,
            "Swap loops",
            [](graph_to_test g) -> graph_to_test {
              g.swap_edges(0, 1, 0);
              return g;
            }
          }
        }, // end 'node_0_0'
        {  // begin 'node_node'
          {
            graph_description::empty,
            "Clear graph",
            [](graph_to_test g) -> graph_to_test {
              g.clear();
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
          },
          {
            graph_description::node_1_node,
            "Join nodes 0,1",
            [](graph_to_test g) -> graph_to_test {
              g.join(0, 1);
              return g;
            }
          }
        }, // end 'node_node'
        {  // begin 'node_1_node'
          {
            graph_description::empty,
            "Clear graph",
            [](graph_to_test g) -> graph_to_test {
              g.clear();
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
          },
          {
            graph_description::node_0_1_node,
            "Add loop to node 0 and then swap it with link",
            [](graph_to_test g) -> graph_to_test {
              g.join(0, 0);
              g.swap_edges(0, 0, 1);
              return g;
            }
          },
          {
            graph_description::node_node_0,
            "Swap nodes {0,1}",
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(0,1);
              return g;
            }
          },
          {
            graph_description::node_node_0,
            "Swap nodes {1,0}",
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(1,0);
              return g;
            }
          },
          {
            graph_description::node_1_1_node,
            "Join {0,1}",
            [](graph_to_test g) {
              g.join(0, 1);
              return g;
            }
          }
        }, // end 'node_1_node'
        {  // begin node_node_0
          {
            graph_description::empty,
            "Clear graph",
            [](graph_to_test g) -> graph_to_test {
              g.clear();
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
          },
          {
            graph_description::node_1_node,
            "Swap nodes {0,1}",
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(0,1);
              return g;
            }
          },
          {
            graph_description::node_1_node,
            "Swap nodes {1,0}",
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(1,0);
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            "Join nodes {0,1}",
            [](graph_to_test g) {
              g.join(0, 1);
              return g;
            }
          }
        }, // end 'node_node_0'
        {  // begin 'node_0_1_node'
          {
            graph_description::empty,
            "Clear graph",
            [](graph_to_test g) -> graph_to_test {
              g.clear();
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
            graph_description::node_0,
            "Erase node 1",
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_1_node,
            "Remove loop",
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(0));
              return g;
            }
          },
          {
            graph_description::node_0_node,
            "Remove link",
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(std::ranges::next(g.cbegin_edges(0)));
              return g;
            }
          }
        }, // end 'node_0_1_node'
        {  // begin 'node_0_node'
          {
            graph_description::empty,
            "Clear graph",
            [](graph_to_test g) -> graph_to_test {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node_0_node,
            "Remove link",
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(std::ranges::next(g.cbegin_edges(0)));
              return g;
            }
          },
          {
            graph_description::node_node_1_node,
            "Insert node",
            [this](graph_to_test g) -> graph_to_test {
              check(equality, report_line("Index of added node is 0"), g.insert_node(0), 0_sz);
              return g;
            }
          },
          {
            graph_description::node_node_1,
            "Swap nodes",
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(0,1);
              return g;
            }
          }
        }, // end 'node_0_node'
        {  // begin 'node_node_1'
          {
            graph_description::empty,
            "Clear graph",
            [](graph_to_test g) -> graph_to_test {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node_0,
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
            graph_description::node_node,
            "Remove link",
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(1));
              return g;
            }
          },
          {
            graph_description::node_0_node,
            "swap nodes",
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(0,1);
              return g;
            }
          }
        }, // end 'node_node_1'
        {  // begin 'node_1_1_node'

          {
            graph_description::empty,
            "Clear graph",
            [](graph_to_test g) -> graph_to_test {
              g.clear();
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
          },
          {
            graph_description::node_1_node,
            "Erase node 0 zeroth link",
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(0));
              return g;
            }
          },
          {
            graph_description::node_1_node,
            "Erase node 0 first link",
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(0)+1);
              return g;
            }
          },
          {
            graph_description::node_1_1_node,
            "Swap edges",
            [](graph_to_test g) -> graph_to_test {
              g.swap_edges(0, 0, 1);
              return g;
            }
          },
          {
            graph_description::node_1_1_node,
            "Swap edges",
            [](graph_to_test g) -> graph_to_test {
              g.swap_edges(0, 1, 0);
              return g;
            }
          }
        }, // end 'node_1_1_node'
        {  // beging 'node_1_node_0'
          {
            graph_description::empty,
            "Clear graph",
            [](graph_to_test g) -> graph_to_test {
              g.clear();
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
            "Remove link",
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_node_0,
            "Erase node 0 link",
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(0));
              return g;
            }
          },
          {
            graph_description::node_1_node,
            "Erase node 1 link",
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(1));
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            "Swap nodes",
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(1,0);
              return g;
            }
          }
        }, // end 'node_1_node_0'
        {  // begin 'node_node_node'
          {
            graph_description::empty,
            "Clear graph",
            [](graph_to_test g) -> graph_to_test {
              g.clear();
              return g;
            }
          },
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
            graph_description::node_1_node_node,
            "Join {0,1}",
            [](graph_to_test g) -> graph_to_test {
              g.join(0,1);
              return g;
            }
          },
          {
            graph_description::node_node_node_1,
            "Join {2,1}",
            [](graph_to_test g) -> graph_to_test {
              g.join(2,1);
              return g;
            }
          }
        }, // end 'node_node_node'
        {  // begin 'node_1_node_node'

          {
            graph_description::empty,
            "Clear graph",
            [](graph_to_test g) -> graph_to_test {
              g.clear();
              return g;
            }
          },
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
            graph_description::node_1_node,
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
            graph_description::node_1_node_2_node,
            "Join {2,1}",
            [](graph_to_test g) -> graph_to_test {
              g.join(1,2);
              return g;
            }
          },
          {
            graph_description::node_1_node_node_1,
            "Join {2,1}",
            [](graph_to_test g) -> graph_to_test {
              g.join(2,1);
              return g;
            }
          },
          {
            graph_description::node_node_node_1,
            "Swap nodes {0,2}",
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(0,2);
              return g;
            }
          },
          {
            graph_description::node_node_node_1,
            "Swap nodes {2,0}",
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(2,0);
              return g;
            }
          }
        }, // end 'node_1_node_node'
        {  // begin 'node_node_node_1'
          {
            graph_description::empty,
            "Clear graph",
            [](graph_to_test g) -> graph_to_test {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node_node_0,
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
            graph_description::node_1_node_node_1,
            "Join {0,1}",
            [](graph_to_test g) -> graph_to_test {
              g.join(0,1);
              return g;
            }
          },
          {
            graph_description::node_1_node_node,
            "Swap nodes {0,2}",
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(0,2);
              return g;
            }
          },
          {
            graph_description::node_1_node_node,
            "Swap nodes {2,0}",
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(2,0);
              return g;
            }
          }
        }, // end 'node_node_node_1'
        {  // begin 'node_1_node_2_node'
          {
            graph_description::empty,
            "Clear graph",
            [](graph_to_test g) -> graph_to_test {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node_1_node,
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
            graph_description::node_1_node,
            "Erase node 2",
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(2);
              return g;
            }
          },
          {
            graph_description::node_1_node_node,
            "Remove link {1,2}",
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(1));
              return g;
            }
          }
        }, // end 'node_1_node_2_node'
        {  // begin 'node_1_node_node_1'
          {
            graph_description::empty,
            "Clear graph",
            [](graph_to_test g) -> graph_to_test {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node_node_0,
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
            graph_description::node_1_node,
            "Erase node 2",
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(2);
              return g;
            }
          },
          {
            graph_description::node_node_node_1,
            "Remove link {0,1}",
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(0));
              return g;
            }
          },
          {
            graph_description::node_1_node_node,
            "Remove link {2,1}",
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(2));
              return g;
            }
          }
        }, // end 'node_1_node_node_1'
        {  // begin 'node_1_node_2_node_0'
          {
            graph_description::empty,
            "Clear graph",
            [](graph_to_test g) -> graph_to_test {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node_1_node,
            "Erase node 0",
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_node_0,
            "Erase node 1",
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_1_node,
            "Erase node 2",
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(2);
              return g;
            }
          },
          {
            graph_description::node_1_node_2_node,
            "Remove {2,0}",
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(2));
              return g;
            }
          }
        }, // end 'node_1_node_2_node_0'
        {  // begin 'node_node_1_node'
          {
            graph_description::empty,
            "Clear graph",
            [](graph_to_test g) -> graph_to_test {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node_0_node,
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
            graph_description::node_node_1,
            "Erase node 2",
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(2);
              return g;
            }
          },
        }, // end 'node_node_1_node'
        {  // begin 'node_1_node_1_node'
          {
            graph_description::node_0_node,
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
            graph_description::node_node_1_node,
            "Remove {0,1}",
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(0));
              return g;
            }
          },
          {
            graph_description::node_1_node_1_0_node,
            "Join {1,0}",
            [](graph_to_test g) -> graph_to_test {
              g.join(1, 0);
              return g;
            }
          }
        }, // end 'node_1_node_1_node'
        {  // begin 'node_1_node_1_0_node'
          {
            graph_description::node_0_node,
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
            graph_description::node_1_node_1_node,
            "Remove {1,0}",
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(++g.cbegin_edges(1));
              return g;
            }
          },
          {
            graph_description::node_1_node_1_0_2_node,
            "Join {1,2}",
            [](graph_to_test g) -> graph_to_test {
              g.join(1, 2);
              return g;
            }
          }
        }, // end 'node_1_node_1_0_node'
        {  // begin 'node_1_node_1_0_2_node'
          {
            graph_description::node_0_1_node,
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
            graph_description::node_1_node_1_2_node,
            "Remove {1,0}",
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(++g.cbegin_edges(1));
              return g;
            }
          },
          {
            graph_description::node_1_node_1_0_node,
            "Remove {1,2}",
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(1)+2);
              return g;
            }
          }
        }, // end 'node_1_node_1_0_2_node'
        {  // begin 'node_1_node_1_2_node'
          {
            graph_description::node_1_node_2_node,
            "Remove {1,1}",
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(1));
              return g;
            }
          }
        }, // end 'node_1_node_1_2_node'
        {  // begin 'node_2_node_node_2'
          {
            graph_description::node_1_node_1_node,
            "Swap {1,2}",
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(1,2);
              return g;
            }
          }
        }, // end 'node_2_node_node_2'
        {  // begin 'node_1_1_2_2_node_2_node'
          {
            graph_description::node_1_node,
            "Erase node 0",
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_1_1_node,
            "Erase node 1",
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_1_1_node,
            "Erase node 2",
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(2);
              return g;
            }
          },
          {
            graph_description::node_1_1_2_2_node_2_node_1,
            "Join {2, 1}",
            [](graph_to_test g) -> graph_to_test {
              g.join(2,1);
              return g;
            }
          }
        }, // end 'node_1_1_2_2_node_2_node'
        {  // begin 'node_1_1_2_2_node_2_node_1'
          {
            graph_description::node_1_node_0,
            "Erase node 0",
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_1_1_node,
            "Erase node 1",
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_1_1_node,
            "Erase node 2",
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(2);
              return g;
            }
          },
          {
            graph_description::node_2_2_1_1_node_2_node_1,
            "Swap {1,2}",
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(1,2);
              return g;
            }
          },
          {
            graph_description::node_2_2_1_1_node_2_node_1,
            "Swap {2,1}",
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(2,1);
              return g;
            }
          },
          {
            graph_description::node_2_2_1_1_node_2_node_1,
            "Swap node 0 edges: {0,2}, {1,3}",
            [](graph_to_test g) -> graph_to_test {
              g.swap_edges(0, 0, 2);
              g.swap_edges(0, 1, 3);
              return g;
            }
          }
        }, // end 'node_1_1_2_2_node_2_node_1'
        {  // begin 'node_2_2_1_1_node_2_node_1'
          {
            graph_description::node_1_1_2_2_node_2_node_1,
            "Swap {1,2}",
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(1,2);
              return g;
            }
          },
          {
            graph_description::node_1_1_2_2_node_2_node_1,
            "Swap {2,1}",
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(2,1);
              return g;
            }
          },
          {
            graph_description::node_1_1_2_2_node_2_node_1,
            "Sort Edges for node 0",
            [](graph_to_test g) -> graph_to_test {
              g.sort_edges(g.cbegin_edges(0), g.cend_edges(0), [](const auto& lhs, const auto& rhs) { return lhs.target_node() < rhs.target_node(); });
              return g;
            }
          },
          {
            graph_description::node_1_1_2_2_node_2_node_1,
            "Swap node 0 edges: {0,2}, {1,3}",
            [](graph_to_test g) -> graph_to_test {
              g.swap_edges(0, 0, 2);
              g.swap_edges(0, 1, 3);
              return g;
            }
          }
        }, // end 'node_2_2_1_1_node_2_node_1'
        {  // begin 'node_3_1_node_2_node_node'
          {
             graph_description::node_1_node_node,
             "Erase node 0",
             [](graph_to_test g) -> graph_to_test {
               g.erase_node(0);
               return g;
             }
          }
        }, // end 'node_3_1_node_2_node_node'
        {  // begin 'node_1_node_node_node_2'
          {
            graph_description::node_2_node_node_node_1,
            "Swap {2,1}",
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(2,1);
              return g;
            }
          },
          {
            graph_description::node_2_node_node_node_1,
            "Swap {0,3}",
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(0,3);
              return g;
            }
          }
        }, // end 'node_1_node_node_node_2'
        {  // begin 'node_2_node_node_node_1'
          {
            graph_description::node_1_node_node_node_2,
            "Swap {2,1}",
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(2,1);
              return g;
            }
          },
          {
            graph_description::node_1_node_node_node_2,
            "Swap {0,3}",
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(0,3);
              return g;
            }
          }
        }, // end 'node_2_node_node_node_1'
      },
      {
        //  'empty'
        [this](){
          graph_to_test g{};
          check(equivalence, report_line(""), g, edges_equivalent_t{});

          return g;
        }(),

        //  'node'
        [this](){
          graph_to_test g{{}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{}});

          return g;
        }(),

        //  'node_0'
        [this](){
          graph_to_test g{{edge_t{0}}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{edge_t{0}}});

          return g;
        }(),

        //  'node_0_0'
        [this](){
          graph_to_test g{{edge_t{0}, edge_t{0}}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{edge_t{0}, edge_t{0}}});

          return g;
        }(),

        //  'node_node'
        [this](){
          graph_to_test g{{}, {}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{}, {}});

          return g;
        }(),

        //  'node_1_node'
        [this](){
          graph_to_test g{{edge_t{1}}, {}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{edge_t{1}}, {}});

          return g;
        }(),

        //  'node_node_0'
        [this](){
          graph_to_test g{{}, {edge_t{0}}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{}, {edge_t{0}}});

          return g;
        }(),

        //  'node_0_1_node'
        [this](){
          graph_to_test g{{edge_t{0}, edge_t{1}}, {}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{edge_t{0}, edge_t{1}}, {}});

          return g;
        }(),

        //  'node_0_node'
        [this](){
          graph_to_test g{{edge_t{0}}, {}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{edge_t{0}}, {}});

          return g;
        }(),

        //  'node_node_1'
        [this](){
          graph_to_test g{{}, {edge_t{1}}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{}, {edge_t{1}}});

          return g;
        }(),

        // 'node_1_1_node'
        [this](){
          graph_to_test g{{edge_t{1}, edge_t{1}}, {}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{edge_t{1}, edge_t{1}}, {}});

          return g;
        }(),

        // 'node_1_node_0'
        [this](){
          graph_to_test g{{edge_t{1}}, {edge_t{0}}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{edge_t{1}}, {edge_t{0}}});

          return g;
        }(),

        //  'node_node_node'
        [this](){
          graph_to_test g{{}, {}, {}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{}, {}, {}});

          return g;
        }(),

        //  'node_1_node_node'
        [this](){
          graph_to_test g{{edge_t{1}}, {}, {}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{edge_t{1}}, {}, {}});

          return g;
        }(),

        //  'node_node_node_1'
        [this](){
          graph_to_test g{{}, {}, {edge_t{1}}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{}, {}, {edge_t{1}}});

          return g;
        }(),

        // 'node_1_node_2_node'
        [this](){
          graph_to_test g{{edge_t{1}}, {edge_t{2}}, {}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{edge_t{1}}, {edge_t{2}}, {}});

          return g;
        }(),

        // 'node_1_node_node_1'
        [this](){
          graph_to_test g{{edge_t{1}}, {}, {edge_t{1}}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{edge_t{1}}, {}, {edge_t{1}}});

          return g;
        }(),

        // 'node_1_node_2_node_0'
        [this](){
          graph_to_test g{{edge_t{1}}, {edge_t{2}}, {edge_t{0}}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{edge_t{1}}, {edge_t{2}}, {edge_t{0}}});

          return g;
        }(),

        // 'node_node_1_node'
        [this](){
          graph_to_test g{{}, {edge_t{1}}, {}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{}, {edge_t{1}}, {}});

          return g;
        }(),

        // 'node_1_node_1_node'
        [this](){
          graph_to_test g{{edge_t{1}}, {edge_t{1}}, {}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{edge_t{1}}, {edge_t{1}}, {}});

          return g;
        }(),

        // 'node_1_node_1_0_node'
        [this](){
          graph_to_test g{{edge_t{1}}, {edge_t{1}, edge_t{0}}, {}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{edge_t{1}}, {edge_t{1}, edge_t{0}}, {}});

          return g;
        }(),

        // 'node_1_node_1_0_2_node'
        [this](){
          graph_to_test g{{edge_t{1}}, {edge_t{1}, edge_t{0}, edge_t{2}}, {}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{edge_t{1}}, {edge_t{1}, edge_t{0}, edge_t{2}}, {}});

          return g;
        }(),

        // 'node_1_node_1_2_node'
        [this](){
          graph_to_test g{{edge_t{1}}, {edge_t{1}, edge_t{2}}, {}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{edge_t{1}}, {edge_t{1}, edge_t{2}}, {}});

          return g;
        }(),

        // 'node_2_node_node_2'
        [this](){
          graph_to_test g{{edge_t{2}}, {}, {edge_t{2}}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{edge_t{2}}, {}, {edge_t{2}}});

          return g;
        }(),

        // 'node_1_1_2_2_node_2_node'
        [this](){
          graph_to_test g{{edge_t{1}, edge_t{1}, edge_t{2}, edge_t{2}}, {edge_t{2}}, {}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{edge_t{1}, edge_t{1}, edge_t{2}, edge_t{2}}, {edge_t{2}}, {}});

          return g;
        }(),

        // 'node_1_1_2_2_node_2_node_1'
        [this](){
          graph_to_test g{{edge_t{1}, edge_t{1}, edge_t{2}, edge_t{2}}, {edge_t{2}}, {edge_t{1}}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{edge_t{1}, edge_t{1}, edge_t{2}, edge_t{2}}, {edge_t{2}}, {edge_t{1}}});

          return g;
        }(),

        // 'node_2_2_1_1_node_2_node_1'
        [this](){
          graph_to_test g{{edge_t{2}, edge_t{2}, edge_t{1}, edge_t{1}}, {edge_t{2}}, {edge_t{1}}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{edge_t{2}, edge_t{2}, edge_t{1}, edge_t{1}}, {edge_t{2}}, {edge_t{1}}});

          return g;
        }(),

        // 'node_3_1_node_2_node_node'
        [this](){
          graph_to_test g{{edge_t{3}, edge_t{1}}, {edge_t{2}}, {}, {}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{edge_t{3}, edge_t{1}}, {edge_t{2}}, {}, {}});

          return g;
        }(),

        // 'node_1_node_node_node_2'
        [this](){
          graph_to_test g{{edge_t{1}}, {}, {}, {edge_t{2}}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{edge_t{1}}, {}, {}, {edge_t{2}}});

          return g;
        }(),

        // 'node_2_node_node_node_1'
        [this](){
          graph_to_test g{{edge_t{2}}, {}, {}, {edge_t{1}}};
          check(equivalence, report_line(""), g, edges_equivalent_t{{edge_t{2}}, {}, {}, {edge_t{1}}});

          return g;
        }()
      }
    };

    auto checker{
        [this](std::string_view description, const graph_to_test& obtained, const graph_to_test& prediction, const graph_to_test& parent, std::size_t host, std::size_t target) {
          check(equality, description, obtained, prediction);
          if(host != target) check_semantics(description, prediction, parent);
        }
    };

    transition_checker<graph_to_test>::check(report_line(""), trg, checker);
  }
}