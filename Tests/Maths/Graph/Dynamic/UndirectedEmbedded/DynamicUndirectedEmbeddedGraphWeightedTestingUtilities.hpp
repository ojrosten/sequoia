////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "DynamicUndirectedEmbeddedGraphTestingUtilities.hpp"

namespace sequoia::testing
{
  namespace undirected_embedded_graph{
    /// Convention: the indices following 'node' - separated by underscores - give the target node of the associated edges
    enum weighted_graph_description : std::size_t {
      // x
      nodew = graph_description::end,

      //  /\
      //  \/
      //  x
      nodew_0,

      //  /\
      //  \/
      //  x
      node_0w,

      //  /\ /\
      //  \/ \/
      //    x
      node_0w_0w,

      //  /\ /\
      //  \/ \/
      //    x
      node_0_0w,

      //  /\ /\
      //  \/ \/
      //    x
      node_0w_0,

      //     /-\/-\
      //    /  /\  \
      //    \ /  \ /
      //        x
      node_0w_0_interleaved,

      //     /-\/-\
      //    /  /\  \
      //    \ /  \ /
      //        x
      node_0_0w_interleaved,

      //     /-\/-\
      //    /  /\  \
      //    \ /  \ /
      //        x
      node_0w_0w_interleaved,

      //  x    x
      node_nodew,

      //  x    x
      nodew_node,

      //  x ---- x
      node_1_nodew_0,

      //  x ---- x
      nodew_1_node_0,

      //  x ==== x
      node_1_1w_node_0_0w,

      //  x ==== x
      node_1w_1w_node_0w_0w,

      // x ---- x
      //   ----
      node_1w_1_node_0_0w,

      // x ---- x
      //   ----
      node_1_1w_node_0w_0,

      //    ----
      //  x ==== x
      node_1_1w_1x_node_0_0w_0x,

      // /-\
      // \ /
      //  x ==== x
      //    ----
      node_0y_1_1w_1x_node_0_0w_0x,
    };
  }

  template
  <
    class EdgeWeight,
    class NodeWeight,
    class EdgeStorageConfig,
    class NodeWeightStorage
  >
  class dynamic_undirected_embedded_graph_weighted_operations
  {
   public:
    using graph_t            = maths::embedded_graph<EdgeWeight, NodeWeight, maths::null_meta_data, EdgeStorageConfig, NodeWeightStorage>;
    using edge_t             = typename graph_t::edge_init_type;
    using node_weight_type   = typename graph_t::node_weight_type;
    using edges_equivalent_t = std::initializer_list<std::initializer_list<edge_t>>;
    using transition_graph   = typename transition_checker<graph_t>::transition_graph;

    static void execute_operations(regular_test& t)
    {
      auto trg{make_weighted_transition_graph(t)};

      auto checker{
          [&t](std::string_view description, const graph_t& obtained, const graph_t& prediction, const graph_t& parent, std::size_t host, std::size_t target) {
            t.check(equality, {description, no_source_location}, obtained, prediction);
            if(host != target) t.check_semantics({description, no_source_location}, prediction, parent);
          }
      };

      transition_checker<graph_t>::check(t.report(""), trg, checker);
    }

    [[nodiscard]]
    static graph_t make_and_check(regular_test& t, std::string_view description, edges_equivalent_t edgeInit, std::initializer_list<node_weight_type> nodeInit)
    {
      return graph_initialization_checker<graph_t>::make_and_check(t, description, edgeInit, nodeInit);
    }

    static void check_initialization_exceptions(regular_test& t)
    {
      using nodes = std::initializer_list<node_weight_type>;

      // One node
      t.check_exception_thrown<std::logic_error>("Mismatched weights", [](){ return graph_t{{{0, 1, 1.0}, {0, 0, 0.0}}}; });
 
      // Two nodes
      t.check_exception_thrown<std::logic_error>("IMismatched weights", [](){ return graph_t{{{1, 0, 1.0}}, {{0, 0, 2.0}}}; });

      t.check_exception_thrown<std::logic_error>("Mismatched edge/node initialization", [](){ return graph_t{{}, nodes{1.0}}; });
      t.check_exception_thrown<std::logic_error>("Mismatched edge/node initialization", [](){ return graph_t{{{}}, nodes{1.0, 2.0}}; });
      t.check_exception_thrown<std::logic_error>("Mismatched edge/node initialization", [](){ return graph_t{{{edge_t{0, 1, 1.0}, edge_t{0, 0, 1.0}}}, nodes{1.0, 2.0}}; });
      t.check_exception_thrown<std::logic_error>("Mismatched edge/node initialization", [](){ return graph_t{{{}, {}}, nodes{1.0}}; });
      t.check_exception_thrown<std::logic_error>("Mismatched edge/node initialization", [](){ return graph_t{{{edge_t{1, 0}}, {edge_t{0, 0}}}, nodes{1.0}}; });
    }
    

