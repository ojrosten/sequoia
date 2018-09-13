#include "TestGraph.hpp"

namespace sequoia::unit_testing
{
  void test_graph::run_tests()
  {
    using std::complex;
    using namespace maths;
    using namespace data_structures;

    struct null_weight {};
      
    {
      graph_test_helper<null_weight, null_weight> helper{"unweighted"};
      
      helper.run_tests<generic_graph_operations>(*this);
      helper.run_storage_tests<contiguous_edge_storage_traits, graph_contiguous_capacity>(*this);
      helper.run_storage_tests<bucketed_edge_storage_traits, graph_bucketed_capacity>(*this);
    }
      
    {
      graph_test_helper<int, complex<double>>  helper{"int, complex<double>"};
      
      helper.run_tests<generic_weighted_graph_tests>(*this);

      helper.run_storage_tests<contiguous_edge_storage_traits, graph_contiguous_capacity>(*this);
      helper.run_storage_tests<bucketed_edge_storage_traits, graph_bucketed_capacity>(*this);
    }

    {
      graph_test_helper<complex<int>, complex<double>>  helper{"complex<int>, complex<double>"};
      helper.run_tests<generic_weighted_graph_tests>(*this);
    }

    {
      graph_test_helper<complex<double>, int> helper{"complex<double>, int"};
      helper.run_tests<more_generic_weighted_graph_tests>(*this);
    }
      
    {
      graph_test_helper<std::vector<int>, std::vector<int>>  helper{"Weighted (vector<int>, vector<int>)"};
      helper.run_tests<test_copy_move>(*this);
    }
  }

  // Generic Graph Operations
  
