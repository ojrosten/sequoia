////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "DynamicGraphUpdateTest.hpp"
#include "DynamicGraphTestingUtilities.hpp"

namespace sequoia::testing
{
  template<class G>
  std::tuple<std::size_t, std::size_t, std::size_t> nth_connection_indices(const G& graph, const std::size_t node, const std::size_t localEdgeIndex)
  {
    auto begin = graph.cbegin_edges(node);
    auto end   = graph.cend_edges(node);

    if(localEdgeIndex >= static_cast<std::size_t>(distance(begin, end)))
      throw std::out_of_range("graph_utilities::nth_connection_indices - localEdgeIndex out of range");

    auto& edge = *(begin + localEdgeIndex);
    const std::size_t targ = edge.target_node();
    std::size_t nthConnection{};

    for(auto citer = begin; citer != begin + localEdgeIndex; ++citer)
      {
        auto& currentEdge = (*citer);
        if(targ == currentEdge.target_node()) ++nthConnection;
      }

    return std::make_tuple(node, targ, nthConnection);
  }

  [[nodiscard]]
  std::string_view test_graph_update::source_file() const noexcept
  {
    return __FILE__;
  }

  void test_graph_update::run_tests()
  {
    {
      graph_test_helper<std::vector<double>, std::vector<int>, test_graph_update> helper{*this};
      helper.run_tests();
    }

    {
      graph_test_helper<size_t, size_t, test_graph_update> helper{*this};
      helper.run_tests();
    }
  }

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
  void test_graph_update::execute_operations()
  {
    using ESTraits = EdgeStorageTraits;
    using NSTraits = NodeWeightStorageTraits;
    using graph_type = graph_type_generator_t<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightCreator, NodeWeightCreator, ESTraits, NSTraits>;
    if constexpr(std::is_same_v<EdgeWeight, std::vector<double>>)
    {
      test_bf_update<graph_type>();
    }
    else
    {
      test_update<graph_type>();
    }
  }

  //============================= General traversal tests ============================//
  // Cyclic graph created and updated using all algorithms

  template<class Graph>
  [[nodiscard]]
  Graph test_graph_update::make_graph()
  {
    Graph graph;

    graph.add_node(5_sz);
    graph.add_node(2_sz);
    graph.add_node(10_sz);
    graph.add_node(4_sz);

    graph.join(0, 1, 1_sz);
    graph.join(1, 3, 3_sz);
    graph.join(3, 2, 4_sz);
    graph.join(0, 2, 7_sz);
    graph.join(2, 0, 2_sz);

    //       7
    //(0)5=======10(2)
    //   |   2   |
    //  1|       |4
    //   |       |
    //(1)2-------4(3)
    //       3

    return graph;
  }

  //============================= check_setup =============================//

  template<class Graph>
  void test_graph_update::check_setup(const Graph& graph)
  {
    using ei_t = typename Graph::edge_init_type;
    using flavour = maths::graph_flavour;
    constexpr auto GraphFlavour{Graph::flavour};

    if constexpr(GraphFlavour == flavour::undirected)
    {
      Graph expected{
        {{ei_t{1,1_sz}, ei_t{2,7_sz}, ei_t{2,2_sz}},
         {ei_t{0,1_sz}, ei_t{3,3_sz}},
         {ei_t{3,4_sz}, ei_t{0,7_sz}, ei_t{0,2_sz}},
         {ei_t{1,3_sz}, ei_t{2,4_sz}}},
        {5_sz, 2_sz, 10_sz, 4_sz}
      };

      expected.swap_edges(0, 1, 2);
      expected.swap_edges(2, 0, 2);
      check(equality, LINE(""), graph, expected);
    }
    else if constexpr(GraphFlavour == flavour::directed)
    {
      Graph expected{
        {{ei_t{1,1_sz}, ei_t{2,7_sz}},
         {ei_t{3,3_sz}},
         {ei_t{0,2_sz}},
         {ei_t{2,4_sz}}},
        {5_sz, 2_sz, 10_sz, 4_sz}
      };

      check(equality, LINE(""), graph, expected);
    }
    else if constexpr(GraphFlavour == flavour::undirected_embedded)
    {
      Graph expected{
        {{ei_t{1,0,1_sz}, ei_t{2,1,7_sz}, ei_t{2,2,2_sz}},
         {ei_t{0,0,1_sz}, ei_t{3,0,3_sz}},
         {ei_t{3,1,4_sz}, ei_t{0,1,7_sz}, ei_t{0,2,2_sz}},
         {ei_t{1,1,3_sz}, ei_t{2,0,4_sz}}},
        {5_sz, 2_sz, 10_sz, 4_sz}
      };

      check(equality, LINE(""), graph, expected);
    }
    else if constexpr(GraphFlavour == flavour::directed_embedded)
    {
      Graph expected{
        {{ei_t{0,1,0,1_sz}, ei_t{0,2,1,7_sz}, ei_t{2,0,2,2_sz}},
         {ei_t{0,1,0,1_sz}, ei_t{1,3,0,3_sz}},
         {ei_t{3,2,1,4_sz}, ei_t{0,2,1,7_sz}, ei_t{2,0,2,2_sz}},
         {ei_t{1,3,1,3_sz}, ei_t{3,2,0,4_sz}}},
        {5_sz, 2_sz, 10_sz, 4_sz}
      };

      check(equality, LINE(""), graph, expected);
    }
    else
    {
      static_assert(dependent_false<Graph>::value);
    }
  }