    [[nodiscard]]
    static transition_graph make_weighted_transition_graph(regular_test& t)
    {
      using base_ops = dynamic_undirected_embedded_graph_operations<EdgeWeight, NodeWeight, EdgeStorageConfig, NodeWeightStorage>;
      using namespace undirected_embedded_graph;

      auto trg{base_ops::make_transition_graph(t)};

      check_initialization_exceptions(t);

      // 'weighted_graph_description::nodew'
      trg.add_node(make_and_check(t, t.report(""), {{}}, {1.0}));

      // 'weighted_graph_description::nodew_0'
      trg.add_node(make_and_check(t, t.report(""), {{{0, 1, 0.0}, {0, 0, 0.0}}}, {1.0}));

      // 'weighted_graph_description::node_0w'
      trg.add_node(make_and_check(t, t.report(""), {{{0, 1, 1.0}, {0, 0, 1.0}}}, {0.0}));

      // 'weighted_graph_description::node_0w_0w'
      trg.add_node(make_and_check(t, t.report(""), {{{0, 1, 1.0}, {0, 0, 1.0}, {0, 3, 1.0}, {0, 2, 1.0}}}, {0.0}));

      // 'weighted_graph_description::node_0_0w'
      trg.add_node(make_and_check(t, t.report(""), {{{0, 1, 0.0}, {0, 0, 0.0}, {0, 3, 1.0}, {0, 2, 1.0}}}, {0.0}));

      // 'weighted_graph_description::node_0w_0'
      trg.add_node(make_and_check(t, t.report(""), {{{0, 1, 1.0}, {0, 0, 1.0}, {0, 3, 0.0}, {0, 2, 0.0}}}, {0.0}));

      // 'weighted_graph_description::node_0w_0_interleaved'
      trg.add_node(make_and_check(t, t.report(""), {{{0, 2, 1.0}, {0, 3, 0.0}, {0, 0, 1.0}, {0, 1, 0.0}}}, {0.0}));

      // 'weighted_graph_description::node_0_0w_interleaved'
      trg.add_node(make_and_check(t, t.report(""), {{{0, 2, 0.0}, {0, 3, 1.0}, {0, 0, 0.0}, {0, 1, 1.0}}}, {0.0}));

      // 'weighted_graph_description::node_0w_0w_interleaved'
      trg.add_node(make_and_check(t, t.report(""), {{{0, 2, 1.0}, {0, 3, 1.0}, {0, 0, 1.0}, {0, 1, 1.0}}}, {0.0}));

      // 'weighted_graph_description::node_nodew'
      trg.add_node(make_and_check(t, t.report(""), {{}, {}}, {0.0, 1.0}));

      // 'weighted_graph_description::nodew_node'
      trg.add_node(make_and_check(t, t.report(""), {{}, {}}, {1.0, 0.0}));

      // 'weighted_graph_description::node_1_nodew_0'
      trg.add_node(make_and_check(t, t.report(""), {{{1, 0, 0.0}}, {{0, 0, 0.0}}}, {0.0, 1.0}));

      // 'weighted_graph_description::nodew_1_node_0'
      trg.add_node(make_and_check(t, t.report(""), {{{1, 0, 0.0}}, {{0, 0, 0.0}}}, {1.0, 0.0}));

      // 'weighted_graph_description::node_1_1w_node_0_0w'
      trg.add_node(make_and_check(t, t.report(""), {{{1, 0, 0.0}, {1, 1, 1.0}}, {{0, 0, 0.0}, {0, 1, 1.0}}}, {0.0, 0.0}));

      // 'weighted_graph_description::node_1w_1w_node_0w_0w'
      trg.add_node(make_and_check(t, t.report(""), {{{1, 0, 1.0}, {1, 1, 1.0}}, {{0, 0, 1.0}, {0, 1, 1.0}}}, {0.0, 0.0}));

      // 'weighted_graph_description::node_1w_1_node_0_0w'
      trg.add_node(make_and_check(t, t.report(""), {{{1, 1, 1.0}, {1, 0, 0.0}}, {{0, 1, 0.0}, {0, 0, 1.0}}}, {0.0, 0.0}));

      // 'weighted_graph_description::node_1_1w_node_0w_0'
      trg.add_node(make_and_check(t, t.report(""), {{{1, 1, 0.0}, {1, 0, 1.0}}, {{0, 1, 1.0}, {0, 0, 0.0}}}, {0.0, 0.0}));

      // 'weighted_graph_description::node_1_1w_1x_node_0_0w_0x'
      trg.add_node(make_and_check(t, t.report(""), {{{1, 0, 0.0}, {1, 1, 1.0}, {1, 2, 2.0}}, {{0, 0, 0.0}, {0, 1, 1.0}, {0, 2, 2.0}}}, {0.0, 0.0}));

      // 'weighted_graph_description::node_0y_1_1w_1x_node_0_0w_0x'
      trg.add_node(make_and_check(t, t.report(""), {{{0, 1, 3.0}, {0, 0, 3.0}, {1, 0, 0.0}, {1, 1, 1.0}, {1, 2, 2.0}}, {{0, 2, 0.0}, {0, 3, 1.0}, {0, 4, 2.0}}}, {0.0, 0.0}));

      // begin 'graph_description::empty'

      trg.join(
        graph_description::empty,
        graph_description::empty,
        t.report(""),
        [&t](graph_t g) -> graph_t {
          t.check_exception_thrown<std::out_of_range>("Attempt to set a node weight which does not exist", [&g](){ g.set_node_weight(g.cbegin_node_weights(), 1.0); });
          return g;
        }
      );

      trg.join(
        graph_description::empty,
        graph_description::empty,
        t.report("Attempt to mutate a node weight which does not exist"),
        [&t](graph_t g) -> graph_t {
          t.check_exception_thrown<std::out_of_range>("Attempt to mutate a node weight which does not exist", [&g](){ g.mutate_node_weight(g.cbegin_node_weights(), [](double&){}); });
          return g;
        }
      );

      trg.join(
        graph_description::empty,
        weighted_graph_description::nodew,
        t.report("Add weighted node"),
        [](graph_t g) -> graph_t {
          g.add_node(1.0);
          return g;
        }
      );

      trg.join(
        graph_description::empty,
        weighted_graph_description::nodew,
        t.report("Insert weighted node"),
        [](graph_t g) -> graph_t {
          g.insert_node(0, 1.0);
          return g;
        }
      );

      // end 'graph_description::empty'

      // begin 'graph_description::node'

      trg.join(
        graph_description::node,
        graph_description::node,
        t.report(""),
        [&t](graph_t g) -> graph_t {
          t.check_exception_thrown<std::out_of_range>("Attempt to set a node weight which does not exist", [&g](){ g.set_node_weight(g.cend_node_weights(), 1.0); });
          return g;
        }
      );

      trg.join(
        graph_description::node,
        graph_description::node,
        t.report("Attempt to mutate a node weight which does not exist"),
        [&t](graph_t g) -> graph_t {
          t.check_exception_thrown<std::out_of_range>("Attempt to mutate a node weight which does not exist", [&g](){ g.mutate_node_weight(g.cend_node_weights(), [](double&){}); });
          return g;
        }
      );

      trg.join(
        graph_description::node,
        weighted_graph_description::nodew,
        t.report("Change node weight"),
        [](graph_t g) -> graph_t {
          g.set_node_weight(g.cbegin_node_weights(), 1.0);
          return g;
        }
      );

      trg.join(
        graph_description::node,
        weighted_graph_description::nodew,
        t.report("Mutate node weight"),
        [](graph_t g) -> graph_t {
          g.mutate_node_weight(g.cbegin_node_weights(), [](double& x) { x += 1.0; });
          return g;
        }
      );

      trg.join(
        graph_description::node,
        weighted_graph_description::nodew_node,
        t.report("Insert weighted node"),
        [](graph_t g) -> graph_t {
          g.insert_node(0, 1.0);
          return g;
        }
      );

      trg.join(
        graph_description::node,
        weighted_graph_description::node_0w,
        t.report("Join {0,0}"),
        [](graph_t g) -> graph_t {
          g.join(0, 0, 1.0);
          return g;
        }
      );

      // end 'graph_description::node'

      // begin 'graph_description::node_0'

      trg.join(
        graph_description::node_0,
        weighted_graph_description::node_0w,
        t.report("Set edge weight"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(g.cbegin_edges(0), 1.0);
          return g;
        }
      );

      trg.join(
        graph_description::node_0,
        weighted_graph_description::node_0w,
        t.report("Set edge weight via second insertion"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(++g.cbegin_edges(0), 1.0);
          return g;
        }
      );

      trg.join(
        graph_description::node_0,
        weighted_graph_description::node_0w,
        t.report("Mutate edge weight"),
        [](graph_t g) -> graph_t {
          g.mutate_edge_weight(g.cbegin_edges(0), [](double& x){ x += 1.0; });
          return g;
        }
      );

      trg.join(
        graph_description::node_0,
        weighted_graph_description::node_0w,
        t.report("Mutate edge weight via second insertion"),
        [](graph_t g) -> graph_t {
          g.mutate_edge_weight(++g.cbegin_edges(0), [](double& x){ x += 1.0; });
          return g;
        }
      );

      trg.join(
        graph_description::node_0,
        weighted_graph_description::node_0_0w,
        t.report("Join {0,0}"),
        [](graph_t g) -> graph_t {
          g.join(0, 0, 1.0);
          return g;
        }
      );

      trg.join(
        graph_description::node_0,
        weighted_graph_description::node_0w_0_interleaved,
        t.report("Insert weighted join {0,0}"),
        [](graph_t g) -> graph_t {
          g.insert_join(g.cbegin_edges(0), 2, 1.0);
          return g;
        }
      );

      trg.join(
        graph_description::node_0,
        weighted_graph_description::node_0_0w_interleaved,
        t.report("Insert weighted join {0,0}"),
        [](graph_t g) -> graph_t {
          g.insert_join(g.cbegin_edges(0)+1, 3, 1.0);
          return g;
        }
      );

      // end 'graph_description::node_0'

      // begin 'graph_description::node_0_0'

      trg.join(
        graph_description::node_0_0,
        weighted_graph_description::node_0w_0,
        t.report("Set edge weight via zeroth partial weight"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(g.cbegin_edges(0), 1.0);
          return g;
        }
      );

      trg.join(
        graph_description::node_0_0,
        weighted_graph_description::node_0w_0,
        t.report("Set edge weight via first partial weight"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(g.cbegin_edges(0) + 1, 1.0);
          return g;
        }
      );

      trg.join(
        graph_description::node_0_0,
        weighted_graph_description::node_0_0w,
        t.report("Set edge weight via second partial weight"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(g.cbegin_edges(0) + 2, 1.0);
          return g;
        }
      );

      trg.join(
        graph_description::node_0_0,
        weighted_graph_description::node_0_0w,
        t.report("Set edge weight via third partial weight"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(g.cbegin_edges(0) + 3, 1.0);
          return g;
        }
      );

      trg.join(
        graph_description::node_0_0,
        weighted_graph_description::node_0w_0,
        t.report("Mutate edge weight via zeroth partial weight"),
        [](graph_t g) -> graph_t {
          g.mutate_edge_weight(g.cbegin_edges(0), [](double& x) { x += 1.0; });
          return g;
        }
      );

      trg.join(
        graph_description::node_0_0,
        weighted_graph_description::node_0w_0,
        t.report("Mutate edge weight via first partial weight"),
        [](graph_t g) -> graph_t {
          g.mutate_edge_weight(g.cbegin_edges(0)+1, [](double& x) { x += 1.0; });
          return g;
        }
      );

      trg.join(
        graph_description::node_0_0,
        weighted_graph_description::node_0_0w,
        t.report("Mutate edge weight via second partial weight"),
        [](graph_t g) -> graph_t {
          g.mutate_edge_weight(g.cbegin_edges(0)+2, [](double& x) { x += 1.0; });
          return g;
        }
      );

      trg.join(
        graph_description::node_0_0,
        weighted_graph_description::node_0_0w,
        t.report("Mutate edge weight via third partial weight"),
        [](graph_t g) -> graph_t {
          g.mutate_edge_weight(g.cbegin_edges(0)+3, [](double& x) { x += 1.0; });
          return g;
        }
      );

      // end 'graph_description::node_0_0'

      // begin 'graph_description::node_0_0_interleaved'

      trg.join(
        graph_description::node_0_0_interleaved,
        weighted_graph_description::node_0w_0_interleaved,
        t.report("Set edge weight via zeroth partial weight"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(g.cbegin_edges(0), 1.0);
          return g;
        }
      );

      trg.join(
        graph_description::node_0_0_interleaved,
        weighted_graph_description::node_0_0w_interleaved,
        t.report("Set edge weight via first partial weight"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(++g.cbegin_edges(0), 1.0);
          return g;
        }
      );

      trg.join(
        graph_description::node_0_0_interleaved,
        weighted_graph_description::node_0w_0_interleaved,
        t.report("Set edge weight via second partial weight"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(g.cbegin_edges(0)+2, 1.0);
          return g;
        }
      );

      trg.join(
        graph_description::node_0_0_interleaved,
        weighted_graph_description::node_0_0w_interleaved,
        t.report("Set edge weight via third partial weight"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(g.cbegin_edges(0)+3, 1.0);
          return g;
        }
      );

      trg.join(
        graph_description::node_0_0_interleaved,
        weighted_graph_description::node_0w_0_interleaved,
        t.report("Muteate edge weight via zeroth partial weight"),
        [](graph_t g) -> graph_t {
          g.mutate_edge_weight(g.cbegin_edges(0), [](double& x) { x += 1.0; });
          return g;
        }
      );

      trg.join(
        graph_description::node_0_0_interleaved,
        weighted_graph_description::node_0_0w_interleaved,
        t.report("Mutate edge weight via first partial weight"),
        [](graph_t g) -> graph_t {
          g.mutate_edge_weight(++g.cbegin_edges(0), [](double& x) { x += 1.0; });
          return g;
        }
      );

      trg.join(
        graph_description::node_0_0_interleaved,
        weighted_graph_description::node_0w_0_interleaved,
        t.report("Mutate edge weight via second partial weight"),
        [](graph_t g) -> graph_t {
          g.mutate_edge_weight(g.cbegin_edges(0) + 2, [](double& x) { x += 1.0; });
          return g;
        }
      );

      trg.join(
        graph_description::node_0_0_interleaved,
        weighted_graph_description::node_0_0w_interleaved,
        t.report("Mutate edge weight via third partial weight"),
        [](graph_t g) -> graph_t {
          g.mutate_edge_weight(g.cbegin_edges(0) + 3, [](double& x) { x += 1.0; });
          return g;
        }
      );

      // end 'graph_description::node_0_0_interleaved'

      // begin 'graph_description::node_1_node_0'

      trg.join(
        graph_description::node_1_node_0,
        weighted_graph_description::node_1w_1_node_0_0w,
        t.report("Inserted braided join"),
        [](graph_t g) -> graph_t {
          g.insert_join(g.cbegin_edges(0), g.cbegin_edges(1)+1, 1.0);
          return g;
        }
      );

      trg.join(
        graph_description::node_1_node_0,
        weighted_graph_description::node_1_1w_node_0w_0,
        t.report("Inserted braided join"),
        [](graph_t g) -> graph_t {
          g.insert_join(g.cbegin_edges(0) + 1, g.cbegin_edges(1), 1.0);
          return g;
        }
      );

      // end 'graph_description::node_1_node_0'

      // begin 'graph_description::node_1_1_node_0_0'

      trg.join(
        graph_description::node_1_1_node_0_0,
        weighted_graph_description::node_1_1w_node_0_0w,
        t.report("Set edge weight via node 0, first partial edge"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(++g.cbegin_edges(0), 1.0);
          return g;
        }
      );

      trg.join(
        graph_description::node_1_1_node_0_0,
        weighted_graph_description::node_1_1w_node_0_0w,
        t.report("Set edge weight via node 1, first partial edge"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(++g.cbegin_edges(1), 1.0);
          return g;
        }
      );

      trg.join(
        graph_description::node_1_1_node_0_0,
        weighted_graph_description::node_1_1w_node_0_0w,
        t.report("Mutate edge weight via node 0, first partial edge"),
        [](graph_t g) -> graph_t {
          g.mutate_edge_weight(++g.cbegin_edges(0), [](double& x) { x += 1.0; });
          return g;
        }
      );

      trg.join(
        graph_description::node_1_1_node_0_0,
        weighted_graph_description::node_1_1w_node_0_0w,
        t.report("Mutate edge weight via node 1, first partial edge"),
        [](graph_t g) -> graph_t {
          g.mutate_edge_weight(++g.cbegin_edges(1), [](double& x) { x += 1.0; });
          return g;
        }
      );

      // end 'graph_description::node_1_1_node_0_0'

      // begin 'graph_description::node_1pos1_1pos0_node_0pos1_0pos0'

      trg.join(
        graph_description::node_1pos1_1pos0_node_0pos1_0pos0,
        weighted_graph_description::node_1w_1_node_0_0w,
        t.report("Set edge weight via node 0, zeroth partial edge"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(g.cbegin_edges(0), 1.0);
          return g;
        }
      );

      trg.join(
        graph_description::node_1pos1_1pos0_node_0pos1_0pos0,
        weighted_graph_description::node_1_1w_node_0w_0,
        t.report("Set edge weight via node 0, first partial edge"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(g.cbegin_edges(0) + 1, 1.0);
          return g;
        }
      );

      trg.join(
        graph_description::node_1pos1_1pos0_node_0pos1_0pos0,
        weighted_graph_description::node_1_1w_node_0w_0,
        t.report("Set edge weight via node 1, zeroth partial edge"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(g.cbegin_edges(1), 1.0);
          return g;
        }
      );

      trg.join(
        graph_description::node_1pos1_1pos0_node_0pos1_0pos0,
        weighted_graph_description::node_1w_1_node_0_0w,
        t.report("Set edge weight via node 1, first partial edge"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(g.cbegin_edges(1) + 1, 1.0);
          return g;
        }
      );

      trg.join(
        graph_description::node_1pos1_1pos0_node_0pos1_0pos0,
        weighted_graph_description::node_1w_1_node_0_0w,
        t.report("Mutate edge weight via node 0, zeroth partial edge"),
        [](graph_t g) -> graph_t {
          g.mutate_edge_weight(g.cbegin_edges(0), [](double& x) { x += 1.0; });
          return g;
        }
      );

      trg.join(
        graph_description::node_1pos1_1pos0_node_0pos1_0pos0,
        weighted_graph_description::node_1_1w_node_0w_0,
        t.report("Mutate edge weight via node 0, first partial edge"),
        [](graph_t g) -> graph_t {
          g.mutate_edge_weight(g.cbegin_edges(0) + 1, [](double& x) { x += 1.0; });
          return g;
        }
      );

      trg.join(
        graph_description::node_1pos1_1pos0_node_0pos1_0pos0,
        weighted_graph_description::node_1_1w_node_0w_0,
        t.report("Mutate edge weight via node 1, zeroth partial edge"),
        [](graph_t g) -> graph_t {
          g.mutate_edge_weight(g.cbegin_edges(1), [](double& x) { x += 1.0; });
          return g;
        }
      );

      trg.join(
        graph_description::node_1pos1_1pos0_node_0pos1_0pos0,
        weighted_graph_description::node_1w_1_node_0_0w,
        t.report("Mutate edge weight via node 1, first partial edge"),
        [](graph_t g) -> graph_t {
          g.mutate_edge_weight(g.cbegin_edges(1) + 1, [](double& x) { x += 1.0; });
          return g;
        }
      );

      // end 'graph_description::node_1pos1_1pos0_node_0pos1_0pos0'

      //======================================= joins from new nodes =======================================//

      // begin 'weighted_graph_description::node_0w'

      trg.join(
        weighted_graph_description::node_0w,
        weighted_graph_description::node_0_0w_interleaved,
        t.report("Insert join {0,0}"),
        [](graph_t g) -> graph_t {
          g.insert_join(g.cbegin_edges(0), 2, 0.0);
          return g;
        }
      );

      trg.join(
        weighted_graph_description::node_0w,
        weighted_graph_description::node_0w_0_interleaved,
        t.report("Insert join {0,0}"),
        [](graph_t g) -> graph_t {
          g.insert_join(g.cbegin_edges(0)+1, 3, 0.0);
          return g;
        }
      );

      // end 'weighted_graph_description::node_0w'

      // begin 'weighted_graph_description::node_0_0w'

      trg.join(
        weighted_graph_description::node_0_0w,
        weighted_graph_description::node_0w,
        t.report("Remove zeroth partial edge"),
        [](graph_t g) -> graph_t {
          g.erase_edge(g.cbegin_edges(0));
          return g;
        }
      );

      trg.join(
        weighted_graph_description::node_0_0w,
        weighted_graph_description::node_0w,
        t.report("Remove first partial edge"),
        [](graph_t g) -> graph_t {
          g.erase_edge(++g.cbegin_edges(0));
          return g;
        }
      );

      trg.join(
        weighted_graph_description::node_0_0w,
        graph_description::node_0,
        t.report("Remove second partial edge"),
        [](graph_t g) -> graph_t {
          g.erase_edge(g.cbegin_edges(0)+2);
          return g;
        }
      );

      trg.join(
        weighted_graph_description::node_0_0w,
        graph_description::node_0,
        t.report("Remove third partial edge"),
        [](graph_t g) -> graph_t {
          g.erase_edge(g.cbegin_edges(0)+3);
          return g;
        }
      );

      // end 'weighted_graph_description::node_0_0w'

      // begin 'graph_description::node_0w_0_interleaved'

      trg.join(
        weighted_graph_description::node_0w_0_interleaved,
        graph_description::node_0_0_interleaved,
        t.report("Set edge weight via zeroth partial weight"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(g.cbegin_edges(0), 0.0);
          return g;
        }
      );

      trg.join(
        weighted_graph_description::node_0w_0_interleaved,
        weighted_graph_description::node_0w_0w_interleaved,
        t.report("Set edge weight via first partial weight"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(++g.cbegin_edges(0), 1.0);
          return g;
        }
      );

      trg.join(
        weighted_graph_description::node_0w_0_interleaved,
        graph_description::node_0_0_interleaved,
        t.report("Set edge weight via second partial weight"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(g.cbegin_edges(0) + 2, 0.0);
          return g;
        }
      );

      trg.join(
        weighted_graph_description::node_0w_0_interleaved,
        weighted_graph_description::node_0w_0w_interleaved,
        t.report("Set edge weight via third partial weight"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(g.cbegin_edges(0) + 3, 1.0);
          return g;
        }
      );

      trg.join(
        weighted_graph_description::node_0w_0_interleaved,
        graph_description::node_0_0_interleaved,
        t.report("Muteate edge weight via zeroth partial weight"),
        [](graph_t g) -> graph_t {
          g.mutate_edge_weight(g.cbegin_edges(0), [](double& x) { x -= 1.0; });
          return g;
        }
      );

      trg.join(
        weighted_graph_description::node_0w_0_interleaved,
        weighted_graph_description::node_0w_0w_interleaved,
        t.report("Mutate edge weight via first partial weight"),
        [](graph_t g) -> graph_t {
          g.mutate_edge_weight(++g.cbegin_edges(0), [](double& x) { x += 1.0; });
          return g;
        }
      );

      trg.join(
        weighted_graph_description::node_0w_0_interleaved,
        graph_description::node_0_0_interleaved,
        t.report("Mutate edge weight via second partial weight"),
        [](graph_t g) -> graph_t {
          g.mutate_edge_weight(g.cbegin_edges(0) + 2, [](double& x) { x -= 1.0; });
          return g;
        }
      );

      trg.join(
        weighted_graph_description::node_0w_0_interleaved,
        weighted_graph_description::node_0w_0w_interleaved,
        t.report("Mutate edge weight via third partial weight"),
        [](graph_t g) -> graph_t {
          g.mutate_edge_weight(g.cbegin_edges(0) + 3, [](double& x) { x += 1.0; });
          return g;
        }
      );

      // end 'graph_description::node_0w_0_interleaved'

      // begin 'weighted_graph_description::node_nodew'

      trg.join(
        weighted_graph_description::node_nodew,
        weighted_graph_description::nodew_node,
        t.report("Swap nodes"),
        [](graph_t g) -> graph_t {
          g.swap_nodes(0, 1);
          return g;
        }
      );

      // end 'weighted_graph_description::node_nodew'

      // begin 'weighted_graph_description::nodew_node'

      trg.join(
        weighted_graph_description::nodew_node,
        weighted_graph_description::node_nodew,
        t.report("Swap nodes"),
        [](graph_t g) -> graph_t {
          g.swap_nodes(1, 0);
          return g;
        }
      );

      // end 'weighted_graph_description::nodew_node'

      // begin 'weighted_graph_description::node_1_nodew_0'

      trg.join(
        weighted_graph_description::node_1_nodew_0,
        weighted_graph_description::nodew_1_node_0,
        t.report("Swap nodes"),
        [](graph_t g) -> graph_t {
          g.swap_nodes(0, 1);
          return g;
        }
      );

      // end 'weighted_graph_description::node_1_nodew_0'

      // begin 'weighted_graph_description::nodew_node_0'

      trg.join(
        weighted_graph_description::nodew_1_node_0,
        weighted_graph_description::node_1_nodew_0,
        t.report("Swap nodes"),
        [](graph_t g) -> graph_t {
          g.swap_nodes(1, 0);
          return g;
        }
      );

      // end 'weighted_graph_description::nodew_node_0'

      // begin 'weighted_graph_description::node_1_1w_node_0_0w'
      
      trg.join(
        weighted_graph_description::node_1_1w_node_0_0w,
        weighted_graph_description::node_1w_1w_node_0w_0w,
        t.report("Set edge weight {0, 1}"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(g.cbegin_edges(0), 1.0);
          return g;
        }
      );

      trg.join(
        weighted_graph_description::node_1_1w_node_0_0w,
        weighted_graph_description::node_1w_1w_node_0w_0w,
        t.report("Set edge weight {1, 0}"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(g.cbegin_edges(1), 1.0);
          return g;
        }
      );

      // end 'weighted_graph_description::node_1_1w_node_0_0w'

      // begin 'weighted_graph_description::node_1w_1w_node_0w_0w'

      trg.join(
        weighted_graph_description::node_1w_1w_node_0w_0w,
        weighted_graph_description::node_1_1w_node_0_0w,
        t.report("Set edge weight {0, 1}"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(g.cbegin_edges(0), 0.0);
          return g;
        }
      );

      trg.join(
        weighted_graph_description::node_1w_1w_node_0w_0w,
        weighted_graph_description::node_1_1w_node_0_0w,
        t.report("Set edge weight {1, 0}"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(g.cbegin_edges(1), 0.0);
          return g;
        }
      );

      trg.join(
        weighted_graph_description::node_1w_1w_node_0w_0w,
        weighted_graph_description::node_1_1w_node_0_0w,
        t.report("Mutate edge weight {0, 1}"),
        [](graph_t g) -> graph_t {
          g.mutate_edge_weight(g.cbegin_edges(0), [](double& x){ x -= 1.0; });
          return g;
        }
      );

      trg.join(
        weighted_graph_description::node_1w_1w_node_0w_0w,
        weighted_graph_description::node_1_1w_node_0_0w,
        t.report("Mutate edge weight {1, 0}"),
        [](graph_t g) -> graph_t {
          g.mutate_edge_weight(g.cbegin_edges(1), [](double& x){ x -= 1.0; });
          return g;
        }
      );

      // end 'weighted_graph_description::node_1w_1w_node_0w_0w'

      // begin 'weighted_graph_description::node_1_1w_1x_node_0_0w_0x'

      trg.join(
        weighted_graph_description::node_1_1w_1x_node_0_0w_0x,
        weighted_graph_description::node_0y_1_1w_1x_node_0_0w_0x,
        t.report("Join {0,0} and sort"),
        [](graph_t g) -> graph_t {
          g.insert_join(g.cbegin_edges(0), 1, 3.0);
          return g;
        }
      );

      // end 'weighted_graph_description::node_1_1w_1x_node_0_0w_0x'

      // begin 'weighted_graph_description::node_0y_1_1w_1x_node_0_0w_0x'

      trg.join(
        weighted_graph_description::node_0y_1_1w_1x_node_0_0w_0x,
        weighted_graph_description::node_1_1w_1x_node_0_0w_0x,
        t.report("Remove zeroth partial edge"),
        [](graph_t g) -> graph_t {
          g.erase_edge(g.cbegin_edges(0));
          return g;
        }
      );

      trg.join(
        weighted_graph_description::node_0y_1_1w_1x_node_0_0w_0x,
        weighted_graph_description::node_1_1w_1x_node_0_0w_0x,
        t.report("Remove first partial edge"),
        [](graph_t g) -> graph_t {
          g.erase_edge(++g.cbegin_edges(0));
          return g;
        }
      );

      // end'weighted_graph_description::node_0y_1_1w_1x_node_0_0w_0x'

      return trg;
    }
  };


}