  template
  <
    maths::graph_flavour GraphFlavour,
    class EdgeWeight,
    class NodeWeight,      
    template <class> class EdgeWeightPooling,
    template <class> class NodeWeightPooling,
    template <class, template<class> class> class EdgeStorageTraits,
    template <class, template<class> class, bool> class NodeWeightStorageTraits
  >
  void generic_graph_operations<
      GraphFlavour,    
      EdgeWeight,
      NodeWeight,
      EdgeWeightPooling,
      NodeWeightPooling,
      EdgeStorageTraits,
      NodeWeightStorageTraits
  >::execute_operations()
  {
    using namespace maths;
        
    GGraph network;
    using Edge = edge<EdgeWeight, utilities::protective_wrapper<EdgeWeight>>;
    using E_Edge = embedded_edge<EdgeWeight, data_sharing::independent, utilities::protective_wrapper<EdgeWeight>>;

    check_exception_thrown<std::out_of_range>([&network]() { network.cbegin_edges(0); }, LINE("cbegin_edges throws for empty graph"));
    check_exception_thrown<std::out_of_range>([&network]() { network.cend_edges(0); }, LINE("cend_edges throws for empty graph"));
          
    check_equality<std::size_t>(0,network.add_node(), LINE("Index of added node is 0"));
    //    0

    check_graph(network, {{}}, {}, LINE("Graph is empty"));

    check_exception_thrown<std::out_of_range>([&network](){ get_edge(network, 0, 0, 0); },  LINE("For network with no edges, trying to obtain a reference to one throws an exception"));

    check_exception_thrown<std::out_of_range>([&network]() { network.join(0, 1); }, LINE("Unable to join zeroth node to non-existant first node"));
    check_equality<std::size_t>(1, network.add_node(), LINE("Index of added node is 1"));
    //    0    1

    check_graph(network, {{}, {}}, {}, LINE("Graph is empty"));

    network.join(0, 1);
    //    0----1

    if constexpr (mutual_info(GraphFlavour))
    {
      check_graph(network, {{E_Edge{0,1,0}}, {E_Edge{0,1,0}}}, {}, LINE(""));
    }
    else
    {
      check_graph(network, {{Edge{0,1}}, {}}, {}, LINE(""));
    }

    std::size_t nodeNum{network.add_node()};
    check_equality<std::size_t>(2, nodeNum, LINE("Index of added node is 2"));
    network.join(1, nodeNum);
    //    0----1----2

    if constexpr (mutual_info(GraphFlavour))
    {
      check_graph(network, {{E_Edge{0,1,0}}, {E_Edge{0,1,0}, E_Edge{1,2,0}}, {E_Edge{1,2,1}}}, {}, LINE(""));
    }
    else
    {
      check_graph(network, {{Edge{0,1}}, {Edge{1,2}}, {}}, {}, LINE(""));
    }

    nodeNum = network.add_node();
    check_equality<std::size_t>(3, nodeNum, LINE("Index of added node is 3"));
    network.join(0, nodeNum);
    //    0----1----2
    //    |
    //    3

    if constexpr (mutual_info(GraphFlavour))
    {
      check_graph(network, {{E_Edge(0,1,0), E_Edge(0,3,0)}, {E_Edge(0,1,0), E_Edge(1,2,0)}, {E_Edge(1,2,1)}, {E_Edge(0,3,1)}}, {}, LINE(""));
    }
    else
    {
      check_graph(network, {{Edge(0,1), Edge(0,3)}, {Edge(1,2)}, {}, {}}, {}, LINE(""));
    }

    network.delete_node(0);
    //    0----1
    //
    //    2

    if constexpr (mutual_info(GraphFlavour))
    {
      check_graph(network, {{E_Edge(0,1,0)}, {E_Edge(0,1,0)}, {}}, {}, LINE(""));
    }
    else
    {
      check_graph(network, {{Edge(0,1)}, {}, {}}, {}, LINE(""));
    }
         
    network.delete_edge(network.cbegin_edges(0));
    //    0    1
    //
    //    2

    check_graph(network, {{}, {}, {}}, {}, LINE(""));
          
    network.join(0, 1);
    network.join(1, 1);
    network.join(1, 0);
    //    0======1
    //           /\
    //    2      \/

    if constexpr (mutual_info(GraphFlavour))
    {
      check_graph(network, {{E_Edge(0,1,0), E_Edge(1,0,3)}, {E_Edge(0,1,0), E_Edge(1,1,2), E_Edge(1,1,1), E_Edge(1,0,1)}, {}}, {}, LINE(""));
    }
    else
    {
      check_graph(network, {{Edge(0,1)}, {Edge(1,1), Edge(1,0)}, {}}, {}, LINE(""));
    }

    network.delete_edge(mutual_info(GraphFlavour) ? ++network.cbegin_edges(0) : network.cbegin_edges(0));
    //    0------1
    //           /\
    //    2      \/

    if constexpr (mutual_info(GraphFlavour))
    {
      if constexpr(GraphFlavour == graph_flavour::undirected)
      {
        check_graph(network, {{E_Edge(0,1,1)}, {E_Edge(1,1,1), E_Edge(1,1,0), E_Edge(0,1,0)}, {}}, {}, LINE(""));
      }
      else
      {
        check_graph(network, {{E_Edge(0,1,0)}, {E_Edge(0,1,0), E_Edge(1,1,2), E_Edge(1,1,1)}, {}}, {}, LINE(""));
      }
    }
    else
    {
      check_graph(network, {{}, {Edge(1,1), Edge(1,0)}, {}}, {}, LINE(""));
    }

    network.join(2,1);
    //    0------1------2
    //           /\
    //           \/

    if constexpr (mutual_info(GraphFlavour))
    {
      if constexpr(GraphFlavour == graph_flavour::undirected)
      {
        check_graph(network, {{E_Edge(0,1,0)}, {E_Edge(1,1,1), E_Edge(1,1,0), E_Edge(0,1,0), E_Edge(2,1,0)}, {E_Edge(2,1,3)}}, {}, LINE(""));
      }
      else
      {
        check_graph(network, {{E_Edge(0,1,0)}, {E_Edge(0,1,0), E_Edge(1,1,2), E_Edge(1,1,1), E_Edge(2,1,0)}, {E_Edge(2,1,3)}}, {}, LINE(""));
      }
    }
    else
    {
      check_graph(network, {{}, {Edge(1,1), Edge(1,0)}, {Edge(2,1)}}, {}, LINE(""));
    }
          
    network.delete_edge(mutual_info(GraphFlavour) ? ++network.cbegin_edges(1) : network.cbegin_edges(1));
    //    0------1------2
 
    if constexpr (mutual_info(GraphFlavour))
    {
      check_graph(network, {{E_Edge(0,1,0)}, {E_Edge(0,1,0), E_Edge(2,1,0)}, {E_Edge(2,1,1)}}, {}, LINE("Check deletion of tadpole"));
    }
    else
    {
      check_graph(network, {{}, {Edge(1,0)}, {Edge(2,1)}}, {}, LINE("Check deletion of tadpole"));
    }

    network.delete_edge(network.cbegin_edges(2));
    network.join(1,1);
    network.join(2,1);

    //    0------1------2
    //           /\
    //           \/

    if constexpr (mutual_info(GraphFlavour))
    {
      check_graph(network, {{E_Edge(0,1,0)}, {E_Edge(0,1,0), E_Edge(1,1,2), E_Edge(1,1,1), E_Edge(2,1,0)}, {E_Edge(2,1,3)}}, {}, LINE(""));
    }
    else
    {
      check_graph(network, {{}, {Edge(1,0), Edge(1,1)}, {Edge(2,1)}}, {}, LINE(""));
    }

    network.delete_edge(mutual_info(GraphFlavour) ? network.cbegin_edges(1)+2 : network.cbegin_edges(1)+1);
    //    0------1------2
 
    if constexpr (mutual_info(GraphFlavour))
    {
      check_graph(network, {{E_Edge(0,1,0)}, {E_Edge(0,1,0), E_Edge(2,1,0)}, {E_Edge(2,1,1)}}, {}, LINE("Check deletion of tadpole from top"));
    }
    else
    {
      check_graph(network, {{}, {Edge(1,0)}, {Edge(2,1)}}, {}, LINE("Check deletion of tadpole"));
    }
        
    network.delete_edge(network.cbegin_edges(1));
    network.delete_edge(network.cbegin_edges(2));
    network.join(1,1);
    //    0      1
    //           /\
    //    2      \/

    if constexpr (mutual_info(GraphFlavour))
    {
      check_graph(network, {{}, {E_Edge(1,1,1), E_Edge(1,1,0)}, {}}, {}, LINE(""));
    }
    else
    {
      check_graph(network, {{}, {Edge(1,1)}, {}}, {}, LINE(""));
    }
 
    network.delete_edge(network.cbegin_edges(1));
    //    0    1
    //
    //    2

    check_graph(network, {{}, {}, {}}, {});         
          
    network.join(0, 1);
    network.join(0, 1);
    network.join(0, 2);
    network.join(0, 2);
    network.join(1, 2);
    network.join(2, 1);
    //    0=====1
    //    ||  //
    //    ||//
    //     2

    if constexpr (mutual_info(GraphFlavour))
    {
      check_graph(network,
                  { {E_Edge(0,1,0), E_Edge(0,1,1), E_Edge(0,2,0), E_Edge(0,2,1)}
                    , {E_Edge(0,1,0), E_Edge(0,1,1), E_Edge(1,2,2), E_Edge(2,1,3)}
                    , {E_Edge(0,2,2), E_Edge(0,2,3), E_Edge(1,2,2), E_Edge(2,1,3)}
                  }, {}, LINE(""));
    }
    else
    {
      check_graph(network, {{Edge(0,1), Edge(0,1), Edge(0,2), Edge(0,2)}, {Edge(1,2)}, {Edge(2,1)}}, {}, LINE(""));
    }

    mutual_info(GraphFlavour) ? network.delete_edge(network.cbegin_edges(2)+2) : network.delete_edge(network.cbegin_edges(2));

    //    0=====1
    //    ||  /
    //    ||/
    //     2

    if constexpr (mutual_info(GraphFlavour))
    {
      check_graph(network,
                  { {E_Edge(0,1,0), E_Edge(0,1,1), E_Edge(0,2,0), E_Edge(0,2,1)}
                    , {E_Edge(0,1,0), E_Edge(0,1,1), E_Edge(2,1,2)}
                    , {E_Edge(0,2,2), E_Edge(0,2,3), E_Edge(2,1,2)}
                  }, {}, LINE(""));
    }
    else
    {
      check_graph(network, {{Edge(0,1), Edge(0,1), Edge(0,2), Edge(0,2)}, {Edge(1,2)}, {}}, {}, LINE(""));
    }

    network.delete_node(0);
    //  0----1

    if constexpr (mutual_info(GraphFlavour))
    {
      check_graph(network, {{E_Edge(1,0,0)}, {E_Edge(1,0,0)}}, {}, LINE(""));
    }
    else
    {
      check_graph(network, {{Edge(0,1)}, {}}, {}, LINE(""));
    }

    network.join(0, 0);
    // /\
    // \/
    //  0---1

    if constexpr (mutual_info(GraphFlavour))
    {
      check_graph(network, {{E_Edge(1,0,0), E_Edge(0,0,2), E_Edge(0,0,1)}, {E_Edge(1,0,0)}}, {}, LINE(""));
    }
    else
    {
      check_graph(network, {{Edge(0,1), Edge(0,0)}, {}}, {}, LINE(""));
    }
          
    network.delete_edge(++network.cbegin_edges(0));
    //  0----1

    if constexpr (mutual_info(GraphFlavour))
    {
      check_graph(network, {{E_Edge(1,0,0)}, {E_Edge(1,0,0)}}, {}, LINE(""));
    }
    else
    {
      check_graph(network, {{Edge(0,1)}, {}}, {}, LINE(""));
    }

    network.delete_node(0);
    // 0

    check_graph(network, {{}}, {}, LINE(""));

    network.delete_node(0);
    //

    check_graph(network, {}, {});

    check_equality<std::size_t>(0, network.add_node(), LINE("Node added back to graph"));
    // 0

    network.join(0, 0);
    // /\
    // \/
    // 0

    if constexpr (mutual_info(GraphFlavour))
    {
      check_graph(network, {{E_Edge(0,0,1), E_Edge(0,0,0)}}, {}, LINE(""));
    }
    else
    {
      check_graph(network, {{Edge(0,0)}}, {}, LINE(""));
    }

    check_equality<std::size_t>(0, network.insert_node(0), LINE("Prepend a node to the exising graph"));
    //    /\
    //    \/
    // 0  0

    if constexpr (mutual_info(GraphFlavour))
    {
      check_graph(network, {{}, {E_Edge(1,1,1), E_Edge(1,1,0)}}, {}, LINE(""));
    }
    else
    {
      check_graph(network, {{}, {Edge(1,1)}}, {}, LINE(""));
    }

    network.join(0,1);
    network.insert_node(1);
    //       /\
    //       \/
    // 0  0  0
    // \    /
    //  \  /
    //   \/

    if constexpr (mutual_info(GraphFlavour))
    {
      check_graph(network, {{E_Edge(0,2,2)}, {}, {E_Edge(2,2,1), E_Edge(2,2,0), E_Edge(0,2,0)}}, {}, LINE(""));
    }
    else
    {
      check_graph(network, {{Edge(0,2)}, {}, {Edge(2,2)}}, {}, LINE(""));
    }

    network.delete_node(1);
    network.delete_node(0);
    // /\
    // \/
    // 0

    if constexpr (mutual_info(GraphFlavour))
    {
      check_graph(network, {{E_Edge(0,0,1), E_Edge(0,0,0)}}, {}, LINE(""));
    }
    else
    {
      check_graph(network, {{Edge(0,0)}}, {}, LINE(""));
    }
        
    GGraph network2;
      
    check_equality<std::size_t>(0, network2.add_node(), LINE("Node added to second network"));
    check_equality<std::size_t>(1, network2.add_node(), LINE("Second node added to second network"));
    check_equality<std::size_t>(2, network2.add_node(), LINE("Third node added to second network"));
    check_equality<std::size_t>(3, network2.add_node(), LINE("Fourth node added to second network"));

    network2.join(0, 1);
    network2.join(2, 3);
    // 0----0    0----0

    if constexpr (mutual_info(GraphFlavour))
    {
      check_graph(network2, {{E_Edge(0,1,0)}, {E_Edge(0,1,0)}, {E_Edge(2,3,0)}, {E_Edge(2,3,0)}}, {}, LINE("Check di-component graph"));
    }
    else
    {
      check_graph(network2, {{Edge(0,1)}, {}, {Edge(2,3)}, {}}, {}, LINE("Check di-component graph"));
    }

    std::swap(network, network2);

    if constexpr (mutual_info(GraphFlavour))
    {
      check_graph(network2, {{E_Edge(0,0,1), E_Edge(0,0,0)}}, {}, LINE(""));
      check_graph(network, {{E_Edge(0,1,0)}, {E_Edge(0,1,0)}, {E_Edge(2,3,0)}, {E_Edge(2,3,0)}}, {}, LINE("Check swapped di-component graph"));
    }
    else
    {
      check_graph(network2, {{Edge(0,0)}}, {}, LINE(""));
      check_graph(network, {{Edge(0,1)}, {}, {Edge(2,3)}, {}}, {}, LINE("Check swapped di-component graph"));
    }
  }