  //============================= check_df_update =============================//

  template<class Graph>
  void test_graph_update::check_df_update(Graph graph)
  {
    using namespace maths;

    graph_updater<Graph> updater(graph);
    auto firstNodeFn = [&updater](const std::size_t index){ updater.firstNodeTraversal(index); };
    pseudo_depth_first_search(graph, find_disconnected_t{}, firstNodeFn);

    // node_weight *=  (2 + traversal index)
    //
    //        7
    //(0)10=======50(2)
    //    |   2   |
    //   1|       |4
    //    |       |
    // (1)6-------16(3)
    //        3

    std::array<std::size_t, 4> expectedNodeWeights{10,6,50,16};
    check(equality, LINE(""), graph.cbegin_node_weights(), graph.cend_node_weights(), expectedNodeWeights.cbegin(), expectedNodeWeights.cend());

    auto secondNodeFn = [&updater](const std::size_t index){ updater.secondNodeTraversal(index); };
    pseudo_depth_first_search(graph, find_disconnected_t{}, null_func_obj{}, secondNodeFn);

    // node_weight / = (2 + traversal index)
    //
    //       7
    //(0)5=======10(2)
    //   |   2   |
    //  1|       |4
    //   |       |
    //(1)2-------4(3)
    //       3

    expectedNodeWeights = {5,2,10,4};
    check(equality, LINE(""), graph.cbegin_node_weights(), graph.cend_node_weights(), expectedNodeWeights.cbegin(), expectedNodeWeights.cend());

    auto firstEdgeFn = [&updater](auto citer) { updater.firstEdgeTraversal(citer); };
    pseudo_depth_first_search(graph, find_disconnected_t{}, null_func_obj{}, null_func_obj{}, firstEdgeFn);

    // edge_weight += 10 + traversal index
    //
    //  Undirected            Directed        0-1;1-3;3-2;0-2;2-0;
    //      18                  17
    //(0)5=======10(2)   (0)5=======10(2)
    //   |  12   |          |   16   |
    // 13|       |18      12|        |17
    //   |       |          |        |
    //(1)2-------4(3)    (1)2--------4(3)
    //      16                  15

    using namespace maths;
    using ei_t = typename Graph::edge_init_type;
    using flavour = maths::graph_flavour;
    constexpr auto GraphFlavour{Graph::flavour};
    constexpr bool undirected{maths::undirected(GraphFlavour)};

    if constexpr(GraphFlavour == flavour::undirected)
    {
      Graph expected{
        {{ei_t{1,13_sz}, ei_t{2,18_sz}, ei_t{2,12_sz}},
         {ei_t{0,13_sz}, ei_t{3,16_sz}},
         {ei_t{3,18_sz}, ei_t{0,18_sz}, ei_t{0,12_sz}},
         {ei_t{1,16_sz}, ei_t{2,18_sz}}},
        {5_sz, 2_sz, 10_sz, 4_sz}
      };

      expected.swap_edges(0, 1, 2);
      expected.swap_edges(2, 0, 2);
      check(equality, LINE(""), graph, expected);
    }
    else if constexpr(GraphFlavour == flavour::directed)
    {
      Graph expected{
        {{ei_t{1,12_sz}, ei_t{2,17_sz}},
         {ei_t{3,15_sz}},
         {ei_t{0,16_sz}},
         {ei_t{2,17_sz}}},
        {5_sz, 2_sz, 10_sz, 4_sz}
      };

      check(equality, LINE(""), graph, expected);
    }
    else if constexpr(GraphFlavour == flavour::undirected_embedded)
    {
      Graph expected{
        {{ei_t{1,0,13_sz}, ei_t{2,1,18_sz}, ei_t{2,2,12_sz}},
         {ei_t{0,0,13_sz}, ei_t{3,0,16_sz}},
         {ei_t{3,1,18_sz}, ei_t{0,1,18_sz}, ei_t{0,2,12_sz}},
         {ei_t{1,1,16_sz}, ei_t{2,0,18_sz}}},
        {5_sz, 2_sz, 10_sz, 4_sz}
      };

      check(equality, LINE(""), graph, expected);
    }
    else if constexpr(GraphFlavour == flavour::directed_embedded)
    {
      Graph expected{
        {{ei_t{0,1,0,12_sz}, ei_t{0,2,1,17_sz}, ei_t{2,0,2,16_sz}},
         {ei_t{0,1,0,12_sz}, ei_t{1,3,0,15_sz}},
         {ei_t{3,2,1,17_sz}, ei_t{0,2,1,17_sz}, ei_t{2,0,2,16_sz}},
         {ei_t{1,3,1,15_sz}, ei_t{3,2,0,17_sz}}},
        {5_sz, 2_sz, 10_sz, 4_sz}
      };

      check(equality, LINE(""), graph, expected);
    }
    else
    {
      static_assert(dependent_false<Graph>::value);
    }

    using namespace maths;
    auto secondEdgeFn = [&updater](auto citer) { updater.secondEdgeTraversal(citer); };
    if constexpr(undirected)
    {
      pseudo_depth_first_search(graph, find_disconnected_t{}, null_func_obj{}, null_func_obj{}, null_func_obj{}, secondEdgeFn);
    }
    else
    {
      pseudo_depth_first_search(graph, find_disconnected_t{}, null_func_obj{}, null_func_obj{}, secondEdgeFn);
    }

    // edge_weight -= (10 + traversal index)
    //
    //  Undirected            Directed        0-1;1-3;3-2;0-2;2-0;
    //       5                   7
    //(0)5=======10(2)   (0)5=======10(2)
    //   |   0   |          |    2   |
    //  3|       |4        1|        |4
    //   |       |          |        |
    //(1)2-------4(3)    (1)2--------4(3)
    //       5                  3

    if constexpr(GraphFlavour == flavour::undirected)
    {
      Graph expected{
        {{ei_t{1,3_sz}, ei_t{2,5_sz}, ei_t{2,0_sz}},
         {ei_t{0,3_sz}, ei_t{3,5_sz}},
         {ei_t{3,4_sz}, ei_t{0,5_sz}, ei_t{0,0_sz}},
         {ei_t{1,5_sz}, ei_t{2,4_sz}}},
        {5_sz, 2_sz, 10_sz, 4_sz}
      };

      expected.swap_edges(0, 1, 2);
      expected.swap_edges(2, 0, 2);
      check(equality, LINE(""), graph, expected);
    }
    else if constexpr(GraphFlavour == flavour::directed)
    {
      Graph expected{
        {{ei_t{1,1_sz}, ei_t{2,7_sz}},
         {ei_t{3,3_sz}},
         {ei_t{0,2_sz}},
         {ei_t{2,4_sz}}},
        {5_sz, 2_sz, 10_sz, 4_sz}
      };

      check(equality, LINE(""), graph, expected);
    }
    else if constexpr(GraphFlavour == flavour::undirected_embedded)
    {
      Graph expected{
        {{ei_t{1,0,3_sz}, ei_t{2,1,5_sz}, ei_t{2,2,0_sz}},
         {ei_t{0,0,3_sz}, ei_t{3,0,5_sz}},
         {ei_t{3,1,4_sz}, ei_t{0,1,5_sz}, ei_t{0,2,0_sz}},
         {ei_t{1,1,5_sz}, ei_t{2,0,4_sz}}},
        {5_sz, 2_sz, 10_sz, 4_sz}
      };

      check(equality, LINE(""), graph, expected);
    }
    else if constexpr(GraphFlavour == flavour::directed_embedded)
    {
      Graph expected{
        {{ei_t{0,1,0,1_sz}, ei_t{0,2,1,7_sz}, ei_t{2,0,2,2_sz}},
         {ei_t{0,1,0,1_sz}, ei_t{1,3,0,3_sz}},
         {ei_t{3,2,1,4_sz}, ei_t{0,2,1,7_sz}, ei_t{2,0,2,2_sz}},
         {ei_t{1,3,1,3_sz}, ei_t{3,2,0,4_sz}}},
        {5_sz, 2_sz, 10_sz, 4_sz}
      };

      check(equality, LINE(""), graph, expected);
    }
    else
    {
      static_assert(dependent_false<Graph>::value);
    }
  }

