////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "DynamicGraphUpdateTest.hpp"

namespace sequoia::unit_testing
{
  void test_graph_update::run_tests()
  {
    {
      graph_test_helper<std::vector<double>, std::vector<int>> helper{};
      helper.run_tests<test_bf_update>(*this);
    }

    {
      graph_test_helper<size_t, size_t> helper;
      helper.run_tests<test_update>(*this);
    }
  }

  //============================= General traversal tests ============================//

  template
  <
    maths::graph_flavour GraphFlavour,
    class EdgeWeight,
    class NodeWeight,      
    template <class> class EdgeWeightPooling,
    template <class> class NodeWeightPooling,
    template <maths::graph_flavour, class, template<class> class> class EdgeStorageTraits,
    template <class, template<class> class, bool> class NodeWeightStorageTraits
  >
  auto test_update<
    GraphFlavour,
    EdgeWeight,
    NodeWeight,
    EdgeWeightPooling,
    NodeWeightPooling,
    EdgeStorageTraits,
    NodeWeightStorageTraits>::make_graph() -> graph_t
  {
    graph_t graph;

    graph.add_node(5ul);
    graph.add_node(2ul);
    graph.add_node(10ul);
    graph.add_node(4ul);

    graph.join(0, 1, 1ul);
    graph.join(1, 3, 3ul);
    graph.join(3, 2, 4ul);
    graph.join(0, 2, 7ul);
    graph.join(2, 0, 2ul);

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
  
  template
  <
    maths::graph_flavour GraphFlavour,
    class EdgeWeight,
    class NodeWeight,      
    template <class> class EdgeWeightPooling,
    template <class> class NodeWeightPooling,
    template <maths::graph_flavour, class, template<class> class> class EdgeStorageTraits,
    template <class, template<class> class, bool> class NodeWeightStorageTraits
  >
  void test_update<
    GraphFlavour,
    EdgeWeight,
    NodeWeight,
    EdgeWeightPooling,
    NodeWeightPooling,
    EdgeStorageTraits,
    NodeWeightStorageTraits>::check_setup(const graph_t& graph)
  {
    if constexpr(GraphFlavour == flavour::undirected)
    {
      graph_t expected{
        {{ei_t{1,1ul}, ei_t{2,7ul}, ei_t{2,2ul}},
         {ei_t{0,1ul}, ei_t{3,3ul}},
         {ei_t{3,4ul}, ei_t{0,7ul}, ei_t{0,2ul}},
         {ei_t{1,3ul}, ei_t{2,4ul}}},
        {5ul, 2ul, 10ul, 4ul}
      };
      
      expected.swap_edges(0, 1, 2);
      expected.swap_edges(2, 0, 2);
      check_equality(graph, expected, LINE(""));
    }
    else if constexpr(GraphFlavour == flavour::directed)
    {
      graph_t expected{
        {{ei_t{1,1ul}, ei_t{2,7ul}},
         {ei_t{3,3ul}},
         {ei_t{0,2ul}},
         {ei_t{2,4ul}}},
        {5ul, 2ul, 10ul, 4ul}
      };
      
      check_equality(graph, expected, LINE(""));
    }
    else if constexpr(GraphFlavour == flavour::undirected_embedded)
    {
      graph_t expected{
        {{ei_t{1,0,1ul}, ei_t{2,1,7ul}, ei_t{2,2,2ul}},
         {ei_t{0,0,1ul}, ei_t{3,0,3ul}},
         {ei_t{3,1,4ul}, ei_t{0,1,7ul}, ei_t{0,2,2ul}},
         {ei_t{1,1,3ul}, ei_t{2,0,4ul}}},
        {5ul, 2ul, 10ul, 4ul}
      };

      check_equality(graph, expected, LINE(""));
    }
    else if constexpr(GraphFlavour == flavour::directed_embedded)
    {
      graph_t expected{
        {{ei_t{0,1,0,1ul}, ei_t{0,2,1,7ul}, ei_t{2,0,2,2ul}},
         {ei_t{0,1,0,1ul}, ei_t{1,3,0,3ul}},
         {ei_t{3,2,1,4ul}, ei_t{0,2,1,7ul}, ei_t{2,0,2,2ul}},
         {ei_t{1,3,1,3ul}, ei_t{3,2,0,4ul}}},
        {5ul, 2ul, 10ul, 4ul}
      };

      check_equality(graph, expected, LINE(""));
    }
    else
    {
      static_assert(dependent_false<graph_t>::value);
    }
  }

  //============================= check_df_update =============================//

  template
  <
    maths::graph_flavour GraphFlavour,
    class EdgeWeight,
    class NodeWeight,      
    template <class> class EdgeWeightPooling,
    template <class> class NodeWeightPooling,
    template <maths::graph_flavour, class, template<class> class> class EdgeStorageTraits,
    template <class, template<class> class, bool> class NodeWeightStorageTraits
  >
  void test_update<
    GraphFlavour,
    EdgeWeight,
    NodeWeight,
    EdgeWeightPooling,
    NodeWeightPooling,
    EdgeStorageTraits,
    NodeWeightStorageTraits>::check_df_update(graph_t graph)
  {
    GraphUpdaterFunctors<graph_t> updater(graph);
    auto firstNodeFn = [&updater](const std::size_t index){ updater.firstNodeTraversal(index); };
    maths::depth_first_search(graph, false, 0, firstNodeFn);

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
    check_range(graph.cbegin_node_weights(), graph.cend_node_weights(), expectedNodeWeights.cbegin(), expectedNodeWeights.cend(), LINE(""));

    auto secondNodeFn = [&updater](const std::size_t index){ updater.secondNodeTraversal(index); };
    maths::depth_first_search(graph, false, 0, maths::null_functor(), secondNodeFn);

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
    check_range(graph.cbegin_node_weights(), graph.cend_node_weights(), expectedNodeWeights.cbegin(), expectedNodeWeights.cend(), LINE(""));

    auto firstEdgeFn = [&updater](auto citer) { updater.firstEdgeTraversal(citer); };
    maths::depth_first_search(graph, false, 0, maths::null_functor(), maths::null_functor(), firstEdgeFn);

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


    if constexpr(GraphFlavour == flavour::undirected)
    {
      graph_t expected{
        {{ei_t{1,13ul}, ei_t{2,18ul}, ei_t{2,12ul}},
         {ei_t{0,13ul}, ei_t{3,16ul}},
         {ei_t{3,18ul}, ei_t{0,18ul}, ei_t{0,12ul}},
         {ei_t{1,16ul}, ei_t{2,18ul}}},
        {5ul, 2ul, 10ul, 4ul}
      };
      
      expected.swap_edges(0, 1, 2);
      expected.swap_edges(2, 0, 2);
      check_equality(graph, expected, LINE(""));
    }
    else if constexpr(GraphFlavour == flavour::directed)
    {
      graph_t expected{
        {{ei_t{1,12ul}, ei_t{2,17ul}},
         {ei_t{3,15ul}},
         {ei_t{0,16ul}},
         {ei_t{2,17ul}}},
        {5ul, 2ul, 10ul, 4ul}
      };
      
      check_equality(graph, expected, LINE(""));
    }
    else if constexpr(GraphFlavour == flavour::undirected_embedded)
    {
      graph_t expected{
        {{ei_t{1,0,13ul}, ei_t{2,1,18ul}, ei_t{2,2,12ul}},
         {ei_t{0,0,13ul}, ei_t{3,0,16ul}},
         {ei_t{3,1,18ul}, ei_t{0,1,18ul}, ei_t{0,2,12ul}},
         {ei_t{1,1,16ul}, ei_t{2,0,18ul}}},
        {5ul, 2ul, 10ul, 4ul}
      };

      check_equality(graph, expected, LINE(""));
    }
    else if constexpr(GraphFlavour == flavour::directed_embedded)
    {
      graph_t expected{
        {{ei_t{0,1,0,12ul}, ei_t{0,2,1,17ul}, ei_t{2,0,2,16ul}},
         {ei_t{0,1,0,12ul}, ei_t{1,3,0,15ul}},
         {ei_t{3,2,1,17ul}, ei_t{0,2,1,17ul}, ei_t{2,0,2,16ul}},
         {ei_t{1,3,1,15ul}, ei_t{3,2,0,17ul}}},
        {5ul, 2ul, 10ul, 4ul}
      };

      check_equality(graph, expected, LINE(""));
    }
    else
    {
      static_assert(dependent_false<graph_t>::value);
    }

    auto secondEdgeFn = [&updater](auto citer) { updater.secondEdgeTraversal(citer); };
    dfu_second_edge_traversal(UndirectedType(), graph, secondEdgeFn);

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
      graph_t expected{
        {{ei_t{1,3ul}, ei_t{2,5ul}, ei_t{2,0ul}},
         {ei_t{0,3ul}, ei_t{3,5ul}},
         {ei_t{3,4ul}, ei_t{0,5ul}, ei_t{0,0ul}},
         {ei_t{1,5ul}, ei_t{2,4ul}}},
        {5ul, 2ul, 10ul, 4ul}
      };
      
      expected.swap_edges(0, 1, 2);
      expected.swap_edges(2, 0, 2);
      check_equality(graph, expected, LINE(""));
    }
    else if constexpr(GraphFlavour == flavour::directed)
    {
      graph_t expected{
        {{ei_t{1,1ul}, ei_t{2,7ul}},
         {ei_t{3,3ul}},
         {ei_t{0,2ul}},
         {ei_t{2,4ul}}},
        {5ul, 2ul, 10ul, 4ul}
      };
      
      check_equality(graph, expected, LINE(""));
    }
    else if constexpr(GraphFlavour == flavour::undirected_embedded)
    {
      graph_t expected{
        {{ei_t{1,0,3ul}, ei_t{2,1,5ul}, ei_t{2,2,0ul}},
         {ei_t{0,0,3ul}, ei_t{3,0,5ul}},
         {ei_t{3,1,4ul}, ei_t{0,1,5ul}, ei_t{0,2,0ul}},
         {ei_t{1,1,5ul}, ei_t{2,0,4ul}}},
        {5ul, 2ul, 10ul, 4ul}
      };

      check_equality(graph, expected, LINE(""));
    }
    else if constexpr(GraphFlavour == flavour::directed_embedded)
    {
      graph_t expected{
        {{ei_t{0,1,0,1ul}, ei_t{0,2,1,7ul}, ei_t{2,0,2,2ul}},
         {ei_t{0,1,0,1ul}, ei_t{1,3,0,3ul}},
         {ei_t{3,2,1,4ul}, ei_t{0,2,1,7ul}, ei_t{2,0,2,2ul}},
         {ei_t{1,3,1,3ul}, ei_t{3,2,0,4ul}}},
        {5ul, 2ul, 10ul, 4ul}
      };

      check_equality(graph, expected, LINE(""));
    }
    else
    {
      static_assert(dependent_false<graph_t>::value);
    }
  }

