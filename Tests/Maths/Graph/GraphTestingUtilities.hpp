////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "Components/Edges/EdgeTestingUtilities.hpp"
#include "Components/Nodes/NodeStorageTestingUtilities.hpp"

#include "sequoia/Core/DataStructures/PartitionedData.hpp"
#include "sequoia/Maths/Graph/GraphPrimitive.hpp"
#include "sequoia/Maths/Graph/GraphTraits.hpp"

namespace sequoia::testing
{
  template<maths::graph_flavour Flavour, class EdgeStorage>
  struct value_tester<maths::connectivity<Flavour, EdgeStorage>>
  {
    using type            = maths::connectivity<Flavour, EdgeStorage>;
    using edge_index_type = typename type::edge_index_type;

    template<class E>
    using connectivity_equivalent_type = std::initializer_list<std::initializer_list<E>>;

    template<class CheckType, test_mode Mode>
    static void test(CheckType flavour, test_logger<Mode>& logger, const type& connectivity, const type& prediction)
    {
      check(equality, "Connectivity size incorrect", logger, connectivity.size(), prediction.size());

      if(check(equality, "Connectivity order incorrect", logger, connectivity.order(), prediction.order()))
      {
        for(edge_index_type i{}; i<connectivity.order(); ++i)
        {
          const auto message{std::string{"Partition "}.append(std::to_string(i))};
          check(flavour, append_lines(message, "cedge_iterator"), logger, connectivity.cbegin_edges(i), connectivity.cend_edges(i), prediction.cbegin_edges(i), prediction.cend_edges(i));
        }
      }
    }

    template<class CheckType, test_mode Mode, class E>
    static void test(CheckType flavour, test_logger<Mode>& logger, const type& connectivity, connectivity_equivalent_type<E> prediction)
    {
      check_connectivity(flavour, logger, connectivity, prediction);
    }
  private:
    template<class CheckType, test_mode Mode, class E>
    static void check_connectivity(CheckType flavour, test_logger<Mode>& logger, const type& connectivity, connectivity_equivalent_type<E> prediction)
    {
      if(check(equality, "Connectivity order incorrect", logger, connectivity.order(), prediction.size()))
      {
        for(edge_index_type i{}; i < connectivity.order(); ++i)
        {
          const auto message{"Partition " + std::to_string(i)};
          check(flavour, append_lines(message, "cedge_iterator"),  logger, connectivity.cbegin_edges(i),   connectivity.cend_edges(i),   std::begin(*(prediction.begin() + i)),  std::end(*(prediction.begin() + i)));
          check(flavour, append_lines(message, "credge_iterator"), logger, connectivity.crbegin_edges(i),  connectivity.crend_edges(i),  std::rbegin(*(prediction.begin() + i)), std::rend(*(prediction.begin() + i)));
          check(flavour, append_lines(message, "cedges"),          logger, connectivity.cedges(i).begin(), connectivity.cedges(i).end(), std::begin(*(prediction.begin() + i)),  std::end(*(prediction.begin() + i)));

          if constexpr((type::flavour == maths::graph_flavour::directed) && !std::is_empty_v<typename E::weight_type>)
          {
            using init_iterator   = typename std::initializer_list<E>::iterator;
            using weight_iterator = utilities::iterator<init_iterator, maths::edge_weight_dereference_policy<init_iterator>>;
            using reverse_init_iterator   = std::reverse_iterator<init_iterator>;
            using reverse_weight_iterator = utilities::iterator<reverse_init_iterator, maths::edge_weight_dereference_policy<reverse_init_iterator>>;

            check(flavour, append_lines(message, "edge_weight_iterator"),   logger, connectivity.begin_edge_weights(i),  connectivity.end_edge_weights(i),    weight_iterator{std::begin(*(prediction.begin() + i))},  weight_iterator{std::end(*(prediction.begin() + i))});
            check(flavour, append_lines(message, "cedge_weight_iterator"),  logger, connectivity.cbegin_edge_weights(i),  connectivity.cend_edge_weights(i),  weight_iterator{std::begin(*(prediction.begin() + i))},  weight_iterator{std::end(*(prediction.begin() + i))});
            check(flavour, append_lines(message, "redge_weight_iterator"),  logger, connectivity.rbegin_edge_weights(i),  connectivity.rend_edge_weights(i),  reverse_weight_iterator{std::rbegin(*(prediction.begin() + i))}, reverse_weight_iterator{std::rend(*(prediction.begin() + i))});
            check(flavour, append_lines(message, "credge_weight_iterator"), logger, connectivity.crbegin_edge_weights(i), connectivity.crend_edge_weights(i), reverse_weight_iterator{std::rbegin(*(prediction.begin() + i))}, reverse_weight_iterator{std::rend(*(prediction.begin() + i))});
            check(flavour, append_lines(message, "edge_weights"),           logger, connectivity.edge_weights(i).begin(), connectivity.edge_weights(i).end(), weight_iterator{std::begin(*(prediction.begin() + i))},  weight_iterator{std::end(*(prediction.begin() + i))});
            check(flavour, append_lines(message, "cedge_weights"),          logger, connectivity.edge_weights(i).begin(), connectivity.edge_weights(i).end(), weight_iterator{std::begin(*(prediction.begin() + i))},  weight_iterator{std::end(*(prediction.begin() + i))});

          }
        }
      }
    }
  };

