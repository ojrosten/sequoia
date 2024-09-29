////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "Maths/Graph/Dynamic/DynamicGraphTestingUtilities.hpp"

#include "sequoia/TestFramework/RegularTestCore.hpp"
#include "sequoia/TestFramework/StateTransitionUtilities.hpp"

namespace sequoia::testing
{
  namespace undirected_graph{
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

      //         /\
      //         \/
      //   x --- x
      node_1_node_0_1,

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
      node_2_node_3_node_0_node_1,

      end
    };
  }

  template
  <
    class EdgeWeight,
    class NodeWeight,
    class EdgeMetaData,
    class EdgeStorageConfig,
    class NodeWeightStorage
  >
  class dynamic_undirected_graph_operations
  {
  public:
    using graph_t            = maths::undirected_graph<EdgeWeight, NodeWeight, EdgeMetaData, EdgeStorageConfig, NodeWeightStorage>;
    using edge_t             = typename graph_t::edge_init_type;
    using edges_equivalent_t = std::initializer_list<std::initializer_list<edge_t>>;
    using transition_graph   = typename transition_checker<graph_t>::transition_graph;

    static void execute_operations(regular_test& t)
    {
      auto trg{make_transition_graph(t)};

      auto checker{
          [&t](std::string_view description, const graph_t& obtained, const graph_t& prediction, const graph_t& parent, std::size_t host, std::size_t target) {
            t.check(equality, {description, no_source_location}, obtained, prediction);
            if(host != target) t.check_semantics({description, no_source_location}, prediction, parent);
          }
      };

      transition_checker<graph_t>::check(t.report_line(""), trg, checker);
    }

    [[nodiscard]]
    static graph_t make_and_check(regular_test& t, std::string_view description, edges_equivalent_t init)
    {
      return graph_initialization_checker<graph_t>::make_and_check(t, description, init);
    }

    static void check_initialization_exceptions(regular_test& t)
    {
      using namespace maths;

      // One node
      t.check_exception_thrown<std::out_of_range>("Target index of edge out of range", [](){ return graph_t{{edge_t{1}}}; });
      t.check_exception_thrown<std::logic_error>("Mismatched loop", [](){ return graph_t{{edge_t{0}}}; });

      // Two nodes
      t.check_exception_thrown<std::logic_error>("Mismatched partial edges", [](){ return graph_t{{edge_t{1}}, {edge_t{1}}}; });
      t.check_exception_thrown<std::logic_error>("Mismatched loop", [](){ return graph_t{{edge_t{1}}, {edge_t{0}, edge_t{1}}}; });
    }

    [[nodiscard]]
    static transition_graph make_transition_graph(regular_test& t)
    {
      using namespace undirected_graph;

      check_initialization_exceptions(t);

      return transition_graph{
      {
        { // begin, 'empty'
          {
            graph_description::empty,
            t.report_line(""),
            [&t](const graph_t& g) -> const graph_t& {
              t.check_exception_thrown<std::out_of_range>("cbegin_edges throws for empty graph", [&g]() { return g.cbegin_edges(0); });
              return g;
            }
          },
          {
            graph_description::empty,
            t.report_line(""),
            [&t](const graph_t& g) -> const graph_t& {
              t.check_exception_thrown<std::out_of_range>("cend_edges throws for empty graph", [&g]() { return g.cend_edges(0); });
              return g;
            }
          },
          {
            graph_description::empty,
            t.report_line(""),
            [&t](const graph_t& g) -> const graph_t& {
              t.check_exception_thrown<std::out_of_range>("crbegin_edges throws for empty graph", [&g]() { return g.crbegin_edges(0); });
              return g;
            }
          },
          {
            graph_description::empty,
            t.report_line(""),
            [&t](const graph_t& g) -> const graph_t& {
              t.check_exception_thrown<std::out_of_range>("crend_edges throws for empty graph", [&g]() { return g.crend_edges(0); });
              return g;
            }
          },
          {
            graph_description::empty,
            t.report_line(""),
            [&t](const graph_t& g) -> const graph_t& {
              t.check_exception_thrown<std::out_of_range>("cedges throws for empty graph", [&g]() { return g.cedges(0); });
              return g;
            }
          },
          {
            graph_description::empty,
            t.report_line(""),
            [&t](const graph_t& g) -> const graph_t& {
              t.check_exception_thrown<std::out_of_range>("swapping nodes throws for empty graph", [g{g}]() mutable { g.swap_nodes(0, 0); });
              return g;
            }
          },
          {
            graph_description::empty,
            t.report_line(""),
            [&t](const graph_t& g) -> const graph_t& {
              t.check_exception_thrown<std::out_of_range>("swapping edges throws for empty graph", [g{g}]() mutable { g.swap_edges(0, 0, 0); });
              return g;
            }
          },
          {
            graph_description::empty,
            t.report_line(""),
            [&t](const graph_t& g) -> const graph_t& {
              t.check_exception_thrown<std::out_of_range>("joining nodes throws for empty graph", [g{g}]() mutable { g.join(0, 0); });
              return g;
            }
          },
          {
            graph_description::empty,
            t.report_line("Clear empty graph"),
            [](graph_t g) -> graph_t {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node,
            t.report_line("Add node to empty graph"),
            [&t](graph_t g) -> graph_t {
              t.check(equality, t.report_line("Index of added node is 0"), g.add_node(), 0_sz);
              return g;
            }
          },
          {
            graph_description::node,
            t.report_line("insert node into empty graph"),
            [&t](graph_t g) -> graph_t {
              t.check(equality, t.report_line("Index of added node is 0"), g.insert_node(0), 0_sz);
              return g;
            }
          }
        }, // end 'empty'
        {  // begin 'node'
          {
            graph_description::node,
            t.report_line(""),
            [&t](const graph_t& g) -> const graph_t& {
              t.check_exception_thrown<std::out_of_range>("cbegin_edges throws when index is out of range", [&g]() { return g.cbegin_edges(1); });
              return g;
            }
          },
          {
            graph_description::node,
            t.report_line(""),
            [&t](const graph_t& g) -> const graph_t& {
              t.check_exception_thrown<std::out_of_range>("cend_edges throws when index is out of range", [&g]() { return g.cend_edges(1); });
              return g;
            }
          },
          {
            graph_description::node,
            t.report_line(""),
            [&t](const graph_t& g) -> const graph_t& {
              t.check_exception_thrown<std::out_of_range>("crbegin_edges throws when index is out of range", [&g]() { return g.crbegin_edges(1); });
              return g;
            }
          },
          {
            graph_description::node,
            t.report_line(""),
            [&t](const graph_t& g) -> const graph_t& {
              t.check_exception_thrown<std::out_of_range>("crend_edges throws when index is out of range", [&g]() { return g.crend_edges(1); });
              return g;
            }
          },
          {
            graph_description::node,
            t.report_line(""),
            [&t](const graph_t& g) -> const graph_t& {
              t.check_exception_thrown<std::out_of_range>("cedges throws when index is out of range", [&g]() { return g.cedges(1); });
              return g;
            }
          },
          {
            graph_description::node,
            t.report_line(""),
            [&t](const graph_t& g) -> const graph_t& {
              t.check_exception_thrown<std::out_of_range>("swapping nodes throws if first index out of range", [g{g}]() mutable { g.swap_nodes(1, 0); });
              return g;
            }
          },
          {
            graph_description::node,
            t.report_line(""),
            [&t](const graph_t& g) -> const graph_t& {
              t.check_exception_thrown<std::out_of_range>("swapping nodes throws if second index out of range", [g{g}]() mutable { g.swap_nodes(0, 1); });
              return g;
            }
          },
          {
            graph_description::node,
            t.report_line(""),
            [&t](const graph_t& g) -> const graph_t& {
              t.check_exception_thrown<std::out_of_range>("joining nodes throws if first index out of range", [g{g}]() mutable { g.join(1, 0); });
              return g;
            }
          },
          {
            graph_description::node,
            t.report_line(""),
            [&t](const graph_t& g) -> const graph_t& {
              t.check_exception_thrown<std::out_of_range>("joining nodes throws if second index out of range", [g{g}]() mutable { g.join(0, 1); });
              return g;
            }
          },
          {
            graph_description::empty,
            t.report_line("Clear graph"),
            [](graph_t g) -> graph_t {
              g.clear();
              return g;
            }
          },
          {
            graph_description::empty,
            t.report_line("Erase node to give empty graph"),
            [](graph_t g) -> graph_t {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node,
            t.report_line("Attempt to erase edge past the end"),
            [](graph_t g) -> const graph_t {
              g.erase_edge(g.cend_edges(0));
              return g;
            }
          },
          {
            graph_description::node_0,
            t.report_line("Add loop"),
            [](graph_t g) -> graph_t {
              g.join(0, 0);
              return g;
            }
          },
          {
            graph_description::node_node,
            t.report_line("Add second node"),
            [&t](graph_t g) -> graph_t {
              t.check(equality, t.report_line("Index of added node is 1"), g.add_node(), 1_sz);
              return g;
            }
          },
          {
            graph_description::node_node,
            t.report_line("Insert second node"),
            [&t](graph_t g) -> graph_t {
              t.check(equality, t.report_line("Index of added node is 0"), g.insert_node(0), 0_sz);
              return g;
            }
          },
          {
            graph_description::node_node,
            t.report_line("Insert second node at end"),
            [&t](graph_t g) -> graph_t {
              t.check(equality, t.report_line("Index of added node is 1"), g.insert_node(1), 1_sz);
              return g;
            }
          },
          {
            graph_description::node,
            t.report_line("Swap node with self"),
            [](graph_t g) -> graph_t {
              g.swap_nodes(0,0);
              return g;
            }
          }
        }, // end 'node'
        {  // begin 'node_0'
          {
            graph_description::empty,
            t.report_line("Clear graph"),
            [](graph_t g) -> graph_t {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node,
            t.report_line("Remove loop"),
            [](graph_t g) -> graph_t {
              g.erase_edge(g.cbegin_edges(0));
              return g;
            }
          },
          {
            graph_description::node,
            t.report_line("Remove loop via second partial edge"),
            [](graph_t g) -> graph_t {
              g.erase_edge(std::ranges::next(g.cbegin_edges(0)));
              return g;
            }
          },
          {
            graph_description::node_0_0,
            t.report_line("Add a second loop"),
            [](graph_t g) -> graph_t {
              g.join(0, 0);
              return g;
            }
          },
          {
            graph_description::node_node_1,
            t.report_line("Insert node"),
            [&t](graph_t g) -> graph_t {
              t.check(equality, t.report_line("Index of added node is 0"), g.insert_node(0), 0_sz);
              return g;
            }
          },
          {
            graph_description::node_0_node,
            t.report_line("Insert node at end"),
            [&t](graph_t g) -> graph_t {
              t.check(equality, t.report_line("Index of added node is 1"), g.insert_node(1), 1_sz);
              return g;
            }
          },
          {
            graph_description::node_0,
            t.report_line("Swap node with self"),
            [](graph_t g) -> graph_t {
              g.swap_nodes(0,0);
              return g;
            }
          },
          {
            graph_description::node_0,
            t.report_line("Swap edge with self"),
            [](graph_t g) -> graph_t {
              g.swap_edges(0, 0, 0);
              return g;
            }
          },
          {
            graph_description::node_0,
            t.report_line("Swap first partial edge with partner"),
            [](graph_t g) -> graph_t {
              g.swap_edges(0, 0, 1);
              return g;
            }
          },
          {
            graph_description::node_0,
            t.report_line("Swap second partial edge with partner"),
            [](graph_t g) -> graph_t {
              g.swap_edges(0, 1, 0);
              return g;
            }
          },
          {
            graph_description::node_0,
            t.report_line(""),
            [&t](const graph_t& g) -> const graph_t& {
              t.check_exception_thrown<std::out_of_range>("swapping edges throws for first edge index out of range", [g{g}]() mutable { g.swap_edges(0, 2, 0); });
              return g;
            }
          },
          {
            graph_description::node_0,
            t.report_line(""),
            [&t](const graph_t& g) -> const graph_t& {
              t.check_exception_thrown<std::out_of_range>("swapping edges throws for second edge index out of range", [g{g}]() mutable { g.swap_edges(0, 0, 2); });
              return g;
            }
          },
          {
            graph_description::node_0,
            t.report_line(""),
            [&t](const graph_t& g) -> const graph_t& {
              t.check_exception_thrown<std::out_of_range>("swapping edges throws for node index out of range", [g{g}]() mutable { g.swap_edges(1, 0, 0); });
              return g;
            }
          }
        }, // end 'node_0'
        {  // begin 'node_0_0'
          {
            graph_description::empty,
            t.report_line("Clear graph"),
            [](graph_t g) -> graph_t {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node_0,
            t.report_line("Remove first loop, via first partial edge"),
            [](graph_t g) -> graph_t {
              g.erase_edge(g.cbegin_edges(0));
              return g;
            }
          },
          {
            graph_description::node_0,
            t.report_line("Remove first loop, via second partial edge"),
            [](graph_t g) -> graph_t {
              g.erase_edge(g.cbegin_edges(0) + 1);
              return g;
            }
          },
          {
            graph_description::node_0,
            t.report_line("Remove second loop, via first partial edge"),
            [](graph_t g) -> graph_t {
              g.erase_edge(g.cbegin_edges(0) + 2);
              return g;
            }
          },
          {
            graph_description::node_0,
            t.report_line("Remove second loop, via second partial edge"),
            [](graph_t g) -> graph_t {
              g.erase_edge(g.cbegin_edges(0) + 3);
              return g;
            }
          },
          {
            graph_description::node_0_0,
            t.report_line("Swap partial edges {0,1}"),
            [](graph_t g) -> graph_t {
              g.swap_edges(0, 0, 1);
              return g;
            }
          },
          {
            graph_description::node_0_0,
            t.report_line("Swap partial edges {0,2}"),
            [](graph_t g) -> graph_t {
              g.swap_edges(0, 0, 2);
              return g;
            }
          },
          {
            graph_description::node_0_0,
            t.report_line("Swap partial edges {0,3}"),
            [](graph_t g) -> graph_t {
              g.swap_edges(0, 0, 3);
              return g;
            }
          },
          {
            graph_description::node_0_0,
            t.report_line("Swap partial edges {1,2}"),
            [](graph_t g) -> graph_t {
              g.swap_edges(0, 1, 2);
              return g;
            }
          },
          {
            graph_description::node_0_0,
            t.report_line("Swap partial edges {2,1}"),
            [](graph_t g) -> graph_t {
              g.swap_edges(0, 2, 1);
              return g;
            }
          }
        }, // end 'node_0_0'
        {  // begin 'node_node'
          {
            graph_description::empty,
            t.report_line("Clear graph"),
            [](graph_t g) -> graph_t {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node,
            t.report_line("Erase node 0"),
            [](graph_t g) -> graph_t {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node,
            t.report_line("Erase node 1"),
            [](graph_t g) -> graph_t {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            t.report_line("Join {0,1}"),
            [](graph_t g) -> graph_t {
              g.join(0, 1);
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            t.report_line("Join {1,0}"),
            [](graph_t g) -> graph_t {
              g.join(0, 1);
              return g;
            }
          }
        }, // end 'node_node'
        {  // begin 'node_1_node_0'
          {
            graph_description::empty,
            t.report_line("Clear graph"),
            [](graph_t g) -> graph_t {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node,
            t.report_line("Erase node 0"),
            [](graph_t g) -> graph_t {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node,
            t.report_line("Erase node 1"),
            [](graph_t g) -> graph_t {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_0_1_node_0,
            t.report_line("Add loop to node 0 and then swap it with link"),
            [](graph_t g) -> graph_t {
              g.join(0, 0);
              g.swap_edges(0, 0, 2);
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            t.report_line("Swap nodes {0,1}"),
            [](graph_t g) -> graph_t {
              g.swap_nodes(0,1);
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            t.report_line("Swap nodes {1,0}"),
            [](graph_t g) -> graph_t {
              g.swap_nodes(1,0);
              return g;
            }
          },
          {
            graph_description::node_1_1_node_0_0,
            t.report_line("Join {0,1}"),
            [](graph_t g) {
              g.join(0, 1);
              return g;
            }
          }
        }, // end 'node_1_node_0'
        {  // begin 'node_0_1_node_0'
          {
            graph_description::empty,
            t.report_line("Clear graph"),
            [](graph_t g) -> graph_t {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node,
            t.report_line("Erase node 0"),
            [](graph_t g) -> graph_t {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_0,
            t.report_line("Erase node 1"),
            [](graph_t g) -> graph_t {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            t.report_line("Remove loop via first partial edge"),
            [](graph_t g) -> graph_t {
              g.erase_edge(g.cbegin_edges(0));
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            t.report_line("Remove loop via second partial edge"),
            [](graph_t g) -> graph_t {
              g.erase_edge(++g.cbegin_edges(0));
              return g;
            }
          },
          {
            graph_description::node_0_node,
            t.report_line("Remove edge {0,1}"),
            [](graph_t g) -> graph_t {
              g.erase_edge(g.cbegin_edges(0) + 2);
              return g;
            }
          },
          {
            graph_description::node_0_node,
            t.report_line("Remove edge {1,0}"),
            [](graph_t g) -> graph_t {
              g.erase_edge(g.cbegin_edges(1));
              return g;
            }
          }
        }, // end 'node_0_1_node_0'
        {  // begin 'node_1_node_0_1'
          {
            graph_description::node_0,
            t.report_line("Erase node 0"),
            [](graph_t g) -> graph_t {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node,
            t.report_line("Erase node 1"),
            [](graph_t g) -> graph_t {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_node_1,
            t.report_line("Remove edge {0,1}"),
            [](graph_t g) -> graph_t {
              g.erase_edge(g.cbegin_edges(0));
              return g;
            }
          },
          {
            graph_description::node_node_1,
            t.report_line("Remove edge {1,0}"),
            [](graph_t g) -> graph_t {
              g.erase_edge(g.cbegin_edges(1));
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            t.report_line("Remove loop via first partial edge"),
            [](graph_t g) -> graph_t {
              g.erase_edge(g.cbegin_edges(1)+1);
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            t.report_line("Remove loop via second partial edge"),
            [](graph_t g) -> graph_t {
              g.erase_edge(g.cbegin_edges(1)+2);
              return g;
            }
          }
        }, // end 'node_1_node_0_1'
        {  // begin 'node_0_node'
          {
            graph_description::empty,
            t.report_line("Clear graph"),
            [](graph_t g) -> graph_t {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node_node_1_node,
            t.report_line("Insert node"),
            [&t](graph_t g) -> graph_t {
              t.check(equality, t.report_line("Index of added node is 0"), g.insert_node(0), 0_sz);
              return g;
            }
          },
          {
            graph_description::node_node_1,
            t.report_line("Swap nodes"),
            [](graph_t g) -> graph_t {
              g.swap_nodes(0,1);
              return g;
            }
          }
          ,
          {
            graph_description::node_node_1,
            t.report_line("Swap nodes"),
            [](graph_t g) -> graph_t {
              g.swap_nodes(1,0);
              return g;
            }
          }
        }, // end 'node_0_node'
        {  // begin 'node_node_1'
          {
            graph_description::empty,
            t.report_line("Clear graph"),
            [](graph_t g) -> graph_t {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node_0,
            t.report_line("Erase node 0"),
            [](graph_t g) -> graph_t {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node,
            t.report_line("Erase node 1"),
            [](graph_t g) -> graph_t {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_0_node,
            t.report_line("Swap nodes"),
            [](graph_t g) -> graph_t {
              g.swap_nodes(0,1);
              return g;
            }
          }
        }, // end 'node_node_1'
        {  // begin 'node_1_1_node_0_0'
          {
            graph_description::empty,
            t.report_line("Clear graph"),
            [](graph_t g) -> graph_t {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node,
            t.report_line("Erase node 0"),
            [](graph_t g) -> graph_t {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node,
            t.report_line("Erase node 1"),
            [](graph_t g) -> graph_t {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            t.report_line("Erase node 0 zeroth link"),
            [](graph_t g) -> graph_t {
              g.erase_edge(g.cbegin_edges(0));
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            t.report_line("Erase node 0 first link"),
            [](graph_t g) -> graph_t {
              g.erase_edge(g.cbegin_edges(0) + 1);
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            t.report_line("Erase node 1 zeroth link"),
            [](graph_t g) -> graph_t {
              g.erase_edge(g.cbegin_edges(1));
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            t.report_line("Erase node 1 first link"),
            [](graph_t g) -> graph_t {
              g.erase_edge(g.cbegin_edges(1) + 1);
              return g;
            }
          },
          {
            graph_description::node_1_1_node_0_0,
            t.report_line("Swap edges"),
            [](graph_t g) -> graph_t {
              g.swap_edges(0, 0, 1);
              return g;
            }
          },
          {
            graph_description::node_1_1_node_0_0,
            t.report_line("Swap node 0 edges"),
            [](graph_t g) -> graph_t {
              g.swap_edges(0, 1, 0);
              return g;
            }
          },
          {
            graph_description::node_1_1_node_0_0,
            t.report_line("Swap node 1 edges"),
            [](graph_t g) -> graph_t {
              g.swap_edges(1, 0, 1);
              return g;
            }
          }
        }, // end 'node_1_1_node_0_0'
        {  // begin 'node_node_node'
          {
            graph_description::empty,
            t.report_line("Clear graph"),
            [](graph_t g) -> graph_t {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node_node,
            t.report_line("Erase node 0"),
            [](graph_t g) -> graph_t {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_node,
            t.report_line("Erase node 1"),
            [](graph_t g) -> graph_t {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_node,
            t.report_line("Erase node 2"),
            [](graph_t g) -> graph_t {
              g.erase_node(2);
              return g;
            }
          },
          {
            graph_description::node_1_node_0_node,
            t.report_line("Join {0,1}"),
            [](graph_t g) -> graph_t {
              g.join(0,1);
              return g;
            }
          },
          {
            graph_description::node_node_2_node_1,
            t.report_line("Join {2,1}"),
            [](graph_t g) -> graph_t {
              g.join(2,1);
              return g;
            }
          }
        }, // end 'node_node_node'
        {  // begin 'node_1_node_0_node'
          {
            graph_description::empty,
            t.report_line("Clear graph"),
            [](graph_t g) -> graph_t {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node_node,
            t.report_line("Erase node 0"),
            [](graph_t g) -> graph_t {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_node,
            t.report_line("Erase node 1"),
            [](graph_t g) -> graph_t {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            t.report_line("Erase node 2"),
            [](graph_t g) -> graph_t {
              g.erase_node(2);
              return g;
            }
          },
          {
            graph_description::node_node_node,
            t.report_line("Remove link {0,1}"),
            [](graph_t g) -> graph_t {
              g.erase_edge(g.cbegin_edges(0));
              return g;
            }
          },
          {
            graph_description::node_node_node,
            t.report_line("Remove link {1,0}"),
            [](graph_t g) -> graph_t {
              g.erase_edge(g.cbegin_edges(1));
              return g;
            }
          },
          {
            graph_description::node_1_node_0_2_node_1,
            t.report_line("Join {1,2}"),
            [](graph_t g) -> graph_t {
              g.join(1,2);
              return g;
            }
          },
          {
            graph_description::node_1_node_0_2_node_1,
            t.report_line("Join {2,1}"),
            [](graph_t g) -> graph_t {
              g.join(2,1);
              return g;
            }
          },
          {
            graph_description::node_node_2_node_1,
            t.report_line("Swap nodes {0,2}"),
            [](graph_t g) -> graph_t {
              g.swap_nodes(0,2);
              return g;
            }
          },
          {
            graph_description::node_node_2_node_1,
            t.report_line("Swap nodes {2,0}"),
            [](graph_t g) -> graph_t {
              g.swap_nodes(2,0);
              return g;
            }
          }
        }, // end 'node_1_node_0_node'
        {  // begin 'node_node_2_node_1'
          {
            graph_description::empty,
            t.report_line("Clear graph"),
            [](graph_t g) -> graph_t {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            t.report_line("Erase node 0"),
            [](graph_t g) -> graph_t {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_node,
            t.report_line("Erase node 1"),
            [](graph_t g) -> graph_t {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_node,
            t.report_line("Erase node 2"),
            [](graph_t g) -> graph_t {
              g.erase_node(2);
              return g;
            }
          },
          {
            graph_description::node_node_node,
            t.report_line("Remove link {1,2}"),
            [](graph_t g) -> graph_t {
              g.erase_edge(g.cbegin_edges(1));
              return g;
            }
          },
          {
            graph_description::node_node_node,
            t.report_line("Remove link {2,1}"),
            [](graph_t g) -> graph_t {
              g.erase_edge(g.cbegin_edges(2));
              return g;
            }
          },
          {
            graph_description::node_1_node_0_2_node_1,
            t.report_line("Join {0,1}, the swap node 1's edge"),
            [](graph_t g) -> graph_t {
              g.join(0,1);
              g.swap_edges(1, 0, 1);
              return g;
            }
          },
          {
            graph_description::node_1_node_0_node,
            t.report_line("Swap nodes {0,2}"),
            [](graph_t g) -> graph_t {
              g.swap_nodes(0,2);
              return g;
            }
          },
          {
            graph_description::node_1_node_0_node,
            t.report_line("Swap nodes {2,0}"),
            [](graph_t g) -> graph_t {
              g.swap_nodes(2,0);
              return g;
            }
          }
        }, // end 'node_node_2_node_1'
        {  // begin 'node_1_node_0_2_node_1'
          {
            graph_description::empty,
            t.report_line("Clear graph"),
            [](graph_t g) -> graph_t {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            t.report_line("Erase node 0"),
            [](graph_t g) -> graph_t {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_node,
            t.report_line("Erase node 1"),
            [](graph_t g) -> graph_t {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            t.report_line("Erase node 2"),
            [](graph_t g) -> graph_t {
              g.erase_node(2);
              return g;
            }
          },
          {
            graph_description::node_1_node_0_node,
            t.report_line("Remove link {1,2}"),
            [](graph_t g) -> graph_t {
              g.erase_edge(g.cbegin_edges(1) + 1);
              return g;
            }
          },
          {
            graph_description::node_1_node_0_2_node_1,
            t.report_line("Swap nodes {0,2} then swap node 1's edges"),
            [](graph_t g) -> graph_t {
              g.swap_nodes(0, 2);
              g.swap_edges(1, 0, 1);
              return g;
            }
          },
          {
            graph_description::node_1_node_0_2_node_1,
            t.report_line("Swap nodes {2,0} then swap node 1's edges"),
            [](graph_t g) -> graph_t {
              g.swap_nodes(2, 0);
              g.swap_edges(1, 1, 0);
              return g;
            }
          }
        }, // end 'node_1_node_0_2_node_1'
        {  // begin 'node_1_2_node_2_0_node_0_1'
          {
            graph_description::empty,
            t.report_line("Clear graph"),
            [](graph_t g) -> graph_t {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            t.report_line("Erase node 0"),
            [](graph_t g) -> graph_t {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            t.report_line("Erase node 1"),
            [](graph_t g) -> graph_t {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            t.report_line("Erase node 2"),
            [](graph_t g) -> graph_t {
              g.erase_node(2);
              return g;
            }
          },
          {
            graph_description::node_1_node_0_2_node_1,
            t.report_line("Remove {2,0}"),
            [](graph_t g) -> graph_t {
              g.erase_edge(g.cbegin_edges(2));
              return g;
            }
          }
        }, // end 'node_1_2_node_0_2_node_0_1'
        {  // begin 'node_node_1_node'
          {
            graph_description::empty,
            t.report_line("Clear graph"),
            [](graph_t g) -> graph_t {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node_0_node,
            t.report_line("Erase node 0"),
            [](graph_t g) -> graph_t {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_node,
            t.report_line("Erase node 1"),
            [](graph_t g) -> graph_t {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_node_1,
            t.report_line("Erase node 2"),
            [](graph_t g) -> graph_t {
              g.erase_node(2);
              return g;
            }
          },
        }, // end 'node_node_1_node'
        {  // begin 'node_1_node_0_1_node'
          {
            graph_description::node_0_node,
            t.report_line("Erase node 0"),
            [](graph_t g) -> graph_t {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_node,
            t.report_line("Erase node 1"),
            [](graph_t g) -> graph_t {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_node_1_node,
            t.report_line("Remove {0,1}"),
            [](graph_t g) -> graph_t {
              g.erase_edge(g.cbegin_edges(0));
              return g;
            }
          },
          {
            graph_description::node_1_node_0_1_2_node_1,
            t.report_line("Join {1,2}"),
            [](graph_t g) -> graph_t {
              g.join(1, 2);
              return g;
            }
          }
        }, // end 'node_1_node_0_1_node'
        {  // begin 'node_1_node_0_1_2_node_1'
          {
            graph_description::node_0_1_node_0,
            t.report_line("Erase node 0"),
            [](graph_t g) -> graph_t {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_node,
            t.report_line("Erase node 1"),
            [](graph_t g) -> graph_t {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_1_node_0_1_node,
            t.report_line("Remove {1,2}"),
            [](graph_t g) -> graph_t {
              g.erase_edge(g.cbegin_edges(1) + 3);
              return g;
            }
          }
        }, // end 'node_1_node_0_1_2_node_1'
        {  // begin 'node_2_node_node_0_2'
          {
            graph_description::node_1_node_0_1_node,
            t.report_line("Swap {1,2}"),
            [](graph_t g) -> graph_t {
              g.swap_nodes(1,2);
              return g;
            }
          }
        }, // end 'node_2_node_node_0_2'
        {  // begin 'node_1_1_2_2_node_0_0_2_node_0_0_1'
          {
            graph_description::node_1_node_0,
            t.report_line("Erase node 0"),
            [](graph_t g) -> graph_t {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_1_1_node_0_0,
            t.report_line("Erase node 1"),
            [](graph_t g) -> graph_t {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_1_1_node_0_0,
            t.report_line("Erase node 2"),
            [](graph_t g) -> graph_t {
              g.erase_node(2);
              return g;
            }
          },
          {
            graph_description::node_1_1_2_2_node_0_0_2_node_0_0_1,
            t.report_line("Swap nodes {2, 1}"),
            [](graph_t g) -> graph_t {
              g.swap_nodes(2,1);
              g.sort_edges(g.cbegin_edges(0), g.cend_edges(0), [](const auto& lhs, const auto& rhs) { return lhs.target_node() < rhs.target_node(); });
              return g;
            }
          },
          {
            graph_description::node_1_1_2_2_node_0_0_2_node_0_0_1,
            t.report_line("Swap nodes {2, 1}"),
            [](graph_t g) -> graph_t {
              g.swap_nodes(2,1);
              g.stable_sort_edges(g.cbegin_edges(0), g.cend_edges(0), [](const auto& lhs, const auto& rhs) { return lhs.target_node() < rhs.target_node(); });
              return g;
            }
          }
        }, // end 'node_1_1_2_2_node_0_0_2_node_0_0_1'
        {  // begin 'node_3_1_node_0_2_node_1_node_0'
          {
             graph_description::node_1_node_0_node,
             t.report_line("Erase node 0"),
             [](graph_t g) -> graph_t {
               g.erase_node(0);
               return g;
             }
          }
        }, // end 'node_3_1_node_0_2_node_1_node_0'
        {  // begin 'node_1_node_0_node_3_node_2'
          {
            graph_description::node_1_node_0_node,
            t.report_line("Erase node 2"),
            [](graph_t g) -> graph_t {
              g.erase_node(2);
              return g;
            }
          },
          {
            graph_description::node_2_node_3_node_0_node_1,
            t.report_line("Swap {2,1}"),
            [](graph_t g) -> graph_t {
              g.swap_nodes(2,1);
              return g;
            }
          },
          {
            graph_description::node_2_node_3_node_0_node_1,
            t.report_line("Swap {0,3}"),
            [](graph_t g) -> graph_t {
              g.swap_nodes(0,3);
              return g;
            }
          }
        }, // end 'node_1_node_0_node_3_node_2'
        {  // begin 'node_2_node_3_node_0_node_1'
          {
            graph_description::node_1_node_0_node_3_node_2,
            t.report_line("Swap {2,1}"),
            [](graph_t g) -> graph_t {
              g.swap_nodes(2,1);
              return g;
            }
          },
          {
            graph_description::node_1_node_0_node_3_node_2,
            t.report_line("Swap {0,3}"),
            [](graph_t g) -> graph_t {
              g.swap_nodes(0,3);
              return g;
            }
          }
        }, // end 'node_2_node_3_node_0_node_1'
      },
      {
        //  'empty'
        make_and_check(t, t.report_line(""), {}),

        //  'node'
        make_and_check(t, t.report_line(""), {{}}),

        //  'node_0'
        make_and_check(t, t.report_line(""), {{edge_t{0}, edge_t{0}}}),

        //  'node_0_0'
        make_and_check(t, t.report_line(""), {{edge_t{0}, edge_t{0}, edge_t{0}, edge_t{0}}}),

        //  'node_node'
        make_and_check(t, t.report_line(""), {{}, {}}),

        //  'node_1_node_0'
        make_and_check(t, t.report_line(""), {{edge_t{1}}, {edge_t{0}}}),

        //  'node_0_1_node_0'
        make_and_check(t, t.report_line(""), {{edge_t{0}, edge_t{0}, edge_t{1}}, {edge_t{0}}}),

        // 'node_1_node_0_1'
        make_and_check(t, t.report_line(""), {{edge_t{1}}, {edge_t{0}, edge_t{1}, edge_t{1}}}),

        //  'node_0_node'
        make_and_check(t, t.report_line(""), {{edge_t{0}, edge_t{0}}, {}}),

        //  'node_node_1'
        make_and_check(t, t.report_line(""), {{}, {edge_t{1}, edge_t{1}}}),

        // 'node_1_1_node_0_0'
        make_and_check(t, t.report_line(""), {{edge_t{1}, edge_t{1}}, {edge_t{0}, edge_t{0}}}),

        //  'node_node_node'
        make_and_check(t, t.report_line(""), {{}, {}, {}}),

        //  'node_1_node_0_node'
        make_and_check(t, t.report_line(""), {{edge_t{1}}, {edge_t{0}}, {}}),

        //  'node_node_2_node_1'
        make_and_check(t, t.report_line(""), {{}, {edge_t{2}}, {edge_t{1}}}),

        // 'node_1_node_0_2_node_1'
        make_and_check(t, t.report_line(""), {{edge_t{1}}, {edge_t{0}, edge_t{2}}, {edge_t{1}}}),

        // 'node_1_2_node_0_2_node_0_1'
        [&t](){
          auto g{make_and_check(t, t.report_line(""), {{edge_t{1}, edge_t{2}}, {edge_t{0}, edge_t{2}}, {edge_t{0}, edge_t{1}}})};
          t.check(equality, t.report_line("Check sorting of edges on construction"), graph_t{{edge_t{2}, edge_t{1}}, {edge_t{2}, edge_t{0}}, {edge_t{1}, edge_t{0}}}, g);

          return g;
        }(),

        // 'node_node_1_node'
        make_and_check(t, t.report_line(""), {{}, {edge_t{1}, edge_t{1}}, {}}),

        // 'node_1_node_0_1_node'
        make_and_check(t, t.report_line(""), {{edge_t{1}}, {edge_t{0}, edge_t{1}, edge_t{1}}, {}}),

        // 'node_1_node_0_1_2_node_1'
        make_and_check(t, t.report_line(""), {{edge_t{1}}, {edge_t{0}, edge_t{1}, edge_t{1}, edge_t{2}}, {edge_t{1}}}),

        // 'node_2_node_node_0_2'
        make_and_check(t, t.report_line(""), {{edge_t{2}}, {}, {edge_t{0}, edge_t{2}, edge_t{2}}}),

        // 'node_1_1_2_2_node_0_0_2_node_0_0_1'
        [&t](){
          auto g{make_and_check(t, t.report_line(""), {{edge_t{1}, edge_t{1}, edge_t{2}, edge_t{2}}, {edge_t{0}, edge_t{0}, edge_t{2}}, {edge_t{0}, edge_t{0}, edge_t{1}}})};
          t.check(equality,
                t.report_line("Check sorting of edges on construction"),
                graph_t{{edge_t{2}, edge_t{1}, edge_t{2}, edge_t{1}}, {edge_t{0}, edge_t{2}, edge_t{0}}, {edge_t{1}, edge_t{0}, edge_t{0}}},
                g);

          return g;
        }(),

        // 'node_3_1_node_0_2_node_1_node_0'
        make_and_check(t, t.report_line(""), {{edge_t{1}, edge_t{3}}, {edge_t{0}, edge_t{2}}, {edge_t{1}}, {edge_t{0}}}),

        // 'node_1_node_0_node_3_node_2'
        make_and_check(t, t.report_line(""), {{edge_t{1}}, {edge_t{0}}, {edge_t{3}}, {edge_t{2}}}),

        // 'node_2_node_3_node_0_node_1'
        make_and_check(t, t.report_line(""), {{edge_t{2}}, {edge_t{3}}, {edge_t{0}}, {edge_t{1}}}),
      }
      };
    }
  };
}