  template
  <
    maths::graph_flavour GraphFlavour,
    class EdgeWeight,
    class NodeWeight,      
    template <class> class EdgeWeightPooling,
    template <class> class NodeWeightPooling,
    template <maths::graph_flavour, class, template<class> class> class EdgeStorageTraits,
    template <class, template<class> class, bool> class NodeWeightStorageTraits
  >
  void test_update<
    GraphFlavour,
    EdgeWeight,
    NodeWeight,
    EdgeWeightPooling,
    NodeWeightPooling,
    EdgeStorageTraits,
    NodeWeightStorageTraits>::check_bf_update(graph_t graph)
  {
    GraphUpdaterFunctors<graph_t> updater(graph);
    auto firstNodeFn = [&updater](const std::size_t index){ updater.firstNodeTraversal(index); };
    maths::breadth_first_search(graph, false, 0, firstNodeFn);

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
    check_range(graph.cbegin_node_weights(), graph.cend_node_weights(), expectedNodeWeights.cbegin(), expectedNodeWeights.cend(), LINE(""));

    auto secondNodeFn = [&updater](const std::size_t index){ updater.secondNodeTraversal(index); };
    maths::breadth_first_search(graph, false, 0, maths::null_functor(), secondNodeFn);

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
    check_range(graph.cbegin_node_weights(), graph.cend_node_weights(), expectedNodeWeights.cbegin(), expectedNodeWeights.cend(), LINE(""));

    auto firstEdgeFn = [&updater](auto citer) { updater.firstEdgeTraversal(citer); };
    maths::breadth_first_search(graph, false, 0, maths::null_functor(), maths::null_functor(), firstEdgeFn);

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

    if constexpr(GraphFlavour == flavour::undirected)
    {
      graph_t expected{
        {{ei_t{1,11ul}, ei_t{2,18ul}, ei_t{2,14ul}},
         {ei_t{0,11ul}, ei_t{3,16ul}},
         {ei_t{3,18ul}, ei_t{0,18ul}, ei_t{0,14ul}},
         {ei_t{1,16ul}, ei_t{2,18ul}}},
        {5ul, 2ul, 10ul, 4ul}
      };

      expected.swap_edges(0, 1, 2);
      expected.swap_edges(2, 0, 2);
      check_equality(graph, expected, LINE(""));
    }
    else if constexpr(GraphFlavour == flavour::directed)
    {
      graph_t expected{
        {{ei_t{1,11ul}, ei_t{2,18ul}},
         {ei_t{3,15ul}},
         {ei_t{0,15ul}},
         {ei_t{2,18ul}}},
        {5ul, 2ul, 10ul, 4ul}
      };

      check_equality(graph, expected, LINE(""));
    }
    else if constexpr(GraphFlavour == flavour::undirected_embedded)
    {
      graph_t expected{
        {{ei_t{1,0,11ul}, ei_t{2,1,18ul}, ei_t{2,2,14ul}},
         {ei_t{0,0,11ul}, ei_t{3,0,16ul}},
         {ei_t{3,1,18ul}, ei_t{0,1,18ul}, ei_t{0,2,14ul}},
         {ei_t{1,1,16ul}, ei_t{2,0,18ul}}},
        {5ul, 2ul, 10ul, 4ul}
      };

      check_equality(graph, expected, LINE(""));
    }
    else if constexpr(GraphFlavour == flavour::directed_embedded)
    {
      graph_t expected{
        {{ei_t{0,1,0,11ul}, ei_t{0,2,1,18ul}, ei_t{2,0,2,15ul}},
         {ei_t{0,1,0,11ul}, ei_t{1,3,0,15ul}},
         {ei_t{3,2,1,18ul}, ei_t{0,2,1,18ul}, ei_t{2,0,2,15ul}},
         {ei_t{1,3,1,15ul}, ei_t{3,2,0,18ul}}},
        {5ul, 2ul, 10ul, 4ul}
      };

      check_equality(graph, expected, LINE(""));
    }
    else
    {
      static_assert(dependent_false<graph_t>::value);
    }
  }

