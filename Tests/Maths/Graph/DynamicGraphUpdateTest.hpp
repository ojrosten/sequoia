////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "DynamicGraphTestingUtilities.hpp"

#include "DynamicGraphTraversals.hpp"
#include "ConcurrencyModels.hpp"

#include <functional>

namespace sequoia
{
  namespace testing
  {
    class test_graph_update final : public graph_unit_test
    {
    public:
      using graph_unit_test::graph_unit_test;

      [[nodiscard]]
    std::string_view source_file() const noexcept final;
    private:
      void run_tests() final;
    };

    template<class G>
    std::tuple<std::size_t, std::size_t, std::size_t>
    nth_connection_indices(const G& graph, const std::size_t node, const std::size_t localEdgeIndex);

    //=======================================================================================//
    // Class which will be used to provide graph update functions

    template <class G>
    class graph_updater
    {
    public:
      graph_updater(G& graph) : m_Graph(graph) {}
      
      void firstNodeTraversal(const std::size_t index)
      {
        auto iter = m_Graph.cbegin_node_weights() + index;
        const auto newWeight = (2 + m_NodeTraversalIndex) * *iter;
        m_Graph.node_weight(iter, newWeight);

        ++m_NodeTraversalIndex;
      }

      void secondNodeTraversal(const std::size_t index)
      {
        --m_NodeTraversalIndex;
        auto iter = m_Graph.cbegin_node_weights() + index;
        const auto newWeight =  *iter / (5 - m_NodeTraversalIndex);
        m_Graph.node_weight(iter, newWeight);
      }

      template<class Iter>
      void firstEdgeTraversal(Iter citer)
      {
        const auto newWeight = 10 + m_EdgeTraversalIndex + citer->weight();
        m_Graph.set_edge_weight(citer, newWeight);

        ++m_EdgeTraversalIndex;
      }

      template<class Iter>
      void secondEdgeTraversal(Iter citer)
      {
        --m_EdgeTraversalIndex;
        const auto newWeight = citer->weight() - 14 + m_EdgeTraversalIndex;
        m_Graph.set_edge_weight(citer, newWeight);
      }
    private:
      G& m_Graph;
      
      std::size_t m_NodeTraversalIndex{},
                  m_EdgeTraversalIndex{};
    };

    //=======================================================================================//
    // Cyclic graph created and updated using all algorithms
    
    template
    <
      maths::graph_flavour GraphFlavour,      
      class EdgeWeight,
      class NodeWeight,      
      class EdgeWeightPooling,
      class NodeWeightPooling,
      class EdgeStorageTraits,
      class NodeWeightStorageTraits
    >
    class test_update
      : public graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>
    {
    private:
      using base_t = graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>;

      using graph_t = typename base_t::graph_type;
      using checker_t = typename base_t::checker_type;

      using ei_t = typename graph_t::edge_init_type;

      using flavour = maths::graph_flavour;

      using checker_t::check_equality;
      using checker_t::check_range;

      static constexpr bool undirected{maths::undirected(GraphFlavour)};

      auto make_graph() -> graph_t;
      void check_setup(const graph_t& graph);
      void check_df_update(graph_t graph);
      void check_bf_update(graph_t graph);
      void check_pr_update(graph_t graph);
      
      void execute_operations() override;
    };
    
    //=======================================================================================//
    // Graph (tree) created and searched; at each vertex,
    // and edge, an update of the weights is performed

    template
    <
      maths::graph_flavour GraphFlavour,
      class EdgeWeight,
      class NodeWeight,      
      class EdgeWeightPooling,
      class NodeWeightPooling,
      class EdgeStorageTraits,
      class NodeWeightStorageTraits
    >
    class test_bf_update
      : public graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>
    {
    private:
      using base_t = graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>;
      
      using graph_t = typename base_t::graph_type;
      using checker_t = typename base_t::checker_type;

      using ei_t = typename graph_t::edge_init_type;
      using ew_t = std::vector<double>;
      
      using checker_t::check_equality;
      using checker_t::check_exception_thrown;
      using checker_t::check_range;
      
      void execute_operations() override;

      void test_second_edge_traversal_update(graph_t& graph);
    };
  }
}