  // Capacity

  template
  <
    maths::graph_flavour GraphFlavour,
    class EdgeWeight,
    class NodeWeight,      
    template <class> class EdgeWeightPooling,
    template <class> class NodeWeightPooling,
    template <class, template<class> class> class EdgeStorageTraits,
    template <class, template<class> class, bool> class NodeWeightStorageTraits
  >
  void graph_contiguous_capacity<
      GraphFlavour,
      EdgeWeight,
      NodeWeight,
      EdgeWeightPooling,
      NodeWeightPooling,
      EdgeStorageTraits,
      NodeWeightStorageTraits
  >::execute_operations()
  {
    graph_t g{};

    check_equality<std::size_t>(0, g.edges_capacity(), LINE(""));
    check_equality<std::size_t>(0, g.node_capacity());

    g.reserve_edges(4);
    check_equality<std::size_t>(4, g.edges_capacity(), LINE(""));

    g.reserve_nodes(4);
    check_equality<std::size_t>(4, g.node_capacity());

    g.shrink_to_fit();
    check_equality<std::size_t>(0, g.edges_capacity(), LINE("May fail if stl implementation doesn't actually shrink to fit!"));
    check_equality<std::size_t>(0, g.node_capacity(), LINE("May fail if stl implementation doesn't actually shrink to fit!"));    
  }