  //============================= execute_operations =============================//
  
  template
  <
    maths::graph_flavour GraphFlavour,
    class EdgeWeight,
    class NodeWeight,      
    template <class> class EdgeWeightPooling,
    template <class> class NodeWeightPooling,
    template <maths::graph_flavour, class, template<class> class> class EdgeStorageTraits,
    template <class, template<class> class, bool> class NodeWeightStorageTraits
  >
  void test_update<
    GraphFlavour,
    EdgeWeight,
    NodeWeight,
    EdgeWeightPooling,
    NodeWeightPooling,
    EdgeStorageTraits,
    NodeWeightStorageTraits>::execute_operations()
  {
    const graph_t graph{make_graph()};
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
  }

  //============================= Breadth-first only tests ===========================//

  template
  <
    maths::graph_flavour GraphFlavour,
    class EdgeWeight,
    class NodeWeight,      
    template <class> class EdgeWeightPooling,
    template <class> class NodeWeightPooling,
    template <maths::graph_flavour, class, template<class> class> class EdgeStorageTraits,
    template <class, template<class> class, bool> class NodeWeightStorageTraits
  >
  void test_bf_update<
    GraphFlavour,
    EdgeWeight,
    NodeWeight,
    EdgeWeightPooling,
    NodeWeightPooling,
    EdgeStorageTraits,
    NodeWeightStorageTraits>::execute_operations()
  {
    using maths::graph_flavour;
    
    graph_t graph;
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
    
    if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t expected{
        {{ei_t{1}, ei_t{1}, ei_t{2}},
        {ei_t{0}, ei_t{0}, ei_t{2}},
        {ei_t{0}, ei_t{1}, ei_t{2}, ei_t{2}, ei_t{2}, ei_t{2}}},
        {std::vector<int>{}, std::vector<int>{}, std::vector<int>{}}
      };
      
      expected.swap_edges(2, 0, 1);
      check_equality(graph, expected, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::directed)
    {
      graph_t expected{
        {{ei_t{1}},
        {ei_t{0}, ei_t{2}},
        {ei_t{0}, ei_t{2}, ei_t{2}}},
        {std::vector<int>{}, std::vector<int>{}, std::vector<int>{}}
      };
      
      check_equality(graph, expected, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected_embedded)
    {
      graph_t expected{
        {{ei_t{1,0}, ei_t{1,1}, ei_t{2,1}},
         {ei_t{0,0}, ei_t{0,1}, ei_t{2,0}},
         {ei_t{1,2}, ei_t{0,2}, ei_t{2,3}, ei_t{2,2}, ei_t{2,5}, ei_t{2,4}}},
        {std::vector<int>{}, std::vector<int>{}, std::vector<int>{}}
      };

      check_equality(graph, expected, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      graph_t expected{
        {{ei_t{0,1,0}, ei_t{1,0,1}, ei_t{2,0,1}},
         {ei_t{0,1,0}, ei_t{1,0,1}, ei_t{1,2,0}},
         {ei_t{1,2,2}, ei_t{2,0,2}, ei_t{2,2,3}, ei_t{2,2,2}, ei_t{2,2,5}, ei_t{2,2,4}}},
        {std::vector<int>{}, std::vector<int>{}, std::vector<int>{}}
      };

      check_equality(graph, expected, LINE(""));
    }
    else
    {
      static_assert(dependent_false<graph_t>::value);
    }

    auto nodeFn1 = [&graph](const std::size_t index) { graph.node_weight(graph.cbegin_node_weights() + index, std::vector<int>{static_cast<int>(index)}); };
    maths::breadth_first_search(graph, false, 0, nodeFn1);

    std::vector<std::vector<int>> expectedNodeWeights{{0}, {1}, {2}};
    check_range(graph.cbegin_node_weights(), graph.cend_node_weights(), expectedNodeWeights.cbegin(), expectedNodeWeights.cend(), LINE(""));

    auto nodeFn2 = [&graph](const std::size_t index) { graph.node_weight(graph.cbegin_node_weights() + index, std::vector<int>{3 - static_cast<int>(index)}); };
    maths::breadth_first_search(graph, false, 0, maths::null_functor(), nodeFn2);

    expectedNodeWeights = std::vector<std::vector<int>>{{3}, {2}, {1}};
    check_range(graph.cbegin_node_weights(), graph.cend_node_weights(), expectedNodeWeights.cbegin(), expectedNodeWeights.cend(), LINE(""));    

    auto edgeFn1 = [&graph](auto edgeIter) {
      const std::size_t node{edgeIter.partition_index()};
      const std::size_t index{static_cast<std::size_t>(distance(graph.cbegin_edges(node), edgeIter))};
      std::tuple<std::size_t, std::size_t, std::size_t> nthConnInds(nth_connection_indices(graph, node, index));
      const std::size_t nthConn = std::get<2>(nthConnInds);
      graph.set_edge_weight(edgeIter, std::vector<double>{static_cast<double>(nthConn)});
    };

    maths::breadth_first_search(graph, false, 0, maths::null_functor(), maths::null_functor(), edgeFn1);

    if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t expected{
        {{ei_t{1, ew_t{0}}, ei_t{1, ew_t{1}}, ei_t{2, ew_t{0}}},
         {ei_t{0, ew_t{0}}, ei_t{0, ew_t{1}}, ei_t{2, ew_t{0}}},
         {ei_t{0, ew_t{0}}, ei_t{1, ew_t{0}}, ei_t{2, ew_t{0}}, ei_t{2, ew_t{0}}, ei_t{2, ew_t{2}}, ei_t{2, ew_t{2}}}},
        {std::vector<int>{3}, std::vector<int>{2}, std::vector<int>{1}}
      };
      
      expected.swap_edges(2, 0, 1);
      check_equality(graph, expected, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::directed)
    {
      graph_t expected{
        {{ei_t{1, ew_t{0}}},
         {ei_t{0, ew_t{0}}, ei_t{2, ew_t{0}}},
         {ei_t{0, ew_t{0}}, ei_t{2, ew_t{0}}, ei_t{2, ew_t{1}}}},
        {std::vector<int>{3}, std::vector<int>{2}, std::vector<int>{1}}
      };
      
      check_equality(graph, expected, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected_embedded)
    {
      graph_t expected{
        {{ei_t{1,0, ew_t{0}}, ei_t{1,1, ew_t{1}}, ei_t{2,1, ew_t{0}}},
         {ei_t{0,0, ew_t{0}}, ei_t{0,1, ew_t{1}}, ei_t{2,0, ew_t{0}}},
         {ei_t{1,2, ew_t{0}}, ei_t{0,2, ew_t{0}}, ei_t{2,3, ew_t{0}}, ei_t{2,2, ew_t{0}}, ei_t{2,5, ew_t{2}}, ei_t{2,4, ew_t{2}}}},
        {std::vector<int>{3}, std::vector<int>{2}, std::vector<int>{1}}
      };

      check_equality(graph, expected, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      graph_t expected{
        {{ei_t{0,1,0,ew_t{0}}, ei_t{1,0,1,ew_t{0}}, ei_t{2,0,1,ew_t{0}}},
         {ei_t{0,1,0,ew_t{0}}, ei_t{1,0,1,ew_t{0}}, ei_t{1,2,0,ew_t{0}}},
           {ei_t{1,2,2,ew_t{0}}, ei_t{2,0,2,ew_t{0}}, ei_t{2,2,3,ew_t{1}}, ei_t{2,2,2,ew_t{1}}, ei_t{2,2,5,ew_t{3}}, ei_t{2,2,4,ew_t{3}}}},
        {std::vector<int>{3}, std::vector<int>{2}, std::vector<int>{1}}
      };

      check_equality(graph, expected, LINE(""));
    }
    else
    {
      static_assert(dependent_false<graph_t>::value);
    }

    if constexpr(maths::undirected(GraphFlavour))
    {
      test_second_edge_traversal_update(graph);
    }
  }

  template
  <
    maths::graph_flavour GraphFlavour,
    class EdgeWeight,
    class NodeWeight,      
    template <class> class EdgeWeightPooling,
    template <class> class NodeWeightPooling,
    template <maths::graph_flavour, class, template<class> class> class EdgeStorageTraits,
    template <class, template<class> class, bool> class NodeWeightStorageTraits
  >
  void test_bf_update<
    GraphFlavour,
    EdgeWeight,
    NodeWeight,
    EdgeWeightPooling,
    NodeWeightPooling,
    EdgeStorageTraits,
    NodeWeightStorageTraits>::test_second_edge_traversal_update(graph_t& graph)
  {
    using maths::graph_flavour; 
    
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

    maths::breadth_first_search(graph, false, 0, maths::null_functor(), maths::null_functor(), maths::null_functor(), edgeFn2);

    if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t expected{
        {{ei_t{1, ew_t{0,1}}, ei_t{1, ew_t{0,2}}, ei_t{2, ew_t{0,1}}},
         {ei_t{0, ew_t{0,1}}, ei_t{0, ew_t{0,2}}, ei_t{2, ew_t{1,1}}},
         {ei_t{0, ew_t{0,1}}, ei_t{1, ew_t{1,1}}, ei_t{2, ew_t{2,2}}, ei_t{2, ew_t{2,2}}, ei_t{2, ew_t{2,4}}, ei_t{2, ew_t{2,4}}}},
        {std::vector<int>{3}, std::vector<int>{2}, std::vector<int>{1}}
      };
      
      expected.swap_edges(2, 0, 1);
      check_equality(graph, expected, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected_embedded)
    {
      graph_t expected{
        {{ei_t{1,0, ew_t{0,1}}, ei_t{1,1, ew_t{0,2}}, ei_t{2,1, ew_t{0,1}}},
         {ei_t{0,0, ew_t{0,1}}, ei_t{0,1, ew_t{0,2}}, ei_t{2,0, ew_t{1,1}}},
         {ei_t{1,2, ew_t{1,1}}, ei_t{0,2, ew_t{0,1}}, ei_t{2,3, ew_t{2,2}}, ei_t{2,2, ew_t{2,2}}, ei_t{2,5, ew_t{2,4}}, ei_t{2,4, ew_t{2,4}}}},
        {std::vector<int>{3}, std::vector<int>{2}, std::vector<int>{1}}
      };

      check_equality(graph, expected, LINE(""));
    }
    else
    {
      static_assert(dependent_false<graph_t>::value);
    }    
  }
}
