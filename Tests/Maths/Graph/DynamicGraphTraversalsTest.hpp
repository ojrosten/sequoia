////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "GraphTraversalTestingUtilities.hpp"

namespace sequoia::testing
{
  class test_graph_traversals final : public performance_test
  {
  public:
    using performance_test::performance_test;

    [[nodiscard]]
    std::string_view source_file() const noexcept final;
  private:
    struct null_weight {};

    template<class, class, class>
    friend class graph_test_helper;

    void run_tests() final;

    void test_prs_details();

    template
    <
      maths::graph_flavour GraphFlavour,
      class EdgeWeight,
      class NodeWeight,
      class EdgeWeightCreator,
      class NodeWeightCreator,
      class EdgeStorageTraits,
      class NodeWeightStorageTraits
    >
    void execute_operations();

    template<maths::dynamic_network Graph, class Traverser>
    void tracker_test();

    template<maths::dynamic_network Graph>
    void test_weighted_BFS_tasks();

    template<maths::dynamic_network Graph>
    void test_priority_traversal();

    template<maths::dynamic_network G>
    void test_square_graph(const G& g, std::size_t start, bfs_type);

    template<maths::dynamic_network G>
    void test_square_graph(const G& g, std::size_t start, dfs_type);

    template<maths::dynamic_network G>
    void test_square_graph(const G& g, const std::size_t start, pdfs_type);

    //=================== For weighted BFS  =================//

    template<maths::dynamic_network Graph>
    void test_node_and_first_edge_traversal();

    template<maths::dynamic_network Graph>
    void test_edge_second_traversal();
  };
}