   template
  <
    maths::graph_flavour GraphFlavour,
    class EdgeWeight,
    class NodeWeight,      
    template <class> class EdgeWeightPooling,
    template <class> class NodeWeightPooling,
    template <class, template<class> class> class EdgeStorageTraits,
    template <class, template<class> class, bool> class NodeWeightStorageTraits
  >
  void graph_bucketed_capacity<
      GraphFlavour,
      EdgeWeight,
      NodeWeight,
      EdgeWeightPooling,
      NodeWeightPooling,
      EdgeStorageTraits,
      NodeWeightStorageTraits
  >::execute_operations()
  {
    graph_t g{};

    check_exception_thrown<std::out_of_range>([&g](){ g.reserve_edges(0, 4);}, LINE(""));
    check_exception_thrown<std::out_of_range>([&g](){ g.edges_capacity(0);}, LINE(""));
    check_equality<std::size_t>(0, g.node_capacity());

    g.add_node();
    check_equality<std::size_t>(0, g.edges_capacity(0), LINE(""));
    check_equality<std::size_t>(1, g.node_capacity(), LINE(""));
    g.reserve_edges(0, 4);
    check_equality<std::size_t>(4, g.edges_capacity(0), LINE(""));

    g.reserve_nodes(4);
    check_equality<std::size_t>(4, g.node_capacity());

    g.shrink_to_fit();
    check_equality<std::size_t>(0, g.edges_capacity(0), LINE("May fail if stl implementation doesn't actually shrink to fit!"));
    check_equality<std::size_t>(1, g.node_capacity(), LINE("May fail if stl implementation doesn't actually shrink to fit!"));
  }

