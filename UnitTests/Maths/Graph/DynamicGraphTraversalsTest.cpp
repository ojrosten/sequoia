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
    test_PRS_helpers();

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

  void test_graph_traversals::test_PRS_helpers()
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
      const std::string num{mutualInfo && forwardIter ? "1" : "0"};
      check_equality(iter->second, mutualInfo && forwardIter ? 1ul : 0ul, LINE("Edge has " + iterDescription + " index " + num));
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
}