  //============================= check_bf_update =============================//

  template<class Graph>
  void test_graph_update::check_bf_update(Graph graph)
  {
    using namespace maths;
    graph_updater<Graph> updater(graph);
    auto firstNodeFn = [&updater](const std::size_t index){ updater.firstNodeTraversal(index); };
    breadth_first_search(graph, find_disconnected_t{}, firstNodeFn);

    // node_weight *=  (2 + traversal index)
    //
    //        7
    //(0)10=======40(2)
    //    |   2   |
    //   1|       |4
    //    |       |
    // (1)6-------20(3)
    //        3

    std::array<std::size_t, 4> expectedNodeWeights{10,6,40,20};
    check(equality, LINE(""), graph.cbegin_node_weights(), graph.cend_node_weights(), expectedNodeWeights.cbegin(), expectedNodeWeights.cend());

    auto secondNodeFn = [&updater](const std::size_t index){ updater.secondNodeTraversal(index); };
    breadth_first_search(graph, find_disconnected_t{}, null_func_obj{}, secondNodeFn);

    // node_weight / = (2 + traversal index)
    //
    //       7
    //(0)5=======10(2)
    //   |   2   |
    //  1|       |4
    //   |       |
    //(1)2-------4(3)
    //       3

    expectedNodeWeights = {5,2,10,4};
    check(equality, LINE(""), graph.cbegin_node_weights(), graph.cend_node_weights(), expectedNodeWeights.cbegin(), expectedNodeWeights.cend());

    auto firstEdgeFn = [&updater](auto citer) { updater.firstEdgeTraversal(citer); };
    breadth_first_search(graph, find_disconnected_t{}, null_func_obj{}, null_func_obj{}, firstEdgeFn);

    //  edge_weight += 10 + traversal index
    //
    //  Undirected            Directed        0-1;1-3;3-2;0-2;2-0;
    //      18                  18
    //(0)5=======10(2)   (0)5=======10(2)
    //   |  14   |          |   15   |
    // 11|       |18      11|        |18
    //   |       |          |        |
    //(1)2-------4(3)    (1)2--------4(3)
    //      16                  15

    using ei_t = typename Graph::edge_init_type;
    using flavour = maths::graph_flavour;
    constexpr auto GraphFlavour{Graph::flavour};
    constexpr bool undirected{maths::undirected(GraphFlavour)};

    if constexpr(GraphFlavour == flavour::undirected)
    {
      Graph expected{
        {{ei_t{1,11_sz}, ei_t{2,18_sz}, ei_t{2,14_sz}},
         {ei_t{0,11_sz}, ei_t{3,16_sz}},
         {ei_t{3,18_sz}, ei_t{0,18_sz}, ei_t{0,14_sz}},
         {ei_t{1,16_sz}, ei_t{2,18_sz}}},
        {5_sz, 2_sz, 10_sz, 4_sz}
      };

      expected.swap_edges(0, 1, 2);
      expected.swap_edges(2, 0, 2);
      check(equality, LINE(""), graph, expected);
    }
    else if constexpr(GraphFlavour == flavour::directed)
    {
      Graph expected{
        {{ei_t{1,11_sz}, ei_t{2,18_sz}},
         {ei_t{3,15_sz}},
         {ei_t{0,15_sz}},
         {ei_t{2,18_sz}}},
        {5_sz, 2_sz, 10_sz, 4_sz}
      };

      check(equality, LINE(""), graph, expected);
    }
    else if constexpr(GraphFlavour == flavour::undirected_embedded)
    {
      Graph expected{
        {{ei_t{1,0,11_sz}, ei_t{2,1,18_sz}, ei_t{2,2,14_sz}},
         {ei_t{0,0,11_sz}, ei_t{3,0,16_sz}},
         {ei_t{3,1,18_sz}, ei_t{0,1,18_sz}, ei_t{0,2,14_sz}},
         {ei_t{1,1,16_sz}, ei_t{2,0,18_sz}}},
        {5_sz, 2_sz, 10_sz, 4_sz}
      };

      check(equality, LINE(""), graph, expected);
    }
    else if constexpr(GraphFlavour == flavour::directed_embedded)
    {
      Graph expected{
        {{ei_t{0,1,0,11_sz}, ei_t{0,2,1,18_sz}, ei_t{2,0,2,15_sz}},
         {ei_t{0,1,0,11_sz}, ei_t{1,3,0,15_sz}},
         {ei_t{3,2,1,18_sz}, ei_t{0,2,1,18_sz}, ei_t{2,0,2,15_sz}},
         {ei_t{1,3,1,15_sz}, ei_t{3,2,0,18_sz}}},
        {5_sz, 2_sz, 10_sz, 4_sz}
      };

      check(equality, LINE(""), graph, expected);
    }
    else
    {
      static_assert(dependent_false<Graph>::value);
    }

    auto secondEdgeFn = [&updater](auto citer) { updater.secondEdgeTraversal(citer); };
    if constexpr(undirected)
    {
      breadth_first_search(graph, find_disconnected_t{}, null_func_obj{}, null_func_obj{}, null_func_obj{}, secondEdgeFn);
    }
    else
    {
      breadth_first_search(graph, find_disconnected_t{}, null_func_obj{}, null_func_obj{}, secondEdgeFn);
    }

    // edge_weight -= (10 + traversal index)
    //
    //      7
    //(0)5=======10(2)
    //   |  2    |
    //  1|       |4
    //   |       |
    //(1)2-------4(3)
    //      3

    check_setup(graph);
  }

