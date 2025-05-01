////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "GraphTraversalTestingUtilities.hpp"

#include "sequoia/TestFramework/PerformanceTestCore.hpp"

namespace sequoia::testing
{
  class test_graph_traversals final : public performance_test
  {
  public:
    using performance_test::performance_test;

    [[nodiscard]]
    std::filesystem::path source_file() const;

    void run_tests();
  private:
    template<class, class, concrete_test>
    friend class graph_test_helper;

    void test_prs_details();

    template
    <
      maths::graph_flavour GraphFlavour,
      class EdgeWeight,
      class NodeWeight,
      class EdgeStorageConfig,
      class NodeWeightStorage
    >
    void execute_operations();

    template<maths::dynamic_network Graph, class Traverser>
    void tracker_test();

    template<maths::dynamic_network Graph>
    void test_weighted_BFS_tasks();

    template<maths::dynamic_network Graph>
    void test_priority_traversal();

    template<maths::dynamic_network G, class MessageMaker>
    void test_square_graph(const G& g, std::size_t start, MessageMaker messageMaker, bfs_type);

    template<maths::dynamic_network G, class MessageMaker>
    void test_square_graph(const G& g, std::size_t start, MessageMaker messageMaker, dfs_type);

    template<maths::dynamic_network G, class MessageMaker>
    void test_square_graph(const G& g, const std::size_t start, MessageMaker messageMaker, pdfs_type);

    //=================== For weighted breadth_first  =================//

    template<maths::dynamic_network Graph>
    void test_node_and_first_edge_traversal();

    template<maths::dynamic_network Graph>
    void test_edge_second_traversal();
  };
}
