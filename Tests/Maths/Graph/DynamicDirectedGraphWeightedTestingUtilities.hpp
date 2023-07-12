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
    enum dgraph_description : std::size_t {
      // x
      node = directed_graph::graph_description::end,

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
    : public dynamic_directed_graph_operations<EdgeWeight, NodeWeight, EdgeWeightCreator, NodeWeightCreator, EdgeStorageTraits, NodeWeightStorageTraits>
  {
    template<maths::network>
    friend struct graph_initialization_checker;
   public:
    using graph_t            = maths::graph<maths::directed_flavour::directed, EdgeWeight, NodeWeight, EdgeWeightCreator, NodeWeightCreator, EdgeStorageTraits, NodeWeightStorageTraits>;
    using edge_t             = typename graph_t::edge_init_type;
    using edges_equivalent_t = std::initializer_list<std::initializer_list<edge_t>>;
    using transition_graph   = typename transition_checker<graph_t>::transition_graph;

    [[nodiscard]]
    graph_t make_and_check(std::string_view description, edges_equivalent_t init)
    {
      return graph_initialization_checker<graph_t>::make_and_check(*this, description, init);
    }

    [[nodiscard]]
    transition_graph make_weighted_transition_graph()
    {
      auto trg{this->make_weighted_transition_graph()};

      return trg;
    }
  };
}