  //============================= check_pr_update =============================//

  template<class Graph>
  void test_graph_update::check_pr_update(Graph graph)
  {
    using namespace maths;
    graph_updater<Graph> updater(graph);
    auto firstNodeFn = [&updater](const std::size_t index){ updater.firstNodeTraversal(index); };
    priority_search(graph, find_disconnected_t{}, firstNodeFn);

    // node_weight *=  (2 + traversal index)
    //
    //    Undirected          Directed
    //        7                  7
    //(0)10=======30(2)  (0)10=======30(2)
    //    |   2   |          |   2   |
    //   1|       |4        1|       |4
    //    |       |          |       |
    //(1)10-------16(3)  (1) 8-------20(3)
    //        3                  3

    auto expectedNodeWeights =
      undirected(Graph::flavour) ? std::array<std::size_t, 4>{10,10,30,16} : std::array<std::size_t, 4>{10,8,30,20};
    check(equality, LINE(""), graph.cbegin_node_weights(), graph.cend_node_weights(), expectedNodeWeights.cbegin(), expectedNodeWeights.cend());

    auto secondNodeFn = [&updater](const std::size_t index){ updater.secondNodeTraversal(index); };
    priority_search(graph, find_disconnected_t{}, null_func_obj{}, secondNodeFn);

    // node_weight / = (2 + traversal index)
    //
    //       7
    //(0)5=======10(2)
    //   |   2   |
    //  1|       |4
    //   |       |
    //(1)2-------4(3)
    //       3

    expectedNodeWeights = {5,2,10,4};
    check(equality, LINE(""), graph.cbegin_node_weights(), graph.cend_node_weights(), expectedNodeWeights.cbegin(), expectedNodeWeights.cend());

    auto firstEdgeFn = [&updater](auto citer) { updater.firstEdgeTraversal(citer); };
    priority_search(graph, find_disconnected_t{}, null_func_obj{}, null_func_obj{}, firstEdgeFn);

    // edge_weight += 10 + traversal index
    //
    //  Undirected            Directed        0-1;1-3;3-2;0-2;2-0;
    //      18                  18
    //(0)5=======10(2)   (0)5=======10(2)
    //   |  14   |          |   14   |
    // 11|       |17      11|        |18
    //   |       |          |        |
    //(1)2-------4(3)    (1)2--------4(3)
    //      17                  16

    using ei_t = typename Graph::edge_init_type;
    using flavour = maths::graph_flavour;
    constexpr auto GraphFlavour{Graph::flavour};

    if constexpr(GraphFlavour == flavour::undirected)
    {
      Graph expected{
        {{ei_t{1,11_sz}, ei_t{2,18_sz}, ei_t{2,14_sz}},
         {ei_t{0,11_sz}, ei_t{3,17_sz}},
         {ei_t{3,17_sz}, ei_t{0,18_sz}, ei_t{0,14_sz}},
         {ei_t{1,17_sz}, ei_t{2,17_sz}}},
        {5_sz, 2_sz, 10_sz, 4_sz}
      };

      expected.swap_edges(0, 1, 2);
      expected.swap_edges(2, 0, 2);
      check(equality, LINE(""), graph, expected);
    }
    else if constexpr(GraphFlavour == flavour::directed)
    {
      Graph expected{
        {{ei_t{1,11_sz}, ei_t{2,18_sz}},
         {ei_t{3,16_sz}},
         {ei_t{0,14_sz}},
         {ei_t{2,18_sz}}},
        {5_sz, 2_sz, 10_sz, 4_sz}
      };

      check(equality, LINE(""), graph, expected);
    }
    else if constexpr(GraphFlavour == flavour::undirected_embedded)
    {
      Graph expected{
        {{ei_t{1,0,11_sz}, ei_t{2,1,18_sz}, ei_t{2,2,14_sz}},
         {ei_t{0,0,11_sz}, ei_t{3,0,17_sz}},
         {ei_t{3,1,17_sz}, ei_t{0,1,18_sz}, ei_t{0,2,14_sz}},
         {ei_t{1,1,17_sz}, ei_t{2,0,17_sz}}},
        {5_sz, 2_sz, 10_sz, 4_sz}
      };

      check(equality, LINE(""), graph, expected);
    }
    else if constexpr(GraphFlavour == flavour::directed_embedded)
    {
      Graph expected{
        {{ei_t{0,1,0,11_sz}, ei_t{0,2,1,18_sz}, ei_t{2,0,2,14_sz}},
         {ei_t{0,1,0,11_sz}, ei_t{1,3,0,16_sz}},
         {ei_t{3,2,1,18_sz}, ei_t{0,2,1,18_sz}, ei_t{2,0,2,14_sz}},
         {ei_t{1,3,1,16_sz}, ei_t{3,2,0,18_sz}}},
        {5_sz, 2_sz, 10_sz, 4_sz}
      };

      check(equality, LINE(""), graph, expected);
    }
    else
    {
      static_assert(dependent_false<Graph>::value);
    }

    auto secondEdgeFn = [&updater](auto citer) { updater.secondEdgeTraversal(citer); };
    if constexpr(undirected(Graph::flavour))
    {
      priority_search(graph, find_disconnected_t{}, null_func_obj{}, null_func_obj{}, null_func_obj{}, secondEdgeFn);
    }
    else
    {
      priority_search(graph, find_disconnected_t{}, null_func_obj{}, null_func_obj{}, secondEdgeFn);
    }

    // edge_weight -= (10 + traversal index)
    //
    //  Undirected            Directed        0-1;1-3;3-2;0-2;2-0;
    //       8                   7
    //(0)5=======10(2)   (0)5=======10(2)
    //   |   3   |          |    2   |
    // -2|       | 5       1|        |4
    //   |       |          |        |
    //(1)2-------4(3)    (1)2--------4(3)
    //       3                   3


    if constexpr(GraphFlavour == flavour::undirected)
    {
      constexpr auto w{std::numeric_limits<std::size_t>::max() - 1_sz};
      Graph expected{
        {{ei_t{1,w}, ei_t{2,8_sz}, ei_t{2,3_sz}},
         {ei_t{0,w}, ei_t{3,3_sz}},
         {ei_t{3,5_sz}, ei_t{0,8_sz}, ei_t{0,3_sz}},
         {ei_t{1,3_sz}, ei_t{2,5_sz}}},
        {5_sz, 2_sz, 10_sz, 4_sz}
      };

      expected.swap_edges(0, 1, 2);
      expected.swap_edges(2, 0, 2);
      check(equality, LINE(""), graph, expected);
    }
    else if constexpr(GraphFlavour == flavour::directed)
    {
      Graph expected{
        {{ei_t{1,1_sz}, ei_t{2,7_sz}},
         {ei_t{3,3_sz}},
         {ei_t{0,2_sz}},
         {ei_t{2,4_sz}}},
        {5_sz, 2_sz, 10_sz, 4_sz}
      };

      check(equality, LINE(""), graph, expected);
    }
    else if constexpr(GraphFlavour == flavour::undirected_embedded)
    {
      constexpr auto w{std::numeric_limits<std::size_t>::max() - 1_sz};
      Graph expected{
        {{ei_t{1,0,w}, ei_t{2,1,8_sz}, ei_t{2,2,3_sz}},
         {ei_t{0,0,w}, ei_t{3,0,3_sz}},
         {ei_t{3,1,5_sz}, ei_t{0,1,8_sz}, ei_t{0,2,3_sz}},
         {ei_t{1,1,3_sz}, ei_t{2,0,5_sz}}},
        {5_sz, 2_sz, 10_sz, 4_sz}
      };

      check(equality, LINE(""), graph, expected);
    }
    else if constexpr(GraphFlavour == flavour::directed_embedded)
    {
      Graph expected{
        {{ei_t{0,1,0,1_sz}, ei_t{0,2,1,7_sz}, ei_t{2,0,2,2_sz}},
         {ei_t{0,1,0,1_sz}, ei_t{1,3,0,3_sz}},
         {ei_t{3,2,1,4_sz}, ei_t{0,2,1,7_sz}, ei_t{2,0,2,2_sz}},
         {ei_t{1,3,1,3_sz}, ei_t{3,2,0,4_sz}}},
        {5_sz, 2_sz, 10_sz, 4_sz}
      };

      check(equality, LINE(""), graph, expected);
    }
    else
    {
      static_assert(dependent_false<Graph>::value);
    }
  }