  // Generic Weighted
  
  template
  <
    maths::graph_flavour GraphFlavour,
    class EdgeWeight,
    class NodeWeight,      
    template <class> class EdgeWeightPooling,
    template <class> class NodeWeightPooling,
    template <class, template<class> class> class EdgeStorageTraits,
    template <class, template<class> class, bool> class NodeWeightStorageTraits
  >
  void generic_weighted_graph_tests<
      GraphFlavour,
      EdgeWeight,
      NodeWeight,
      EdgeWeightPooling,
      NodeWeightPooling,
      EdgeStorageTraits,
      NodeWeightStorageTraits
  >::test_basic_operations()
  {
    GGraph graph;
    using Edge = maths::edge<EdgeWeight, utilities::protective_wrapper<EdgeWeight>>;
    using E_Edge = maths::embedded_edge<EdgeWeight, data_sharing::independent, utilities::protective_wrapper<EdgeWeight>>;

    graph.reserve_nodes(4);
    check_equality<std::size_t>(4, graph.node_capacity(), LINE(""));
    graph.shrink_to_fit();
    check_equality<std::size_t>(0, graph.node_capacity(), LINE("May fail if stl implementation doesn't actually shrink to fit!"));

    // NULL
    check_exception_thrown<std::out_of_range>([&graph](){ graph.delete_node(0); }, LINE("No nodes to delete"));
    check_exception_thrown<std::out_of_range>([&graph](){ graph.delete_edge(graph.cbegin_edges(0)); }, LINE("No edges to delete"));
    check_exception_thrown<std::out_of_range>([&graph](){ graph.node_weight(graph.cend_node_weights(), 1.0); }, LINE("No nodes to set weight"));
    check_exception_thrown<std::out_of_range>([&graph](){ graph.set_edge_weight(graph.cbegin_edges(0), 1); }, LINE("No edges to set weight"));
    auto getEdgeFn = [&graph]() { get_edge(graph, 0, 0, 0); };
    check_exception_thrown<std::out_of_range>(getEdgeFn, "No edges so can't acquire a reference to an edge");

    check_graph(graph, {}, {}, LINE(""));
        
    check_equality<std::size_t>(0, graph.add_node(0.0, 1.0), "Node added with weight i");
    //   (i)

    check_graph(graph, {{}}, {{0,1}}, LINE(""));

    check_exception_thrown<std::out_of_range>([&graph](){ graph.node_weight(graph.cend_node_weights(), 0.0); }, LINE("Throw if node out of range"));
    check_exception_thrown<std::out_of_range>(getEdgeFn, LINE("Throw if edge out of range"));

    graph.delete_node(0);
    //    NULL

    check_graph(graph, {}, {}, LINE(""));

    check_equality<std::size_t>(0, graph.add_node(0.0, 1.0), "Node of weight i recreated");
    //    (i)

    check_graph(graph, {{}}, {{0,1}}, LINE(""));
        
    graph.node_weight(graph.cbegin_node_weights(), 1.1, -4.3);
    // (1.1-i4.3)

    check_graph(graph, {{}}, {{1.1,-4.3}}, LINE(""));

    graph.join(0, 0, 1);
    //   /\ 1
    //   \/
    //  (1.1-i4.3)

    if constexpr (mutual_info(GraphFlavour))
    {
      check_graph(graph, {{E_Edge(0,0,1,1), E_Edge(0,0,0,1)}}, {{1.1,-4.3}}, LINE(""));
    }
    else
    {
      check_graph(graph, {{Edge(0,0,1)}}, {{1.1,-4.3}}, LINE(""));
    }

    graph.set_edge_weight(graph.cbegin_edges(0), -2);
    //   /\ -2
    //   \/
    // (1.1-i4.3)

    if constexpr (mutual_info(GraphFlavour))
    {
      check_graph(graph, {{E_Edge(0,0,1,-2), E_Edge(0,0,0,-2)}}, {{1.1,-4.3}}, LINE(""));
    }
    else
    {
      check_graph(graph, {{Edge(0,0,-2)}}, {{1.1,-4.3}}, LINE(""));
    }

    auto begin0 = graph.cbegin_edges(0);
    graph.set_edge_weight(begin0, -4);
    //   /\ -4
    //   \/
    // (1.1-i4.3)

    if constexpr(mutual_info(GraphFlavour))
    {
      check_graph(graph, {{Edge(0,0,-4), Edge(0,0,-4)}}, {{1.1,-4.3}}, LINE(""));
    }
    else
    {
      check_graph(graph, {{Edge(0,0,-4)}}, {{1.1,-4.3}}, LINE(""));
    }        

    graph.delete_edge(graph.cbegin_edges(0));
    //  (1.1-i4.3)

    check_graph(graph, {{}}, {{1.1,-4.3}});

    graph.join(0, 0, 1);
    //   /\ 1
    //   \/
    // (1.1-i4.3)

    if constexpr(mutual_info(GraphFlavour))
    {
      check_graph(graph, {{Edge(0,0,1), Edge(0,0,1)}}, {{1.1,-4.3}}, LINE(""));    
    }
    else
    {
      check_graph(graph, {{Edge(0,0,1)}}, {{1.1,-4.3}}, LINE(""));
    }
        
    graph.join(0, 0, -1);
    //   /\ 1
    //   \/
    // (1.1-i4.3)
    //   /\
    //   \/ -1

    if constexpr(mutual_info(GraphFlavour))
    {
      check_graph(graph, {{Edge(0,0,1), Edge(0,0,1), Edge(0,0,-1), Edge(0,0,-1)}}, {{1.1,-4.3}}, LINE(""));
    }
    else
    {
      check_graph(graph, {{Edge(0,0,1), Edge(0,0,-1)}}, {{1.1,-4.3}}, LINE(""));
    }
        
    check_equality<EdgeWeight>(1, get_edge(graph, 0, 0, 0).weight());
    mutual_info(GraphFlavour) ? check_equality<EdgeWeight>(1, get_edge(graph, 0, 0, 1).weight()) : check_equality<EdgeWeight>(-1, get_edge(graph, 0, 0, 1).weight());

    mutual_info(GraphFlavour) ? graph.set_edge_weight(graph.cbegin_edges(0) + 2, -4) : graph.set_edge_weight(++graph.cbegin_edges(0), -4);
    //   /\ 1
    //   \/
    // (1.1-i4.3)
    //   /\
    //   \/ -4
         
    if constexpr(mutual_info(GraphFlavour))
    {
      check_graph(graph, {{Edge(0,0,1), Edge(0,0,1), Edge(0,0,-4), Edge(0,0,-4)}}, {{1.1,-4.3}}, LINE(""));
    }
    else
    {
      check_graph(graph, {{Edge(0,0,1), Edge(0,0,-4)}}, {{1.1,-4.3}}, LINE(""));
    }

    check_equality<std::size_t>(1, graph.add_node());
    //   /\ 1
    //   \/
    // (1.1-i4.3)   (0)
    //   /\
    //   \/ -4
        
    if constexpr(mutual_info(GraphFlavour))
    {
      check_graph(graph, {{Edge(0,0,1), Edge(0,0,1), Edge(0,0,-4), Edge(0,0,-4)}, {}}, {{1.1,-4.3}, {0,0}}, LINE(""));
    }
    else
    {
      check_graph(graph, {{Edge(0,0,1), Edge(0,0,-4)}, {}}, {{1.1,-4.3}, {0,0}}, LINE(""));
    }

    graph.join(0, 1, 6);
    //   /\1
    //   \/  6
    // (1.1-i4.3)------(0)
    //   /\
    //   \/-4
        
    if constexpr(mutual_info(GraphFlavour))
    {
      check_graph(graph, {{Edge(0,0,1), Edge(0,0,1), Edge(0,0,-4), Edge(0,0,-4), Edge(0,1,6)}, {Edge(0,1,6)}}, {{1.1,-4.3}, {0,0}}, LINE(""));
    }
    else
    {
      check_graph(graph, {{Edge(0,0,1), Edge(0,0,-4), Edge(0,1,6)}, {}}, {{1.1,-4.3}, {0,0}}, LINE(""));
    }

    graph.set_edge_weight(--graph.cend_edges(0), 7);
    //   /\1
    //   \/  7
    // (1.1-i4.3)------(0)
    //   /\
    //   \/-4
        
    if constexpr(mutual_info(GraphFlavour))
    {
      check_graph(graph, {{Edge(0,0,1), Edge(0,0,1), Edge(0,0,-4), Edge(0,0,-4), Edge(0,1,7)}, {Edge(0,1,7)}}, {{1.1,-4.3}, {0,0}}, LINE(""));
    }
    else
    {
      check_graph(graph, {{Edge(0,0,1), Edge(0,0,-4), Edge(0,1,7)}, {}}, {{1.1,-4.3}, {0,0}}, LINE(""));
    }
       
    graph.set_edge_weight(--graph.cend_edges(0), 6);
    //   /\1
    //   \/  6
    // (1.1-i4.3)------(0)
    //   /\
    //   \/-4

    if constexpr(mutual_info(GraphFlavour))
    {
      check_graph(graph, {{Edge(0,0,1), Edge(0,0,1), Edge(0,0,-4), Edge(0,0,-4), Edge(0,1,6)}, {Edge(0,1,6)}}, {{1.1,-4.3}, {0,0}}, LINE(""));
    }
    else
    {
      check_graph(graph, {{Edge(0,0,1), Edge(0,0,-4), Edge(0,1,6)}, {}}, {{1.1,-4.3}, {0,0}}, LINE(""));
    }
        
    graph.join(1, 0, 7);
    //     /\1
    //     \/     6
    // (1.1-i4.3)======(0)
    //     /\     7
    //     \/-4
       
    if constexpr(mutual_info(GraphFlavour))
    {
      check_graph(graph, {{Edge(0,0,1), Edge(0,0,1), Edge(0,0,-4), Edge(0,0,-4), Edge(0,1,6), Edge(1,0,7)}, {Edge(0,1,6), Edge(1,0,7)}}, {{1.1,-4.3}, {0,0}}, LINE(""));
    }
    else
    {
      check_graph(graph, {{Edge(0,0,1), Edge(0,0,-4), Edge(0,1,6)}, {Edge(1,0,7)}}, {{1.1,-4.3}, {0,0}}, LINE(""));
    }
        

    graph.join(0, 1, 8);
    //     /\1   /--\8
    //     \/   /6   \
    // (1.1-i4.3)====(0)
    //     /\     7
    //     \/-4

    if constexpr(mutual_info(GraphFlavour))
    {
      check_graph(graph, {{Edge(0,0,1), Edge(0,0,1), Edge(0,0,-4), Edge(0,0,-4), Edge(0,1,6), Edge(1,0,7), Edge(0,1,8)}, {Edge(0,1,6), Edge(1,0,7), Edge(0,1,8)}}, {{1.1,-4.3}, {0,0}}, LINE(""));
    }
    else
    {
      check_graph(graph, {{Edge(0,0,1), Edge(0,0,-4), Edge(0,1,6), Edge(0,1,8)}, {Edge(1,0,7)}}, {{1.1,-4.3}, {0,0}}, LINE(""));
    }
        
    graph.delete_edge(graph.cbegin_edges(0));
    //            8
    //           /--\
    //          / 6  \
    // (1.1-i4.3)====(0)
    //     /\     7
    //     \/-4
        
    if constexpr(mutual_info(GraphFlavour))
    {
      check_graph(graph, {{Edge(0,0,-4), Edge(0,0,-4), Edge(0,1,6), Edge(1,0,7), Edge(0,1,8)}, {Edge(0,1,6), Edge(1,0,7), Edge(0,1,8)}}, {{1.1,-4.3}, {0,0}}, LINE(""));
    }
    else
    {
      check_graph(graph, {{Edge(0,0,-4), Edge(0,1,6), Edge(0,1,8)}, {Edge(1,0,7)}}, {{1.1,-4.3}, {0,0}}, LINE(""));
    }
        
    graph.mutate_edge_weight(graph.cbegin_edges(0) + (mutual_info(GraphFlavour) ?  3 : 2), [](auto& val){ val = 6; });
    //            8
    //           /--\
    //          / 6  \
    // (1.1-i4.3)====(0)
    //     /\     6
    //     \/-4

    if constexpr(!mutual_info(GraphFlavour))
    {
      check_graph(graph, {{Edge(0,0,-4), Edge(0,1,6), Edge(0,1,6)}, {Edge(1,0,7)}}, {{1.1,-4.3}, {0,0}});
    }
    else
    {
      check_graph(graph, {{Edge(0,0,-4), Edge(0,0,-4), Edge(0,1,6), Edge(1,0,6), Edge(0,1,8)}, {Edge(0,1,6), Edge(1,0,6), Edge(0,1,8)}}, {{1.1,-4.3}, {0,0}});
    }

    graph.set_edge_weight(graph.cbegin_edges(0) + (mutual_info(GraphFlavour) ?  3 : 2), 10);
    //            8
    //           /--\
    //          / 10 \
    // (1.1-i4.3)====(0)
    //     /\     6
    //     \/-4

    if constexpr(GraphFlavour == maths::graph_flavour::directed)
    {
      check_graph(graph, {{Edge(0,0,-4), Edge(0,1,6), Edge(0,1,10)}, {Edge(1,0,7)}}, {{1.1,-4.3}, {0,0}});
    }
    else if constexpr(GraphFlavour == maths::graph_flavour::undirected)
    {
      check_graph(graph, {{Edge(0,0,-4), Edge(0,0,-4), Edge(0,1,6), Edge(1,0,10), Edge(0,1,8)}, {Edge(0,1,10), Edge(1,0,6), Edge(0,1,8)}}, {{1.1,-4.3}, {0,0}});
    }
    else
    {
      check_graph(graph, {{Edge(0,0,-4), Edge(0,0,-4), Edge(0,1,6), Edge(1,0,10), Edge(0,1,8)}, {Edge(0,1,6), Edge(1,0,10), Edge(0,1,8)}}, {{1.1,-4.3}, {0,0}});
    }

    graph.mutate_edge_weight(graph.cbegin_edges(0) + (mutual_info(GraphFlavour) ?  4 : 1), [](auto& val){ val = 10; });
    //            10
    //           /--\
    //          / 10 \
    // (1.1-i4.3)====(0)
    //     /\     6
    //     \/-4

    if constexpr(GraphFlavour == maths::graph_flavour::directed)
    {
      check_graph(graph, {{Edge(0,0,-4), Edge(0,1,10), Edge(0,1,10)}, {Edge(1,0,7)}}, {{1.1,-4.3}, {0,0}});
    }
    else if constexpr(GraphFlavour == maths::graph_flavour::undirected)
    {
      check_graph(graph, {{Edge(0,0,-4), Edge(0,0,-4), Edge(0,1,6), Edge(1,0,10), Edge(0,1,10)}, {Edge(0,1,10), Edge(1,0,6), Edge(0,1,10)}}, {{1.1,-4.3}, {0,0}});
    }
    else
    {
      check_graph(graph, {{Edge(0,0,-4), Edge(0,0,-4), Edge(0,1,6), Edge(1,0,10), Edge(0,1,10)}, {Edge(0,1,6), Edge(1,0,10), Edge(0,1,10)}}, {{1.1,-4.3}, {0,0}});
    }

    graph.mutate_edge_weight(graph.cbegin_edges(0) + (mutual_info(GraphFlavour) ?  4 : 2), [](auto& val){ val = 7; });
    //            7
    //           /--\
    //          / 10 \
    // (1.1-i4.3)====(0)
    //     /\     6
    //     \/-4

    if constexpr(GraphFlavour == maths::graph_flavour::directed)
    {
      check_graph(graph, {{Edge(0,0,-4), Edge(0,1,10), Edge(0,1,7)}, {Edge(1,0,7)}}, {{1.1,-4.3}, {0,0}});
    }
    else if constexpr(GraphFlavour == maths::graph_flavour::undirected)
    {
      check_graph(graph, {{Edge(0,0,-4), Edge(0,0,-4), Edge(0,1,6), Edge(1,0,10), Edge(0,1,7)}, {Edge(0,1,7), Edge(1,0,6), Edge(0,1,10)}}, {{1.1,-4.3}, {0,0}});
    }
    else
    {
      check_graph(graph, {{Edge(0,0,-4), Edge(0,0,-4), Edge(0,1,6), Edge(1,0,10), Edge(0,1,7)}, {Edge(0,1,6), Edge(1,0,10), Edge(0,1,7)}}, {{1.1,-4.3}, {0,0}});
    }
        
    graph.delete_node(0);
    //  (0)

    check_graph(graph, {{}}, {{0}});
        

    graph.join(0, 0, 1);
    //   /\ 1
    //   \/
    //   (0)
        
    if constexpr(mutual_info(GraphFlavour))
    {
      check_graph(graph, {{Edge(0, 0, 1), Edge(0, 0, 1)}}, {{0}}, LINE(""));
    }
    else
    {
      check_graph(graph, {{Edge(0, 0, 1)}}, {{0}}, LINE(""));
    }
 

    check_equality<std::size_t>(0, graph.insert_node(0, 1.0, 1.0), "Node with weight 1+i inserted into slot 0");
    //       /\ 1
    //       \/
    // (1+i) (0)

    if constexpr(mutual_info(GraphFlavour))
    {
      check_graph(graph, {{}, {Edge(1,1,1), Edge(1,1,1)}}, {{1,1}, {0}}, LINE(""));
    }
    else
    {
      check_graph(graph, {{}, {Edge(1,1,1)}}, {{1,1}, {0}}, LINE(""));
       
    }
  }

  
  // Copy, Move...
  
