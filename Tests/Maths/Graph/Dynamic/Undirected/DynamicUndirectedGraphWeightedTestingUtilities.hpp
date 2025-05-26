////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "DynamicUndirectedGraphTestingUtilities.hpp"

namespace sequoia::testing
{
  namespace undirected_graph{
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

      //  x    x
      node_nodew,

      //  x    x
      nodew_node,

      //  x ---- x
      node_1_nodew_0,

      //  x ---- x
      nodew_1_node_0,

      //   /\
      //   \/
      //   x --- x
      node_0w_1_node_0,

      //         /\
      //         \/
      //   x --- x
      node_1_node_0_1w,

      //  x ==== x
      node_1_1w_node_0_0w,

      //  x ==== x
      node_1w_1w_node_0w_0w,

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
  class dynamic_undirected_graph_weighted_operations
  {
   public:
    using graph_t            = maths::undirected_graph<EdgeWeight, NodeWeight, maths::null_meta_data, EdgeStorageConfig, NodeWeightStorage>;
    using edge_t             = typename graph_t::edge_init_type;
    using node_weight_type   = typename graph_t::node_weight_type;
    using edges_equivalent_t = std::initializer_list<std::initializer_list<edge_t>>;
    using transition_graph   = typename transition_checker<graph_t>::transition_graph;

    constexpr static bool has_shared_weight{EdgeStorageConfig::edge_sharing == maths::edge_sharing_preference::shared_weight};

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
      t.check_exception_thrown<std::logic_error>("Mismatched loop weights", [](){ return graph_t{{edge_t{0, 1.0}, edge_t{0, 2.0}}}; });

      // Two nodes
      t.check_exception_thrown<std::logic_error>("Mismatched weights", [](){ return graph_t{{edge_t{1, 1.0}}, {edge_t{0, 2.0}}}; });

      t.check_exception_thrown<std::logic_error>("Mismatched edge/node initialization", [](){ return graph_t{{}, nodes{1.0}}; });
      t.check_exception_thrown<std::logic_error>("Mismatched edge/node initialization", [](){ return graph_t{{{}}, nodes{1.0, 2.0}}; });
      t.check_exception_thrown<std::logic_error>("Mismatched edge/node initialization", [](){ return graph_t{{{edge_t{0, 1.0}, edge_t{0, 1.0}}}, nodes{1.0, 2.0}}; });
      t.check_exception_thrown<std::logic_error>("Mismatched edge/node initialization", [](){ return graph_t{{{}, {}}, nodes{1.0}}; });
      t.check_exception_thrown<std::logic_error>("Mismatched edge/node initialization", [](){ return graph_t{{{edge_t{1}}, {edge_t{0}}}, nodes{1.0}}; });
    }

