////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "GraphTestingUtilities.hpp"
#include "sequoia/Maths/Graph/HeterogeneousStaticGraph.hpp"
#include "NodeStorageTestingUtilities.hpp"

namespace sequoia::testing
{
  namespace impl
  {
    template<class Graph, class... NodeWeights>
    struct heterogeneous_graph_equivalence_checker
    {
      using type = Graph;

      using connectivity_equivalent_type = std::initializer_list<std::initializer_list<typename type::edge_init_type>>;

      using nodes_equivalent_type = std::tuple<NodeWeights...>;

      template<test_mode Mode>
      static void test_equivalence(test_logger<Mode>& logger, const type& graph, connectivity_equivalent_type connPrediction, const nodes_equivalent_type& nodesPrediction)
      {
        using connectivity_t = typename type::connectivity_type;
        using nodes_t = typename type::nodes_type;

        check_equivalence("", logger, static_cast<const connectivity_t&>(graph), connPrediction);
        check_equivalence("", logger, static_cast<const nodes_t&>(graph), nodesPrediction);
      }
    };
  }

  // Details Checkers

  template
  <
    maths::directed_flavour Directedness,
    std::size_t Size,
    std::size_t Order,
    class EdgeWeight,
    class... NodeWeights
  >
  struct value_tester<maths::heterogeneous_static_graph<Directedness, Size, Order, EdgeWeight, NodeWeights...>>
  {
    using graph_type = maths::heterogeneous_static_graph<Directedness, Size, Order, EdgeWeight, NodeWeights...>;

    using connectivity_equivalent_type = std::initializer_list<std::initializer_list<typename graph_type::edge_init_type>>;
    using nodes_equivalent_type        = std::tuple<NodeWeights...>;

    template<test_mode Mode>
    static void test_equality(test_logger<Mode>& logger, const graph_type& graph, const graph_type& prediction)
    {
      graph_equality_tester::test_equality(logger, graph, prediction);
    }

    template<test_mode Mode>
    static void test_equivalence(test_logger<Mode>& logger, const graph_type& graph, connectivity_equivalent_type connPrediction, const nodes_equivalent_type& nodesPrediction)
    {
      graph_equivalence_tester::test_equivalence(logger, graph, connPrediction, nodesPrediction);
    }
  private:
    using graph_equality_tester = impl::graph_value_tester<graph_type>;

    using graph_equivalence_tester = impl::heterogeneous_graph_equivalence_checker<graph_type, NodeWeights...>;
  };

  template
  <
    maths::directed_flavour Directedness,
    std::size_t Size,
    std::size_t Order,
    class EdgeWeight,
    class... NodeWeights
  >
  struct value_tester<maths::heterogeneous_embedded_static_graph<Directedness, Size, Order, EdgeWeight, NodeWeights...>>
  {
    using graph_type = maths::heterogeneous_embedded_static_graph<Directedness, Size, Order, EdgeWeight, NodeWeights...>;

    using connectivity_equivalent_type = std::initializer_list<std::initializer_list<typename graph_type::edge_init_type>>;
    using nodes_equivalent_type        = std::tuple<NodeWeights...>;

    template<test_mode Mode>
    static void test_equality(test_logger<Mode>& logger, const graph_type& graph, const graph_type& prediction)
    {
      graph_equality_tester::test_equality(logger, graph, prediction);
    }

    template<test_mode Mode>
    static void test_equivalence(test_logger<Mode>& logger, const graph_type& graph, connectivity_equivalent_type connPrediction, const nodes_equivalent_type& nodesPrediction)
    {
      graph_equivalence_tester::test_equivalence(logger, graph, connPrediction, nodesPrediction);
    }
  private:
    using graph_equality_tester  = impl::graph_value_tester<graph_type>;

    using graph_equivalence_tester = impl::heterogeneous_graph_equivalence_checker<graph_type, NodeWeights...>;
  };
}