  template
  <
    maths::graph_flavour GraphFlavour,
    class EdgeWeight,
    class NodeWeight,      
    template <class> class EdgeWeightPooling,
    template <class> class NodeWeightPooling,
    template <class, template<class> class> class EdgeStorageTraits,
    template <class, template<class> class, bool> class NodeWeightStorageTraits
  >
  void test_copy_move<
      GraphFlavour,
      EdgeWeight,
      NodeWeight,
      EdgeWeightPooling,
      NodeWeightPooling,
      EdgeStorageTraits,
      NodeWeightStorageTraits
  >::execute_operations()
  {
    const std::vector<int> specialWeight{5, -6};

    GGraph graph1{generate_test_graph1(specialWeight)};
    check_test_graph_1(graph1, specialWeight, LINE("Checking rvalue constructor"));
 
    GGraph graph2;
    check_empty_graph(graph2);

    graph2 = graph1;

    check_test_graph_1(graph2, specialWeight, LINE("Checking assignment operator"));
        
    graph1 = GGraph();
    check_empty_graph(graph1, LINE("Checking assignment of empty graph"));

    std::swap(graph1, graph2);

    check_test_graph_1(graph1, specialWeight, LINE("Checking results of swap"));
    check_empty_graph(graph2, LINE("Checking results of swap giving empty graph"));
        
    GGraph graph3{graph1};
    check_test_graph_1(graph3, specialWeight, LINE("Checking lvalue constructor"));
    check_test_graph_1(graph1, specialWeight, LINE("Checking no unexpected side effects of lvalue constructor"));
        
    const std::vector<int> newWeight{-7, 9};
    graph1.set_edge_weight(mutual_info(GraphFlavour) ? graph1.cbegin_edges(0) + 2: ++graph1.cbegin_edges(0), newWeight);
        
    check_test_graph_1(graph1, newWeight, LINE("Checking result of setting weight"));
    check_test_graph_1(graph3, specialWeight, LINE("Checking no unexpected side effects of result of setting weight"));
        
    GGraph graph4{graph1};
    graph4.set_edge_weight(mutual_info(GraphFlavour) ? graph4.cbegin_edges(0) + 2 : ++graph4.cbegin_edges(0), 8, 4);
    check_test_graph_1(graph4, {8,4});
  }
}
