////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "DynamicDirectedGraphTestingUtilities.hpp"

namespace sequoia::testing
{
  namespace directed_graph{
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

      //  x    x
      node_nodew,

      //  x    x
      nodew_node,

      //  x ---> x
      node_1_nodew,

      //  x <--- x
      nodew_node_0,

      //  x ===> x
      node_1_1w_node,

      //  x ===> x
      node_1w_1_node,

      //  x ===> x
      node_1w_1w_node,

      //    --->
      //  x ===> x
      node_1_1w_1x_node,

      //  x ===> x
      //    --->
      node_1w_1x_1_node,

      //  x ===> x
      //    --->
      node_1x_1w_1_node,

      // />\
      // \ /
      //  x ===> x
      //    --->
      node_1_1w_1x_0y_node,

      // />\
      // \ /
      //  x ===> x
      //    --->
      node_0y_1x_1w_1_node,

      // 18 loops
      // />\
      // \ /
      //  x  18 edges -> x
      node_0uuu_1uuu_0vvv_1vvv_0www_1www_0xxx_1xxx_0yyy_1yyy_0zzz_1zzz_node,

      // 18 loops
      // />\
      // \ /
      //  x  18 edges -> x
      node_0zzz_1zzz_0yyy_1yyy_0xxx_1xxx_0www_1www_0vvv_1vvv_0uuu_1uuu_node
    };
  }

  template
  <
    class EdgeWeight,
    class NodeWeight,
    class EdgeStorageConfig,
    class NodeWeightStorage
  >
  class dynamic_directed_graph_weighted_operations
  {
   public:
    using graph_t            = maths::directed_graph<EdgeWeight, NodeWeight, EdgeStorageConfig, NodeWeightStorage>;
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

    static void check_initialization_exceptions(regular_test& t)
    {
      using nodes = std::initializer_list<node_weight_type>;

      t.check_exception_thrown<std::out_of_range>("Zeroth partial index of edge out of range", [](){ return graph_t{{edge_t{1, 1.0}}}; });
      t.check_exception_thrown<std::out_of_range>("First partial index of edge out of range", [](){ return graph_t{{edge_t{0, 1.0}, edge_t{1, 1.0}}}; });
      t.check_exception_thrown<std::out_of_range>("First partial index of edge out of range", [](){ return graph_t{{edge_t{0, 1.0}, edge_t{2, 1.0}}, {}}; });
      t.check_exception_thrown<std::out_of_range>("Zeroth partial index of node 1's edge out of range", [](){ return graph_t{{edge_t{0, 1.0}, edge_t{1, 1.0}}, {edge_t{2, 1.0}}}; });

      t.check_exception_thrown<std::logic_error>("Mismatched edge/node initialization", [](){ return graph_t{{}, nodes{1.0}}; });
      t.check_exception_thrown<std::logic_error>("Mismatched edge/node initialization", [](){ return graph_t{{{}}, nodes{1.0, 2.0}}; });
      t.check_exception_thrown<std::logic_error>("Mismatched edge/node initialization", [](){ return graph_t{{{edge_t{0, 1.0}}}, nodes{1.0, 2.0}}; });
      t.check_exception_thrown<std::logic_error>("Mismatched edge/node initialization", [](){ return graph_t{{{}, {}}, nodes{1.0}}; });
      t.check_exception_thrown<std::logic_error>("Mismatched edge/node initialization", [](){ return graph_t{{{edge_t{1}}, {edge_t{0}}}, nodes{1.0}}; });
    }


    [[nodiscard]]
    static graph_t make_and_check(regular_test& t, std::string_view description, edges_equivalent_t edgeInit, std::initializer_list<node_weight_type> nodeInit)
    {
      return graph_initialization_checker<graph_t>::make_and_check(t, description, edgeInit, nodeInit);
    }

    [[nodiscard]]
    static transition_graph make_weighted_transition_graph(regular_test& t)
    {
      using base_ops = dynamic_directed_graph_operations<EdgeWeight, NodeWeight, EdgeStorageConfig, NodeWeightStorage>;
      using namespace directed_graph;

      auto trg{base_ops::make_transition_graph(t)};

      check_initialization_exceptions(t);

      // 'weighted_graph_description::nodew'
      trg.add_node(make_and_check(t, t.report(""), {{}}, {1.0}));

      // 'weighted_graph_description::nodew_0'
      trg.add_node(make_and_check(t, t.report(""), {{{0, 0.0}}}, {1.0}));

      // 'weighted_graph_description::node_0w'
      trg.add_node(make_and_check(t, t.report(""), {{{0, 1.0}}}, {0.0}));

      // 'weighted_graph_description::node_0w_0w'
      trg.add_node(make_and_check(t, t.report(""), {{{0, 1.0}, {0, 1.0}}}, {0.0}));

      // 'weighted_graph_description::node_0_0w'
      trg.add_node(make_and_check(t, t.report(""), {{{0, 0.0}, {0, 1.0}}}, {0.0}));

      // 'weighted_graph_description::node_0w_0'
      trg.add_node(make_and_check(t, t.report(""), {{{0, 1.0}, {0, 0.0}}}, {0.0}));

      // 'weighted_graph_description::node_nodew'
      trg.add_node(make_and_check(t, t.report(""), {{}, {}}, {0.0, 1.0}));

      // 'weighted_graph_description::nodew_node'
      trg.add_node(make_and_check(t, t.report(""), {{}, {}}, {1.0, 0.0}));

      // 'weighted_graph_description::node_1_nodew'
      trg.add_node(make_and_check(t, t.report(""), {{{1, 0.0}}, {}}, {0.0, 1.0}));

      // 'weighted_graph_description::nodew_node_0'
      trg.add_node(make_and_check(t, t.report(""), {{}, {{0, 0.0}}}, {1.0, 0.0}));

      // 'weighted_graph_description::node_1_1w_node'
      trg.add_node(make_and_check(t, t.report(""), {{{1, 0.0}, {1, 1.0}}, {}}, {0.0, 0.0}));

      // 'weighted_graph_description::node_1w_1_node'
      trg.add_node(make_and_check(t, t.report(""), {{{1, 1.0}, {1, 0.0}}, {}}, {0.0, 0.0}));

      // 'weighted_graph_description::node_1w_1w_node'
      trg.add_node(make_and_check(t, t.report(""), {{{1, 1.0}, {1, 1.0}}, {}}, {0.0, 0.0}));

      // 'weighted_graph_description::node_1_1w_1x_node'
      trg.add_node(make_and_check(t, t.report(""), {{{1, 0.0}, {1, 1.0}, {1, 2.0}}, {}}, {0.0, 0.0}));

      // 'weighted_graph_description::node_1w_1x_1_node'
      trg.add_node(make_and_check(t, t.report(""), {{{1, 1.0}, {1, 2.0}, {1, 0.0}}, {}}, {0.0, 0.0}));

      // 'weighted_graph_description::node_1x_1w_1_node'
      trg.add_node(make_and_check(t, t.report(""), {{{1, 2.0}, {1, 1.0}, {1, 0.0}}, {}}, {0.0, 0.0}));

      // 'weighted_graph_description::node_1_1w_1x_0y_node'
      trg.add_node(make_and_check(t, t.report(""), {{{1, 0.0}, {1, 1.0}, {1, 2.0}, {0, 3.0}}, {}}, {0.0, 0.0}));

      // 'weighted_graph_description::node_0y_1x_1w_1_node'
      trg.add_node(make_and_check(t, t.report(""), {{{0, 3.0}, {1, 2.0}, {1, 1.0}, {1, 0.0}}, {}}, {0.0, 0.0}));

      // 'weighted_graph_description::node_0uuu_1uuu_0vvv_1vvv_0www_1www_0xxx_1xxx_0yyy_1yyy_0zzz_1zzz_node'
      trg.add_node(make_and_check(t, t.report(""), {{
                                                           {0, -2.0}, {0, -2.0}, {0, -2.0}, {1, -2.0}, {1, -2.0}, {1, -2.0},
                                                           {0, -1.0}, {0, -1.0}, {0, -1.0}, {1, -1.0}, {1, -1.0}, {1, -1.0},
                                                           {0, 0.0},  {0, 0.0},  {0, 0.0},  {1, 0.0},  {1, 0.0},  {1, 0.0},
                                                           {0, 1.0},  {0, 1.0},  {0, 1.0},  {1, 1.0},  {1, 1.0},  {1, 1.0},
                                                           {0, 2.0},  {0, 2.0},  {0, 2.0},  {1, 2.0},  {1, 2.0},  {1, 2.0},
                                                           {0, 3.0},  {0, 3.0},  {0, 3.0},  {1, 3.0},  {1, 3.0},  {1, 3.0}
                                                         },
                                                         {}}, {0.0, 0.0}));

      // 'weighted_graph_description::node_0zzz_1zzz_0yyy_1yyy_0xxx_1xxx_0www_1www_0vvv_1vvv_0uuu_1uuu_node'
      trg.add_node(make_and_check(t, t.report(""), {{
                                                           {0, 3.0},  {0, 3.0},  {0, 3.0},  {1, 3.0},  {1, 3.0},  {1, 3.0},
                                                           {0, 2.0},  {0, 2.0},  {0, 2.0},  {1, 2.0},  {1, 2.0},  {1, 2.0},
                                                           {0, 1.0},  {0, 1.0},  {0, 1.0},  {1, 1.0},  {1, 1.0},  {1, 1.0},
                                                           {0, 0.0},  {0, 0.0},  {0, 0.0},  {1, 0.0},  {1, 0.0},  {1, 0.0},
                                                           {0, -1.0}, {0, -1.0}, {0, -1.0}, {1, -1.0}, {1, -1.0}, {1, -1.0},
                                                           {0, -2.0}, {0, -2.0}, {0, -2.0}, {1, -2.0}, {1, -2.0}, {1, -2.0}
                                                         },
                                                         {}}, {0.0, 0.0}));

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
        weighted_graph_description::nodew,
        t.report("Change node weight via iterator"),
        [](graph_t g) -> graph_t {
          *g.begin_node_weights() = 1.0;
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
        t.report("Mutate edge weight"),
        [](graph_t g) -> graph_t {
          g.mutate_edge_weight(g.cbegin_edges(0), [](double& x){ x += 1.0; });
          return g;
        }
      );

      trg.join(
        graph_description::node_0,
        weighted_graph_description::node_0w,
        t.report("Mutate edge weight"),
        [](graph_t g) -> graph_t {
          *g.begin_edge_weights(0) += 1.0;;
          return g;
        }
      );

      trg.join(
        graph_description::node_0,
        weighted_graph_description::node_0w,
        t.report("Mutate edge weight"),
        [](graph_t g) -> graph_t {
          *g.rbegin_edge_weights(0) += 1.0;;
          return g;
        }
      );

      trg.join(
        graph_description::node_0,
        weighted_graph_description::node_0w,
        t.report("Mutate edge weight"),
        [](graph_t g) -> graph_t {
          *g.edge_weights(0).begin() += 1.0;;
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
        weighted_graph_description::node_0w_0,
        t.report("Set zeroth edge weight"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(g.cbegin_edges(0), 1.0);
          return g;
        }
      );

      trg.join(
        graph_description::node_0_0,
        weighted_graph_description::node_0_0w,
        t.report("Set first edge weight"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(++g.cbegin_edges(0), 1.0);
          return g;
        }
      );

      // end 'graph_description::node_0_0'

      // begin 'graph_description::node_1_1_node'

      trg.join(
        graph_description::node_1_1_node,
        weighted_graph_description::node_1_1w_node,
        t.report("Set edge weight"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(++g.cbegin_edges(0), 1.0);
          return g;
        }
      );

      trg.join(
        graph_description::node_1_1_node,
        weighted_graph_description::node_1_1w_node,
        t.report("Mutate edge weight"),
        [](graph_t g) -> graph_t {
          g.mutate_edge_weight(++g.cbegin_edges(0), [](double& x) { x += 1.0; });
          return g;
        }
      );

      trg.join(
        graph_description::node_1_1_node,
        weighted_graph_description::node_1w_1_node,
        t.report("Set edge weight"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(g.cbegin_edges(0), 1.0);
          return g;
        }
      );

      trg.join(
        graph_description::node_1_1_node,
        weighted_graph_description::node_1w_1_node,
        t.report("Mutate edge weight"),
        [](graph_t g) -> graph_t {
          g.mutate_edge_weight(g.cbegin_edges(0), [](double& x) { x += 1.0; });
          return g;
        }
      );

      // end 'graph_description::node_1_1_node'

      //======================================= joins from new nodes =======================================//

      // begin 'weighted_graph_description::node_0_0w'

      trg.join(
        weighted_graph_description::node_0_0w,
        weighted_graph_description::node_0w_0,
        t.report("Swap edges"),
        [](graph_t g) -> graph_t {
          g.swap_edges(0, 0, 1);
          return g;
        }
      );

      trg.join(
        weighted_graph_description::node_0_0w,
        weighted_graph_description::node_0w_0,
        t.report("Swap edges"),
        [](graph_t g) -> graph_t {
          g.swap_edges(0, 1, 0);
          return g;
        }
      );

      trg.join(
        weighted_graph_description::node_0_0w,
        weighted_graph_description::node_0w_0,
        t.report("Sort edges"),
        [](graph_t g) -> graph_t {
          g.sort_edges(g.cbegin_edges(0), g.cend_edges(0), [](const auto& lhs, const auto& rhs) { return lhs.weight() > rhs.weight(); });
          return g;
        }
      );

      // end 'weighted_graph_description::node_0_0w'

      // begin 'weighted_graph_description::node_0w_0'

      trg.join(
        weighted_graph_description::node_0w_0,
        weighted_graph_description::node_0_0w,
        t.report("Swap edges"),
        [](graph_t g) -> graph_t {
          g.swap_edges(0, 0, 1);
          return g;
        }
      );

      trg.join(
        weighted_graph_description::node_0w_0,
        weighted_graph_description::node_0_0w,
        t.report("Swap edges"),
        [](graph_t g) -> graph_t {
          g.swap_edges(0, 1, 0);
          return g;
        }
      );

      trg.join(
        weighted_graph_description::node_0w_0,
        weighted_graph_description::node_0_0w,
        t.report("Sort edges"),
        [](graph_t g) -> graph_t {
          g.sort_edges(g.cbegin_edges(0), g.cend_edges(0), [](const auto& lhs, const auto& rhs) { return lhs.weight() < rhs.weight(); });
          return g;
        }
      );

      // end 'weighted_graph_description::node_0w_0'

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

      // begin 'weighted_graph_description::node_1_nodew'

      trg.join(
        weighted_graph_description::node_1_nodew,
        weighted_graph_description::nodew_node_0,
        t.report("Swap nodes"),
        [](graph_t g) -> graph_t {
          g.swap_nodes(0, 1);
          return g;
        }
      );

      // end 'weighted_graph_description::node_1_nodew'

      // begin 'weighted_graph_description::nodew_node_0'

      trg.join(
        weighted_graph_description::nodew_node_0,
        weighted_graph_description::node_1_nodew,
        t.report("Swap nodes"),
        [](graph_t g) -> graph_t {
          g.swap_nodes(1, 0);
          return g;
        }
      );

      // end 'weighted_graph_description::nodew_node_0'

      // begin 'weighted_graph_description::node_1_1w_node'
      
      trg.join(
        weighted_graph_description::node_1_1w_node,
        weighted_graph_description::node_1w_1w_node,
        t.report("Set edge weight"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(g.cbegin_edges(0), 1.0);
          return g;
        }
      );

      trg.join(
        weighted_graph_description::node_1_1w_node,
        weighted_graph_description::node_1w_1_node,
        t.report("Swap edges"),
        [](graph_t g) -> graph_t {
          g.swap_edges(0, 0, 1);
          return g;
        }
      );

      trg.join(
        weighted_graph_description::node_1_1w_node,
        weighted_graph_description::node_1w_1_node,
        t.report("Sort edges"),
        [](graph_t g) -> graph_t {
          g.sort_edges(g.cbegin_edges(0), g.cend_edges(0), [](const auto& lhs, const auto& rhs) { return lhs.weight() > rhs.weight(); });
          return g;
        }
      );

      // end 'weighted_graph_description::node_1_1w_node'

      // begin 'weighted_graph_description::node_1w_1_node'

      trg.join(
        weighted_graph_description::node_1w_1_node,
        weighted_graph_description::node_1w_1w_node,
        t.report("Set edge weight"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(++g.cbegin_edges(0), 1.0);
          return g;
        }
      );

      trg.join(
        weighted_graph_description::node_1w_1_node,
        weighted_graph_description::node_1_1w_node,
        t.report("Swap edges"),
        [](graph_t g) -> graph_t {
          g.swap_edges(0, 0, 1);
          return g;
        }
      );

      trg.join(
        weighted_graph_description::node_1w_1_node,
        weighted_graph_description::node_1_1w_node,
        t.report("Sort edges"),
        [](graph_t g) -> graph_t {
          g.sort_edges(g.cbegin_edges(0), g.cend_edges(0), [](const auto& lhs, const auto& rhs) { return lhs.weight() < rhs.weight(); });
          return g;
        }
      );

      // end 'weighted_graph_description::node_1w_1_node'

      // begin 'weighted_graph_description::node_1w_1w_node'

      trg.join(
        weighted_graph_description::node_1w_1w_node,
        weighted_graph_description::node_1_1w_node,
        t.report("Set edge weight"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(g.cbegin_edges(0), 0.0);
          return g;
        }
      );

      trg.join(
        weighted_graph_description::node_1w_1w_node,
        weighted_graph_description::node_1_1w_node,
        t.report("Mutate edge weight"),
        [](graph_t g) -> graph_t {
          g.mutate_edge_weight(g.cbegin_edges(0), [](double& x){ x -= 1.0; });
          return g;
        }
      );

      trg.join(
        weighted_graph_description::node_1w_1w_node,
        weighted_graph_description::node_1w_1_node,
        t.report("Set edge weight"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(++g.cbegin_edges(0), 0.0);
          return g;
        }
      );

      trg.join(
        weighted_graph_description::node_1w_1w_node,
        weighted_graph_description::node_1w_1_node,
        t.report("Mutate edge weight"),
        [](graph_t g) -> graph_t {
          g.mutate_edge_weight(++g.cbegin_edges(0), [](double& x){ x -= 1.0; });
          return g;
        }
      );

      // end 'weighted_graph_description::node_1w_1w_node'

      // begin 'weighted_graph_description::node_1_1w_1x_node'

      trg.join(
        weighted_graph_description::node_1_1w_1x_node,
        weighted_graph_description::node_1_1w_1x_0y_node,
        t.report("Join {0,0}"),
        [](graph_t g) -> graph_t {
          g.join(0, 0, 3.0);
          return g;
        }
      );

      trg.join(
        weighted_graph_description::node_1_1w_1x_node,
        weighted_graph_description::node_1w_1x_1_node,
        t.report("Set multiple edge weights"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(g.cbegin_edges(0), 1.0);
          g.set_edge_weight(g.cbegin_edges(0) + 1, 2.0);
          g.set_edge_weight(g.cbegin_edges(0) + 2 , 0.0);
          return g;
        }
      );

      trg.join(
        weighted_graph_description::node_1_1w_1x_node,
        weighted_graph_description::node_1w_1x_1_node,
        t.report("Mutate mutliple edge weights"),
        [](graph_t g) -> graph_t {
          g.mutate_edge_weight(g.cbegin_edges(0),     [](double& x){ x += 1.0; });
          g.mutate_edge_weight(g.cbegin_edges(0) + 1, [](double& x){ x += 1.0; });
          g.mutate_edge_weight(g.cbegin_edges(0) + 2, [](double& x){ x -= 2.0; });
          return g;
        }
      );

      // end 'weighted_graph_description::node_1_1w_1x_node'

      // begin 'weighted_graph_description::node_1w_1x_1_node'

      trg.join(
        weighted_graph_description::node_1w_1x_1_node,
        weighted_graph_description::node_1w_1_node,
        t.report("Remove {0,1}"),
        [](graph_t g) -> graph_t {
          g.erase_edge(++g.cbegin_edges(0));
          return g;
        }
      );

      trg.join(
        weighted_graph_description::node_1w_1x_1_node,
        weighted_graph_description::node_1_1w_1x_node,
        t.report("Sort edges"),
        [](graph_t g) -> graph_t {
          g.sort_edges(g.cbegin_edges(0), g.cend_edges(0), [](const auto& lhs, const auto& rhs) { return lhs.weight() < rhs.weight(); });
          return g;
        }
      );

      // end 'weighted_graph_description::node_1w_1x_1_node'

      // begin 'weighted_graph_description::node_1x_1w_1_node'

      trg.join(
        weighted_graph_description::node_1x_1w_1_node,
        weighted_graph_description::node_1w_1x_1_node,
        t.report("Swap edges"),
        [](graph_t g) -> graph_t {
          g.swap_edges(0, 1, 0);
          return g;
        }
      );

      trg.join(
        weighted_graph_description::node_1x_1w_1_node,
        weighted_graph_description::node_1_1w_1x_node,
        t.report("Sort edges"),
        [](graph_t g) -> graph_t {
          g.sort_edges(g.cbegin_edges(0), g.cend_edges(0), [](const auto& lhs, const auto& rhs) { return lhs.weight() < rhs.weight(); });
          return g;
        }
      );

      // end 'weighted_graph_description::node_1x_1w_1_node'

      // begin 'weighted_graph_description::node_1_1w_1x_0y_node'

      trg.join(
        weighted_graph_description::node_1_1w_1x_0y_node,
        weighted_graph_description::node_0y_1x_1w_1_node,
        t.report("Sort edges"),
        [](graph_t g) -> graph_t {
          g.sort_edges(g.cbegin_edges(0), g.cend_edges(0), [](const auto& lhs, const auto& rhs) { return lhs.weight() > rhs.weight(); });
          return g;
        }
      );

      // end 'weighted_graph_description::node_1_1w_1x_0y_node'

      // begin 'weighted_graph_description::node_0y_1x_1w_1_node'

      trg.join(
        weighted_graph_description::node_0y_1x_1w_1_node,
        weighted_graph_description::node_1x_1w_1_node,
        t.report("Remove {0,0}"),
        [](graph_t g) -> graph_t {
          g.erase_edge(g.cbegin_edges(0));
          return g;
        }
      );

      trg.join(
        weighted_graph_description::node_0y_1x_1w_1_node,
        weighted_graph_description::node_1_1w_1x_0y_node,
        t.report("Sort edges"),
        [](graph_t g) -> graph_t {
          g.sort_edges(g.cbegin_edges(0), g.cend_edges(0), [](const auto& lhs, const auto& rhs) { return lhs.weight() < rhs.weight(); });
          return g;
        }
      );

      // end'weighted_graph_description::node_0y_1x_1w_1_node'

      // begin 'weighted_graph_description::node_0uuu_1uuu_0vvv_1vvv_0www_1www_0xxx_1xxx_0yyy_1yyy_0zzz_1zzz_node'

      trg.join(
        weighted_graph_description::node_0uuu_1uuu_0vvv_1vvv_0www_1www_0xxx_1xxx_0yyy_1yyy_0zzz_1zzz_node,
        weighted_graph_description::node_0zzz_1zzz_0yyy_1yyy_0xxx_1xxx_0www_1www_0vvv_1vvv_0uuu_1uuu_node,
        t.report("Sort edges"),
        [](graph_t g) -> graph_t {
          g.stable_sort_edges(g.cedges(0), std::ranges::greater{}, [](const auto& e) { return e.weight(); });
          return g;
        }
      );

      // end'weighted_graph_description::node_0uuu_1uuu_0vvv_1vvv_0www_1www_0xxx_1xxx_0yyy_1yyy_0zzz_1zzz_node'

      // begin 'weighted_graph_description::node_0zzz_1zzz_0yyy_1yyy_0xxx_1xxx_0www_1www_0vvv_1vvv_0uuu_1uuu_node'

      trg.join(
        weighted_graph_description::node_0zzz_1zzz_0yyy_1yyy_0xxx_1xxx_0www_1www_0vvv_1vvv_0uuu_1uuu_node,
        weighted_graph_description::node_0uuu_1uuu_0vvv_1vvv_0www_1www_0xxx_1xxx_0yyy_1yyy_0zzz_1zzz_node,
        t.report("Sort edges"),
        [](graph_t g) -> graph_t {
          g.stable_sort_edges(g.cbegin_edges(0), g.cend_edges(0), [](const auto& lhs, const auto& rhs) { return lhs.weight() < rhs.weight(); });
          return g;
        }
      );

      // end'weighted_graph_description::node_0zzz_1zzz_0yyy_1yyy_0xxx_1xxx_0www_1www_0vvv_1vvv_0uuu_1uuu_node'

      return trg;
    }
  };


}