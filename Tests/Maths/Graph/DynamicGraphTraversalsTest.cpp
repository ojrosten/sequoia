////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "DynamicGraphTraversalsTest.hpp"

namespace sequoia::unit_testing
{
  [[nodiscard]]
  std::string_view test_graph_traversals::source_file_name() const noexcept
  {
    return __FILE__;
  }

  void test_graph_traversals::run_tests()
  {
    test_prs_details();

    {
      graph_test_helper<null_weight, null_weight> helper{concurrent_execution()};
      helper.run_tests<tracker_test>(*this);
    }

    {
      graph_test_helper<null_weight, int> helper{concurrent_execution()};
      helper.run_tests<test_weighted_BFS_tasks>(*this);
    }
      
    {
      graph_test_helper<null_weight, int> helper{concurrent_execution()};
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

    check(LINE("node_comparer sees that weight_0 > weight_1 and so returns false"), !compare(0, 1));

    auto stack = graph_impl::queue_constructor<graph_type, std::stack<std::size_t>>::make(graph);
    stack.push(0);
    stack.push(1);
    check_equality(LINE(""), stack.top(), 1ul);
    stack.pop();
    check_equality(LINE(""), stack.top(), 0ul);

    using PQ_t = std::priority_queue<std::size_t, std::vector<std::size_t>, graph_impl::node_comparer<graph_type, std::less<int>>>;

    auto pqueue = graph_impl::queue_constructor<graph_type, PQ_t>::make(graph);
    pqueue.push(0);
    pqueue.push(1);

    check_equality(LINE(""), pqueue.top(), 0ul);
  }

  //=============================== Tracker Test ===============================//
  
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
    constexpr bool undirected{maths::undirected(GraphFlavour)};
    
    using TraversalType = std::bool_constant<isBFS>;
   
    const std::string iterDescription{Traverser::iterator_description()};
    constexpr bool forwardIter{Traverser::uses_forward_iterator()};

    auto make_message{
      [](std::string_view message) {
        return combine_messages(demangle<Traverser>(), message);
      }
    };

    graph_t network;
    node_tracker discovery, discovery2;
    edge_tracker<graph_t, Traverser> edgeDiscovery{network}, edgeDiscovery2{network};

    if constexpr(undirected)
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
    
    check(LINE(make_message("No nodes to discover")), order.empty());
    check_equality(LINE(make_message("")), order, order2);
    check(LINE(make_message("No edges to discover")), edgeOrder.empty());
    if constexpr(undirected)
    {
      check_equality(LINE(make_message("")), edgeDiscovery.order(), edge_results{});
    }
    
    check_equality(LINE(make_message("First node added")), network.add_node(), 0ul);
    // 0

    if constexpr(undirected)
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

    if(check_equality(LINE(make_message("One node to discover")), order.size(), 1ul))
    {
      check_equality(LINE(make_message("Node 0 must be discovered first in single node network")), order.front(), 0ul);
    }
    check_equality(LINE(make_message("")), order, order2);
    check(LINE(make_message("No edges to discover")), edgeOrder.empty());
    if constexpr(undirected)
    {
      check_equality(LINE(make_message("")), edgeDiscovery.order(), edge_results{});
    }  

    check_equality(LINE(make_message("Second node added")), network.add_node(), 1ul);
    // 0 0

    if constexpr(undirected)
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

    check_equality(LINE(make_message("Two nodes to discover")), order.size(), 2ul);
    if(order.size() == 2)
    {
      auto iter = order.begin();
      check_equality(LINE(make_message("Node 0 discovered first")), *iter, 0ul);
      check_equality(LINE(make_message("Node 1 discovered second")), *++iter, 1ul);
    }
    
    check_equality(LINE(make_message("")), order, order2);
    check(LINE(make_message("No edges to discover")), edgeOrder.empty());
    if constexpr(undirected)
    {
      check_equality(LINE(make_message("")), edgeDiscovery.order(), edge_results{});
    }
    
    if constexpr(undirected)
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
    
    check_equality(LINE(make_message("Two nodes to discover but this time in reverse order")), order.size(), 2ul);

    if(order.size() == 2)
    {
      auto iter = order.begin();
      check_equality(LINE(make_message("Node 1 discovered first")), *iter, 1ul);
      check_equality(LINE(make_message("Node 0 discovered second")), *++iter, 0ul);
    }
    check_equality(LINE(make_message("")), order, order2);
    check(LINE(make_message("No edges to discover")), edgeOrder.empty());
    if constexpr(undirected)
    {
      check_equality(LINE(make_message("")), edgeDiscovery.order(), edge_results{});
    }
    
    if constexpr(undirected)
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

    if(check_equality(LINE(make_message("Of two disconnected nodes, only one will be discovered")), order.size(), 1ul))
    {
      auto iter = order.begin();
      check_equality(LINE(make_message("Node 0, alone, discovered first")), *iter, 0ul);
    }
    check_equality(LINE(make_message("")), order, order2);
    check(LINE(make_message("No edges to discover")), edgeOrder.empty());
    if constexpr(undirected)
    {
      check_equality(LINE(make_message("")), edgeDiscovery.order(), edge_results{});
    }
    
    check_equality(LINE(make_message("Third node added")), network.add_node(), 2ul);
    network.join(0, 1);
    network.join(1, 2);
    // 0----0----0

    if constexpr(undirected)
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
    
    if(check_equality(LINE(make_message("Three nodes to discover")), order.size(), 3ul))
    {
      auto iter = order.begin();
      check_equality(LINE(make_message("Starting node 0 discovered first")), *iter, 0ul);
      check_equality(LINE(make_message("Node 1 discovered next")), *++iter, 1ul);
      check_equality(LINE(make_message("Node 2 discovered last")), *++iter, 2ul);
    }
    check_equality(LINE(make_message("")), order, order2);

    if(check_equality(LINE(make_message("Two edges to discover")), edgeOrder.size(), 2ul))
    {
      auto iter = edgeOrder.begin();
      check_equality(LINE(make_message("Edge attached to node 0")), iter->first, 0ul);
      check_equality(LINE(make_message("Edge has " + iterDescription + " index 0")), iter->second, 0ul);
      ++iter;
      check_equality(LINE(make_message("Edge attached to node 1")), iter->first, 1ul);
      const auto expected{mutualInfo && forwardIter ? 1ul : 0ul};
      check_equality(LINE(make_message("Edge has " + iterDescription + " index " + std::to_string(expected))), iter->second, expected);
    }

    if constexpr(undirected)
    {
      check_equality(LINE(make_message("")), edgeDiscovery2.order(), isBFS ? edge_results{{1,0}, {2,0}} : edge_results{{1, 1}, {2, 0}});
    }                 

    if constexpr(undirected)
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

    if constexpr(undirected)
    {
      if(check_equality(LINE(make_message("Search from middle; still three nodes to discover")), order.size(), 3ul))
      {
        auto iter = order.begin();
        check_equality(LINE(make_message("Middle node 1 discovered first")), *iter, 1ul);
        check_equality(LINE(make_message("Node 0 discovered next")), *++iter, 0ul);
        check_equality(LINE(make_message("Node 2 discovered last")), *++iter, 2ul);
      }
    }
    else
    {
      if(check_equality(LINE(make_message("Search from middle; only two nodes to discover")), order.size(), 2ul))
      {
        auto iter = order.begin();
        check_equality(LINE(make_message("Middle node 1 discovered first")), *iter, 1ul);
        check_equality(LINE(make_message("Node 2 discovered next")), *++iter, 2ul);
      }
    }
    
    check_equality(LINE(make_message("")), order, order2);
    
    if constexpr(undirected)
    {
      if(check_equality(LINE(make_message("Search from middle; two edges to discover")), edgeOrder.size(), 2ul))
      {
        auto iter = edgeOrder.begin();
        check_equality(LINE(make_message("Edge attached to node 1")), iter->first, 1ul);
        check_equality(LINE(make_message("Edge has " + iterDescription + " index 0")), iter->second, 0ul);
        ++iter;
        check_equality(LINE(make_message("Edge attached to node 1")), iter->first, 1ul);
        check_equality(LINE(make_message("Edge has " + iterDescription + " index 1")), iter->second, 1ul);
      }

      check_equality(LINE(make_message("")), edgeDiscovery2.order(), isBFS ? edge_results{{0, 0}, {2, 0}} : edge_results{{0, 0}, {2, 0}});
    }
    else
    {
      if(check_equality(LINE(make_message("Search from middle; only one edge to discover")), edgeOrder.size(), 1ul))
      {
        auto iter = edgeOrder.begin();
        check_equality(LINE(make_message("Edge attached to node 1")), iter->first, 1ul);
        const std::string num{mutualInfo ? "1" : "0"};
        const auto expected{(mutualInfo && isBFS) ? 1ul : 0ul};
        check_equality(LINE(make_message("Edge has " + iterDescription + " index " + std::to_string(expected))), iter->second, expected);
      }
    }
                                              
    if constexpr(undirected)
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

    if constexpr(undirected)
    {
      if(check_equality(LINE(make_message("Search from end; still three nodes to discover")), order.size(), 3ul))
      {
        auto iter = order.begin();
        check_equality(LINE(make_message("End node 2 discovered first")), *iter, 2ul);
        check_equality(LINE(make_message("Middle node 1 discovered next")), *++iter, 1ul);
        check_equality(LINE(make_message("First node 0 discovered last")), *++iter, 0ul);
      }

      if(check_equality(LINE(make_message("Search from end; two edges to discover")), edgeOrder.size(), 2ul))
      {
        auto iter = edgeOrder.begin();
        check_equality(LINE(make_message("Edge attached to node 2")), iter->first, 2ul);
        check_equality(LINE(make_message("Edge has " + iterDescription + " index 0")), iter->second, 0ul);
        ++iter;
        check_equality(LINE(make_message("Edge attached to node 1")), iter->first, 1ul);
        const auto expected{forwardIter ? 0ul : 1ul};
        check_equality(LINE(make_message("Edge has " + iterDescription + " index " + std::to_string(expected))), iter->second, expected);
      }

      check_equality(LINE(make_message("")), edgeDiscovery2.order(), isBFS ? edge_results{{1, 1}, {0, 0}} : edge_results{{1, 0}, {0, 0}});
    }
    else
    {
      if(check_equality(LINE(make_message("Search for connected components from end, so only one node to discover")), order.size(), 1ul))
      {
        auto iter = order.begin();
        check_equality(LINE(make_message("End node 2 discovered first")), *iter, 2ul);
      }
      
      check_equality(LINE(make_message("Search from end; no edges to discover")), edgeOrder.size(), 0ul);
    }
        
    check_equality(LINE(make_message("")), order, order2);
         

    check_equality(LINE(make_message("Fourth node added")), network.add_node(), 3ul);
    network.join(2, 3);
    network.join(3, 0);
    //  0----0
    //  |    |
    //  |    |
    //  0----0

    if constexpr(undirected)
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

    if(check_equality(LINE(make_message("Four nodes to discover")), order.size(), 4ul))
    {
      if constexpr(undirected)
      {
        test_square_graph(discovery, edgeDiscovery, edgeDiscovery2, 0, mutualInfo, TraversalType{});
      }
      else
      {
        test_square_graph(discovery, edgeDiscovery, 0, mutualInfo, TraversalType{});
      }
    }

    check_equality(LINE(make_message("")), order, order2);

    if constexpr(undirected)
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

    if(check_equality(LINE(make_message("Four nodes to discover but this time starting from middle")), order.size(), 4ul))
    {
      if constexpr(undirected)
      {
        test_square_graph(discovery, edgeDiscovery, edgeDiscovery2, 2, mutualInfo, TraversalType{});
      }
      else
      {
        test_square_graph(discovery, edgeDiscovery, 2, mutualInfo, TraversalType{});
      }
    }
    
    check_equality(LINE(make_message("")), order, order2);
  }

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
  template<class NTracker, class ETracker, class ETracker2>
  void tracker_test<
    GraphFlavour,
    EdgeWeight,
    NodeWeight,
    EdgeWeightPooling,
    NodeWeightPooling,
    EdgeStorageTraits,
    NodeWeightStorageTraits>::test_square_graph(const NTracker& tracker, const ETracker& eTracker, const ETracker2& eTracker2, const std::size_t start, const bool, std::true_type)
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
    check_equality(LINE(""), tracker.order(), expected);
    check_equality(LINE("First edge traversal"), eTracker.order(), edgeAnswers);
    check_equality(LINE("Second edge traversal"), eTracker2.order(), edgeAnswers2);
  }

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
    check_equality(LINE("start = " + std::to_string(start) + " "), tracker.order(), expected);
    check_equality(LINE("First edge traversal, start = " + std::to_string(start) + " "), eTracker.order(), edgeAnswers);
  }

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
  template<class NTracker, class ETracker, class ETracker2>
  void tracker_test<
    GraphFlavour,
    EdgeWeight,
    NodeWeight,
    EdgeWeightPooling,
    NodeWeightPooling,
    EdgeStorageTraits,
    NodeWeightStorageTraits>::test_square_graph(const NTracker& tracker, const ETracker& eTracker, const ETracker2& eTracker2, const std::size_t start, const bool, std::false_type)
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
    check_equality(LINE(""), tracker.order(), expected);
    check_equality(LINE("First edge traversal"), eTracker.order(), edgeAnswers);
    check_equality(LINE("Second edge traversal"), eTracker2.order(), edgeAnswers2);
  }

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
    check_equality(LINE("start = " + std::to_string(start) + " "), tracker.order(), expected);
    check_equality(LINE("First edge traversal, start = " + std::to_string(start) + " "), eTracker.order(), edgeAnswers);
  }

  //=============================== Priority Search  ===============================//

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

    node_tracker tracker;
    maths::priority_search(graph, false, 0, tracker);

    auto order = tracker.order();
    check_equality(LINE(""), order, std::vector<std::size_t>{0,2,4,3,6,1,5});
  }

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
    class EdgeWeightPooling,
    class NodeWeightPooling,
    class EdgeStorageTraits,
    class NodeWeightStorageTraits
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

    check_equality(LINE("Null edge first task expected"), serialResults, expected);
    check_equality(LINE("Async edge first task expected"), asyncResults, expected);
    check_equality(LINE("Pool edge first task expected"), poolResults, expected);
  }

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

      check_equality(LINE("Null node task expected"), serialResults, expected);
      check_equality(LINE("Async node task expected"), asyncResults, expected);
      check_equality(LINE("Pool node task expected"), poolResults, expected);
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

      check_equality(LINE("Null edge first task expected"), serialResults, expected);
      check_equality(LINE("Async edge first task expected"), asyncResults, expected);
      check_equality(LINE("Pool edge first task expected"), poolResults, expected);

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

      auto futures = check_relative_performance(LINE("Null versus async check"), asyncFn, serialFn, 2.0, 4.5);
      auto futures2 = check_relative_performance(LINE("Null versus pool check"), poolFn, serialFn, 2.0, 4.5);

      for(auto&& fut : futures.fast_futures)
      {
        auto results = fut.get();
        check_equality(LINE("Async node performance task results wrong"), results, expected);
      }

      for(auto&& fut : futures.slow_futures)
      {
        auto results = fut.get();
        check_equality(LINE("Null node performance task results wrong"), results, expected);
      }

      for(auto&& fut : futures2.fast_futures)
      {
        auto results = fut.get();
        check_equality(LINE("Pool node performance task results wrong"), results, expected);
      }
    }
  }

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
    class EdgeWeightPooling,
    class NodeWeightPooling,
    class EdgeStorageTraits,
    class NodeWeightStorageTraits
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
    class EdgeWeightPooling,
    class NodeWeightPooling,
    class EdgeStorageTraits,
    class NodeWeightStorageTraits
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
    class EdgeWeightPooling,
    class NodeWeightPooling,
    class EdgeStorageTraits,
    class NodeWeightStorageTraits
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
