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
  namespace undirected_embedded_graph
  {
    /// Convention: the indices following 'node' - separated by underscores - give the target node of the associated edges
    enum graph_description : std::size_t {
      empty = 0,

      // x
      node,

      //  /-\
      //  \ /
      //   x
      node_0,

      //  /-\ /-\
      //  \ / \ /
      //     x
      node_0_0,

      //     /-\/-\
      //    /  /\  \
      //    \ /  \ /
      //        x
      node_0_0_interleaved,

      //     /-\/-\
      //    /  /\  \
      //    \ /  \ /
      //       x
      //      / \
      //      \-/
      //
      node_0_0_0_interleaved,

      //  x    x
      node_node,

      //  x ---- x
      node_1_node_0,

      //   /-\
      //   \ /
      //    x ---- x
      node_0_1_node_0,

      //  /-\
      //  \ /
      //   x      x
      node_0_node,

      //      /-\
      //      \ /
      //  x    x
      node_node_1,

      //      /-\/-\
      //     /  /\  \
      //     \ /  \ /
      //  x      x
      node_node_1_1_interleaved,

      //      /-\/-\
      //     /  /\  \
      //     \ /  \ /
      //  x     x
      //       / \
      //       \-/
      //
      node_node_1_1_1_interleaved,

      //      /-\/-\
      //     /  /\  \
      //     \ /  \ /
      // x ---- x
      //       / \
      //       \-/
      //
      node_1_node_1_1_1_0_interleaved,

      //         /-\/-\
      //        /  /\  \
      //        \ /  \ /
      // -- x ---- x-----
      //          / \
      //          \-/
      //
      node_1_1_node_1_0_1_0_1_interleaved,

      //  x ==== x
      node_1_1_node_0_0,

      // x ---- x
      //   ----
      node_1pos1_1pos0_node_0pos1_0pos0,

      //  x    x    x
      node_node_node,

      //  x ---- x    x
      node_1_node_0_node,

      //  x    x ---- x
      node_node_2_node_1,

      //  x ---- x ---- x
      node_1_node_0_2_node_1,

      // -- x ---- x ---- x --
      node_1_node_0_2_node_1_0,

      //      /-\
      //      \ /
      //  x    x    x
      node_node_1_node,

      //        /-\
      //        \ /
      //  x ---- x    x
      node_1_node_0_1_node,


      //        /-\
      //        \ /
      //  x ---- x    x
      //    ----
      node_1_node_1_0_node,

      //        /-\
      //        \ /
      //  x ---- x ---- x
      //    ----
      node_1_node_1_0_2_node_1,

      //        /-\
      //        \ /
      //  x ---- x ---- x
      node_1_node_0_1_2_node_1,

      //             /-\
      //             \ /
      // -- x    x    x --
      node_2_node_node_0_2,

      //     [2]
      //      x
      //    //  \
      //   //    \
      //  //      \
      //  x  ====  x
      // [0]      [1]
      node_1_1_2_2_node_0_2_0_node_0_1_0,

      //      [2]
      //       x
      //    //  \\
      //   //    \\
      //  //      \\
      //  x  ====  x
      // [0]      [1]
      node_1_1_2_2_node_0_2_2_0_node_0_1_1_0,

      //      [2]
      //       x
      //    //  \\
      //   //    \\
      //  //      \\
      //  x  ====  x
      // [0]      [1]
      node_2_2_1_1_node_2pos3_node_1,

      //  x [3]
      //  |
      //  |
      //  |
      //  x ---- x ---- x
      // [0]    [1]    [2]
      node_3_1_node_0_2_node_1_node_0,

      // x ---- x    x ---- x
      node_1_node_0_node_3_node_2,

      // x --   x --   -- x  -- x
      node_2_node_3_node_0_node_1,

      end
    };
  }

  template
  <
    class EdgeWeight,
    class NodeWeight,
    class EdgeStorageConfig,
    class NodeWeightStorage
  >
  class dynamic_undirected_embedded_graph_operations
  {
  public:
    using graph_t            = maths::embedded_graph<EdgeWeight, NodeWeight, maths::null_meta_data, EdgeStorageConfig, NodeWeightStorage>;
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

      transition_checker<graph_t>::check(t.report(""), trg, checker);
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
      t.check_exception_thrown<std::out_of_range>("Target index of edge out of range", [](){ return graph_t{{{1, 0}}}; });
      t.check_exception_thrown<std::out_of_range>("Complimentary index of edge out of range", [](){ return graph_t{{{0, 1}}}; });
      t.check_exception_thrown<std::logic_error>("Self-referential complimentary index", [](){ return graph_t{{{0, 0}}}; });
      t.check_exception_thrown<std::logic_error>("Mismatched complimentary indices", [](){ return graph_t{{{0, 1}, {0, 1}}}; });
      t.check_exception_thrown<std::logic_error>("Mismatched complimentary indices", [](){ return graph_t{{{0, 1}, {0, 2}, {0, 0}}}; });

      // Two nodes
      t.check_exception_thrown<std::logic_error>("Mismatched complimentary indices", [](){ return graph_t{{{1, 0}, {0, 2}, {0, 1}}, {{0, 1}}}; });
    }

    [[nodiscard]]
    static transition_graph make_transition_graph(regular_test& t)
    {
      using namespace undirected_embedded_graph;

      check_initialization_exceptions(t);

      return transition_graph{
        {
        { // begin, 'empty'
          {
            graph_description::empty,
            t.report(""),
            [&t](const graph_t& g) -> const graph_t& {
              t.check_exception_thrown<std::out_of_range>("cbegin_edges throws for empty graph", [&g]() { return g.cbegin_edges(0); });
              return g;
            }
          },
          {
            graph_description::empty,
            t.report(""),
            [&t](const graph_t& g) -> const graph_t& {
              t.check_exception_thrown<std::out_of_range>("cend_edges throws for empty graph", [&g]() { return g.cend_edges(0); });
              return g;
            }
          },
          {
            graph_description::empty,
            t.report(""),
            [&t](const graph_t& g) -> const graph_t& {
              t.check_exception_thrown<std::out_of_range>("crbegin_edges throws for empty graph", [&g]() { return g.crbegin_edges(0); });
              return g;
            }
          },
          {
            graph_description::empty,
            t.report(""),
            [&t](const graph_t& g) -> const graph_t& {
              t.check_exception_thrown<std::out_of_range>("crend_edges throws for empty graph", [&g]() { return g.crend_edges(0); });
              return g;
            }
          },
          {
            graph_description::empty,
            t.report(""),
            [&t](const graph_t& g) -> const graph_t& {
              t.check_exception_thrown<std::out_of_range>("cedges throws for empty graph", [&g]() { return g.cedges(0); });
              return g;
            }
          },
          {
            graph_description::empty,
            t.report(""),
            [&t](const graph_t& g) -> const graph_t& {
              t.check_exception_thrown<std::out_of_range>("swapping nodes throws for empty graph", [g{g}]() mutable { g.swap_nodes(0, 0); });
              return g;
            }
          },
          {
            graph_description::empty,
            t.report(""),
            [&t](const graph_t& g) -> const graph_t& {
              t.check_exception_thrown<std::out_of_range>("joining nodes throws for empty graph", [g{g}]() mutable { g.join(0, 0); });
              return g;
            }
          },
          {
            graph_description::empty,
            t.report("Clear empty graph"),
            [](graph_t g) -> graph_t {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node,
            t.report("Add node to empty graph"),
            [&t](graph_t g) -> graph_t {
              t.check(equality, "Index of added node is 0", g.add_node(), 0uz);
              return g;
            }
          },
          {
            graph_description::node,
            t.report("insert node into empty graph"),
            [&t](graph_t g) -> graph_t {
              t.check(equality, "Index of added node is 0", g.insert_node(0), 0uz);
              return g;
            }
          }
        }, // end 'empty'
        {  // begin 'node'
          {
            graph_description::node,
            t.report(""),
            [&t](const graph_t& g) -> const graph_t& {
              t.check_exception_thrown<std::out_of_range>("cbegin_edges throws when index is out of range", [&g]() { return g.cbegin_edges(1); });
              return g;
            }
          },
          {
            graph_description::node,
            t.report(""),
            [&t](const graph_t& g) -> const graph_t& {
              t.check_exception_thrown<std::out_of_range>("cend_edges throws when index is out of range", [&g]() { return g.cend_edges(1); });
              return g;
            }
          },
          {
            graph_description::node,
            t.report(""),
            [&t](const graph_t& g) -> const graph_t& {
              t.check_exception_thrown<std::out_of_range>("crbegin_edges throws when index is out of range", [&g]() { return g.crbegin_edges(1); });
              return g;
            }
          },
          {
            graph_description::node,
            t.report(""),
            [&t](const graph_t& g) -> const graph_t& {
              t.check_exception_thrown<std::out_of_range>("crend_edges throws when index is out of range", [&g]() { return g.crend_edges(1); });
              return g;
            }
          },
          {
            graph_description::node,
            t.report(""),
            [&t](const graph_t& g) -> const graph_t& {
              t.check_exception_thrown<std::out_of_range>("cedges throws when index is out of range", [&g]() { return g.cedges(1); });
              return g;
            }
          },
          {
            graph_description::node,
            t.report(""),
            [&t](const graph_t& g) -> const graph_t& {
              t.check_exception_thrown<std::out_of_range>("swapping nodes throws if first index out of range", [g{g}]() mutable { g.swap_nodes(1, 0); });
              return g;
            }
          },
          {
            graph_description::node,
            t.report(""),
            [&t](const graph_t& g) -> const graph_t& {
              t.check_exception_thrown<std::out_of_range>("swapping nodes throws if second index out of range", [g{g}]() mutable { g.swap_nodes(0, 1); });
              return g;
            }
          },
          {
            graph_description::node,
            t.report(""),
            [&t](const graph_t& g) -> const graph_t& {
              t.check_exception_thrown<std::out_of_range>("joining nodes throws if first index out of range", [g{g}]() mutable { g.join(1, 0); });
              return g;
            }
          },
          {
            graph_description::node,
            t.report(""),
            [&t](const graph_t& g) -> const graph_t& {
              t.check_exception_thrown<std::out_of_range>("joining nodes throws if second index out of range", [g{g}]() mutable { g.join(0, 1); });
              return g;
            }
          },
          {
            graph_description::node,
            t.report(""),
            [&t](const graph_t& g) -> const graph_t& {
              t.check_exception_thrown<std::out_of_range>("inserting join throws if second index out of range", [g{g}]() mutable { g.insert_join(g.cbegin_edges(0), 2); });
              return g;
            }
          },
          {
            graph_description::empty,
            t.report("Clear graph"),
            [](graph_t g) -> graph_t {
              g.clear();
              return g;
            }
          },
          {
            graph_description::empty,
            t.report("Erase node to give empty graph"),
            [](graph_t g) -> graph_t {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node,
            t.report("Attempt to erase edge past the end"),
            [](graph_t g) -> const graph_t {
              g.erase_edge(g.cend_edges(0));
              return g;
            }
          },
          {
            graph_description::node_0,
            t.report("Add loop"),
            [](graph_t g) -> graph_t {
              g.join(0, 0);
              return g;
            }
          },
          {
            graph_description::node_node,
            t.report("Add second node"),
            [&t](graph_t g) -> graph_t {
              t.check(equality, "Index of added node is 1", g.add_node(), 1uz);
              return g;
            }
          },
          {
            graph_description::node_node,
            t.report("Insert second node"),
            [&t](graph_t g) -> graph_t {
              t.check(equality, "Index of added node is 0", g.insert_node(0), 0uz);
              return g;
            }
          },
          {
            graph_description::node_node,
            t.report("Insert second node at end"),
            [&t](graph_t g) -> graph_t {
              t.check(equality, "Index of added node is 1", g.insert_node(1), 1uz);
              return g;
            }
          },
          {
            graph_description::node,
            t.report("Swap node with self"),
            [](graph_t g) -> graph_t {
              g.swap_nodes(0,0);
              return g;
            }
          }
        }, // end 'node'
        {  // begin 'node_0'
          {
            graph_description::empty,
            t.report("Clear graph"),
            [](graph_t g) -> graph_t {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node,
            t.report("Remove loop"),
            [](graph_t g) -> graph_t {
              g.erase_edge(g.cbegin_edges(0));
              return g;
            }
          },
          {
            graph_description::node_0_0,
            t.report("Add a second loop"),
            [](graph_t g) -> graph_t {
              g.join(0, 0);
              return g;
            }
          },
          {
            graph_description::node_0_0_interleaved,
            t.report("Interleave a second loop"),
            [](graph_t g) -> graph_t {
              g.insert_join(g.cbegin_edges(0)+1, 3);
              return g;
            }
          },
          {
            graph_description::node_0_0_interleaved,
            t.report("Interleave a second loop, backwards"),
            [](graph_t g) -> graph_t {
              g.insert_join(g.cbegin_edges(0) + 1, 0);
              return g;
            }
          },
          {
            graph_description::node_node_1,
            t.report("Insert node"),
            [&t](graph_t g) -> graph_t {
              t.check(equality, "Index of added node is 0", g.insert_node(0), 0uz);
              return g;
            }
          },
          {
            graph_description::node_0_node,
            t.report("Insert node at end"),
            [&t](graph_t g) -> graph_t {
              t.check(equality, "Index of added node is 1", g.insert_node(1), 1uz);
              return g;
            }
          },
          {
            graph_description::node_0,
            t.report("Swap node with self"),
            [](graph_t g) -> graph_t {
              g.swap_nodes(0,0);
              return g;
            }
          }
        }, // end 'node_0'
        {  // begin 'node_0_0'
          {
            graph_description::empty,
            t.report("Clear graph"),
            [](graph_t g) -> graph_t {
              g.clear();
              return g;
            }
          },
          {
            graph_description::empty,
            t.report("Erase node 0"),
            [](graph_t g) -> graph_t {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_0,
            t.report("Remove first loop"),
            [](graph_t g) -> graph_t {
              g.erase_edge(g.cbegin_edges(0));
              return g;
            }
          },
          {
            graph_description::node_0,
            t.report("Remove first loop via second insertion"),
            [](graph_t g) -> graph_t {
              g.erase_edge(g.cbegin_edges(0) + 1);
              return g;
            }
          },
          {
            graph_description::node_0,
            t.report("Remove second loop"),
            [](graph_t g) -> graph_t {
              g.erase_edge(std::ranges::next(g.cbegin_edges(0), 2));
              return g;
            }
          },
          {
            graph_description::node_0,
            t.report("Remove second loop via second insertion"),
            [](graph_t g) -> graph_t {
              g.erase_edge(std::ranges::next(g.cbegin_edges(0), 3));
              return g;
            }
          }
        }, // end 'node_0_0'
        {  // begin 'node_0_0_interleaved'
          {
            graph_description::empty,
            t.report("Clear graph"),
            [](graph_t g) -> graph_t {
              g.clear();
              return g;
            }
          },
          {
            graph_description::empty,
            t.report("Erase node 0"),
            [](graph_t g) -> graph_t {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_0,
            t.report("Remove first loop via first insertion"),
            [](graph_t g) -> graph_t {
              g.erase_edge(g.cbegin_edges(0));
              return g;
            }
          },
          {
            graph_description::node_0_0_0_interleaved,
            t.report("Insert a third loop"),
            [](graph_t g) -> graph_t {
              g.insert_join(g.cbegin_edges(0), ++g.cbegin_edges(0));
              return g;
            }
          }
          ,
          {
            graph_description::node_0_0_0_interleaved,
            t.report("Insert a third loop, inverted"),
            [](graph_t g) -> graph_t {
              g.insert_join(g.cbegin_edges(0), g.cbegin_edges(0));
              return g;
            }
          },
          {
            graph_description::node_node_1_1_interleaved,
            t.report("Insert node"),
            [](graph_t g) -> graph_t {
              g.insert_node(0);
              return g;
            }
          }
        }, // end 'node_0_0_interleaved'
        {  // begin 'node_0_0_0_interleaved'
          {
            graph_description::node_0_0_interleaved,
            t.report("Remove first loop via first insertion"),
            [](graph_t g) -> graph_t {
              g.erase_edge(g.cbegin_edges(0));
              return g;
            }
          },
          {
            graph_description::node_0_0_interleaved,
            t.report("Remove first loop via second insertion"),
            [](graph_t g) -> graph_t {
              g.erase_edge(g.cbegin_edges(0)+1);
              return g;
            }
          },
          {
            graph_description::node_0_0,
            t.report("Remove second loop via first insertion"),
            [](graph_t g) -> graph_t {
              g.erase_edge(g.cbegin_edges(0)+2);
              return g;
            }
          },
          {
            graph_description::node_0_0,
            t.report("Remove second loop via second insertion"),
            [](graph_t g) -> graph_t {
              g.erase_edge(g.cbegin_edges(0)+4);
              return g;
            }
          },
          {
            graph_description::node_0_0,
            t.report("Remove third loop via first insertion"),
            [](graph_t g) -> graph_t {
              g.erase_edge(g.cbegin_edges(0) + 3);
              return g;
            }
          },
          {
            graph_description::node_0_0,
            t.report("Remove third loop via second insertion"),
            [](graph_t g) -> graph_t {
              g.erase_edge(g.cbegin_edges(0) + 5);
              return g;
            }
          },
          {
            graph_description::node_node_1_1_1_interleaved,
            t.report("Insert node"),
            [](graph_t g) -> graph_t {
              g.insert_node(0);
              return g;
            }
          }
        }, // end 'node_0_0_0_interleaved'
        {  // begin 'node_node'
          {
            graph_description::empty,
            t.report("Clear graph"),
            [](graph_t g) -> graph_t {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node,
            t.report("Erase node 0"),
            [](graph_t g) -> graph_t {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node,
            t.report("Erase node 1"),
            [](graph_t g) -> graph_t {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            t.report("Join nodes 0,1"),
            [](graph_t g) -> graph_t {
              g.join(0, 1);
              return g;
            }
          }
        }, // end 'node_node'
        {  // begin 'node_1_node_0'
          {
            graph_description::empty,
            t.report("Clear graph"),
            [](graph_t g) -> graph_t {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node,
            t.report("Erase node 0"),
            [](graph_t g) -> graph_t {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node,
            t.report("Erase node 1"),
            [](graph_t g) -> graph_t {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            t.report("Swap nodes {0,1}"),
            [](graph_t g) -> graph_t {
              g.swap_nodes(0,1);
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            t.report("Swap nodes {1,0}"),
            [](graph_t g) -> graph_t {
              g.swap_nodes(1,0);
              return g;
            }
          },
          {
            graph_description::node_1_1_node_0_0,
            t.report("Join {0,1}"),
            [](graph_t g) {
              g.join(0, 1);
              return g;
            }
          }
        }, // end 'node_1_node_0'
        {  // begin 'node_0_1_node_0'
          {
            graph_description::empty,
            t.report("Clear graph"),
            [](graph_t g) -> graph_t {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node,
            t.report("Erase node 0"),
            [](graph_t g) -> graph_t {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_0,
            t.report("Erase node 1"),
            [](graph_t g) -> graph_t {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            t.report("Remove loop"),
            [](graph_t g) -> graph_t {
              g.erase_edge(g.cbegin_edges(0));
              return g;
            }
          },
          {
            graph_description::node_0_node,
            t.report("Remove link"),
            [](graph_t g) -> graph_t {
              g.erase_edge(std::ranges::next(g.cbegin_edges(0), 2));
              return g;
            }
          }
        }, // end 'node_0_1_node_0'
        {  // begin 'node_0_node'
          {
            graph_description::empty,
            t.report("Clear graph"),
            [](graph_t g) -> graph_t {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node_node,
            t.report("Remove link"),
            [](graph_t g) -> graph_t {
              g.erase_edge(g.cbegin_edges(0));
              return g;
            }
          },
          {
            graph_description::node_node_1_node,
            t.report("Insert node"),
            [&t](graph_t g) -> graph_t {
              t.check(equality, "Index of added node is 0", g.insert_node(0), 0uz);
              return g;
            }
          },
          {
            graph_description::node_node_1,
            t.report("Swap nodes"),
            [](graph_t g) -> graph_t {
              g.swap_nodes(0,1);
              return g;
            }
          }
        }, // end 'node_0_node'
        {  // begin 'node_node_1'
          {
            graph_description::empty,
            t.report("Clear graph"),
            [](graph_t g) -> graph_t {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node_0,
            t.report("Erase node 0"),
            [](graph_t g) -> graph_t {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node,
            t.report("Erase node 1"),
            [](graph_t g) -> graph_t {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_node,
            t.report("Remove link"),
            [](graph_t g) -> graph_t {
              g.erase_edge(g.cbegin_edges(1));
              return g;
            }
          },
          {
            graph_description::node_0_node,
            t.report("swap nodes"),
            [](graph_t g) -> graph_t {
              g.swap_nodes(0,1);
              return g;
            }
          }
        }, // end 'node_node_1'
        {  // begin 'node_node_1_1_interleaved'
          {
            graph_description::node_0_0_interleaved,
            t.report("Erase node 0"),
            [](graph_t g) -> graph_t {
              g.erase_node(0);
              return g;
            }
          }
        }, // end 'node_node_1_1_interleaved'
        {  // begin 'node_node_1_1_1_interleaved'
          {
            graph_description::node_0_0_0_interleaved,
            t.report("Erase node 0"),
            [](graph_t g) -> graph_t {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_1_node_1_1_1_0_interleaved,
            t.report("Join {0, 1; 4}"),
            [](graph_t g) -> graph_t {
              g.insert_join(g.cbegin_edges(0), g.cbegin_edges(1) + 4);
              return g;
            }
          }
        }, // end 'node_node_1_1_1_interleaved'
        {  // begin 'node_1_node_1_1_1_0_interleaved'
          {
            graph_description::node_0_0_0_interleaved,
            t.report("Erase node 0"),
            [](graph_t g) -> graph_t {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_1_1_node_1_0_1_0_1_interleaved,
            t.report("Join {0(1), 1(6)}"),
            [](graph_t g) -> graph_t {
              g.insert_join(g.cbegin_edges(0)+1, g.cbegin_edges(1) + 6);
              return g;
            }
          }
        }, // end 'node_1_node_1_1_1_0_interleaved'
        {  // begin 'node_1_1_node_1_0_1_0_1_interleaved'
          {
            graph_description::node_1_node_1_1_1_0_interleaved,
            t.report("Remove link {1(6), 0(1)}"),
            [](graph_t g) -> graph_t {
              g.erase_edge(g.cbegin_edges(1) + 6);
              return g;
            }
          },
          {
            graph_description::node_1_node_1_1_1_0_interleaved,
            t.report("Remove link {0(1), 1(6)}"),
            [](graph_t g) -> graph_t {
              g.erase_edge(g.cbegin_edges(0) + 1);
              return g;
            }
          },
          {
            graph_description::node_0_0_0_interleaved,
            t.report("Erase node 0"),
            [](graph_t g) -> graph_t {
              g.erase_node(0);
              return g;
            }
          }
        }, // end 'node_1_1_node_1_0_1_0_1_interleaved'
        {  // begin 'node_1_1_node_0_0'
          {
            graph_description::empty,
            t.report("Clear graph"),
            [](graph_t g) -> graph_t {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node,
            t.report("Erase node 0"),
            [](graph_t g) -> graph_t {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node,
            t.report("Erase node 1"),
            [](graph_t g) -> graph_t {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            t.report("Erase node 0 zeroth link"),
            [](graph_t g) -> graph_t {
              g.erase_edge(g.cbegin_edges(0));
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            t.report("Erase node 0 first link"),
            [](graph_t g) -> graph_t {
              g.erase_edge(g.cbegin_edges(0)+1);
              return g;
            }
          }
        }, // end 'node_1_1_node_0_0'
        {  // begin 'node_1pos1_1pos0_node_0pos1_0pos0'
          {
            graph_description::empty,
            t.report("Clear graph"),
            [](graph_t g) -> graph_t {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node,
            t.report("Erase node 0"),
            [](graph_t g) -> graph_t {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node,
            t.report("Erease node 1"),
            [](graph_t g) -> graph_t {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            t.report("Erase node 0 link"),
            [](graph_t g) -> graph_t {
              g.erase_edge(g.cbegin_edges(0));
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            t.report("Erase node 1 link"),
            [](graph_t g) -> graph_t {
              g.erase_edge(g.cbegin_edges(1));
              return g;
            }
          },
        }, // end 'node_1pos1_1pos0_node_0pos1_0pos0'
        {  // begin 'node_node_node'
          {
            graph_description::empty,
            t.report("Clear graph"),
            [](graph_t g) -> graph_t {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node_node,
            t.report("Erase node 0"),
            [](graph_t g) -> graph_t {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_node,
            t.report("Erase node 1"),
            [](graph_t g) -> graph_t {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_node,
            t.report("Erase node 2"),
            [](graph_t g) -> graph_t {
              g.erase_node(2);
              return g;
            }
          },
          {
            graph_description::node_1_node_0_node,
            t.report("Join {0,1}"),
            [](graph_t g) -> graph_t {
              g.join(0,1);
              return g;
            }
          },
          {
            graph_description::node_node_2_node_1,
            t.report("Join {2,1}"),
            [](graph_t g) -> graph_t {
              g.join(2,1);
              return g;
            }
          }
        }, // end 'node_node_node'
        {  // begin 'node_1_node_0_node'
          {
            graph_description::empty,
            t.report("Clear graph"),
            [](graph_t g) -> graph_t {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node_node,
            t.report("Erase node 0"),
            [](graph_t g) -> graph_t {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_node,
            t.report("Erase node 1"),
            [](graph_t g) -> graph_t {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            t.report("Erase node 2"),
            [](graph_t g) -> graph_t {
              g.erase_node(2);
              return g;
            }
          },
          {
            graph_description::node_node_node,
            t.report("Remove link {0,1}"),
            [](graph_t g) -> graph_t {
              g.erase_edge(g.cbegin_edges(0));
              return g;
            }
          },
          {
            graph_description::node_1_node_0_2_node_1,
            t.report("Join {2,1}"),
            [](graph_t g) -> graph_t {
              g.join(1,2);
              return g;
            }
          },
          {
            graph_description::node_1_node_0_2_node_1,
            t.report("Join {2,1}"),
            [](graph_t g) -> graph_t {
              g.join(2,1);
              return g;
            }
          },
          {
            graph_description::node_node_2_node_1,
            t.report("Swap nodes {0,2}"),
            [](graph_t g) -> graph_t {
              g.swap_nodes(0,2);
              return g;
            }
          },
          {
            graph_description::node_node_2_node_1,
            t.report("Swap nodes {2,0}"),
            [](graph_t g) -> graph_t {
              g.swap_nodes(2,0);
              return g;
            }
          }
        }, // end 'node_1_node_0_node'
        {  // begin 'node_node_2_node_1'
          {
            graph_description::empty,
            t.report("Clear graph"),
            [](graph_t g) -> graph_t {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            t.report("Erase node 0"),
            [](graph_t g) -> graph_t {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_node,
            t.report("Erase node 1"),
            [](graph_t g) -> graph_t {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_node,
            t.report("Erase node 2"),
            [](graph_t g) -> graph_t {
              g.erase_node(2);
              return g;
            }
          },
          {
            graph_description::node_node_node,
            t.report("Remove link {2,1}"),
            [](graph_t g) -> graph_t {
              g.erase_edge(g.cbegin_edges(2));
              return g;
            }
          },
          {
            graph_description::node_1_node_0_2_node_1,
            t.report("Join {0,1}"),
            [](graph_t g) -> graph_t {
              g.insert_join(g.cbegin_edges(0), g.cbegin_edges(1));
              return g;
            }
          },
          {
            graph_description::node_1_node_0_node,
            t.report("Swap nodes {0,2}"),
            [](graph_t g) -> graph_t {
              g.swap_nodes(0,2);
              return g;
            }
          },
          {
            graph_description::node_1_node_0_node,
            t.report("Swap nodes {2,0}"),
            [](graph_t g) -> graph_t {
              g.swap_nodes(2,0);
              return g;
            }
          }
        }, // end 'node_node_2_node_1'
        {  // begin 'node_1_node_0_2_node_1'
          {
            graph_description::empty,
            t.report("Clear graph"),
            [](graph_t g) -> graph_t {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            t.report("Erase node 0"),
            [](graph_t g) -> graph_t {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_node,
            t.report("Erase node 1"),
            [](graph_t g) -> graph_t {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            t.report("Erase node 2"),
            [](graph_t g) -> graph_t {
              g.erase_node(2);
              return g;
            }
          },
          {
            graph_description::node_1_node_0_node,
            t.report("Remove link {1,2}"),
            [](graph_t g) -> graph_t {
              g.erase_edge(++g.cbegin_edges(1));
              return g;
            }
          }
        }, // end 'node_1_node_0_2_node_1'
        {  // begin 'node_1_node_0_2_node_1_0'
          {
            graph_description::empty,
            t.report("Clear graph"),
            [](graph_t g) -> graph_t {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            t.report("Erase node 0"),
            [](graph_t g) -> graph_t {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            t.report("Erase node 1"),
            [](graph_t g) -> graph_t {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_1_node_0,
            t.report("Erase node 2"),
            [](graph_t g) -> graph_t {
              g.erase_node(2);
              return g;
            }
          },
          {
            graph_description::node_1_node_0_2_node_1,
            t.report("Remove {2,0}"),
            [](graph_t g) -> graph_t {
              g.erase_edge(++g.cbegin_edges(2));
              return g;
            }
          }
        }, // end 'node_1_node_0_2_node_1_0'
        {  // begin 'node_node_1_node'
          {
            graph_description::empty,
            t.report("Clear graph"),
            [](graph_t g) -> graph_t {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node_0_node,
            t.report("Erase node 0"),
            [](graph_t g) -> graph_t {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_node,
            t.report("Erase node 1"),
            [](graph_t g) -> graph_t {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_node_1,
            t.report("Erase node 2"),
            [](graph_t g) -> graph_t {
              g.erase_node(2);
              return g;
            }
          },
        }, // end 'node_node_1_node'
        {  // begin 'node_1_node_0_1_node'
          {
            graph_description::node_0_node,
            t.report("Erase node 0"),
            [](graph_t g) -> graph_t {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_node,
            t.report("Erase node 1"),
            [](graph_t g) -> graph_t {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_node_1_node,
            t.report("Remove {0,1}"),
            [](graph_t g) -> graph_t {
              g.erase_edge(g.cbegin_edges(0));
              return g;
            }
          },
          {
            graph_description::node_1_node_1_0_node,
            t.report("Join {1,0}"),
            [](graph_t g) -> graph_t {
              g.join(1, 0);
              return g;
            }
          }
        }, // end 'node_1_node_0_1_node'
        {  // begin 'node_1_node_1_0_node'
          {
            graph_description::node_0_node,
            t.report("Erase node 0"),
            [](graph_t g) -> graph_t {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_node,
            t.report("Erase node 1"),
            [](graph_t g) -> graph_t {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_1_node_0_1_node,
            t.report("Remove {1,0}"),
            [](graph_t g) -> graph_t {
              g.erase_edge(g.cbegin_edges(1)+3);
              return g;
            }
          },
          {
            graph_description::node_1_node_1_0_2_node_1,
            t.report("Join {1,2}"),
            [](graph_t g) -> graph_t {
              g.join(1, 2);
              return g;
            }
          }
        }, // end 'node_1_node_1_0_node'
        {  // begin 'node_1_node_1_0_2_node_1'
          {
            graph_description::node_0_1_node_0,
            t.report("Erase node 0"),
            [](graph_t g) -> graph_t {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_node,
            t.report("Erase node 1"),
            [](graph_t g) -> graph_t {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_1_node_0_1_2_node_1,
            t.report("Remove {1,0}"),
            [](graph_t g) -> graph_t {
              g.erase_edge(g.cbegin_edges(1)+3);
              return g;
            }
          },
          {
            graph_description::node_1_node_1_0_node,
            t.report("Remove {1,2}"),
            [](graph_t g) -> graph_t {
              g.erase_edge(g.cbegin_edges(1)+4);
              return g;
            }
          }
        }, // end 'node_1_node_1_0_2_node_1'
        {  // begin 'node_1_node_0_1_2_node_1'
          {
            graph_description::node_1_node_0_2_node_1,
            t.report("Remove {1,1}"),
            [](graph_t g) -> graph_t {
              g.erase_edge(g.cbegin_edges(1)+2);
              return g;
            }
          }
        }, // end 'node_1_node_0_1_2_node_1'
        {  // begin 'node_2_node_node_0_2'
          {
            graph_description::node_1_node_0_1_node,
            t.report("Swap {1,2}"),
            [](graph_t g) -> graph_t {
              g.swap_nodes(1,2);
              return g;
            }
          }
        }, // end 'node_2_node_node_0_2'
        {  // begin 'node_1_1_2_2_node_0_2_0_node_0_1_0'
          {
            graph_description::node_1_node_0,
            t.report("Erase node 0"),
            [](graph_t g) -> graph_t {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_1_1_node_0_0,
            t.report("Erase node 1"),
            [](graph_t g) -> graph_t {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_1_1_node_0_0,
            t.report("Erase node 2"),
            [](graph_t g) -> graph_t {
              g.erase_node(2);
              return g;
            }
          },
          {
            graph_description::node_1_1_2_2_node_0_2_2_0_node_0_1_1_0,
            t.report("Insert Join {2, 1}"),
            [](graph_t g) -> graph_t {
              g.insert_join(g.cbegin_edges(2)+1, g.cbegin_edges(1)+2);
              return g;
            }
          }
        }, // end 'node_1_1_2_2_node_0_2_0_node_0_1_0'
        {  // begin 'node_1_1_2_2_node_0_2_2_0_node_0_1_1_0'
          {
            graph_description::node_1pos1_1pos0_node_0pos1_0pos0,
            t.report("Erase node 0"),
            [](graph_t g) -> graph_t {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_1_1_node_0_0,
            t.report("Erase node 1"),
            [](graph_t g) -> graph_t {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_1_1_node_0_0,
            t.report("Erase node 2"),
            [](graph_t g) -> graph_t {
              g.erase_node(2);
              return g;
            }
          },
          {
            graph_description::node_2_2_1_1_node_2pos3_node_1,
            t.report("Swap {1,2}"),
            [](graph_t g) -> graph_t {
              g.swap_nodes(1,2);
              return g;
            }
          },
          {
            graph_description::node_2_2_1_1_node_2pos3_node_1,
            t.report("Swap {2,1}"),
            [](graph_t g) -> graph_t {
              g.swap_nodes(2,1);
              return g;
            }
          }
        }, // end 'node_1_1_2_2_node_0_2_2_0_node_0_1_1_0'
        {  // begin node_2_2_1_1_node_0_2_2_0_node_0_1_1_0
          {
            graph_description::node_1_1_2_2_node_0_2_2_0_node_0_1_1_0,
            t.report("Swap {1,2}"),
            [](graph_t g) -> graph_t {
              g.swap_nodes(1,2);
              return g;
            }
          },
          {
            graph_description::node_1_1_2_2_node_0_2_2_0_node_0_1_1_0,
            t.report("Swap {2,1}"),
            [](graph_t g) -> graph_t {
              g.swap_nodes(2,1);
              return g;
            }
          }
        }, // end node_2_2_1_1_node_0_2_2_0_node_0_1_1_0
        {  // begin 'node_3_1_node_0_2_node_1_node_0'
          {
             graph_description::node_1_node_0_node,
             t.report("Erase node 0"),
             [](graph_t g) -> graph_t {
               g.erase_node(0);
               return g;
             }
          }
        }, // end 'node_3_1_node_0_2_node_1_node_0'
        {  // begin 'node_1_node_0_node_3_node_2'
          {
            graph_description::node_2_node_3_node_0_node_1,
            t.report("Swap {2,1}"),
            [](graph_t g) -> graph_t {
              g.swap_nodes(2,1);
              return g;
            }
          },
          {
            graph_description::node_2_node_3_node_0_node_1,
            t.report("Swap {0,3}"),
            [](graph_t g) -> graph_t {
              g.swap_nodes(0,3);
              return g;
            }
          }
        }, // end 'node_1_node_0_node_3_node_2'
        {  // begin 'node_2_node_3_node_0_node_1'
          {
            graph_description::node_1_node_0_node_3_node_2,
            t.report("Swap {2,1}"),
            [](graph_t g) -> graph_t {
              g.swap_nodes(2,1);
              return g;
            }
          },
          {
            graph_description::node_1_node_0_node_3_node_2,
            t.report("Swap {0,3}"),
            [](graph_t g) -> graph_t {
              g.swap_nodes(0,3);
              return g;
            }
          }
        }, // end 'node_2_node_3_node_0_node_1'
      },
      {
        //  'empty'
        make_and_check(t, t.report(""), {}),

        //  'node'
        make_and_check(t, t.report(""), {{}}),

        //  'node_0'
        make_and_check(t, t.report(""), {{edge_t{0, 1}, edge_t{0, 0}}}),

        //  'node_0_0'
        make_and_check(t, t.report(""), {{edge_t{0, 1}, edge_t{0, 0}, edge_t{0, 3}, edge_t{0, 2}}}),

        // 'node_0_0_interleaved'
        make_and_check(t, t.report(""), {{edge_t{0, 2}, edge_t{0, 3}, edge_t{0, 0}, edge_t{0, 1}}}),

        // 'node_0_0_0_interleaved'
        make_and_check(t, t.report(""), {{edge_t{0, 1}, edge_t{0, 0}, edge_t{0, 4}, edge_t{0, 5}, edge_t{0, 2}, edge_t{0, 3}}}),

        //  'node_node'
        make_and_check(t, t.report(""), {{}, {}}),

        //  'node_1_node_0'
        make_and_check(t, t.report(""), {{edge_t{1, 0}}, {edge_t{0, 0}}}),

        //  'node_0_1_node_0'
        make_and_check(t, t.report(""), {{edge_t{0, 1}, edge_t{0, 0}, edge_t{1, 0}}, {edge_t{0, 2}}}),

        //  'node_0_node'
        make_and_check(t, t.report(""), {{edge_t{0, 1}, edge_t{0, 0}}, {}}),

        //  'node_node_1'
        make_and_check(t, t.report(""), {{}, {edge_t{1, 1}, edge_t{1, 0}}}),

        // 'node_node_1_1_interleaved'
        make_and_check(t, t.report(""), {{}, {edge_t{1, 2}, edge_t{1, 3}, edge_t{1, 0}, edge_t{1, 1}}}),

        // 'node_node_1_1_1_interleaved'
        make_and_check(t, t.report(""), {{}, {edge_t{1, 1}, edge_t{1, 0}, edge_t{1, 4}, edge_t{1, 5}, edge_t{1, 2}, edge_t{1, 3}}}),

        // 'node_1_node_1_1_1_0_interleaved'
        make_and_check(t, t.report(""), {{edge_t{1, 4}}, {edge_t{1, 1}, edge_t{1, 0}, edge_t{1, 5}, edge_t{1, 6}, edge_t{0, 0}, edge_t{1, 2}, edge_t{1, 3}}}),

        // 'node_1_1_node_1_0_1_0_1_interleaved'
        make_and_check(t, t.report(""), {{edge_t{1, 4}, edge_t{1, 6}},
                                              {edge_t{1, 1}, edge_t{1, 0}, edge_t{1, 5}, edge_t{1, 7}, edge_t{0, 0}, edge_t{1, 2}, edge_t{0, 1}, edge_t{1, 3}}}),

        // 'node_1_1_node_0_0'
        make_and_check(t, t.report(""), {{edge_t{1, 0}, edge_t{1, 1}}, {edge_t{0, 0}, edge_t{0, 1}}}),

        // 'node_1pos1_1pos0_node_0pos1_0pos0'
        make_and_check(t, t.report(""), {{edge_t{1, 1}, edge_t{1, 0}}, {edge_t{0, 1}, edge_t{0, 0}}}),

        //  'node_node_node'
        make_and_check(t, t.report(""), {{}, {}, {}}),

        //  'node_1_node_0_node'
        make_and_check(t, t.report(""), {{edge_t{1, 0}}, {edge_t{0, 0}}, {}}),

        //  'node_node_2_node_1'
        make_and_check(t, t.report(""), {{}, {edge_t{2, 0}}, {edge_t{1, 0}}}),

        // 'node_1_node_0_2_node_1'
        make_and_check(t, t.report(""), {{edge_t{1, 0}}, {edge_t{0, 0}, edge_t{2, 0}}, {edge_t{1, 1}}}),

        // 'node_1_node_0_2_node_1_0'
        make_and_check(t, t.report(""), {{edge_t{1, 0}, edge_t{2, 1}},
                                              {edge_t{0, 0}, edge_t{2, 0}},
                                              {edge_t{1, 1}, edge_t{0, 1}}}),

        // 'node_node_1_node'
        make_and_check(t, t.report(""), {{}, {edge_t{1, 1}, edge_t{1, 0}}, {}}),

        // 'node_1_node_0_1_node'
        make_and_check(t, t.report(""), {{edge_t{1, 0}}, {edge_t{0, 0}, edge_t{1, 2}, edge_t{1, 1}}, {}}),

        // 'node_1_node_1_0_node'
        make_and_check(t, t.report(""), {{edge_t{1, 0}, edge_t{1, 3}},
                                              {edge_t{0, 0}, edge_t{1, 2}, edge_t{1, 1}, edge_t{0, 1}},
                                              {}}),

        // 'node_1_node_1_0_2_node_1'
        make_and_check(t, t.report(""), {{edge_t{1, 0}, edge_t{1, 3}},
                                              {edge_t{0, 0}, edge_t{1, 2}, edge_t{1, 1}, edge_t{0, 1}, edge_t{2, 0}},
                                              {edge_t{1, 4}}}),

        // 'node_1_node_0_1_2_node_1'
        make_and_check(t, t.report(""), {{edge_t{1, 0}},
                                              {edge_t{0, 0}, edge_t{1, 2}, edge_t{1, 1}, edge_t{2, 0}},
                                              {edge_t{1, 3}}}),

        // 'node_2_node_node_0_2'
        make_and_check(t, t.report(""), {{edge_t{2, 0}}, {}, {edge_t{0, 0}, edge_t{2, 2}, edge_t{2, 1}}}),

        // 'node_1_1_2_2_node_0_2_0_node_0_1_0'
        make_and_check(t, t.report(""), {{edge_t{1, 0}, edge_t{1, 2}, edge_t{2, 0}, edge_t{2, 2}},
                                              {edge_t{0, 0}, edge_t{2, 1}, edge_t{0, 1}},
                                              {edge_t{0, 2}, edge_t{1, 1}, edge_t{0, 3}}}),

        // 'node_1_1_2_2_node_0_2_2_0_node_0_1_1_0'
        make_and_check(t, t.report(""), {{edge_t{1, 0}, edge_t{1, 3}, edge_t{2, 0}, edge_t{2, 3}},
                                              {edge_t{0, 0}, edge_t{2, 2}, edge_t{2, 1}, edge_t{0, 1}},
                                              {edge_t{0, 2}, edge_t{1, 2}, edge_t{1, 1}, edge_t{0, 3}}}),

        // node_2_2_1_1_node_0_2_2_0_node_0_1_1_0
        make_and_check(t, t.report(""), {{edge_t{2, 0}, edge_t{2, 3}, edge_t{1, 0}, edge_t{1, 3}},
                                              {edge_t{0, 2}, edge_t{2, 2}, edge_t{2, 1}, edge_t{0, 3}},
                                              {edge_t{0, 0}, edge_t{1, 2}, edge_t{1, 1}, edge_t{0, 1}}}),

        // 'node_3_1_node_0_2_node_1_node_0'
        make_and_check(t, t.report(""), {{edge_t{3, 0},edge_t{1, 0}},
                                              {edge_t{0, 1}, edge_t{2, 0}},
                                              {edge_t{1, 1}},
                                              {edge_t{0, 0}}}),

        // 'node_1_node_0_node_3_node_2'
        make_and_check(t, t.report(""), {{edge_t{1, 0}}, {edge_t{0, 0}}, {edge_t{3, 0}}, {edge_t{2, 0}}}),

        // 'node_2_node_3_node_0_node_1'
        make_and_check(t, t.report(""), {{edge_t{2, 0}}, {edge_t{3, 0}}, {edge_t{0, 0}}, {edge_t{1, 0}}})
      }
      };
    }
  };
}