    [[nodiscard]]
    static transition_graph make_weighted_transition_graph(regular_test& t)
    {
      using base_ops = dynamic_undirected_graph_operations<EdgeWeight, NodeWeight, maths::null_meta_data, EdgeStorageConfig, NodeWeightStorage>;
      using namespace undirected_graph;

      auto trg{base_ops::make_transition_graph(t)};

      check_initialization_exceptions(t);

      // 'weighted_graph_description::nodew'
      trg.add_node(make_and_check(t, t.report(""), {{}}, {1.0}));

      // 'weighted_graph_description::nodew_0'
      trg.add_node(make_and_check(t, t.report(""), {{{0, 0.0}, {0, 0.0}}}, {1.0}));

      // 'weighted_graph_description::node_0w'
      trg.add_node(make_and_check(t, t.report(""), {{{0, 1.0}, {0, 1.0}}}, {0.0}));

      // 'weighted_graph_description::node_0w_0w'
      trg.add_node(make_and_check(t, t.report(""), {{{0, 1.0}, {0, 1.0}, {0, 1.0}, {0, 1.0}}}, {0.0}));

      // 'weighted_graph_description::node_0_0w'
      trg.add_node(
        [&t](){
          auto g{make_and_check(t, t.report(""), {{{0, 0.0}, {0, 0.0}, {0, 1.0}, {0, 1.0}}}, {0.0})};
          t.check(equality, "Canonical ordering of weighted edges", graph_t{{{{0, 1.0}, {0, 0.0}, {0, 1.0}, {0, 0.0}}}, {0.0}}, g);
          return g;
        }());

      // 'weighted_graph_description::node_nodew'
      trg.add_node(make_and_check(t, t.report(""), {{}, {}}, {0.0, 1.0}));

      // 'weighted_graph_description::nodew_node'
      trg.add_node(make_and_check(t, t.report(""), {{}, {}}, {1.0, 0.0}));

      // 'weighted_graph_description::node_1_nodew_0'
      trg.add_node(make_and_check(t, t.report(""), {{{1, 0.0}}, {{0, 0.0}}}, {0.0, 1.0}));

      // 'weighted_graph_description::nodew_1_node_0'
      trg.add_node(make_and_check(t, t.report(""), {{{1, 0.0}}, {{0, 0.0}}}, {1.0, 0.0}));
      
      // 'weighted_graph_description::node_0w_1_node_0'
      trg.add_node(make_and_check(t, t.report(""), {{{0, 1.0}, {0, 1.0}, {1, 0.0}}, {{0, 0.0}}}, {0.0, 0.0})),

      // 'weighted_graph_description::node_1_node_0_1w'
      trg.add_node(make_and_check(t, t.report(""), {{{1, 0.0}}, {{0, 0.0}, {1, 1.0}, {1, 1.0}}}, {0.0, 0.0})),

      // 'weighted_graph_description::node_1_1w_node_0_0w'
      trg.add_node(
        [&t]() {
          auto g{make_and_check(t, t.report(""), {{{1, 0.0}, {1, 1.0}}, {{0, 0.0}, {0, 1.0}}}, {0.0, 0.0})};
          t.check(equality, "Canonical ordering of weighted edges", graph_t{{{{1, 1.0}, {1, 0.0}}, {{0, 0.0}, {0, 1.0}}}, {0.0, 0.0}}, g);
          t.check(equality, "Canonical ordering of weighted edges", graph_t{{{{1, 1.0}, {1, 0.0}}, {{0, 1.0}, {0, 0.0}}}, {0.0, 0.0}}, g);
          return g;
        }
      );

      // 'weighted_graph_description::node_1w_1w_node_0w_0w'
      trg.add_node(make_and_check(t, t.report(""), {{{1, 1.0}, {1, 1.0}}, {{0, 1.0}, {0, 1.0}}}, {0.0, 0.0}));

      // 'weighted_graph_description::node_1_1w_1x_node_0_0w_0x'
      trg.add_node(
        [&t]() {
          auto g{make_and_check(t, t.report(""), {{{1, 0.0}, {1, 1.0}, {1, 2.0}}, {{0, 0.0}, {0, 1.0}, {0, 2.0}}}, {0.0, 0.0})};
          t.check(equality, "Canonical ordering of weighted edges", graph_t{{{{1, 2.0}, {1, 0.0}, {1, 1.0}}, {{0, 1.0}, {0, 2.0}, {0, 0.0}}}, {0.0, 0.0}}, g);
          return g;
        }
      );

      // 'weighted_graph_description::node_0y_1_1w_1x_node_0_0w_0x'
      trg.add_node(make_and_check(t, t.report(""), {{{0, 3.0}, {0, 3.0}, {1, 0.0}, {1, 1.0}, {1, 2.0}}, {{0, 0.0}, {0, 1.0}, {0, 2.0}}}, {0.0, 0.0}));

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

      // end 'graph_description::node_0'

      // begin 'graph_description::node_0_0'

      trg.join(
        graph_description::node_0_0,
        weighted_graph_description::node_0_0w,
        t.report("Set edge weight via zeroth partial weight"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(g.cbegin_edges(0), 1.0);
          g.swap_edges(0, 0, 2);
          g.swap_edges(0, 1, 3);
          return g;
        }
      );

      trg.join(
        graph_description::node_0_0,
        weighted_graph_description::node_0_0w,
        t.report("Set edge weight via first partial weight"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(g.cbegin_edges(0) + 1, 1.0);
          g.swap_edges(0, 0, 2);
          g.swap_edges(0, 1, 3);
          return g;
        }
      );

      trg.join(
        graph_description::node_0_0,
        weighted_graph_description::node_0_0w,
        t.report("Set edge weight via second partial weight"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(g.cbegin_edges(0) + 2, 1.0);
          if constexpr(!has_shared_weight)
            g.swap_edges(0, 0, 3);

          return g;
        }
      );

      trg.join(
        graph_description::node_0_0,
        weighted_graph_description::node_0_0w,
        t.report("Set edge weight via third partial weight"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(g.cbegin_edges(0) + 3, 1.0);
          if constexpr(!has_shared_weight)
            g.swap_edges(0, 0, 2);
  
          return g;
        }
      );

      trg.join(
        graph_description::node_0_0,
        weighted_graph_description::node_0_0w,
        t.report("Mutate edge weight via zeroth partial weight"),
        [](graph_t g) -> graph_t {
          g.mutate_edge_weight(g.cbegin_edges(0), [](double& x) { x += 1.0; });
          g.swap_edges(0, 0, 2);
          g.swap_edges(0, 1, 3);
          return g;
        }
      );

      trg.join(
        graph_description::node_0_0,
        weighted_graph_description::node_0_0w,
        t.report("Mutate edge weight via first partial weight"),
        [](graph_t g) -> graph_t {
          g.mutate_edge_weight(g.cbegin_edges(0)+1, [](double& x) { x += 1.0; });
          g.swap_edges(0, 0, 2);
          g.swap_edges(0, 1, 3);
          return g;
        }
      );

      trg.join(
        graph_description::node_0_0,
        weighted_graph_description::node_0_0w,
        t.report("Mutate edge weight via second partial weight"),
        [](graph_t g) -> graph_t {
          g.mutate_edge_weight(g.cbegin_edges(0)+2, [](double& x) { x += 1.0; });
          if constexpr(!has_shared_weight)
            g.swap_edges(0, 0, 3);

          return g;
        }
      );

      trg.join(
        graph_description::node_0_0,
        weighted_graph_description::node_0_0w,
        t.report("Mutate edge weight via third partial weight"),
        [](graph_t g) -> graph_t {
          g.mutate_edge_weight(g.cbegin_edges(0)+3, [](double& x) { x += 1.0; });
          if constexpr(!has_shared_weight)
            g.swap_edges(0, 0, 2);

          return g;
        }
      );

      // end 'graph_description::node_0_0'

      // begin 'graph_description::node_0_1_node_0'

      trg.join(
        graph_description::node_0_1_node_0,
        weighted_graph_description::node_0w_1_node_0,
        t.report("Set loop weight via zeroth partial weight"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(g.cbegin_edges(0), 1.0);
          return g;
        }
      );

      trg.join(
        graph_description::node_0_1_node_0,
        weighted_graph_description::node_0w_1_node_0,
        t.report("Set loop weight via first partial weight"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(++g.cbegin_edges(0), 1.0);
          return g;
        }
      );

      trg.join(
        graph_description::node_0_1_node_0,
        weighted_graph_description::node_0w_1_node_0,
        t.report("Mutate loop weight via zeroth partial weight"),
        [](graph_t g) -> graph_t {
          g.mutate_edge_weight(g.cbegin_edges(0), [](double& x) { x += 1.0; });
          return g;
        }
      );

      trg.join(
        graph_description::node_0_1_node_0,
        weighted_graph_description::node_0w_1_node_0,
        t.report("Mutate loop weight via first partial weight"),
        [](graph_t g) -> graph_t {
          g.mutate_edge_weight(++g.cbegin_edges(0), [](double& x) { x += 1.0; });
          return g;
        }
      );

      // end 'graph_description::node_0_1_node_0'

      // begin 'graph_description::node_1_node_0_1'

      trg.join(
        graph_description::node_1_node_0_1,
        weighted_graph_description::node_1_node_0_1w,
        t.report("Set loop weight via zeroth partial weight"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(g.cbegin_edges(1)+1, 1.0);
          return g;
        }
      );

      trg.join(
        graph_description::node_1_node_0_1,
        weighted_graph_description::node_1_node_0_1w,
        t.report("Set loop weight via first partial weight"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(g.cbegin_edges(1)+2, 1.0);
          return g;
        }
      );

      trg.join(
        graph_description::node_1_node_0_1,
        weighted_graph_description::node_1_node_0_1w,
        t.report("Mutate loop weight via zeroth partial weight"),
        [](graph_t g) -> graph_t {
          g.mutate_edge_weight(g.cbegin_edges(1)+1, [](double& x) { x += 1.0; });
          return g;
        }
      );

      trg.join(
        graph_description::node_1_node_0_1,
        weighted_graph_description::node_1_node_0_1w,
        t.report("Mutate loop weight via first partial weight"),
        [](graph_t g) -> graph_t {
          g.mutate_edge_weight(g.cbegin_edges(1)+2, [](double& x) { x += 1.0; });
          return g;
        }
      );

      // end 'graph_description::node_1_node_0_1'

      // begin 'graph_description::node_1_1_node_0_0'

      trg.join(
        graph_description::node_1_1_node_0_0,
        weighted_graph_description::node_1_1w_node_0_0w,
        t.report("Set edge weight via node 0, zeroth partial edge"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(g.cbegin_edges(0), 1.0);
          g.swap_edges(0, 0, 1);
          g.swap_edges(1, 0, 1);
          return g;
        }
      );

      trg.join(
        graph_description::node_1_1_node_0_0,
        weighted_graph_description::node_1_1w_node_0_0w,
        t.report("Set edge weight via node 0, first partial edge"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(++g.cbegin_edges(0), 1.0);
          if constexpr(!has_shared_weight)
            g.swap_edges(1, 0, 1);

          return g;
        }
      );

      trg.join(
        graph_description::node_1_1_node_0_0,
        weighted_graph_description::node_1_1w_node_0_0w,
        t.report("Set edge weight via node 1, zeroth partial edge"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(g.cbegin_edges(1), 1.0);
          g.swap_edges(0, 0, 1);
          g.swap_edges(1, 0, 1);
          return g;
        }
      );

      trg.join(
        graph_description::node_1_1_node_0_0,
        weighted_graph_description::node_1_1w_node_0_0w,
        t.report("Set edge weight via node 1, first partial edge"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(++g.cbegin_edges(1), 1.0);
          if constexpr(!has_shared_weight)
            g.swap_edges(0, 0, 1);

          return g;
        }
      );

      trg.join(
        graph_description::node_1_1_node_0_0,
        weighted_graph_description::node_1_1w_node_0_0w,
        t.report("Mutate edge weight via node 0, zeroth partial edge"),
        [](graph_t g) -> graph_t {
          g.mutate_edge_weight(g.cbegin_edges(0), [](double& x) { x += 1.0; });
          g.swap_edges(0, 0, 1);
          g.swap_edges(1, 0, 1);
          return g;
        }
      );

      trg.join(
        graph_description::node_1_1_node_0_0,
        weighted_graph_description::node_1_1w_node_0_0w,
        t.report("Mutate edge weight via node 0, first partial edge"),
        [](graph_t g) -> graph_t {
          g.mutate_edge_weight(++g.cbegin_edges(0), [](double& x) { x += 1.0; });
          if constexpr(!has_shared_weight)
            g.swap_edges(1, 0, 1);

          return g;
        }
      );

      trg.join(
        graph_description::node_1_1_node_0_0,
        weighted_graph_description::node_1_1w_node_0_0w,
        t.report("Mutate edge weight via node 1, zeroth partial edge"),
        [](graph_t g) -> graph_t {
          g.mutate_edge_weight(g.cbegin_edges(1), [](double& x) { x += 1.0; });
          g.swap_edges(0, 0, 1);
          g.swap_edges(1, 0, 1);
          return g;
        }
      );

      trg.join(
        graph_description::node_1_1_node_0_0,
        weighted_graph_description::node_1_1w_node_0_0w,
        t.report("Mutate edge weight via node 1, first partial edge"),
        [](graph_t g) -> graph_t {
          g.mutate_edge_weight(++g.cbegin_edges(1), [](double& x) { x += 1.0; });
          if constexpr(!has_shared_weight)
            g.swap_edges(0, 0, 1);
 
          return g;
        }
      );

      // end 'graph_description::node_1_1_node_0_0'

      ////======================================= joins from new nodes =======================================//

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

      // end 'weighted_graph_description::node_0_0w'

      // begin 'weighted_graph_description::node_0w_0w'

      trg.join(
        weighted_graph_description::node_0w_0w,
        graph_description::node_0,
        t.report("Ensure edge erasure treats shared weights properly"),
        [](graph_t g) -> graph_t {
          g.swap_edges(0, 1, 2);
          g.erase_edge(g.cbegin_edges(0));
          g.set_edge_weight(g.cbegin_edges(0), 0.0);
          return g;
        }
      );

      trg.join(
        weighted_graph_description::node_0w_0w,
        graph_description::node_0,
        t.report("Ensure edge erasure treats shared weights properly"),
        [](graph_t g) -> graph_t {
          g.erase_edge(g.cbegin_edges(0)+2);
          g.set_edge_weight(g.cbegin_edges(0), 0.0);
          return g;
        }
      );

      trg.join(
        weighted_graph_description::node_0w_0w,
        graph_description::node_0,
        t.report("Ensure edge erasure treats shared weights properly"),
        [](graph_t g) -> graph_t {
          g.erase_edge(g.cbegin_edges(0) + 3);
          g.set_edge_weight(g.cbegin_edges(0), 0.0);
          return g;
        }
      );

      // end 'weighted_graph_description::node_0w_0w'

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

      //// begin 'weighted_graph_description::node_1w_1w_node_0w_0w'

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

      trg.join(
        weighted_graph_description::node_1w_1w_node_0w_0w,
        graph_description::node_1_node_0,
        t.report("Ensure edge erasure treats shared weights properly"),
        [](graph_t g) -> graph_t {
          g.swap_edges(1, 0, 1);
          g.erase_edge(g.cbegin_edges(0));
          g.set_edge_weight(g.cbegin_edges(0), 0.0);
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
          g.join(0, 0, 3.0);
          g.sort_edges(g.cbegin_edges(0),
                       g.cend_edges(0),
                       [](const auto& lhs, const auto& rhs) {
                         return (lhs.target_node() == rhs.target_node()) ? lhs.weight() < rhs.weight() : (lhs.target_node() < rhs.target_node());
                       });
          return g;
        }
      );

      trg.join(
        weighted_graph_description::node_1_1w_1x_node_0_0w_0x,
        weighted_graph_description::node_1_1w_1x_node_0_0w_0x,
        t.report("Set multiple edge weights and sort"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(g.cbegin_edges(0), 1.0);
          g.set_edge_weight(g.cbegin_edges(0) + 1, 2.0);
          g.set_edge_weight(g.cbegin_edges(0) + 2 , 0.0);

          for(auto i : std::views::iota(0uz, 2uz))
            g.sort_edges(g.cbegin_edges(i), g.cend_edges(i), [](const auto& lhs, const auto& rhs) { return lhs.weight() < rhs.weight(); });

          return g;
        }
      );

      trg.join(
        weighted_graph_description::node_1_1w_1x_node_0_0w_0x,
        weighted_graph_description::node_1_1w_1x_node_0_0w_0x,
        t.report("Mutate mutliple edge weights and sort"),
        [](graph_t g) -> graph_t {
          g.mutate_edge_weight(g.cbegin_edges(0),     [](double& x){ x += 1.0; });
          g.mutate_edge_weight(g.cbegin_edges(0) + 1, [](double& x){ x += 1.0; });
          g.mutate_edge_weight(g.cbegin_edges(0) + 2, [](double& x){ x -= 2.0; });

          for(auto i : std::views::iota(0uz, 2uz))
            g.sort_edges(g.cbegin_edges(i), g.cend_edges(i), [](const auto& lhs, const auto& rhs) { return lhs.weight() < rhs.weight(); });

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