////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "DynamicUndirectedGraphUnweightedTest.hpp"

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

      //  x --- x
      node_1_node_0,

      //   /\
      //   \/
      //   x --- x
      node_0_1_node_0,

      //  /\
      //  \/
      //  x      x
      node_0_node,

      //      /\
      //      \/
      //  x    x
      node_node_1,

      //  x === x
      node_1_1_node_0_0,

      //  x    x    x
      node_node_node,

      //   x --- x    x
      node_1_node_0_node,

      //  x    x --- x
      node_node_2_node_1,

      //  x --- x --- x
      node_1_node_0_2_node_1,


      // - x --- x --- x --
      node_1_2_node_0_2_node_0_1,

      //      /\
      //      \/
      //  x    x    x
      node_node_1_node,

      //        /\
      //        \/
      //  x --- x    x
      node_1_node_0_1_node,

      //       /\
      //       \/
      //  x --- x --- x
      node_1_node_0_1_2_node_1,

      //             /\
      //             \/
      // -- x    x    x -
      node_2_node_node_0_2,

      //     [2]
      //      x
      //    //  \
      //   //    \
      //  //      \
      //  x  ===   x
      // [0]      [1]
      node_1_1_2_2_node_0_0_2_node_0_0_1,

      //  x [3]
      //  |
      //  |
      //  |
      //  x --- x --- x
      // [0]   [1]   [2]
      node_3_1_node_0_2_node_1_node_0,

      // x --- x    x --- x
      node_1_node_0_node_3_node_2,

      // x --   x -   - x  -- x
      node_2_node_3_node_0_node_1
    };
  }

  [[nodiscard]]
  std::filesystem::path dynamic_undirected_graph_unweighted_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void dynamic_undirected_graph_unweighted_test::run_tests()
  {
    using namespace maths;
    graph_test_helper<null_weight, null_weight, dynamic_undirected_graph_unweighted_test> helper{*this};

    helper.run_tests<graph_flavour::undirected>();
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
  void dynamic_undirected_graph_unweighted_test::execute_operations()
  {
    using graph_to_test      = graph_type_generator_t<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightCreator, NodeWeightCreator, EdgeStorageTraits, NodeWeightStorageTraits>;
    using edge_t             = typename graph_to_test::edge_init_type;
    using edges_equivalent_t = std::initializer_list<std::initializer_list<edge_t>>;
    using transition_graph   = transition_checker<graph_to_test>::transition_graph;

    auto make_and_check{
      [this](std::string_view description, edges_equivalent_t init){
        return graph_initialization_checker<graph_to_test>::make_and_check(*this, description, init);
      }
    };

    transition_graph trg{
      {
        { // begin, 'empty'
          {
            graph_description::empty,
            report_line(""),
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("cbegin_edges throws for empty graph"), [&g]() { return g.cbegin_edges(0); });
              return g;
            }
          },
          {
            graph_description::empty,
            report_line(""),
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("cend_edges throws for empty graph"), [&g]() { return g.cend_edges(0); });
              return g;
            }
          },
          {
            graph_description::empty,
            report_line(""),
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("crbegin_edges throws for empty graph"), [&g]() { return g.crbegin_edges(0); });
              return g;
            }
          },
          {
            graph_description::empty,
            report_line(""),
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("crend_edges throws for empty graph"), [&g]() { return g.crend_edges(0); });
              return g;
            }
          },
          {
            graph_description::empty,
            report_line(""),
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("cedges throws for empty graph"), [&g]() { return g.cedges(0); });
              return g;
            }
          },
          {
            graph_description::empty,
            report_line(""),
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("swapping nodes throws for empty graph"), [g{g}]() mutable { g.swap_nodes(0, 0); });
              return g;
            }
          },
          {
            graph_description::empty,
            report_line(""),
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("swapping edges throws for empty graph"), [g{g}]() mutable { g.swap_edges(0, 0, 0); });
              return g;
            }
          },
          {
            graph_description::empty,
            report_line("Clear empty graph"),
            [](graph_to_test g) -> graph_to_test {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node,
            report_line("Add node to empty graph"),
            [this](graph_to_test g) -> graph_to_test {
              check(equality, report_line("Index of added node is 0"), g.add_node(), 0_sz);
              return g;
            }
          },
          {
            graph_description::node,
            report_line("insert node into empty graph"),
            [this](graph_to_test g) -> graph_to_test {
              check(equality, report_line("Index of added node is 0"), g.insert_node(0), 0_sz);
              return g;
            }
          }
        }, // end 'empty'
        {  // begin 'node'
          {
            graph_description::node,
            report_line(""),
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("cbegin_edges throws when index is out of range"), [&g]() { return g.cbegin_edges(1); });
              return g;
            }
          },
          {
            graph_description::node,
            report_line(""),
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("cend_edges throws when index is out of range"), [&g]() { return g.cend_edges(1); });
              return g;
            }
          },
          {
            graph_description::node,
            report_line(""),
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("crbegin_edges throws when index is out of range"), [&g]() { return g.crbegin_edges(1); });
              return g;
            }
          },
          {
            graph_description::node,
            report_line(""),
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("crend_edges throws when index is out of range"), [&g]() { return g.crend_edges(1); });
              return g;
            }
          },
          {
            graph_description::node,
            report_line(""),
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("cedges throws when index is out of range"), [&g]() { return g.cedges(1); });
              return g;
            }
          },
          {
            graph_description::node,
            report_line(""),
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("swapping nodes throws if first index out of range"), [g{g}]() mutable { g.swap_nodes(1, 0); });
              return g;
            }
          },
          {
            graph_description::node,
            report_line(""),
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("swapping nodes throws if second index out of range"), [g{g}]() mutable { g.swap_nodes(0, 1); });
              return g;
            }
          },
          {
            graph_description::empty,
            report_line("Clear graph"),
            [](graph_to_test g) -> graph_to_test {
              g.clear();
              return g;
            }
          },
          {
            graph_description::empty,
            report_line("Erase node to give empty graph"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node,
            report_line("Attempt to erase edge past the end"),
            [](graph_to_test g) -> const graph_to_test {
              g.erase_edge(g.cend_edges(0));
              return g;
            }
          },
          {
            graph_description::node_0,
            report_line("Add loop"),
            [](graph_to_test g) -> graph_to_test {
              g.join(0, 0);
              return g;
            }
          },
          {
            graph_description::node_node,
            report_line("Add second node"),
            [this](graph_to_test g) -> graph_to_test {
              check(equality, report_line("Index of added node is 1"), g.add_node(), 1_sz);
              return g;
            }
          },
          {
            graph_description::node_node,
            report_line("Insert second node"),
            [this](graph_to_test g) -> graph_to_test {
              check(equality, report_line("Index of added node is 0"), g.insert_node(0), 0_sz);
              return g;
            }
          },
          {
            graph_description::node_node,
            report_line("Insert second node at end"),
            [this](graph_to_test g) -> graph_to_test {
              check(equality, report_line("Index of added node is 1"), g.insert_node(1), 1_sz);
              return g;
            }
          },
          {
            graph_description::node,
            report_line("Swap node with self"),
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(0,0);
              return g;
            }
          },
          {
            graph_description::node,
            report_line(""),
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("swapping nodes throws if first index is out of range"), [g{g}]() mutable { g.swap_nodes(1, 0); });
              return g;
            }
          },
          {
            graph_description::node,
            report_line(""),
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("swapping nodes throws if second index is out of range"), [g{g}]() mutable { g.swap_nodes(0, 1); });
              return g;
            }
          },
        }, // end 'node'
        {  // begin 'node_0'
          {
            graph_description::empty,
            report_line("Clear graph"),
            [](graph_to_test g) -> graph_to_test {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node,
            report_line("Remove loop"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(0));
              return g;
            }
          },
          {
            graph_description::node,
            report_line("Remove loop via second partial edge"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(std::ranges::next(g.cbegin_edges(0)));
              return g;
            }
          },
          {
            graph_description::node_0_0,
            report_line("Add a second loop"),
            [](graph_to_test g) -> graph_to_test {
              g.join(0, 0);
              return g;
            }
          },
          {
            graph_description::node_node_1,
            report_line("Insert node"),
            [this](graph_to_test g) -> graph_to_test {
              check(equality, report_line("Index of added node is 0"), g.insert_node(0), 0_sz);
              return g;
            }
          },
          {
            graph_description::node_0_node,
            report_line("Insert node at end"),
            [this](graph_to_test g) -> graph_to_test {
              check(equality, report_line("Index of added node is 1"), g.insert_node(1), 1_sz);
              return g;
            }
          },
          {
            graph_description::node_0,
            report_line("Swap node with self"),
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(0,0);
              return g;
            }
          },
          {
            graph_description::node_0,
            report_line("Swap edge with self"),
            [](graph_to_test g) -> graph_to_test {
              g.swap_edges(0, 0, 0);
              return g;
            }
          },
          {
            graph_description::node_0,
            report_line("Swap first partial edge with partner"),
            [](graph_to_test g) -> graph_to_test {
              g.swap_edges(0, 0, 1);
              return g;
            }
          },
          {
            graph_description::node_0,
            report_line("Swap second partial edge with partner"),
            [](graph_to_test g) -> graph_to_test {
              g.swap_edges(0, 1, 0);
              return g;
            }
          },
          {
            graph_description::node_0,
            report_line(""),
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("swapping edges throws for first edge index out of range"), [g{g}]() mutable { g.swap_edges(0, 2, 0); });
              return g;
            }
          },
          {
            graph_description::node_0,
            report_line(""),
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("swapping edges throws for second edge index out of range"), [g{g}]() mutable { g.swap_edges(0, 0, 2); });
              return g;
            }
          },
          {
            graph_description::node_0,
            report_line(""),
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("swapping edges throws for node index out of range"), [g{g}]() mutable { g.swap_edges(1, 0, 0); });
              return g;
            }
          }
        }, // end 'node_0'
        {  // begin 'node_0_0'
          {
            graph_description::empty,
            report_line("Clear graph"),
            [](graph_to_test g) -> graph_to_test {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node_0,
            report_line("Remove first loop, via first partial edge"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(0));
              return g;
            }
          },
          {
            graph_description::node_0,
            report_line("Remove first loop, via second partial edge"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(0)+1);
              return g;
            }
          },
          {
            graph_description::node_0,
            report_line("Remove second loop, via first partial edge"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(0)+2);
              return g;
            }
          },
          {
            graph_description::node_0,
            report_line("Remove second loop, via second partial edge"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(0) + 3);
              return g;
            }
          },
          {
            graph_description::node_0_0,
            report_line("Swap partial edges {0,1}"),
            [](graph_to_test g) -> graph_to_test {
              g.swap_edges(0, 0, 1);
              return g;
            }
          },
          {
            graph_description::node_0_0,
            report_line("Swap partial edges {0,2}"),
            [](graph_to_test g) -> graph_to_test {
              g.swap_edges(0, 0, 2);
              return g;
            }
          },
          {
            graph_description::node_0_0,
            report_line("Swap partial edges {0,3}"),
            [](graph_to_test g) -> graph_to_test {
              g.swap_edges(0, 0, 3);
              return g;
            }
          },
          {
            graph_description::node_0_0,
            report_line("Swap partial edges {1,2}"),
            [](graph_to_test g) -> graph_to_test {
              g.swap_edges(0, 1, 2);
              return g;
            }
          },
          {
            graph_description::node_0_0,
            report_line("Swap partial edges {2,1}"),
            [](graph_to_test g) -> graph_to_test {
              g.swap_edges(0, 2, 1);
              return g;
            }
          }
        }, // end 'node_0_0'
        {  // begin 'node_node'
          {
            graph_description::empty,
            report_line("Clear graph"),
            [](graph_to_test g) -> graph_to_test {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node,
            report_line("Erase node 0"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node,
            report_line("Erase node 1"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            report_line("Join {0,1}"),
            [](graph_to_test g) -> graph_to_test {
              g.join(0, 1);
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            report_line("Join {1,0}"),
            [](graph_to_test g) -> graph_to_test {
              g.join(0, 1);
              return g;
            }
          }
        }, // end 'node_node'
        {  // begin 'node_1_node_0'
          {
            graph_description::empty,
            report_line("Clear graph"),
            [](graph_to_test g) -> graph_to_test {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node,
            report_line("Erase node 0"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node,
            report_line("Erase node 1"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_0_1_node_0,
            report_line("Add loop to node 0 and then swap it with link"),
            [](graph_to_test g) -> graph_to_test {
              g.join(0, 0);
              g.swap_edges(0, 0, 2);
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            report_line("Swap nodes {0,1}"),
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(0,1);
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            report_line("Swap nodes {1,0}"),
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(1,0);
              return g;
            }
          },
          {
            graph_description::node_1_1_node_0_0,
            report_line("Join {0,1}"),
            [](graph_to_test g) {
              g.join(0, 1);
              return g;
            }
          }
        }, // end 'node_1_node_0'
        {  // begin 'node_0_1_node_0'
          {
            graph_description::empty,
            report_line("Clear graph"),
            [](graph_to_test g) -> graph_to_test {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node,
            report_line("Erase node 0"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_0,
            report_line("Erase node 1"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            report_line("Remove loop via first partial edge"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(0));
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            report_line("Remove loop via second partial edge"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(++g.cbegin_edges(0));
              return g;
            }
          },
          {
            graph_description::node_0_node,
            report_line("Remove edge {0,1}"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(0) + 2);
              return g;
            }
          },
          {
            graph_description::node_0_node,
            report_line("Remove edge {1,0}"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(1));
              return g;
            }
          }
        }, // end 'node_0_1_node_0'
        {  // begin 'node_0_node'
          {
            graph_description::empty,
            report_line("Clear graph"),
            [](graph_to_test g) -> graph_to_test {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node_node_1_node,
            report_line("Insert node"),
            [this](graph_to_test g) -> graph_to_test {
              check(equality, report_line("Index of added node is 0"), g.insert_node(0), 0_sz);
              return g;
            }
          },
          {
            graph_description::node_node_1,
            report_line("Swap nodes"),
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(0,1);
              return g;
            }
          }
          ,
          {
            graph_description::node_node_1,
            report_line("Swap nodes"),
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(1,0);
              return g;
            }
          }
        }, // end 'node_0_node'
        {  // begin 'node_node_1'
          {
            graph_description::empty,
            report_line("Clear graph"),
            [](graph_to_test g) -> graph_to_test {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node_0,
            report_line("Erase node 0"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node,
            report_line("Erase node 1"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_0_node,
            report_line("Swap nodes"),
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(0,1);
              return g;
            }
          }
        }, // end 'node_node_1'
        {  // begin 'node_1_1_node_0_0'
          {
            graph_description::empty,
            report_line("Clear graph"),
            [](graph_to_test g) -> graph_to_test {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node,
            report_line("Erase node 0"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node,
            report_line("Erase node 1"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            report_line("Erase node 0 zeroth link"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(0));
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            report_line("Erase node 0 first link"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(0)+1);
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            report_line("Erase node 1 zeroth link"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(1));
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            report_line("Erase node 1 first link"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(1) + 1);
              return g;
            }
          },
          {
            graph_description::node_1_1_node_0_0,
            report_line("Swap edges"),
            [](graph_to_test g) -> graph_to_test {
              g.swap_edges(0, 0, 1);
              return g;
            }
          },
          {
            graph_description::node_1_1_node_0_0,
            report_line("Swap node 0 edges"),
            [](graph_to_test g) -> graph_to_test {
              g.swap_edges(0, 1, 0);
              return g;
            }
          },
          {
            graph_description::node_1_1_node_0_0,
            report_line("Swap node 1 edges"),
            [](graph_to_test g) -> graph_to_test {
              g.swap_edges(1, 0, 1);
              return g;
            }
          }
        }, // end 'node_1_1_node_0_0'
        {  // begin 'node_node_node'
          {
            graph_description::empty,
            report_line("Clear graph"),
            [](graph_to_test g) -> graph_to_test {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node_node,
            report_line("Erase node 0"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_node,
            report_line("Erase node 1"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_node,
            report_line("Erase node 2"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(2);
              return g;
            }
          },
          {
            graph_description::node_1_node_0_node,
            report_line("Join {0,1}"),
            [](graph_to_test g) -> graph_to_test {
              g.join(0,1);
              return g;
            }
          },
          {
            graph_description::node_node_2_node_1,
            report_line("Join {2,1}"),
            [](graph_to_test g) -> graph_to_test {
              g.join(2,1);
              return g;
            }
          }
        }, // end 'node_node_node'
        {  // begin 'node_1_node_0_node'
          {
            graph_description::empty,
            report_line("Clear graph"),
            [](graph_to_test g) -> graph_to_test {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node_node,
            report_line("Erase node 0"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_node,
            report_line("Erase node 1"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            report_line("Erase node 2"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(2);
              return g;
            }
          },
          {
            graph_description::node_node_node,
            report_line("Remove link {0,1}"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(0));
              return g;
            }
          },
          {
            graph_description::node_node_node,
            report_line("Remove link {1,0}"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(1));
              return g;
            }
          },
          {
            graph_description::node_1_node_0_2_node_1,
            report_line("Join {1,2}"),
            [](graph_to_test g) -> graph_to_test {
              g.join(1,2);
              return g;
            }
          },
          {
            graph_description::node_1_node_0_2_node_1,
            report_line("Join {2,1}"),
            [](graph_to_test g) -> graph_to_test {
              g.join(2,1);
              return g;
            }
          },
          {
            graph_description::node_node_2_node_1,
            report_line("Swap nodes {0,2}"),
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(0,2);
              return g;
            }
          },
          {
            graph_description::node_node_2_node_1,
            report_line("Swap nodes {2,0}"),
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(2,0);
              return g;
            }
          }
        }, // end 'node_1_node_0_node'
        {  // begin 'node_node_2_node_1'
          {
            graph_description::empty,
            report_line("Clear graph"),
            [](graph_to_test g) -> graph_to_test {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            report_line("Erase node 0"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_node,
            report_line("Erase node 1"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_node,
            report_line("Erase node 2"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(2);
              return g;
            }
          },
          {
            graph_description::node_node_node,
            report_line("Remove link {1,2}"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(1));
              return g;
            }
          },
          {
            graph_description::node_node_node,
            report_line("Remove link {2,1}"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(2));
              return g;
            }
          },
          {
            graph_description::node_1_node_0_2_node_1,
            report_line("Join {0,1}, the swap node 1's edge"),
            [](graph_to_test g) -> graph_to_test {
              g.join(0,1);
              g.swap_edges(1, 0, 1);
              return g;
            }
          },
          {
            graph_description::node_1_node_0_node,
            report_line("Swap nodes {0,2}"),
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(0,2);
              return g;
            }
          },
          {
            graph_description::node_1_node_0_node,
            report_line("Swap nodes {2,0}"),
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(2,0);
              return g;
            }
          }
        }, // end 'node_node_2_node_1'
        {  // begin 'node_1_node_0_2_node_1'
          {
            graph_description::empty,
            report_line("Clear graph"),
            [](graph_to_test g) -> graph_to_test {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            report_line("Erase node 0"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_node,
            report_line("Erase node 1"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            report_line("Erase node 2"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(2);
              return g;
            }
          },
          {
            graph_description::node_1_node_0_node,
            report_line("Remove link {1,2}"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(1)+1);
              return g;
            }
          },
          {
            graph_description::node_1_node_0_2_node_1,
            report_line("Swap nodes {0,2} then swap node 1's edges"),
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(0, 2);
              g.swap_edges(1, 0, 1);
              return g;
            }
          },
          {
            graph_description::node_1_node_0_2_node_1,
            report_line("Swap nodes {2,0} then swap node 1's edges"),
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(2, 0);
              g.swap_edges(1, 1, 0);
              return g;
            }
          }
        }, // end 'node_1_node_0_2_node_1'
        {  // begin 'node_1_2_node_2_0_node_0_1'
          {
            graph_description::empty,
            report_line("Clear graph"),
            [](graph_to_test g) -> graph_to_test {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            report_line("Erase node 0"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            report_line("Erase node 1"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            report_line("Erase node 2"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(2);
              return g;
            }
          },
          {
            graph_description::node_1_node_0_2_node_1,
            report_line("Remove {2,0}"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(2));
              return g;
            }
          }
        }, // end 'node_1_2_node_0_2_node_0_1'
        {  // begin 'node_node_1_node'
          {
            graph_description::empty,
            report_line("Clear graph"),
            [](graph_to_test g) -> graph_to_test {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node_0_node,
            report_line("Erase node 0"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_node,
            report_line("Erase node 1"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_node_1,
            report_line("Erase node 2"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(2);
              return g;
            }
          },
        }, // end 'node_node_1_node'
        {  // begin 'node_1_node_0_1_node'
          {
            graph_description::node_0_node,
            report_line("Erase node 0"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_node,
            report_line("Erase node 1"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_node_1_node,
            report_line("Remove {0,1}"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(0));
              return g;
            }
          },
          {
            graph_description::node_1_node_0_1_2_node_1,
            report_line("Join {1,2}"),
            [](graph_to_test g) -> graph_to_test {
              g.join(1, 2);
              return g;
            }
          }
        }, // end 'node_1_node_0_1_node'
        {  // begin 'node_1_node_0_1_2_node_1'
          {
            graph_description::node_0_1_node_0,
            report_line("Erase node 0"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_node,
            report_line("Erase node 1"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_1_node_0_1_node,
            report_line("Remove {1,2}"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(1)+3);
              return g;
            }
          }
        }, // end 'node_1_node_0_1_2_node_1'
        {  // begin 'node_2_node_node_0_2'
          {
            graph_description::node_1_node_0_1_node,
            report_line("Swap {1,2}"),
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(1,2);
              return g;
            }
          }
        }, // end 'node_2_node_node_0_2'
        {  // begin 'node_1_1_2_2_node_0_0_2_node_0_0_1'
          {
            graph_description::node_1_node_0,
            report_line("Erase node 0"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_1_1_node_0_0,
            report_line("Erase node 1"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_1_1_node_0_0,
            report_line("Erase node 2"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(2);
              return g;
            }
          },
          {
            graph_description::node_1_1_2_2_node_0_0_2_node_0_0_1,
            report_line("Swap nodes {2, 1}"),
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(2,1);
              g.sort_edges(g.cbegin_edges(0), g.cend_edges(0), [](const auto& lhs, const auto& rhs) { return lhs.target_node() < rhs.target_node(); });
              return g;
            }
          }
        }, // end 'node_1_1_2_2_node_0_0_2_node_0_0_1'
        {  // begin 'node_3_1_node_0_2_node_1_node_0'
          {
             graph_description::node_1_node_0_node,
             report_line("Erase node 0"),
             [](graph_to_test g) -> graph_to_test {
               g.erase_node(0);
               return g;
             }
          }
        }, // end 'node_3_1_node_0_2_node_1_node_0'
        {  // begin 'node_1_node_0_node_3_node_2'
          {
            graph_description::node_1_node_0_node,
            report_line("Erase node 2"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(2);
              return g;
            }
          },
          {
            graph_description::node_2_node_3_node_0_node_1,
            report_line("Swap {2,1}"),
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(2,1);
              return g;
            }
          },
          {
            graph_description::node_2_node_3_node_0_node_1,
            report_line("Swap {0,3}"),
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(0,3);
              return g;
            }
          }
        }, // end 'node_1_node_0_node_3_node_2'
        {  // begin 'node_2_node_3_node_0_node_1'
          {
            graph_description::node_1_node_0_node_3_node_2,
            report_line("Swap {2,1}"),
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(2,1);
              return g;
            }
          },
          {
            graph_description::node_1_node_0_node_3_node_2,
            report_line("Swap {0,3}"),
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(0,3);
              return g;
            }
          }
        }, // end 'node_2_node_3_node_0_node_1'
      },
      {
        //  'empty'
        [this](){
          graph_to_test g{};
          check(equivalence, report_line(""), g, edges_equivalent_t{});

          return g;
        }(),

        //  'node'
        make_and_check(report_line(""), {{}}),

        //  'node_0'
        make_and_check(report_line(""), {{edge_t{0}, edge_t{0}}}),

        //  'node_0_0'
        make_and_check(report_line(""), {{edge_t{0}, edge_t{0}, edge_t{0}, edge_t{0}}}),

        //  'node_node'
        make_and_check(report_line(""), {{}, {}}),

        //  'node_1_node_0'
        make_and_check(report_line(""), {{edge_t{1}}, {edge_t{0}}}),

        //  'node_0_1_node_0'
        make_and_check(report_line(""), {{edge_t{0}, edge_t{0}, edge_t{1}}, {edge_t{0}}}),

        //  'node_0_node'
        make_and_check(report_line(""), {{edge_t{0}, edge_t{0}}, {}}),

        //  'node_node_1'
        make_and_check(report_line(""), {{}, {edge_t{1}, edge_t{1}}}),

        // 'node_1_1_node_0_0'
        make_and_check(report_line(""), {{edge_t{1}, edge_t{1}}, {edge_t{0}, edge_t{0}}}),

        //  'node_node_node'
        make_and_check(report_line(""), {{}, {}, {}}),

        //  'node_1_node_0_node'
        make_and_check(report_line(""), {{edge_t{1}}, {edge_t{0}}, {}}),

        //  'node_node_2_node_1'
        make_and_check(report_line(""), {{}, {edge_t{2}}, {edge_t{1}}}),

        // 'node_1_node_0_2_node_1'
        make_and_check(report_line(""), {{edge_t{1}}, {edge_t{0}, edge_t{2}}, {edge_t{1}}}),

        // 'node_1_2_node_0_2_node_0_1'
        [this,make_and_check](){
          auto g{make_and_check(report_line(""), {{edge_t{1}, edge_t{2}}, {edge_t{0}, edge_t{2}}, {edge_t{0}, edge_t{1}}})};
          check(equality, report_line("Check sorting of edges on construction"), graph_to_test{{edge_t{2}, edge_t{1}}, {edge_t{2}, edge_t{0}}, {edge_t{1}, edge_t{0}}}, g);

          return g;
        }(),

        // 'node_node_1_node'
        make_and_check(report_line(""), {{}, {edge_t{1}, edge_t{1}}, {}}),

        // 'node_1_node_0_1_node'
        make_and_check(report_line(""), {{edge_t{1}}, {edge_t{0}, edge_t{1}, edge_t{1}}, {}}),

        // 'node_1_node_0_1_2_node_1'
        make_and_check(report_line(""), {{edge_t{1}}, {edge_t{0}, edge_t{1}, edge_t{1}, edge_t{2}}, {edge_t{1}}}),

        // 'node_2_node_node_0_2'
        make_and_check(report_line(""), {{edge_t{2}}, {}, {edge_t{0}, edge_t{2}, edge_t{2}}}),

        // 'node_1_1_2_2_node_0_0_2_node_0_0_1'
        [this, make_and_check](){
          auto g{make_and_check(report_line(""), {{edge_t{1}, edge_t{1}, edge_t{2}, edge_t{2}}, {edge_t{0}, edge_t{0}, edge_t{2}}, {edge_t{0}, edge_t{0}, edge_t{1}}})};
          check(equality,
                report_line("Check sorting of edges on construction"),
                graph_to_test{{edge_t{2}, edge_t{1}, edge_t{2}, edge_t{1}}, {edge_t{0}, edge_t{2}, edge_t{0}}, {edge_t{1}, edge_t{0}, edge_t{0}}},
                g);

          return g;
        }(),

        // 'node_3_1_node_0_2_node_1_node_0'
        make_and_check(report_line(""), {{edge_t{1}, edge_t{3}}, {edge_t{0}, edge_t{2}}, {edge_t{1}}, {edge_t{0}}}),

        // 'node_1_node_0_node_3_node_2'
        make_and_check(report_line(""), {{edge_t{1}}, {edge_t{0}}, {edge_t{3}}, {edge_t{2}}}),

        // 'node_2_node_3_node_0_node_1'
        make_and_check(report_line(""), {{edge_t{2}}, {edge_t{3}}, {edge_t{0}}, {edge_t{1}}}),
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