  //============================= execute_operations =============================//

  template<class Graph>
  void test_graph_update::test_update()
  {
    const Graph graph{make_graph<Graph>()};
    check_setup(graph);

    //       7
    //(0)5=======10(2)
    //   |   2   |
    //  1|       |4
    //   |       |
    //(1)2-------4(3)
    //       3

    check_df_update(graph);
    check_bf_update(graph);
    check_pr_update(graph);
  }

  //============================= Breadth-first only tests ===========================//

  template<class Graph>
  void test_graph_update::test_bf_update()
  {
    using maths::graph_flavour;

    Graph graph;
    graph.add_node();
    graph.add_node();
    graph.add_node();

    graph.join(0, 1);
    graph.join(1, 0);
    graph.join(1, 2);
    graph.join(2, 0);
    graph.join(2, 2);
    graph.join(2, 2);

    //
    // /\    /\
    // --  0 --
    //    / \
    //   /   \
    //  0=====0

    using ei_t = typename Graph::edge_init_type;
    constexpr auto GraphFlavour{Graph::flavour};

    if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      Graph expected{
        {{ei_t{1}, ei_t{1}, ei_t{2}},
        {ei_t{0}, ei_t{0}, ei_t{2}},
        {ei_t{0}, ei_t{1}, ei_t{2}, ei_t{2}, ei_t{2}, ei_t{2}}},
        {std::vector<int>{}, std::vector<int>{}, std::vector<int>{}}
      };

      expected.swap_edges(2, 0, 1);
      check(equality, LINE(""), graph, expected);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed)
    {
      Graph expected{
        {{ei_t{1}},
        {ei_t{0}, ei_t{2}},
        {ei_t{0}, ei_t{2}, ei_t{2}}},
        {std::vector<int>{}, std::vector<int>{}, std::vector<int>{}}
      };

      check(equality, LINE(""), graph, expected);
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected_embedded)
    {
      Graph expected{
        {{ei_t{1,0}, ei_t{1,1}, ei_t{2,1}},
         {ei_t{0,0}, ei_t{0,1}, ei_t{2,0}},
         {ei_t{1,2}, ei_t{0,2}, ei_t{2,3}, ei_t{2,2}, ei_t{2,5}, ei_t{2,4}}},
        {std::vector<int>{}, std::vector<int>{}, std::vector<int>{}}
      };

      check(equality, LINE(""), graph, expected);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      Graph expected{
        {{ei_t{0,1,0}, ei_t{1,0,1}, ei_t{2,0,1}},
         {ei_t{0,1,0}, ei_t{1,0,1}, ei_t{1,2,0}},
         {ei_t{1,2,2}, ei_t{2,0,2}, ei_t{2,2,3}, ei_t{2,2,2}, ei_t{2,2,5}, ei_t{2,2,4}}},
        {std::vector<int>{}, std::vector<int>{}, std::vector<int>{}}
      };

      check(equality, LINE(""), graph, expected);
    }
    else
    {
      static_assert(dependent_false<Graph>::value);
    }

