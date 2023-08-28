////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "GraphTraversalTestingUtilities.hpp"

#include "sequoia/Maths/Graph/DynamicGraphTraversals.hpp"

namespace sequoia::testing
{
  class test_graph_update final : public free_test
  {
  public:
    using free_test::free_test;

    [[nodiscard]]
    std::filesystem::path source_file() const;

    void run_tests();
  private:
    template <class, class, concrete_test>
    friend class graph_test_helper;

    template
    <
      maths::graph_flavour GraphFlavour,
      class EdgeWeight,
      class NodeWeight,
      class EdgeStorage,
      class NodeWeightStorageTraits
     >
    void execute_operations();

    //============================= General traversal tests ============================//

    template<class Graph>
    void test_update();

    template<class Graph>
    [[nodiscard]]
    Graph make_graph();

    template<class Graph>
    void check_setup(const Graph& graph);

    template<class Graph>
    void check_df_update(Graph graph);

    template<class Graph>
    void check_bf_update(Graph graph);

    template<class Graph>
    void check_pr_update(Graph graph);

    //============================= Breadth-first only tests ===========================//

    template<class Graph>
    void test_bf_update();

    template<class Graph>
    void test_second_edge_traversal_update(Graph& graph);
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

    template<std::input_or_output_iterator Iter>
    void firstEdgeTraversal(Iter citer)
    {
      const auto newWeight = 10 + m_EdgeTraversalIndex + citer->weight();
      m_Graph.set_edge_weight(citer, newWeight);

      ++m_EdgeTraversalIndex;
    }

    template<std::input_or_output_iterator Iter>
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
}
