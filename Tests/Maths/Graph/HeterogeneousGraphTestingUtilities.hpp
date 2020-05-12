////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "GraphTestingUtilities.hpp"
#include "HeterogeneousStaticGraph.hpp"
#include "NodeStorageTestingUtilities.hpp"

namespace sequoia::unit_testing
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
      static void check(std::string_view description, test_logger<Mode>& logger, const type& graph, connectivity_equivalent_type connPrediction, const nodes_equivalent_type& nodesPrediction)
      {
        using connectivity_t = typename type::connectivity_type;
        using nodes_t = typename type::nodes_type;

        check_equivalence(description, logger, static_cast<const connectivity_t&>(graph), connPrediction);
        check_equivalence(description, logger, static_cast<const nodes_t&>(graph), nodesPrediction);
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
  struct detailed_equality_checker<maths::heterogeneous_static_graph<Directedness, Size, Order, EdgeWeight, NodeWeights...>>
    : impl::graph_detailed_equality_checker<maths::heterogeneous_static_graph<Directedness, Size, Order, EdgeWeight, NodeWeights...>>
  {   
  };

  template
  <
    maths::directed_flavour Directedness,      
    std::size_t Size,
    std::size_t Order,      
    class EdgeWeight,
    class... NodeWeights
  >
  struct detailed_equality_checker<maths::heterogeneous_embedded_static_graph<Directedness, Size, Order, EdgeWeight, NodeWeights...>>
    : impl::graph_detailed_equality_checker<maths::heterogeneous_embedded_static_graph<Directedness, Size, Order, EdgeWeight, NodeWeights...>>
  {   
  };

  // Equivalence Checkers

  template
  <
    maths::directed_flavour Directedness,      
    std::size_t Size,
    std::size_t Order,      
    class EdgeWeight,
    class... NodeWeights
  >
  struct equivalence_checker<maths::heterogeneous_static_graph<Directedness, Size, Order, EdgeWeight, NodeWeights...>>
    : impl::heterogeneous_graph_equivalence_checker<maths::heterogeneous_static_graph<Directedness, Size, Order, EdgeWeight, NodeWeights...>, NodeWeights...>
  {
  };

  template
  <
    maths::directed_flavour Directedness,      
    std::size_t Size,
    std::size_t Order,      
    class EdgeWeight,
    class... NodeWeights
  >
  struct equivalence_checker<maths::heterogeneous_embedded_static_graph<Directedness, Size, Order, EdgeWeight, NodeWeights...>>
    : impl::heterogeneous_graph_equivalence_checker<maths::heterogeneous_embedded_static_graph<Directedness, Size, Order, EdgeWeight, NodeWeights...>, NodeWeights...>
  {
  };
}
