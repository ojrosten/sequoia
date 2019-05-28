////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "DynamicGraphTraversalsTest.hpp"

namespace sequoia::unit_testing
{
  void test_graph_traversals::run_tests()
  {
    test_prs_details();

    {
      graph_test_helper<null_weight, null_weight> helper;
      helper.run_tests<tracker_test>(*this);
    }

    {
      graph_test_helper<null_weight, int> helper;
      helper.run_tests<test_weighted_BFS_tasks>(*this);
    }
      
    {
      graph_test_helper<null_weight, int> helper;
      helper.run_tests<test_priority_traversal>(*this);
    }
  }

  void test_graph_traversals::test_prs_details()
  {
    using namespace maths;

    using graph_type = embedded_graph<directed_flavour::undirected, null_weight, int>;
    graph_type graph;

    graph.add_node(3);
    graph.add_node(2);

    using node_comparer = graph_impl::node_comparer<graph_type, std::less<int>>;
    node_comparer compare(graph);

    check(!compare(0, 1), "node_comparer sees that weight_0 > weight_1 and so returns false");

    auto stack = graph_impl::queue_constructor<graph_type, std::stack<std::size_t>>::make(graph);
    stack.push(0);
    stack.push(1);
    check_equality(stack.top(), 1ul, LINE(""));
    stack.pop();
    check_equality(stack.top(), 0ul, LINE(""));

    using PQ_t = std::priority_queue<std::size_t, std::vector<std::size_t>, graph_impl::node_comparer<graph_type, std::less<int>>>;

    auto pqueue = graph_impl::queue_constructor<graph_type, PQ_t>::make(graph);
    pqueue.push(0);
    pqueue.push(1);

    check_equality(pqueue.top(), 0ul, LINE(""));
  }

