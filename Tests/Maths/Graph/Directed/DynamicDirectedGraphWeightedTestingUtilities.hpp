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
  namespace weighted_directed_graph{
    /// Convention: the indices following 'node' - separated by underscores - give the target node of the associated edges
    enum graph_description : std::size_t {
      // x
      nodew = directed_graph::graph_description::end,

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
      node_0w_0,

      //  /\ /\
      //  \/ \/
      //    x
      node_0_0w,

      //  x    x
      node_nodew,

      //  x    x
      nodew_node,

      //  x ---> x
      node_1_nodew,

      //  x <--- x
      nodew_node_0,

      ////   /\
      ////   \/
      ////   x ---> x
      //node_0_1_node,

      ////  /\
      ////  \/
      ////  x      x
      //node_0_node,

      ////      /\
      ////      \/
      ////  x    x
      //node_node_1,

      //  x ===> x
      node_1_1w_node,

      //  x ===> x
      node_1w_1_node,

      //  x ===> x
      node_1w_1w_node,

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
      node_2_node_node_node_1,

      end
    };
  }

  template
  <
    class EdgeWeight,
    class NodeWeight,
    class EdgeWeightCreator,
    class NodeWeightCreator,
    class EdgeStorageTraits,
    class NodeWeightStorageTraits
  >
  class dynamic_directed_graph_weighted_operations
  {
    template<maths::network>
    friend struct graph_initialization_checker;
   public:
    using graph_t            = maths::graph<maths::directed_flavour::directed, EdgeWeight, NodeWeight, EdgeWeightCreator, NodeWeightCreator, EdgeStorageTraits, NodeWeightStorageTraits>;
    using edge_t             = typename graph_t::edge_init_type;
    using node_weight_type   = typename graph_t::node_weight_type;
    using edges_equivalent_t = std::initializer_list<std::initializer_list<edge_t>>;
    using transition_graph   = typename transition_checker<graph_t>::transition_graph;

    static void execute_operations(regular_test& t)
    {
      auto trg{make_weighted_transition_graph(t)};

      auto checker{
          [&t](std::string_view description, const graph_t& obtained, const graph_t& prediction, const graph_t& parent, std::size_t host, std::size_t target) {
            t.check(equality, description, obtained, prediction);
            if(host != target) t.check_semantics(description, prediction, parent);
          }
      };

      transition_checker<graph_t>::check(report_line(""), trg, checker);
    }

    [[nodiscard]]
    static graph_t make_and_check(regular_test& t, std::string_view description, edges_equivalent_t edgeInit, std::initializer_list<node_weight_type> nodeInit)
    {
      return graph_initialization_checker<graph_t>::make_and_check(t, description, edgeInit, nodeInit);
    }

    [[nodiscard]]
    static transition_graph make_weighted_transition_graph(regular_test& t)
    {
      using base_ops = dynamic_directed_graph_operations<EdgeWeight, NodeWeight, EdgeWeightCreator, NodeWeightCreator, EdgeStorageTraits, NodeWeightStorageTraits>;
      using namespace weighted_directed_graph;

      auto trg{base_ops::make_transition_graph(t)};

      // 'weighted_directed_graph::graph_description::nodew'
      trg.add_node(make_and_check(t, t.report_line(""), {{}}, {1.0}));

      // 'weighted_directed_graph::graph_description::nodew_0'
      trg.add_node(make_and_check(t, t.report_line(""), {{{0, 0.0}}}, {1.0}));

      // 'weighted_directed_graph::graph_description::node_0w'
      trg.add_node(make_and_check(t, t.report_line(""), {{{0, 1.0}}}, {0.0}));

      // 'weighted_directed_graph::graph_description::node_0w_0w'
      trg.add_node(make_and_check(t, t.report_line(""), {{{0, 1.0}, {0, 1.0}}}, {0.0}));

      // 'weighted_directed_graph::graph_description::node_0w_0'
      trg.add_node(make_and_check(t, t.report_line(""), {{{0, 0.0}, {0, 1.0}}}, {0.0}));

      // 'weighted_directed_graph::graph_description::node_0_0w'
      trg.add_node(make_and_check(t, t.report_line(""), {{{0, 1.0}, {0, 0.0}}}, {0.0}));

      // 'weighted_directed_graph::graph_description::node_nodew'
      trg.add_node(make_and_check(t, t.report_line(""), {{}, {}}, {0.0, 1.0}));

      // 'weighted_directed_graph::graph_description::nodew_node'
      trg.add_node(make_and_check(t, t.report_line(""), {{}, {}}, {1.0, 0.0}));

      // 'weighted_directed_graph::graph_description::node_1_nodew'
      trg.add_node(make_and_check(t, t.report_line(""), {{{1, 0.0}}, {}}, {0.0, 1.0}));

      // 'weighted_directed_graph::graph_description::nodew_node_0'
      trg.add_node(make_and_check(t, t.report_line(""), {{}, {{0, 0.0}}}, {1.0, 0.0}));

      // 'weighted_directed_graph::graph_description::node_1_1w_node'
      trg.add_node(make_and_check(t, t.report_line(""), {{{1, 0.0}, {1, 1.0}}, {}}, {0.0, 0.0}));

      // 'weighted_directed_graph::graph_description::node_1w_1_node'
      trg.add_node(make_and_check(t, t.report_line(""), {{{1, 1.0}, {1, 0.0}}, {}}, {0.0, 0.0}));

      // 'weighted_directed_graph::graph_description::node_1w_1w_node'
      trg.add_node(make_and_check(t, t.report_line(""), {{{1, 1.0}, {1, 1.0}}, {}}, {0.0, 0.0}));

      // begin 'directed_graph::graph_description::empty'

      trg.join(
        directed_graph::graph_description::empty,
        directed_graph::graph_description::empty,
        t.report_line(""),
        [&t](graph_t g) -> graph_t {
          t.check_exception_thrown<std::out_of_range>(t.report_line("Attempt to set a node weight which does not exist"), [&g](){ g.node_weight(g.cbegin_node_weights(), 1.0); });
          return g;
        }
      );

      trg.join(
        directed_graph::graph_description::empty,
        directed_graph::graph_description::empty,
        t.report_line("Attempt to mutate a node weight which does not exist"),
        [&t](graph_t g) -> graph_t {
          t.check_exception_thrown<std::out_of_range>(t.report_line("Attempt to mutate a node weight which does not exist"), [&g](){ g.mutate_node_weight(g.cbegin_node_weights(), [](double&){}); });
          return g;
        }
      );

      trg.join(
        directed_graph::graph_description::empty,
        weighted_directed_graph::graph_description::nodew,
        t.report_line("Add weighted node"),
        [](graph_t g) -> graph_t {
          g.add_node(1.0);
          return g;
        }
      );

      trg.join(
        directed_graph::graph_description::empty,
        weighted_directed_graph::graph_description::nodew,
        t.report_line("Insert weighted node"),
        [](graph_t g) -> graph_t {
          g.insert_node(0, 1.0);
          return g;
        }
      );

      // end 'directed_graph::graph_description::empty'

      // begin 'directed_graph::graph_description::node'

      trg.join(
        directed_graph::graph_description::node,
        directed_graph::graph_description::node,
        t.report_line(""),
        [&t](graph_t g) -> graph_t {
          t.check_exception_thrown<std::out_of_range>(t.report_line("Attempt to set a node weight which does not exist"), [&g](){ g.node_weight(g.cend_node_weights(), 1.0); });
          return g;
        }
      );

      trg.join(
        directed_graph::graph_description::node,
        directed_graph::graph_description::node,
        t.report_line("Attempt to mutate a node weight which does not exist"),
        [&t](graph_t g) -> graph_t {
          t.check_exception_thrown<std::out_of_range>(t.report_line("Attempt to mutate a node weight which does not exist"), [&g](){ g.mutate_node_weight(g.cend_node_weights(), [](double&){}); });
          return g;
        }
      );

      trg.join(
        directed_graph::graph_description::node,
        weighted_directed_graph::graph_description::nodew,
        t.report_line("Change node weight"),
        [](graph_t g) -> graph_t {
          g.node_weight(g.cbegin_node_weights(), 1.0);
          return g;
        }
      );

      trg.join(
        directed_graph::graph_description::node,
        weighted_directed_graph::graph_description::nodew,
        t.report_line("Mutate node weight"),
        [](graph_t g) -> graph_t {
          g.mutate_node_weight(g.cbegin_node_weights(), [](double& x) { x += 1.0; });
          return g;
        }
      );

      if constexpr(std::same_as<object::faithful_producer<double>, NodeWeightCreator>)
      {
        trg.join(
          directed_graph::graph_description::node,
          weighted_directed_graph::graph_description::nodew,
          t.report_line("Change node weight via iterator"),
          [](graph_t g) -> graph_t {
            *g.begin_node_weights() = 1.0;
            return g;
          }
        );
      }

      trg.join(
        directed_graph::graph_description::node,
        weighted_directed_graph::graph_description::nodew_node,
        t.report_line("Insert weighted node"),
        [](graph_t g) -> graph_t {
          g.insert_node(0, 1.0);
          return g;
        }
      );

      // end 'directed_graph::graph_description::node'

      // begin 'directed_graph::graph_description::node_0'

      trg.join(
        directed_graph::graph_description::node_0,
        weighted_directed_graph::graph_description::node_0w,
        t.report_line("Set edge weight"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(g.cbegin_edges(0), 1.0);
          return g;
        }
      );

      trg.join(
        directed_graph::graph_description::node_0,
        weighted_directed_graph::graph_description::node_0w,
        t.report_line("Mutate edge weight"),
        [](graph_t g) -> graph_t {
          g.mutate_edge_weight(g.cbegin_edges(0), [](double& x){ x += 1.0; });
          return g;
        }
      );

      trg.join(
        directed_graph::graph_description::node_0,
        weighted_directed_graph::graph_description::node_0w_0,
        t.report_line("Join {0,0}"),
        [](graph_t g) -> graph_t {
          g.join(0, 0, 1.0);
          return g;
        }
      );

      // end 'directed_graph::graph_description::node_0'

      // begin 'directed_graph::graph_description::node_0_0'

      trg.join(
        directed_graph::graph_description::node_0_0,
        weighted_directed_graph::graph_description::node_0_0w,
        t.report_line("Set first edge weight"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(g.cbegin_edges(0), 1.0);
          return g;
        }
      );

      trg.join(
        directed_graph::graph_description::node_0_0,
        weighted_directed_graph::graph_description::node_0w_0,
        t.report_line("Set first edge weight"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(++g.cbegin_edges(0), 1.0);
          return g;
        }
      );

      // end 'directed_graph::graph_description::node_0_0'

      // begin 'directed_graph::graph_description::node_1_1_node'

      trg.join(
        directed_graph::graph_description::node_1_1_node,
        weighted_directed_graph::graph_description::node_1_1w_node,
        t.report_line("Set edge weight"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(++g.cbegin_edges(0), 1.0);
          return g;
        }
      );

      trg.join(
        directed_graph::graph_description::node_1_1_node,
        weighted_directed_graph::graph_description::node_1_1w_node,
        t.report_line("Mutate edge weight"),
        [](graph_t g) -> graph_t {
          g.mutate_edge_weight(++g.cbegin_edges(0), [](double& x) { x += 1.0; });
          return g;
        }
      );

      trg.join(
        directed_graph::graph_description::node_1_1_node,
        weighted_directed_graph::graph_description::node_1w_1_node,
        t.report_line("Set edge weight"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(g.cbegin_edges(0), 1.0);
          return g;
        }
      );

      trg.join(
        directed_graph::graph_description::node_1_1_node,
        weighted_directed_graph::graph_description::node_1w_1_node,
        t.report_line("Mutate edge weight"),
        [](graph_t g) -> graph_t {
          g.mutate_edge_weight(g.cbegin_edges(0), [](double& x) { x += 1.0; });
          return g;
        }
      );

      // end 'ddirected_graph::graph_description::node_1_1_node'

      //======================================= joins from new nodes =======================================//

      // begin 'weighted_directed_graph::graph_description::node_0w_0'

      trg.join(
        weighted_directed_graph::graph_description::node_0w_0,
        weighted_directed_graph::graph_description::node_0_0w,
        t.report_line("Swap edges"),
        [](graph_t g) -> graph_t {
          g.swap_edges(0, 0, 1);
          return g;
        }
      );

      trg.join(
        weighted_directed_graph::graph_description::node_0w_0,
        weighted_directed_graph::graph_description::node_0_0w,
        t.report_line("Swap edges"),
        [](graph_t g) -> graph_t {
          g.swap_edges(0, 1, 0);
          return g;
        }
      );

      trg.join(
        weighted_directed_graph::graph_description::node_0w_0,
        weighted_directed_graph::graph_description::node_0_0w,
        t.report_line("Sort edges"),
        [](graph_t g) -> graph_t {
          g.sort_edges(g.cbegin_edges(0), g.cend_edges(0), [](const auto& lhs, const auto& rhs) { return lhs.weight() > rhs.weight(); });
          return g;
        }
      );

      // end 'weighted_directed_graph::graph_description::node_0w_0'

      // begin 'weighted_directed_graph::graph_description::node_0_0w'

      trg.join(
        weighted_directed_graph::graph_description::node_0_0w,
        weighted_directed_graph::graph_description::node_0w_0,
        t.report_line("Swap edges"),
        [](graph_t g) -> graph_t {
          g.swap_edges(0, 0, 1);
          return g;
        }
      );

      trg.join(
        weighted_directed_graph::graph_description::node_0_0w,
        weighted_directed_graph::graph_description::node_0w_0,
        t.report_line("Swap edges"),
        [](graph_t g) -> graph_t {
          g.swap_edges(0, 1, 0);
          return g;
        }
      );

      trg.join(
        weighted_directed_graph::graph_description::node_0_0w,
        weighted_directed_graph::graph_description::node_0w_0,
        t.report_line("Sort edges"),
        [](graph_t g) -> graph_t {
          g.sort_edges(g.cbegin_edges(0), g.cend_edges(0), [](const auto& lhs, const auto& rhs) { return lhs.weight() < rhs.weight(); });
          return g;
        }
      );

      // end 'weighted_directed_graph::graph_description::node_0_0w'

      // begin 'weighted_directed_graph::graph_description::node_nodew'

      trg.join(
        weighted_directed_graph::graph_description::node_nodew,
        weighted_directed_graph::graph_description::nodew_node,
        t.report_line("Swap nodes"),
        [](graph_t g) -> graph_t {
          g.swap_nodes(0, 1);
          return g;
        }
      );

      // end 'weighted_directed_graph::graph_description::node_nodew'

      // begin 'weighted_directed_graph::graph_description::nodew_node'

      trg.join(
        weighted_directed_graph::graph_description::nodew_node,
        weighted_directed_graph::graph_description::node_nodew,
        t.report_line("Swap nodes"),
        [](graph_t g) -> graph_t {
          g.swap_nodes(1, 0);
          return g;
        }
      );

      // end 'weighted_directed_graph::graph_description::nodew_node'

      // begin 'weighted_directed_graph::graph_description::node_1_nodew'

      trg.join(
        weighted_directed_graph::graph_description::node_1_nodew,
        weighted_directed_graph::graph_description::nodew_node_0,
        t.report_line("Swap nodes"),
        [](graph_t g) -> graph_t {
          g.swap_nodes(0, 1);
          return g;
        }
      );

      // end 'weighted_directed_graph::graph_description::node_1_nodew'

      // begin 'weighted_directed_graph::graph_description::nodew_node_0'

      trg.join(
        weighted_directed_graph::graph_description::nodew_node_0,
        weighted_directed_graph::graph_description::node_1_nodew,
        t.report_line("Swap nodes"),
        [](graph_t g) -> graph_t {
          g.swap_nodes(1, 0);
          return g;
        }
      );

      // end 'weighted_directed_graph::graph_description::nodew_node_0'

      // begin 'weighted_directed_graph::graph_description::node_1_1w_node'
      
      trg.join(
        weighted_directed_graph::graph_description::node_1_1w_node,
        weighted_directed_graph::graph_description::node_1w_1w_node,
        t.report_line("Set edge weight"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(g.cbegin_edges(0), 1.0);
          return g;
        }
      );

      trg.join(
        weighted_directed_graph::graph_description::node_1_1w_node,
        weighted_directed_graph::graph_description::node_1w_1_node,
        t.report_line("Swap edges"),
        [](graph_t g) -> graph_t {
          g.swap_edges(0, 0, 1);
          return g;
        }
      );

      trg.join(
        weighted_directed_graph::graph_description::node_1_1w_node,
        weighted_directed_graph::graph_description::node_1w_1_node,
        t.report_line("Sort edges"),
        [](graph_t g) -> graph_t {
          g.sort_edges(g.cbegin_edges(0), g.cend_edges(0), [](const auto& lhs, const auto& rhs) { return lhs.weight() > rhs.weight(); });
          return g;
        }
      );

      // end 'weighted_directed_graph::graph_description::node_1_1w_node'

      // begin 'weighted_directed_graph::graph_description::node_1w_1_node'

      trg.join(
        weighted_directed_graph::graph_description::node_1w_1_node,
        weighted_directed_graph::graph_description::node_1w_1w_node,
        t.report_line("Set edge weight"),
        [](graph_t g) -> graph_t {
          g.set_edge_weight(++g.cbegin_edges(0), 1.0);
          return g;
        }
      );

      trg.join(
        weighted_directed_graph::graph_description::node_1w_1_node,
        weighted_directed_graph::graph_description::node_1_1w_node,
        t.report_line("Swap edges"),
        [](graph_t g) -> graph_t {
          g.swap_edges(0, 0, 1);
          return g;
        }
      );

      trg.join(
        weighted_directed_graph::graph_description::node_1w_1_node,
        weighted_directed_graph::graph_description::node_1_1w_node,
        t.report_line("Sort edges"),
        [](graph_t g) -> graph_t {
          g.sort_edges(g.cbegin_edges(0), g.cend_edges(0), [](const auto& lhs, const auto& rhs) { return lhs.weight() < rhs.weight(); });
          return g;
        }
      );

      // end 'weighted_directed_graph::graph_description::node_1w_1_node'

      // begin 'weighted_directed_graph::graph_description::node_1w_1w_node'

      // end 'weighted_directed_graph::graph_description::node_1w_1w_node'

      return trg;
    }
  };


}