    using namespace maths;
    auto nodeFn1 = [&graph](const std::size_t index) { graph.node_weight(graph.cbegin_node_weights() + index, std::vector<int>{static_cast<int>(index)}); };
    breadth_first_search(graph, find_disconnected_t{}, nodeFn1);

    std::vector<std::vector<int>> expectedNodeWeights{{0}, {1}, {2}};
    check(equality, LINE(""), graph.cbegin_node_weights(), graph.cend_node_weights(), expectedNodeWeights.cbegin(), expectedNodeWeights.cend());

    auto nodeFn2 = [&graph](const std::size_t index) { graph.node_weight(graph.cbegin_node_weights() + index, std::vector<int>{3 - static_cast<int>(index)}); };
    maths::breadth_first_search(graph, find_disconnected_t{}, null_func_obj{}, nodeFn2);

    expectedNodeWeights = std::vector<std::vector<int>>{{3}, {2}, {1}};
    check(equality, LINE(""), graph.cbegin_node_weights(), graph.cend_node_weights(), expectedNodeWeights.cbegin(), expectedNodeWeights.cend());

    auto edgeFn1 = [&graph](auto edgeIter) {
      const std::size_t node{edgeIter.partition_index()};
      const std::size_t index{static_cast<std::size_t>(distance(graph.cbegin_edges(node), edgeIter))};
      std::tuple<std::size_t, std::size_t, std::size_t> nthConnInds(nth_connection_indices(graph, node, index));
      const std::size_t nthConn = std::get<2>(nthConnInds);
      graph.set_edge_weight(edgeIter, std::vector<double>{static_cast<double>(nthConn)});
    };