  //=============================== Tracker Test ===============================//
  
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
  template<class Traverser>
  void tracker_test<
    GraphFlavour,
    EdgeWeight,
    NodeWeight,
    EdgeWeightPooling,
    NodeWeightPooling,
    EdgeStorageTraits,
    NodeWeightStorageTraits>::test_tracker_algorithm()
  {
    using namespace maths;
        
    constexpr bool isBFS{std::is_same<typename Traverser::type, BFS>::value};
    constexpr bool mutualInfo{mutual_info(GraphFlavour)};
        
    using UndirectedType = std::bool_constant<maths::undirected(GraphFlavour)>;        
    using TraversalType = std::integral_constant<bool, isBFS>;

    const std::string iterDescription{Traverser::iterator_description()};
    constexpr bool forwardIter{Traverser::uses_forward_iterator()};

    Connectivity network;
    NodeTracker discovery, discovery2;
    EdgeTracker<Connectivity, Traverser> edgeDiscovery{network}, edgeDiscovery2{network};

    if constexpr(UndirectedType::value)
    {
      traverse_graph<Traverser>(network, true, 0, discovery, discovery2, edgeDiscovery, edgeDiscovery2);
    }
    else
    {
      traverse_graph<Traverser>(network, true, 0, discovery, discovery2, edgeDiscovery);
    }
        
    auto order = discovery.order();
    auto order2 = discovery2.order();
    auto edgeOrder = edgeDiscovery.order();
    
    check(order.empty(), LINE("No nodes to discover"));
    check_equality(order, order2, LINE(""));
    check(edgeOrder.empty(), LINE("No edges to discover"));
    if constexpr(UndirectedType::value)
    {
      check_equality(edgeDiscovery.order(), edge_results{}, LINE(""));
    }
    
    check_equality(network.add_node(), 0ul, LINE("First node added"));
    // 0

    if constexpr(UndirectedType::value)
    {
      traverse_graph<Traverser>(network, true, 0, discovery, discovery2, edgeDiscovery, edgeDiscovery2);
    }
    else
    {
      traverse_graph<Traverser>(network, true, 0, discovery, discovery2, edgeDiscovery);
    }

    order = discovery.order();
    order2 = discovery2.order();
    edgeOrder = edgeDiscovery.order();

    if(check_equality(order.size(), 1ul, LINE("One node to discover")))
    {
      check_equality(order.front(), 0ul, LINE("Node 0 must be discovered first in single node network"));
    }
    check_equality(order, order2, LINE(""));
    check(edgeOrder.empty(), "No edges to discover");
    if constexpr(UndirectedType::value)
    {
      check_equality(edgeDiscovery.order(), edge_results{}, LINE(""));
    }  

    check_equality(network.add_node(), 1ul, LINE("Second node added"));
    // 0 0

    if constexpr(UndirectedType::value)
    {
      traverse_graph<Traverser>(network, true, 0, discovery, discovery2, edgeDiscovery, edgeDiscovery2);
    }
    else
    {
      traverse_graph<Traverser>(network, true, 0, discovery, discovery2, edgeDiscovery);
    }
 
    order = discovery.order();
    order2 = discovery2.order();
    edgeOrder = edgeDiscovery.order();

    check_equality(order.size(), 2ul, LINE("Two nodes to discover"));
    if(order.size() == 2)
     {
       auto iter = order.begin();
       check_equality(*iter, 0ul, LINE("Node 0 discovered first"));
       check_equality(*++iter, 1ul, LINE("Node 1 discovered second"));
     }
    
    check_equality(order, order2, LINE(""));
    check(edgeOrder.empty(), LINE("No edges to discover"));
    if constexpr(UndirectedType::value)
    {
      check_equality(edgeDiscovery.order(), edge_results{}, LINE(""));
    }
    
    if constexpr(UndirectedType::value)
    {
      traverse_graph<Traverser>(network, true, 1, discovery, discovery2, edgeDiscovery, edgeDiscovery2);
    }
    else
    {
      traverse_graph<Traverser>(network, true, 1, discovery, discovery2, edgeDiscovery);
    }

    order = discovery.order();
    order2 = discovery2.order();
    edgeOrder = edgeDiscovery.order();
    
    check_equality(order.size(), 2ul, LINE("Two nodes to discover but this time in reverse order"));

    if(order.size() == 2)
      {
        auto iter = order.begin();
        check_equality(*iter, 1ul, LINE("Node 1 discovered first"));
        check_equality(*++iter, 0ul, LINE("Node 0 discovered second"));
      }
    check_equality(order, order2, LINE(""));
    check(edgeOrder.empty(), LINE("No edges to discover"));
    if constexpr(UndirectedType::value)
    {
      check_equality(edgeDiscovery.order(), edge_results{}, LINE(""));
    }
    
    if constexpr(UndirectedType::value)
    {
      traverse_graph<Traverser>(network, false, 0, discovery, discovery2, edgeDiscovery, edgeDiscovery2);
    }
    else
    {
      traverse_graph<Traverser>(network, false, 0, discovery, discovery2, edgeDiscovery);
    }

    order = discovery.order();
    order2 = discovery2.order();
    edgeOrder = edgeDiscovery.order();

    if(check_equality(order.size(), 1ul, LINE("Of two disconnected nodes, only one will be discovered")))
    {
      auto iter = order.begin();
      check_equality(*iter, 0ul, LINE("Node 0, alone, discovered first"));
    }
    check_equality(order, order2, LINE(""));
    check(edgeOrder.empty(), LINE("No edges to discover"));
    if constexpr(UndirectedType::value)
    {
      check_equality(edgeDiscovery.order(), edge_results{}, LINE(""));
    }
    
    check_equality(network.add_node(), 2ul, LINE("Third node added"));
    network.join(0, 1);
    network.join(1, 2);
    // 0----0----0

    if constexpr(UndirectedType::value)
    {
      traverse_graph<Traverser>(network, true, 0, discovery, discovery2, edgeDiscovery, edgeDiscovery2);
    }
    else
    {
      traverse_graph<Traverser>(network, true, 0, discovery, discovery2, edgeDiscovery);
    }

    order = discovery.order();
    order2 = discovery2.order();
    edgeOrder = edgeDiscovery.order();
    
    if(check_equality(order.size(), 3ul, LINE("Three nodes to discover")))
    {
      auto iter = order.begin();
      check_equality(*iter, 0ul, LINE("Starting node 0 discovered first"));
      check_equality(*++iter, 1ul, LINE("Node 1 discovered next"));
      check_equality(*++iter, 2ul, LINE("Node 2 discovered last"));
    }
    check_equality(order, order2, LINE(""));

    if(check_equality(edgeOrder.size(), 2ul, LINE("Two edges to discover")))
    {
      auto iter = edgeOrder.begin();
      check_equality(iter->first, 0ul, LINE("Edge attached to node 0"));
      check_equality(iter->second, 0ul, LINE("Edge has " + iterDescription + " index 0"));
      ++iter;
      check_equality(iter->first, 1ul, LINE("Edge attached to node 1"));
      const auto expected{mutualInfo && forwardIter ? 1ul : 0ul};
      check_equality(iter->second, expected, LINE("Edge has " + iterDescription + " index " + std::to_string(expected)));
    }

    if constexpr(UndirectedType::value)
    {
      check_equality(edgeDiscovery2.order(), isBFS ? edge_results{{1,0}, {2,0}} : edge_results{{1, 1}, {2, 0}}, LINE(""));
    }                 

    std::string message;
    if constexpr(UndirectedType::value)
    {
      traverse_graph<Traverser>(network, false, 1, discovery, discovery2, edgeDiscovery, edgeDiscovery2);
    }
    else
    {
      traverse_graph<Traverser>(network, false, 1, discovery, discovery2, edgeDiscovery);
    }

    order = discovery.order();
    order2 = discovery2.order();
    edgeOrder = edgeDiscovery.order();

    if constexpr(UndirectedType::value)
    {
      if(check_equality(order.size(), 3ul, LINE("Search from middle; still three nodes to discover")))
      {
        auto iter = order.begin();
        check_equality(*iter, 1ul, LINE("Middle node 1 discovered first"));
        check_equality(*++iter, 0ul, LINE("Node 0 discovered next"));
        check_equality(*++iter, 2ul, LINE("Node 2 discovered last"));
      }
    }
    else
    {
      if(check_equality(order.size(), 2ul, LINE("Search from middle; only two nodes to discover")))
      {
        auto iter = order.begin();
        check_equality(*iter, 1ul, LINE("Middle node 1 discovered first"));
        check_equality(*++iter, 2ul, LINE("Node 2 discovered next"));
      }
    }
    
    check_equality(order, order2, LINE(""));
    
    if constexpr(UndirectedType::value)
    {
      if(check_equality(edgeOrder.size(), 2ul, LINE("Search from middle; two edges to discover")))
      {
        auto iter = edgeOrder.begin();
        check_equality(iter->first, 1ul, LINE("Edge attached to node 1"));
        check_equality(iter->second, 0ul, LINE("Edge has " + iterDescription + " index 0"));
        ++iter;
        check_equality(iter->first, 1ul, LINE("Edge attached to node 1"));
        check_equality(iter->second, 1ul, LINE("Edge has " + iterDescription + " index 1"));
      }

      check_equality(edgeDiscovery2.order(), isBFS ? edge_results{{0, 0}, {2, 0}} : edge_results{{0, 0}, {2, 0}}, LINE(""));
    }
    else
    {
      if(check_equality(edgeOrder.size(), 1ul, LINE("Search from middle; only one edge to discover")))
      {
        auto iter = edgeOrder.begin();
        check_equality(iter->first, 1ul, LINE("Edge attached to node 1"));
        const std::string num{mutualInfo ? "1" : "0"};
        const auto expected{(mutualInfo && isBFS) ? 1ul : 0ul};
        check_equality(iter->second, expected, LINE("Edge has " + iterDescription + " index " + std::to_string(expected)));
      }
    }
                                              
    if constexpr(UndirectedType::value)
    {
      traverse_graph<Traverser>(network, false, 2, discovery, discovery2, edgeDiscovery, edgeDiscovery2);
    }
    else
    {
      traverse_graph<Traverser>(network, false, 2, discovery, discovery2, edgeDiscovery);
    }

    order = discovery.order();
    order2 = discovery2.order();
    edgeOrder = edgeDiscovery.order();

    if constexpr(UndirectedType::value)
    {
      if(check_equality(order.size(), 3ul, LINE("Search from end; still three nodes to discover")))
      {
        auto iter = order.begin();
        check_equality(*iter, 2ul, LINE("End node 2 discovered first"));
        check_equality(*++iter, 1ul, LINE("Middle node 1 discovered next"));
        check_equality(*++iter, 0ul, LINE("First node 0 discovered last"));
      }

      if(check_equality(edgeOrder.size(), 2ul, LINE("Search from end; two edges to discover")))
      {
        auto iter = edgeOrder.begin();
        check_equality(iter->first, 2ul, LINE("Edge attached to node 2"));
        check_equality(iter->second, 0ul, LINE("Edge has " + iterDescription + " index 0"));
        ++iter;
        check_equality(iter->first, 1ul, LINE("Edge attached to node 1"));
        const auto expected{forwardIter ? 0ul : 1ul};
        check_equality(iter->second, expected, LINE("Edge has " + iterDescription + " index " + std::to_string(expected)));
      }

      check_equality(edgeDiscovery2.order(), isBFS ? edge_results{{1, 1}, {0, 0}} : edge_results{{1, 0}, {0, 0}}, LINE(""));
    }
    else
    {
      if(check_equality(order.size(), 1ul, LINE("Search for connected components from end, so only one node to discover")))
      {
        auto iter = order.begin();
        check_equality(*iter, 2ul, LINE("End node 2 discovered first"));
      }
      
      check_equality(edgeOrder.size(), 0ul, LINE("Search from end; no edges to discover"));
    }
        
    check_equality(order, order2, LINE(""));
         

    check_equality(network.add_node(), 3ul, LINE("Fourth node added"));
    network.join(2, 3);
    network.join(3, 0);
    //  0----0
    //  |    |
    //  |    |
    //  0----0

    if constexpr(UndirectedType::value)
    {
      traverse_graph<Traverser>(network, false, 0, discovery, discovery2, edgeDiscovery, edgeDiscovery2);
    }
    else
    {
      traverse_graph<Traverser>(network, false, 0, discovery, discovery2, edgeDiscovery);
    }
    
    order = discovery.order();
    order2 = discovery2.order();
    edgeOrder = edgeDiscovery.order();

    if(check_equality(order.size(), 4ul, LINE("Four nodes to discover")))
    {
      if constexpr(UndirectedType::value)
      {
        test_square_graph(discovery, edgeDiscovery, edgeDiscovery2, 0, mutualInfo, TraversalType{});
      }
      else
      {
        test_square_graph(discovery, edgeDiscovery, 0, mutualInfo, TraversalType{});
      }
    }

    check_equality(order, order2, LINE(""));

    if constexpr(UndirectedType::value)
    {
      traverse_graph<Traverser>(network, false, 2, discovery, discovery2, edgeDiscovery, edgeDiscovery2);
    }
    else
    {
      traverse_graph<Traverser>(network, false, 2, discovery, discovery2, edgeDiscovery);
    }

    order = discovery.order();
    order2 = discovery2.order();
    edgeOrder = edgeDiscovery.order();

    if(check_equality(order.size(), 4ul, LINE("Four nodes to discover but this time starting from middle")))
    {
      if constexpr(UndirectedType::value)
      {
        test_square_graph(discovery, edgeDiscovery, edgeDiscovery2, 2, mutualInfo, TraversalType{});
      }
      else
      {
        test_square_graph(discovery, edgeDiscovery, 2, mutualInfo, TraversalType{});
      }
    }
    
    check_equality(order, order2, LINE(""));
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
  template<class NTracker, class ETracker, class ETracker2>
  void tracker_test<
    GraphFlavour,
    EdgeWeight,
    NodeWeight,
    EdgeWeightPooling,
    NodeWeightPooling,
    EdgeStorageTraits,
    NodeWeightStorageTraits>::test_square_graph(const NTracker& tracker, const ETracker& eTracker, const ETracker2& eTracker2, const std::size_t start, const bool mutualInfo, std::true_type)
  {
    std::vector<std::size_t> expected;
    edge_results edgeAnswers, edgeAnswers2;
    if(start == 0)
    {
      expected = std::vector<std::size_t>{0,1,3,2};
      edgeAnswers = edge_results{{0, 0}, {0, 1}, {1, 1}, {3, 0}};
      edgeAnswers2 = edge_results{{1, 0}, {3, 1}, {2, 0}, {2, 1}};
    }
    else if(start == 2)
    {
      expected = std::vector<std::size_t>{2,1,3,0};
      edgeAnswers = edge_results{{2, 0}, {2, 1}, {1, 0}, {3, 1}};
      edgeAnswers2 = edge_results{{1, 1}, {3, 0}, {0, 0}, {0, 1}};
    }
    check_equality(expected, tracker.order(), LINE(""));
    check_equality(edgeAnswers, eTracker.order(), LINE("First edge traversal"));
    check_equality(edgeAnswers2, eTracker2.order(), LINE("Second edge traversal"));
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
  template<class NTracker, class ETracker>
  void tracker_test<
    GraphFlavour,
    EdgeWeight,
    NodeWeight,
    EdgeWeightPooling,
    NodeWeightPooling,
    EdgeStorageTraits,
    NodeWeightStorageTraits>::test_square_graph(const NTracker& tracker, const ETracker& eTracker, const std::size_t start, const bool mutualInfo, std::true_type)
  {
    std::vector<std::size_t> expected;
    edge_results edgeAnswers;
    if(start == 0)
    {
      expected = std::vector<std::size_t>{0,1,2,3};
      edgeAnswers = mutualInfo ? edge_results{{0, 0}, {1, 1}, {2, 1}, {3, 1}} : edge_results{{0, 0}, {1, 0}, {2, 0}, {3, 0}};
    }
    else if(start ==2)
    {
      expected = std::vector<std::size_t>{2,3,0,1};
      edgeAnswers = mutualInfo ? edge_results{{2, 1}, {3, 1}, {0, 0}, {1, 1}} : edge_results{{2, 0}, {3, 0}, {0, 0}, {1, 0}};
    }
    check_equality(expected, tracker.order(), LINE("start = " + std::to_string(start) + " "));
    check_equality(edgeAnswers, eTracker.order(), LINE("First edge traversal, start = " + std::to_string(start) + " "));
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
  template<class NTracker, class ETracker, class ETracker2>
  void tracker_test<
    GraphFlavour,
    EdgeWeight,
    NodeWeight,
    EdgeWeightPooling,
    NodeWeightPooling,
    EdgeStorageTraits,
    NodeWeightStorageTraits>::test_square_graph(const NTracker& tracker, const ETracker& eTracker, const ETracker2& eTracker2, const std::size_t start, const bool mutualInfo, std::false_type)
  {
    std::vector<std::size_t> expected;
    edge_results edgeAnswers, edgeAnswers2;
    if(start == 0)
    {
      expected = std::vector<std::size_t>{0,1,2,3};
      edgeAnswers = edge_results{{0, 0}, {0, 1}, {1, 0}, {2, 0}};
      edgeAnswers2 = edge_results{{1, 1}, {2, 1}, {3, 0}, {3, 1}};
    }
    else if(start ==2)
    {
      expected = std::vector<std::size_t>{2,1,0,3};
      edgeAnswers = edge_results{{2, 0}, {2, 1}, {1, 1}, {0, 0}};
      edgeAnswers2 = edge_results{{1, 0}, {0, 1}, {3, 0}, {3, 1}};
    }
    check_equality(expected, tracker.order(), LINE(""));
    check_equality(edgeAnswers, eTracker.order(), LINE("First edge traversal"));
    check_equality(edgeAnswers2, eTracker2.order(), LINE("Second edge traversal"));
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
  template<class NTracker, class ETracker>
  void tracker_test<
    GraphFlavour,
    EdgeWeight,
    NodeWeight,
    EdgeWeightPooling,
    NodeWeightPooling,
    EdgeStorageTraits,
    NodeWeightStorageTraits>::test_square_graph(const NTracker& tracker, const ETracker& eTracker, const std::size_t start, const bool mutualInfo, std::false_type)
  {
    std::vector<std::size_t> expected;
    edge_results edgeAnswers;
    if(start == 0)
    {
      expected = std::vector<std::size_t>{0,1,2,3};
      edgeAnswers = mutualInfo ? edge_results{{0, 1}, {1, 0}, {2, 0}, {3, 0}} : edge_results{{0, 0}, {1, 0}, {2, 0}, {3, 0}};
    }
    else if(start ==2)
    {
      expected = std::vector<std::size_t>{2,3,0,1};
      edgeAnswers = mutualInfo ? edge_results{{2, 0}, {3, 0}, {0, 1}, {1, 0}} : edge_results{{2, 0}, {3, 0}, {0, 0}, {1, 0}};
    }
    check_equality(expected, tracker.order(), LINE("start = " + std::to_string(start) + " "));
    check_equality(edgeAnswers, eTracker.order(), LINE("First edge traversal, start = " + std::to_string(start) + " "));
  }

  //=============================== Priority Search  ===============================//

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
  void test_priority_traversal<
    GraphFlavour,
    EdgeWeight,
    NodeWeight,
    EdgeWeightPooling,
    NodeWeightPooling,
    EdgeStorageTraits,
    NodeWeightStorageTraits>::test_prs()
  {
    auto graph{generate_test_graph()};

    NodeTracker tracker;
    maths::priority_search(graph, false, 0, tracker);

    auto order = tracker.order();
    check_equality(order, std::vector<std::size_t>{0,2,4,3,6,1,5}, LINE(""));
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
  auto test_priority_traversal<
    GraphFlavour,
    EdgeWeight,
    NodeWeight,
    EdgeWeightPooling,
    NodeWeightPooling,
    EdgeStorageTraits,
    NodeWeightStorageTraits>::generate_test_graph() -> graph_t
  {
    graph_t graph;
        
    graph.add_node(1);
    graph.add_node(5);
    graph.add_node(10);
    graph.add_node(7);
    graph.add_node(8);
    graph.add_node(4);
    graph.add_node(6);

    graph.join(0,1);
    graph.join(0,2);
    graph.join(0,3);
    graph.join(0,4);
    graph.join(3,5);
    graph.join(3,6);

    //     1
    //     |
    // --------
    // |  | | |
    // 5 10 7 8
    //     /\
    //    4  6

    return graph;
  }

  //=============================== Weighted BFS  ===============================//

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
  void test_weighted_BFS_tasks<
    GraphFlavour,
    EdgeWeight,
    NodeWeight,
    EdgeWeightPooling,
    NodeWeightPooling,
    EdgeStorageTraits,
    NodeWeightStorageTraits>::test_edge_second_traversal(std::true_type)
  {
    graph_t graph{generate_test_graph()};

    constexpr int upper = 6;

    auto serialFn = [&graph](){
      return edge_second_traversal_task<concurrency::serial<int>>(graph, upper);
    };

    auto asyncFn = [&graph](){
      return edge_second_traversal_task<concurrency::asynchronous<int>>(graph, upper);
    };
    
    auto poolFn = [&graph](){
      return edge_second_traversal_task<concurrency::thread_pool<int>>(graph, upper, 4u);
    };

    auto answers{
      [](const int upper) -> std::vector<int> {
        std::vector<int> answers;

        for(int i=0; i < 4; ++i)
        {
          const int shift = -i - 1;
          answers.push_back((upper + shift)*(upper + shift + 1) / 2);
        }

        return answers;
      }
    };

    const std::vector<int>
      serialResults = serialFn(),
      asyncResults = asyncFn(),
      poolResults = poolFn(),
      expected = answers(upper);

    check_equality(serialResults, expected, "Null edge first task expected");
    check_equality(asyncResults, expected, "Async edge first task expected");
    check_equality(poolResults, expected, "Pool edge first task expected");
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
  void test_weighted_BFS_tasks<
    GraphFlavour,
    EdgeWeight,
    NodeWeight,
    EdgeWeightPooling,
    NodeWeightPooling,
    EdgeStorageTraits,
    NodeWeightStorageTraits>::test_node_and_first_edge_traversal()
  {
    graph_t graph{generate_test_graph()};

    //================================ Node functors =========================//

    int upper = 10;
    for(int i=0; i < 2; ++i)
    {
      const bool early{i == 0};
      using microseconds = std::chrono::microseconds;
      auto serialFn  = [&graph, upper, early](){ return task<concurrency::serial<int>>(graph, upper, early, microseconds{}); };
      auto asyncFn = [&graph, upper, early](){ return task<concurrency::asynchronous<int>>(graph, upper, early, microseconds{}); };
      auto poolFn  = [&graph, upper, early](){ return task<concurrency::thread_pool<int>>(graph, upper, early, microseconds{}, 4u); };
      const std::vector<int>
        serialResults = serialFn(),
        asyncResults = asyncFn(),
        poolResults = poolFn(),
        expected = node_task_answers(upper);

      check_equality(serialResults, expected, "Null node task expected");
      check_equality(asyncResults, expected, "Async node task expected");
      check_equality(poolResults, expected, "Pool node task expected");
    }

    //================================ Edge First Traversal functors =========================//

    {
      upper = 6;
          
      auto serialFn  = [&graph, upper](){
        return edge_first_traversal_task<concurrency::serial<int>>(graph, upper);
      };

      auto asyncFn = [&graph, upper](){
        return edge_first_traversal_task<concurrency::asynchronous<int>>(graph, upper);
      };
      
      auto poolFn  = [&graph, upper](){
        return edge_first_traversal_task<concurrency::thread_pool<int>>(graph, upper, 4u);
      };

      const std::vector<int>
        serialResults = serialFn(),
        asyncResults = asyncFn(),
        poolResults = poolFn(),
        expected = edge_task_answers(upper);

      check_equality(serialResults, expected, "Null edge first task expected");
      check_equality(asyncResults, expected, "Async edge first task expected");
      check_equality(poolResults, expected, "Pool edge first task expected");

    }


    //================================ Now check performance =========================//

    {
      upper = 10;

      const auto pause{std::chrono::microseconds(500)};
          
      auto serialFn{
        [&graph, upper, pause]() {
          return task<concurrency::serial<int>>(graph, upper, true, pause);
        }
      };
          
      auto asyncFn{
        [&graph, upper, pause](){
          return task<concurrency::asynchronous<int>>(graph, upper, true, pause);
        }
      };
          
      auto poolFn{
        [&graph, upper, pause](){
          return task<concurrency::thread_pool<int>>(graph, upper, true, pause, 4u);
        }
      };

      auto expected = node_task_answers(upper);

      auto futures = check_relative_performance(asyncFn, serialFn, 2.0, 4.5, LINE("Null versus async check"));
      auto futures2 = check_relative_performance(poolFn, serialFn, 2.0, 4.5, LINE("Null versus pool check"));

      for(auto&& fut : futures.fast_futures)
      {
        auto results = fut.get();
        check_equality(results, expected, LINE("Async node performance task results wrong"));
      }

      for(auto&& fut : futures.slow_futures)
      {
        auto results = fut.get();
        check_equality(results, expected, LINE("Null node performance task results wrong"));
      }

      for(auto&& fut : futures2.fast_futures)
      {
        auto results = fut.get();
        check_equality(results, expected, LINE("Pool node performance task results wrong"));
      }
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
  template<class ProcessingModel, class... Args>
  std::vector<int> test_weighted_BFS_tasks<
    GraphFlavour,
    EdgeWeight,
    NodeWeight,
    EdgeWeightPooling,
    NodeWeightPooling,
    EdgeStorageTraits,
    NodeWeightStorageTraits>::task(graph_t& graph, const int upper, const bool early, const std::chrono::microseconds pause, Args&&... args)
  {
    auto fn = [upper, pause](const std::size_t index) {
      std::this_thread::sleep_for(pause);
      const auto n{upper + static_cast<int>(index)};
      return n * (n + 1) /2;
    };

    using maths::null_functor;
    if constexpr(graph_t::directedness == maths::directed_flavour::directed)
    {
      return early
        ? maths::breadth_first_search(graph, false, 0, fn, null_functor{}, null_functor{}, ProcessingModel{std::forward<Args>(args)...})
        : maths::breadth_first_search(graph, false, 0, null_functor{}, fn, null_functor{}, ProcessingModel{std::forward<Args>(args)...});
    }
    else
    {
      return early
        ? maths::breadth_first_search(graph, false, 0, fn, null_functor{}, null_functor{}, null_functor{}, ProcessingModel{std::forward<Args>(args)...})
        : maths::breadth_first_search(graph, false, 0, null_functor{}, fn, null_functor{}, null_functor{}, ProcessingModel{std::forward<Args>(args)...});
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
  template<class ProcessingModel, class... Args>
  std::vector<int> test_weighted_BFS_tasks<
    GraphFlavour,
    EdgeWeight,
    NodeWeight,
    EdgeWeightPooling,
    NodeWeightPooling,
    EdgeStorageTraits,
    NodeWeightStorageTraits>::edge_first_traversal_task(graph_t& graph, const int upper, Args&&... args)
  {
    auto fn = [upper](auto edgeIter) {
      const auto n{upper - static_cast<int>(edgeIter.partition_index())};
      return n*(n+1)/2;
    };

    using maths::null_functor;
    if constexpr(graph_t::directedness == maths::directed_flavour::directed)
    {
      return maths::breadth_first_search(graph, false, 0, null_functor{}, null_functor{}, fn, ProcessingModel{std::forward<Args>(args)...});
    }
    else
    {
      return maths::breadth_first_search(graph, false, 0, null_functor{}, null_functor{}, fn, null_functor{}, ProcessingModel{std::forward<Args>(args)...});
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
  template<class ProcessingModel, class... Args>
  std::vector<int> test_weighted_BFS_tasks<
    GraphFlavour,
    EdgeWeight,
    NodeWeight,
    EdgeWeightPooling,
    NodeWeightPooling,
    EdgeStorageTraits,
    NodeWeightStorageTraits>::edge_second_traversal_task(graph_t& graph, const int upper, Args&&... args)
  {
    auto fn = [upper](auto edgeIter) {
      const auto n{upper - static_cast<int>(edgeIter.partition_index())};
      return n*(n+1)/2;
    };

    using maths::null_functor;
    return maths::breadth_first_search(graph, false, 0, null_functor{}, null_functor{}, null_functor{}, fn, ProcessingModel{std::forward<Args>(args)...});
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
  auto test_weighted_BFS_tasks<
    GraphFlavour,
    EdgeWeight,
    NodeWeight,
    EdgeWeightPooling,
    NodeWeightPooling,
    EdgeStorageTraits,
    NodeWeightStorageTraits>::generate_test_graph() -> graph_t
  {
    graph_t graph;

    std::size_t depth{};
    int value = 1;
    std::queue<std::size_t> parents;
    parents.push(graph.add_node(value));

    //    0
    //   / \
    //  0   0
    //  /\
    // 0  0

    auto add_daughters{
      [&graph](const std::size_t index, std::pair<int, int> values)
        -> std::pair<std::size_t, std::size_t> {
        const std::size_t index0{graph.add_node(values.first)};
        const std::size_t index1{graph.add_node(values.second)};

        graph.join(index, index0);
        graph.join(index, index1);

        return std::make_pair(index0, index1);
      }
    };

    while(depth < 2)
    {
      ++value;
      const auto node = parents.front();
      auto daughters = add_daughters(node, std::make_pair(value, value));

      parents.pop();
      parents.push(daughters.first);
      parents.push(daughters.second);
      ++depth;
    }

    return graph;
  }
}
