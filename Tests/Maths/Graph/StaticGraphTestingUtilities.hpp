////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "GraphTestingUtilities.hpp"
#include "sequoia/Maths/Graph/StaticGraph.hpp"

namespace sequoia::testing
{
  // Details Checkers

  template
  <
    maths::directed_flavour Directedness,
    std::size_t Size,
    std::size_t Order,
    class EdgeWeight,
    class NodeWeight,
    class Traits
  >
  struct value_tester<maths::static_graph<Directedness, Size, Order, EdgeWeight, NodeWeight, Traits>>
  {
    using graph_type = maths::static_graph<Directedness, Size, Order, EdgeWeight, NodeWeight, Traits>;

    using connectivity_equivalent_type = std::initializer_list<std::initializer_list<typename graph_type::edge_init_type>>;
    using nodes_equivalent_type        = std::initializer_list<NodeWeight>;

    template<test_mode Mode>
    static void test_equality(test_logger<Mode>& logger, const graph_type& graph, const graph_type& prediction)
    {
      graph_equality_tester::test_equality(logger, graph, prediction);
    }

    template<test_mode Mode>
      requires (!std::is_empty_v<NodeWeight>)
    static void test_equivalence(test_logger<Mode>& logger, const graph_type& graph, connectivity_equivalent_type connPrediction, const nodes_equivalent_type& nodesPrediction)
    {
      graph_equivalence_tester::test_general_equivalence(logger, graph, connPrediction, nodesPrediction);
    }

    template<test_mode Mode>
      requires std::is_empty_v<NodeWeight>
    static void test_equivalence(test_logger<Mode>& logger, const graph_type& graph, connectivity_equivalent_type connPrediction)
    {
      graph_equivalence_tester::test_general_equivalence(logger, graph, connPrediction);
    }
  private:
    using graph_equality_tester
      = impl::graph_value_tester<graph_type>;

    using graph_equivalence_tester
      = impl::graph_equivalence_checker<graph_type>;
  };

  template
  <
    maths::directed_flavour Directedness,
    std::size_t Size,
    std::size_t Order,
    class EdgeWeight,
    class NodeWeight,
    class Traits
  >
  struct value_tester<maths::static_embedded_graph<Directedness, Size, Order, EdgeWeight, NodeWeight, Traits>>
  {
    using graph_type = maths::static_embedded_graph<Directedness, Size, Order, EdgeWeight, NodeWeight, Traits>;

    using connectivity_equivalent_type = std::initializer_list<std::initializer_list<typename graph_type::edge_init_type>>;
    using nodes_equivalent_type        = std::initializer_list<NodeWeight>;

    template<test_mode Mode>
    static void test_equality(test_logger<Mode>& logger, const graph_type& graph, const graph_type& prediction)
    {
      graph_equality_tester::test_equality(logger, graph, prediction);
    }

    template<test_mode Mode>
      requires (!std::is_empty_v<NodeWeight>)
    static void test_equivalence(test_logger<Mode>& logger, const graph_type& graph, connectivity_equivalent_type connPrediction, const nodes_equivalent_type& nodesPrediction)
    {
      graph_equivalence_tester::test_general_equivalence(logger, graph, connPrediction, nodesPrediction);
    }

    template<test_mode Mode>
      requires std::is_empty_v<NodeWeight>
    static void test_equivalence(test_logger<Mode>& logger, const graph_type& graph, connectivity_equivalent_type connPrediction)
    {
      graph_equivalence_tester::test_general_equivalence(logger, graph, connPrediction);
    }
  private:
    using graph_equality_tester = impl::graph_value_tester<graph_type>;

    using graph_equivalence_tester = impl::graph_equivalence_checker<graph_type>;
  };
}