    maths::breadth_first_search(graph, find_disconnected_t{}, null_func_obj{}, null_func_obj{}, edgeFn1);
    using ew_t = std::vector<double>;

    if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      Graph expected{
        {{ei_t{1, ew_t{0}}, ei_t{1, ew_t{1}}, ei_t{2, ew_t{0}}},
         {ei_t{0, ew_t{0}}, ei_t{0, ew_t{1}}, ei_t{2, ew_t{0}}},
         {ei_t{0, ew_t{0}}, ei_t{1, ew_t{0}}, ei_t{2, ew_t{0}}, ei_t{2, ew_t{0}}, ei_t{2, ew_t{2}}, ei_t{2, ew_t{2}}}},
        {std::vector<int>{3}, std::vector<int>{2}, std::vector<int>{1}}
      };

      expected.swap_edges(2, 0, 1);
      check(equality, LINE(""), graph, expected);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed)
    {
      Graph expected{
        {{ei_t{1, ew_t{0}}},
         {ei_t{0, ew_t{0}}, ei_t{2, ew_t{0}}},
         {ei_t{0, ew_t{0}}, ei_t{2, ew_t{0}}, ei_t{2, ew_t{1}}}},
        {std::vector<int>{3}, std::vector<int>{2}, std::vector<int>{1}}
      };

      check(equality, LINE(""), graph, expected);
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected_embedded)
    {
      Graph expected{
        {{ei_t{1,0, ew_t{0}}, ei_t{1,1, ew_t{1}}, ei_t{2,1, ew_t{0}}},
         {ei_t{0,0, ew_t{0}}, ei_t{0,1, ew_t{1}}, ei_t{2,0, ew_t{0}}},
         {ei_t{1,2, ew_t{0}}, ei_t{0,2, ew_t{0}}, ei_t{2,3, ew_t{0}}, ei_t{2,2, ew_t{0}}, ei_t{2,5, ew_t{2}}, ei_t{2,4, ew_t{2}}}},
        {std::vector<int>{3}, std::vector<int>{2}, std::vector<int>{1}}
      };

      check(equality, LINE(""), graph, expected);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      Graph expected{
        {{ei_t{0,1,0,ew_t{0}}, ei_t{1,0,1,ew_t{0}}, ei_t{2,0,1,ew_t{0}}},
         {ei_t{0,1,0,ew_t{0}}, ei_t{1,0,1,ew_t{0}}, ei_t{1,2,0,ew_t{0}}},
           {ei_t{1,2,2,ew_t{0}}, ei_t{2,0,2,ew_t{0}}, ei_t{2,2,3,ew_t{1}}, ei_t{2,2,2,ew_t{1}}, ei_t{2,2,5,ew_t{3}}, ei_t{2,2,4,ew_t{3}}}},
        {std::vector<int>{3}, std::vector<int>{2}, std::vector<int>{1}}
      };

      check(equality, LINE(""), graph, expected);
    }
    else
    {
      static_assert(dependent_false<Graph>::value);
    }

    if constexpr(maths::undirected(GraphFlavour))
    {
      test_second_edge_traversal_update(graph);
    }
  }

  template<class Graph>
  void test_graph_update::test_second_edge_traversal_update(Graph& graph)
  {
    using namespace maths;

    auto edgeFn2 = [&graph](auto edgeIter) {
      const std::size_t node{edgeIter.partition_index()};
      const std::size_t index{static_cast<std::size_t>(distance(graph.cbegin_edges(node), edgeIter))};
      std::tuple<std::size_t, std::size_t, std::size_t> nthConnInds(nth_connection_indices(graph, node, index));
      const std::size_t target = std::get<1>(nthConnInds);
      const std::size_t nthConn = std::get<2>(nthConnInds);
      const double val1{static_cast<double>(target)},
      val2{static_cast<double>(nthConn) + 1};
      graph.set_edge_weight(edgeIter, std::vector<double>{val1, val2});
    };

    breadth_first_search(graph, find_disconnected_t{}, null_func_obj{}, null_func_obj{}, null_func_obj{}, edgeFn2);

    using ei_t = typename Graph::edge_init_type;
    using ew_t = std::vector<double>;
    constexpr auto GraphFlavour{Graph::flavour};

    if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      Graph expected{
        {{ei_t{1, ew_t{0,1}}, ei_t{1, ew_t{0,2}}, ei_t{2, ew_t{0,1}}},
         {ei_t{0, ew_t{0,1}}, ei_t{0, ew_t{0,2}}, ei_t{2, ew_t{1,1}}},
         {ei_t{0, ew_t{0,1}}, ei_t{1, ew_t{1,1}}, ei_t{2, ew_t{2,2}}, ei_t{2, ew_t{2,2}}, ei_t{2, ew_t{2,4}}, ei_t{2, ew_t{2,4}}}},
        {std::vector<int>{3}, std::vector<int>{2}, std::vector<int>{1}}
      };

      expected.swap_edges(2, 0, 1);
      check(equality, LINE(""), graph, expected);
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected_embedded)
    {
      Graph expected{
        {{ei_t{1,0, ew_t{0,1}}, ei_t{1,1, ew_t{0,2}}, ei_t{2,1, ew_t{0,1}}},
         {ei_t{0,0, ew_t{0,1}}, ei_t{0,1, ew_t{0,2}}, ei_t{2,0, ew_t{1,1}}},
         {ei_t{1,2, ew_t{1,1}}, ei_t{0,2, ew_t{0,1}}, ei_t{2,3, ew_t{2,2}}, ei_t{2,2, ew_t{2,2}}, ei_t{2,5, ew_t{2,4}}, ei_t{2,4, ew_t{2,4}}}},
        {std::vector<int>{3}, std::vector<int>{2}, std::vector<int>{1}}
      };

      check(equality, LINE(""), graph, expected);
    }
    else
    {
      static_assert(dependent_false<Graph>::value);
    }
  }
}