  template<maths::network Graph>
  struct graph_value_tester_base
  {
    using type = Graph;

    template<class E>
    using connectivity_equivalent_type = std::initializer_list<std::initializer_list<E>>;

    using connectivity_type = typename type::connectivity_type;
    using nodes_type = typename type::nodes_type;

    template<class CheckType, test_mode Mode, maths::network G>
      requires std::is_same_v<Graph, G> // inhibit implicit conversions
    static void test(CheckType flavour, test_logger<Mode>& logger, const Graph& graph, const G& prediction)
    {
      check(flavour, "", logger, static_cast<const connectivity_type&>(graph), static_cast<const connectivity_type&>(prediction));
      check(flavour, "", logger, static_cast<const nodes_type &>(graph), static_cast<const nodes_type&>(prediction));
    }

    template<class CheckType, test_mode Mode, class E, class NodesEquivalentType>
      requires (!std::is_empty_v<nodes_type>)
    static void test(CheckType flavour, test_logger<Mode>& logger, const type& graph, connectivity_equivalent_type<E> connPrediction, NodesEquivalentType&& nodesPrediction)
    {
      check(flavour, "", logger, static_cast<const connectivity_type&>(graph), connPrediction);
      check(flavour, "", logger, static_cast<const nodes_type&>(graph), std::forward<NodesEquivalentType>(nodesPrediction));
    }

    template<class CheckType, test_mode Mode, class E>
      requires (!std::is_empty_v<nodes_type>) && (!maths::heterogeneous_network<Graph>)
    static void test(CheckType flavour, test_logger<Mode>& logger, const type& graph, connectivity_equivalent_type<E> connPrediction)
    {
      check(flavour, "", logger, static_cast<const connectivity_type&>(graph), connPrediction);
      const std::vector<typename Graph::node_weight_type> defaultNodes(connPrediction.size());
      check(flavour, "", logger, graph.cbegin_node_weights(), graph.cend_node_weights(), defaultNodes.begin(), defaultNodes.end());
    }

    template<class CheckType, test_mode Mode, class E>
      requires std::is_empty_v<nodes_type>
    static void test(CheckType flavour, test_logger<Mode>& logger, const type& graph, connectivity_equivalent_type<E> connPrediction)
    {
      check(flavour, "", logger, static_cast<const connectivity_type&>(graph), connPrediction);
    }
  };

  template<maths::network Graph>
  struct value_tester<Graph> : graph_value_tester_base<Graph>
  {};
}
