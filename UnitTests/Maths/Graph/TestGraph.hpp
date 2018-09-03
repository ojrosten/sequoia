#pragma once

#include "TestGraphHelper.hpp"

#include <complex>

namespace sequoia
{
  namespace unit_testing
  {
    class test_graph : public graph_unit_test
    {
    public:
      using graph_unit_test::graph_unit_test;

    private:
      void run_tests() override;
    };


    template
    <
      maths::graph_flavour GraphFlavour,
      class NodeWeight,
      class EdgeWeight,
      bool ThrowOnError,
      template <class> class NodeWeightStorage,
      template <class> class EdgeWeightStorage,
      template <class, class, bool, template<class...> class> class EdgeStoragePolicy
    >
    class generic_graph_operations
      : public graph_operations<GraphFlavour, NodeWeight, EdgeWeight, ThrowOnError, NodeWeightStorage, EdgeWeightStorage, EdgeStoragePolicy>
    {
    public:
      
    private:
      using base_t = graph_operations<GraphFlavour, NodeWeight, EdgeWeight, ThrowOnError, NodeWeightStorage, EdgeWeightStorage, EdgeStoragePolicy>;
      
      using GGraph = typename base_t::graph_type;

      using graph_checker<unit_test_logger<test_mode::standard>>::check_equality;      
      using graph_checker<unit_test_logger<test_mode::standard>>::check_exception_thrown;
      using graph_checker<unit_test_logger<test_mode::standard>>::check_graph;

      void execute_operations() override
      {
        using namespace maths;
        
        GGraph network;
        using Edge = edge<EdgeWeight, utilities::protective_wrapper<EdgeWeight>>;
        using Edges = std::vector<std::vector<Edge>>;

        using NP_Edge = embedded_edge<EdgeWeight, data_sharing::independent, utilities::protective_wrapper<EdgeWeight>>;
        using NP_Edges = std::vector<std::vector<NP_Edge>>;

        using NodeWeights = std::vector<typename GGraph::node_weight_type>;
        {
          check_exception_thrown<std::out_of_range>([&network]() { network.cbegin_edges(0); }, LINE("cbegin_edges throws for empty graph"));
          check_exception_thrown<std::out_of_range>([&network]() { network.cend_edges(0); }, LINE("cend_edges throws for empty graph"));
          
          check_equality<std::size_t>(0,network.add_node(), LINE("Index of added node is 0"));
          //    0

          check_graph(network, NP_Edges{{}}, NodeWeights{}, LINE("Graph is empty"));

          check_exception_thrown<std::out_of_range>([&network](){ get_edge(network, 0, 0, 0); },  LINE("For network with no edges, trying to obtain a reference to one throws an exception"));

          check_exception_thrown<std::out_of_range>([&network]() { network.join(0, 1); }, LINE("Unable to join zeroth node to non-existant first node"));
          check_equality<std::size_t>(1, network.add_node(), LINE("Index of added node is 1"));
          //    0    1

          check_graph(network, NP_Edges{{}, {}}, NodeWeights{}, LINE("Graph is empty"));

          network.join(0, 1);
          //    0----1

          if constexpr (mutual_info(GraphFlavour))
          {
            check_graph(network, NP_Edges{{NP_Edge{0,1,0}}, {NP_Edge{0,1,0}}}, NodeWeights{}, LINE(""));
          }
          else
          {
            check_graph(network, Edges{{Edge{0,1}}, {}}, NodeWeights{}, LINE(""));
          }

          std::size_t nodeNum{network.add_node()};
          check_equality<std::size_t>(2, nodeNum, LINE("Index of added node is 2"));
          network.join(1, nodeNum);
          //    0----1----2

          if constexpr (mutual_info(GraphFlavour))
          {
            check_graph(network, NP_Edges{{NP_Edge{0,1,0}}, {NP_Edge{0,1,0}, NP_Edge{1,2,0}}, {NP_Edge{1,2,1}}}, NodeWeights{}, LINE(""));
          }
          else
          {
            check_graph(network, Edges{{Edge{0,1}}, {Edge{1,2}}, {}}, NodeWeights{}, LINE(""));
          }

          nodeNum = network.add_node();
          check_equality<std::size_t>(3, nodeNum, LINE("Index of added node is 3"));
          network.join(0, nodeNum);
          //    0----1----2
          //    |
          //    3

          if constexpr (mutual_info(GraphFlavour))
          {
            check_graph(network, NP_Edges{{NP_Edge(0,1,0), NP_Edge(0,3,0)}, {NP_Edge(0,1,0), NP_Edge(1,2,0)}, {NP_Edge(1,2,1)}, {NP_Edge(0,3,1)}}, NodeWeights{}, LINE(""));
          }
          else
          {
            check_graph(network, Edges{{Edge(0,1), Edge(0,3)}, {Edge(1,2)}, {}, {}}, NodeWeights{}, LINE(""));
          }

          network.delete_node(0);
          //    0----1
          //
          //    2

          if constexpr (mutual_info(GraphFlavour))
          {
            check_graph(network, NP_Edges{{NP_Edge(0,1,0)}, {NP_Edge(0,1,0)}, {}}, NodeWeights{}, LINE(""));
          }
          else
          {
            check_graph(network, Edges{{Edge(0,1)}, {}, {}}, NodeWeights{}, LINE(""));
          }
         
          network.delete_edge(network.cbegin_edges(0));
          //    0    1
          //
          //    2

          check_graph(network, NP_Edges{{}, {}, {}}, NodeWeights{}, LINE(""));
          
          network.join(0, 1);
          network.join(1, 1);
          network.join(1, 0);
          //    0======1
          //           /\
          //    2      \/

          if constexpr (mutual_info(GraphFlavour))
          {
            check_graph(network, NP_Edges{{NP_Edge(0,1,0), NP_Edge(1,0,3)}, {NP_Edge(0,1,0), NP_Edge(1,1,2), NP_Edge(1,1,1), NP_Edge(1,0,1)}, {}}, NodeWeights{}, LINE(""));
          }
          else
          {
            check_graph(network, Edges{{Edge(0,1)}, {Edge(1,1), Edge(1,0)}, {}}, NodeWeights{}, LINE(""));
          }

          network.delete_edge(mutual_info(GraphFlavour) ? ++network.cbegin_edges(0) : network.cbegin_edges(0));
          //    0------1
          //           /\
          //    2      \/

          if constexpr (mutual_info(GraphFlavour))
          {
            if constexpr(GraphFlavour == graph_flavour::undirected)
            {
              check_graph(network, NP_Edges{{NP_Edge(0,1,1)}, {NP_Edge(1,1,1), NP_Edge(1,1,0), NP_Edge(0,1,0)}, {}}, NodeWeights{}, LINE(""));
            }
            else
            {
              check_graph(network, NP_Edges{{NP_Edge(0,1,0)}, {NP_Edge(0,1,0), NP_Edge(1,1,2), NP_Edge(1,1,1)}, {}}, NodeWeights{}, LINE(""));
            }
          }
          else
          {
            check_graph(network, Edges{{}, {Edge(1,1), Edge(1,0)}, {}}, NodeWeights{}, LINE(""));
          }

          network.join(2,1);
          //    0------1------2
          //           /\
          //           \/

          if constexpr (mutual_info(GraphFlavour))
          {
            if constexpr(GraphFlavour == graph_flavour::undirected)
            {
              check_graph(network, NP_Edges{{NP_Edge(0,1,0)}, {NP_Edge(1,1,1), NP_Edge(1,1,0), NP_Edge(0,1,0), NP_Edge(2,1,0)}, {NP_Edge(2,1,3)}}, NodeWeights{}, LINE(""));
            }
            else
            {
              check_graph(network, NP_Edges{{NP_Edge(0,1,0)}, {NP_Edge(0,1,0), NP_Edge(1,1,2), NP_Edge(1,1,1), NP_Edge(2,1,0)}, {NP_Edge(2,1,3)}}, NodeWeights{}, LINE(""));
            }
          }
          else
          {
            check_graph(network, Edges{{}, {Edge(1,1), Edge(1,0)}, {Edge(2,1)}}, NodeWeights{}, LINE(""));
          }
          
          network.delete_edge(mutual_info(GraphFlavour) ? ++network.cbegin_edges(1) : network.cbegin_edges(1));
          //    0------1------2
 
          if constexpr (mutual_info(GraphFlavour))
          {
            check_graph(network, NP_Edges{{NP_Edge(0,1,0)}, {NP_Edge(0,1,0), NP_Edge(2,1,0)}, {NP_Edge(2,1,1)}}, NodeWeights{}, LINE("Check deletion of tadpole"));
          }
          else
          {
            check_graph(network, Edges{{}, {Edge(1,0)}, {Edge(2,1)}}, NodeWeights{}, LINE("Check deletion of tadpole"));
          }

          network.delete_edge(network.cbegin_edges(2));
          network.join(1,1);
          network.join(2,1);

          //    0------1------2
          //           /\
          //           \/

          if constexpr (mutual_info(GraphFlavour))
          {
            check_graph(network, NP_Edges{{NP_Edge(0,1,0)}, {NP_Edge(0,1,0), NP_Edge(1,1,2), NP_Edge(1,1,1), NP_Edge(2,1,0)}, {NP_Edge(2,1,3)}}, NodeWeights{}, LINE(""));
          }
          else
          {
            check_graph(network, Edges{{}, {Edge(1,0), Edge(1,1)}, {Edge(2,1)}}, NodeWeights{}, LINE(""));
          }

          network.delete_edge(mutual_info(GraphFlavour) ? network.cbegin_edges(1)+2 : network.cbegin_edges(1)+1);
          //    0------1------2
 
          if constexpr (mutual_info(GraphFlavour))
          {
            check_graph(network, NP_Edges{{NP_Edge(0,1,0)}, {NP_Edge(0,1,0), NP_Edge(2,1,0)}, {NP_Edge(2,1,1)}}, NodeWeights{}, LINE("Check deletion of tadpole from top"));
          }
          else
          {
            check_graph(network, Edges{{}, {Edge(1,0)}, {Edge(2,1)}}, NodeWeights{}, LINE("Check deletion of tadpole"));
          }
        
          network.delete_edge(network.cbegin_edges(1));
          network.delete_edge(network.cbegin_edges(2));
          network.join(1,1);
          //    0      1
          //           /\
          //    2      \/

          if constexpr (mutual_info(GraphFlavour))
          {
            check_graph(network, NP_Edges{{}, {NP_Edge(1,1,1), NP_Edge(1,1,0)}, {}}, NodeWeights{}, LINE(""));
          }
          else
          {
            check_graph(network, Edges{{}, {Edge(1,1)}, {}}, NodeWeights{}, LINE(""));
          }
 
          network.delete_edge(network.cbegin_edges(1));
          //    0    1
          //
          //    2

          check_graph(network, mutual_info(GraphFlavour) ? NP_Edges{{}, {}, {}} : NP_Edges{{}, {}, {}}, NodeWeights{});

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
                        NP_Edges{ {NP_Edge(0,1,0), NP_Edge(0,1,1), NP_Edge(0,2,0), NP_Edge(0,2,1)}
                                , {NP_Edge(0,1,0), NP_Edge(0,1,1), NP_Edge(1,2,2), NP_Edge(2,1,3)}
                                , {NP_Edge(0,2,2), NP_Edge(0,2,3), NP_Edge(1,2,2), NP_Edge(2,1,3)}
                        }, NodeWeights{}, LINE(""));
          }
          else
          {
            check_graph(network, Edges{{Edge(0,1), Edge(0,1), Edge(0,2), Edge(0,2)}, {Edge(1,2)}, {Edge(2,1)}}, NodeWeights{}, LINE(""));
          }

          mutual_info(GraphFlavour) ? network.delete_edge(network.cbegin_edges(2)+2) : network.delete_edge(network.cbegin_edges(2));

          //    0=====1
          //    ||  /
          //    ||/
          //     2

          if constexpr (mutual_info(GraphFlavour))
          {
            check_graph(network,
                        NP_Edges{ {NP_Edge(0,1,0), NP_Edge(0,1,1), NP_Edge(0,2,0), NP_Edge(0,2,1)}
                                , {NP_Edge(0,1,0), NP_Edge(0,1,1), NP_Edge(2,1,2)}
                                , {NP_Edge(0,2,2), NP_Edge(0,2,3), NP_Edge(2,1,2)}
                        }, NodeWeights{}, LINE(""));
          }
          else
          {
            check_graph(network, Edges{{Edge(0,1), Edge(0,1), Edge(0,2), Edge(0,2)}, {Edge(1,2)}, {}}, NodeWeights{}, LINE(""));
          }

          network.delete_node(0);
          //  0----1

          if constexpr (mutual_info(GraphFlavour))
          {
            check_graph(network, NP_Edges{{NP_Edge(1,0,0)}, {NP_Edge(1,0,0)}}, NodeWeights{}, LINE(""));
          }
          else
          {
            check_graph(network, Edges{{Edge(0,1)}, {}}, NodeWeights{}, LINE(""));
          }

          network.join(0, 0);
          // /\
          // \/
          //  0---1

          if constexpr (mutual_info(GraphFlavour))
          {
            check_graph(network, NP_Edges{{NP_Edge(1,0,0), NP_Edge(0,0,2), NP_Edge(0,0,1)}, {NP_Edge(1,0,0)}}, NodeWeights{}, LINE(""));
          }
          else
          {
            check_graph(network, Edges{{Edge(0,1), Edge(0,0)}, {}}, NodeWeights{}, LINE(""));
          }
          
          network.delete_edge(++network.cbegin_edges(0));
          //  0----1

          if constexpr (mutual_info(GraphFlavour))
          {
            check_graph(network, NP_Edges{{NP_Edge(1,0,0)}, {NP_Edge(1,0,0)}}, NodeWeights{}, LINE(""));
          }
          else
          {
            check_graph(network, Edges{{Edge(0,1)}, {}}, NodeWeights{}, LINE(""));
          }

          network.delete_node(0);
          // 0

          check_graph(network, Edges{{}}, NodeWeights{}, LINE(""));

          network.delete_node(0);
          //

          check_graph(network, Edges{}, NodeWeights{});

          check_equality<std::size_t>(0, network.add_node(), LINE("Node added back to graph"));
          // 0

          network.join(0, 0);
          // /\
          // \/
          // 0

          if constexpr (mutual_info(GraphFlavour))
          {
            check_graph(network, NP_Edges{{NP_Edge(0,0,1), NP_Edge(0,0,0)}}, NodeWeights{}, LINE(""));
          }
          else
          {
            check_graph(network, Edges{{Edge(0,0)}}, NodeWeights{}, LINE(""));
          }

          check_equality<std::size_t>(0, network.insert_node(0), LINE("Prepend a node to the exising graph"));
          //    /\
          //    \/
          // 0  0

          if constexpr (mutual_info(GraphFlavour))
          {
            check_graph(network, NP_Edges{{}, {NP_Edge(1,1,1), NP_Edge(1,1,0)}}, NodeWeights{}, LINE(""));
          }
          else
          {
            check_graph(network, Edges{{}, {Edge(1,1)}}, NodeWeights{}, LINE(""));
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
            check_graph(network, NP_Edges{{NP_Edge(0,2,2)}, {}, {NP_Edge(2,2,1), NP_Edge(2,2,0), NP_Edge(0,2,0)}}, NodeWeights{}, LINE(""));
          }
          else
          {
            check_graph(network, Edges{{Edge(0,2)}, {}, {Edge(2,2)}}, NodeWeights{}, LINE(""));
          }

          network.delete_node(1);
          network.delete_node(0);
          // /\
          // \/
          // 0

          if constexpr (mutual_info(GraphFlavour))
          {
            check_graph(network, NP_Edges{{NP_Edge(0,0,1), NP_Edge(0,0,0)}}, NodeWeights{}, LINE(""));
          }
          else
          {
            check_graph(network, Edges{{Edge(0,0)}}, NodeWeights{}, LINE(""));
          }
        }
        
        GGraph network2;
        {
          check_equality<std::size_t>(0, network2.add_node(), LINE("Node added to second network"));
          check_equality<std::size_t>(1, network2.add_node(), LINE("Second node added to second network"));
          check_equality<std::size_t>(2, network2.add_node(), LINE("Third node added to second network"));
          check_equality<std::size_t>(3, network2.add_node(), LINE("Fourth node added to second network"));

          network2.join(0, 1);
          network2.join(2, 3);
          // 0----0    0----0

          if constexpr (mutual_info(GraphFlavour))
          {
            check_graph(network2, NP_Edges{{NP_Edge(0,1,0)}, {NP_Edge(0,1,0)}, {NP_Edge(2,3,0)}, {NP_Edge(2,3,0)}}, NodeWeights{}, LINE("Check di-component graph"));
          }
          else
          {
            check_graph(network2, Edges{{Edge(0,1)}, {}, {Edge(2,3)}, {}}, NodeWeights{}, LINE("Check di-component graph"));
          }
        }

        std::swap(network, network2);

        if constexpr (mutual_info(GraphFlavour))
        {
          check_graph(network2, NP_Edges{{NP_Edge(0,0,1), NP_Edge(0,0,0)}}, NodeWeights{}, LINE(""));
          check_graph(network, NP_Edges{{NP_Edge(0,1,0)}, {NP_Edge(0,1,0)}, {NP_Edge(2,3,0)}, {NP_Edge(2,3,0)}}, NodeWeights{}, LINE("Check swapped di-component graph"));
        }
        else
        {
          check_graph(network2, Edges{{Edge(0,0)}}, NodeWeights{}, LINE(""));
          check_graph(network, Edges{{Edge(0,1)}, {}, {Edge(2,3)}, {}}, NodeWeights{}, LINE("Check swapped di-component graph"));
        }
      }
    };

    template
    <
      maths::graph_flavour GraphFlavour,
      class NodeWeight,
      class EdgeWeight,
      bool ThrowOnError,
      template <class> class NodeWeightStorage,
      template <class> class EdgeWeightStorage,
      template <class, class, bool, template<class...> class> class EdgeStoragePolicy
    >
    class generic_weighted_graph_tests
      : public graph_operations<GraphFlavour, NodeWeight, EdgeWeight, ThrowOnError, NodeWeightStorage, EdgeWeightStorage, EdgeStoragePolicy>
    {
    public:
    private:
      using GGraph = typename
        graph_operations<GraphFlavour, NodeWeight, EdgeWeight, ThrowOnError, NodeWeightStorage, EdgeWeightStorage, EdgeStoragePolicy>::graph_type;

      using graph_checker<unit_test_logger<test_mode::standard>>::check_equality;      
      using graph_checker<unit_test_logger<test_mode::standard>>::check_exception_thrown;
      using graph_checker<unit_test_logger<test_mode::standard>>::check_graph;
      
      void execute_operations() override
      {
        test_basic_operations();
        test_sub_graph();
      }

      void test_basic_operations()
      {
        GGraph graph;
        using Edge = maths::edge<EdgeWeight, utilities::protective_wrapper<EdgeWeight>>;
        using Edges = std::vector<std::vector<Edge>>;

        using NP_Edge = maths::embedded_edge<EdgeWeight, data_sharing::independent, utilities::protective_wrapper<EdgeWeight>>;
        using NP_Edges = std::vector<std::vector<NP_Edge>>;

        using NodeWeights = std::vector<typename GGraph::node_weight_type>;

        // NULL
        check_exception_thrown<std::out_of_range>([&graph](){ graph.delete_node(0); }, LINE("No nodes to delete"));
        check_exception_thrown<std::out_of_range>([&graph](){ graph.delete_edge(graph.cbegin_edges(0)); }, LINE("No edges to delete"));
        check_exception_thrown<std::out_of_range>([&graph](){ graph.node_weight(graph.cend_node_weights(), 1.0); }, LINE("No nodes to set weight"));
        check_exception_thrown<std::out_of_range>([&graph](){ graph.set_edge_weight(graph.cbegin_edges(0), 1); }, LINE("No edges to set weight"));
        auto getEdgeFn = [&graph]() { get_edge(graph, 0, 0, 0); };
        check_exception_thrown<std::out_of_range>(getEdgeFn, "No edges so can't acquire a reference to an edge");

        check_graph(graph, Edges{}, NodeWeights{}, LINE(""));
        
        check_equality<std::size_t>(0, graph.add_node(0.0, 1.0), "Node added with weight i");
        //   (i)

        check_graph(graph, Edges{{}}, NodeWeights{{0,1}}, LINE(""));

        check_exception_thrown<std::out_of_range>([&graph](){ graph.node_weight(graph.cend_node_weights(), 0.0); }, LINE("Throw if node out of range"));
        check_exception_thrown<std::out_of_range>(getEdgeFn, LINE("Throw if edge out of range"));

        graph.delete_node(0);
        //    NULL

        check_graph(graph, Edges{}, NodeWeights{}, LINE(""));

        check_equality<std::size_t>(0, graph.add_node(0.0, 1.0), "Node of weight i recreated");
        //    (i)

        check_graph(graph, Edges{{}}, NodeWeights{{0,1}}, LINE(""));
        
        graph.node_weight(graph.cbegin_node_weights(), 1.1, -4.3);
        // (1.1-i4.3)

        check_graph(graph, Edges{{}}, NodeWeights{{1.1,-4.3}}, LINE(""));

        graph.join(0, 0, 1);
        //   /\ 1
        //   \/
        //  (1.1-i4.3)

        if constexpr (mutual_info(GraphFlavour))
        {
          check_graph(graph, NP_Edges{{NP_Edge(0,0,1,1), NP_Edge(0,0,0,1)}}, NodeWeights{{1.1,-4.3}}, LINE(""));
        }
        else
        {
          check_graph(graph, Edges{{Edge(0,0,1)}}, NodeWeights{{1.1,-4.3}}, LINE(""));
        }

        graph.set_edge_weight(graph.cbegin_edges(0), -2);
        //   /\ -2
        //   \/
        // (1.1-i4.3)

        if constexpr (mutual_info(GraphFlavour))
        {
          check_graph(graph, NP_Edges{{NP_Edge(0,0,1,-2), NP_Edge(0,0,0,-2)}}, NodeWeights{{1.1,-4.3}}, LINE(""));
        }
        else
        {
          check_graph(graph, Edges{{Edge(0,0,-2)}}, NodeWeights{{1.1,-4.3}}, LINE(""));
        }

        auto begin0 = graph.cbegin_edges(0);
        graph.set_edge_weight(begin0, -4);
        //   /\ -4
        //   \/
        // (1.1-i4.3)

        check_graph(graph, mutual_info(GraphFlavour) ? Edges{{Edge(0,0,-4), Edge(0,0,-4)}} : Edges{{Edge(0,0,-4)}}, NodeWeights{{1.1,-4.3}});

        graph.delete_edge(graph.cbegin_edges(0));
        //  (1.1-i4.3)

        check_graph(graph, Edges{{}}, NodeWeights{{1.1,-4.3}});

        graph.join(0, 0, 1);
        //   /\ 1
        //   \/
        // (1.1-i4.3)

        check_graph(graph, mutual_info(GraphFlavour) ? Edges{{Edge(0,0,1), Edge(0,0,1)}} : Edges{{Edge(0,0,1)}}, NodeWeights{{1.1,-4.3}});

        graph.join(0, 0, -1);
        //   /\ 1
        //   \/
        // (1.1-i4.3)
        //   /\
        //   \/ -1

        check_graph(graph, mutual_info(GraphFlavour) ? Edges{{Edge(0,0,1), Edge(0,0,1), Edge(0,0,-1), Edge(0,0,-1)}} : Edges{{Edge(0,0,1), Edge(0,0,-1)}}, NodeWeights{{1.1,-4.3}});
        
        check_equality<EdgeWeight>(1, get_edge(graph, 0, 0, 0).weight());
        mutual_info(GraphFlavour) ? check_equality<EdgeWeight>(1, get_edge(graph, 0, 0, 1).weight()) : check_equality<EdgeWeight>(-1, get_edge(graph, 0, 0, 1).weight());

        mutual_info(GraphFlavour) ? graph.set_edge_weight(graph.cbegin_edges(0) + 2, -4) : graph.set_edge_weight(++graph.cbegin_edges(0), -4);
        //   /\ 1
        //   \/
        // (1.1-i4.3)
        //   /\
        //   \/ -4
 
        check_graph(graph, mutual_info(GraphFlavour) ? Edges{{Edge(0,0,1), Edge(0,0,1), Edge(0,0,-4), Edge(0,0,-4)}} : Edges{{Edge(0,0,1), Edge(0,0,-4)}}, NodeWeights{{1.1,-4.3}});

        check_equality<std::size_t>(1, graph.add_node());
        //   /\ 1
        //   \/
        // (1.1-i4.3)   (0)
        //   /\
        //   \/ -4

        check_graph(graph, mutual_info(GraphFlavour) ? Edges{{Edge(0,0,1), Edge(0,0,1), Edge(0,0,-4), Edge(0,0,-4)}, {}} : Edges{{Edge(0,0,1), Edge(0,0,-4)}, {}}, NodeWeights{{1.1,-4.3}, {0,0}});

        graph.join(0, 1, 6);
        //   /\1
        //   \/  6
        // (1.1-i4.3)------(0)
        //   /\
        //   \/-4
        check_graph(graph, mutual_info(GraphFlavour) ? Edges{{Edge(0,0,1), Edge(0,0,1), Edge(0,0,-4), Edge(0,0,-4), Edge(0,1,6)}, {Edge(0,1,6)}}
                                      : Edges{{Edge(0,0,1), Edge(0,0,-4), Edge(0,1,6)}, {}}, NodeWeights{{1.1,-4.3}, {0,0}});

        graph.set_edge_weight(--graph.cend_edges(0), 7);
        //   /\1
        //   \/  7
        // (1.1-i4.3)------(0)
        //   /\
        //   \/-4

        check_graph(graph, mutual_info(GraphFlavour) ? Edges{{Edge(0,0,1), Edge(0,0,1), Edge(0,0,-4), Edge(0,0,-4), Edge(0,1,7)}, {Edge(0,1,7)}}
                                      : Edges{{Edge(0,0,1), Edge(0,0,-4), Edge(0,1,7)}, {}}, NodeWeights{{1.1,-4.3}, {0,0}});
       
        graph.set_edge_weight(--graph.cend_edges(0), 6);
        //   /\1
        //   \/  6
        // (1.1-i4.3)------(0)
        //   /\
        //   \/-4

        check_graph(graph, mutual_info(GraphFlavour) ? Edges{{Edge(0,0,1), Edge(0,0,1), Edge(0,0,-4), Edge(0,0,-4), Edge(0,1,6)}, {Edge(0,1,6)}}
                                      : Edges{{Edge(0,0,1), Edge(0,0,-4), Edge(0,1,6)}, {}}, NodeWeights{{1.1,-4.3}, {0,0}});
        
        graph.join(1, 0, 7);
        //     /\1
        //     \/     6
        // (1.1-i4.3)======(0)
        //     /\     7
        //     \/-4

        check_graph(graph, mutual_info(GraphFlavour) ? Edges{{Edge(0,0,1), Edge(0,0,1), Edge(0,0,-4), Edge(0,0,-4), Edge(0,1,6), Edge(1,0,7)}, {Edge(0,1,6), Edge(1,0,7)}}
                                      : Edges{{Edge(0,0,1), Edge(0,0,-4), Edge(0,1,6)}, {Edge(1,0,7)}}, NodeWeights{{1.1,-4.3}, {0,0}});
        

        graph.join(0, 1, 8);
        //     /\1   /--\8
        //     \/   /6   \
        // (1.1-i4.3)====(0)
        //     /\     7
        //     \/-4

        check_graph(graph, mutual_info(GraphFlavour) ? Edges{{Edge(0,0,1), Edge(0,0,1), Edge(0,0,-4), Edge(0,0,-4), Edge(0,1,6), Edge(1,0,7), Edge(0,1,8)}, {Edge(0,1,6), Edge(1,0,7), Edge(0,1,8)}}
                                      : Edges{{Edge(0,0,1), Edge(0,0,-4), Edge(0,1,6), Edge(0,1,8)}, {Edge(1,0,7)}}, NodeWeights{{1.1,-4.3}, {0,0}});
       
        graph.delete_edge(graph.cbegin_edges(0));
        //            8
        //           /--\
        //          / 6  \
        // (1.1-i4.3)====(0)
        //     /\     7
        //     \/-4

        check_graph(graph, mutual_info(GraphFlavour) ? Edges{{Edge(0,0,-4), Edge(0,0,-4), Edge(0,1,6), Edge(1,0,7), Edge(0,1,8)}, {Edge(0,1,6), Edge(1,0,7), Edge(0,1,8)}}
                                      : Edges{{Edge(0,0,-4), Edge(0,1,6), Edge(0,1,8)}, {Edge(1,0,7)}}, NodeWeights{{1.1,-4.3}, {0,0}});
        
        graph.mutate_edge_weight(graph.cbegin_edges(0) + (mutual_info(GraphFlavour) ?  3 : 2), [](auto& val){ val = 6; });
        //            8
        //           /--\
        //          / 6  \
        // (1.1-i4.3)====(0)
        //     /\     6
        //     \/-4

        if constexpr(!mutual_info(GraphFlavour))
        {
          check_graph(graph, Edges{{Edge(0,0,-4), Edge(0,1,6), Edge(0,1,6)}, {Edge(1,0,7)}}, NodeWeights{{1.1,-4.3}, {0,0}});
        }
        else
        {
          check_graph(graph, Edges{{Edge(0,0,-4), Edge(0,0,-4), Edge(0,1,6), Edge(1,0,6), Edge(0,1,8)}, {Edge(0,1,6), Edge(1,0,6), Edge(0,1,8)}}, NodeWeights{{1.1,-4.3}, {0,0}});
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
          check_graph(graph, Edges{{Edge(0,0,-4), Edge(0,1,6), Edge(0,1,10)}, {Edge(1,0,7)}}, NodeWeights{{1.1,-4.3}, {0,0}});
        }
        else if constexpr(GraphFlavour == maths::graph_flavour::undirected)
        {
          check_graph(graph, Edges{{Edge(0,0,-4), Edge(0,0,-4), Edge(0,1,6), Edge(1,0,10), Edge(0,1,8)}, {Edge(0,1,10), Edge(1,0,6), Edge(0,1,8)}}, NodeWeights{{1.1,-4.3}, {0,0}});
        }
        else
        {
          check_graph(graph, Edges{{Edge(0,0,-4), Edge(0,0,-4), Edge(0,1,6), Edge(1,0,10), Edge(0,1,8)}, {Edge(0,1,6), Edge(1,0,10), Edge(0,1,8)}}, NodeWeights{{1.1,-4.3}, {0,0}});
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
          check_graph(graph, Edges{{Edge(0,0,-4), Edge(0,1,10), Edge(0,1,10)}, {Edge(1,0,7)}}, NodeWeights{{1.1,-4.3}, {0,0}});
        }
        else if constexpr(GraphFlavour == maths::graph_flavour::undirected)
        {
          check_graph(graph, Edges{{Edge(0,0,-4), Edge(0,0,-4), Edge(0,1,6), Edge(1,0,10), Edge(0,1,10)}, {Edge(0,1,10), Edge(1,0,6), Edge(0,1,10)}}, NodeWeights{{1.1,-4.3}, {0,0}});
        }
        else
        {
          check_graph(graph, Edges{{Edge(0,0,-4), Edge(0,0,-4), Edge(0,1,6), Edge(1,0,10), Edge(0,1,10)}, {Edge(0,1,6), Edge(1,0,10), Edge(0,1,10)}}, NodeWeights{{1.1,-4.3}, {0,0}});
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
          check_graph(graph, Edges{{Edge(0,0,-4), Edge(0,1,10), Edge(0,1,7)}, {Edge(1,0,7)}}, NodeWeights{{1.1,-4.3}, {0,0}});
        }
        else if constexpr(GraphFlavour == maths::graph_flavour::undirected)
        {
          check_graph(graph, Edges{{Edge(0,0,-4), Edge(0,0,-4), Edge(0,1,6), Edge(1,0,10), Edge(0,1,7)}, {Edge(0,1,7), Edge(1,0,6), Edge(0,1,10)}}, NodeWeights{{1.1,-4.3}, {0,0}});
        }
        else
        {
          check_graph(graph, Edges{{Edge(0,0,-4), Edge(0,0,-4), Edge(0,1,6), Edge(1,0,10), Edge(0,1,7)}, {Edge(0,1,6), Edge(1,0,10), Edge(0,1,7)}}, NodeWeights{{1.1,-4.3}, {0,0}});
        }
        
        graph.delete_node(0);
        //  (0)

        check_graph(graph, Edges{{}}, NodeWeights{{0}});
        

        graph.join(0, 0, 1);
        //   /\ 1
        //   \/
        //   (0)

        check_graph(graph, mutual_info(GraphFlavour) ? Edges{{Edge(0, 0, 1), Edge(0, 0, 1)}} : Edges{{Edge(0, 0, 1)}}, NodeWeights{{0}});
 

        check_equality<std::size_t>(0, graph.insert_node(0, 1.0, 1.0), "Node with weight 1+i inserted into slot 0");
        //       /\ 1
        //       \/
        // (1+i) (0)

        check_graph(graph, mutual_info(GraphFlavour) ? Edges{{}, {Edge(1,1,1), Edge(1,1,1)}} : Edges{{}, {Edge(1,1,1)}}, NodeWeights{{1,1}, {0}});
      }

      void test_sub_graph()
      {
        using std::complex;
        GGraph graph;

        using Edge = maths::edge<EdgeWeight, utilities::protective_wrapper<EdgeWeight>>;
        using Edges = std::vector<std::vector<Edge>>;

        using NP_Edge = maths::embedded_edge<EdgeWeight, data_sharing::independent, utilities::protective_wrapper<EdgeWeight>>;
        using NP_Edges = std::vector<std::vector<NP_Edge>>;

        using NodeWeights = std::vector<typename GGraph::node_weight_type>;

        graph.add_node(1.0, 1.0);

        // Graph: 
        // (1,1)
        //   0

        check_graph(graph, Edges{{}}, NodeWeights{{1,1}}, LINE(""));

        auto subgraph = sub_graph(graph, [](auto&& wt) { return wt == NodeWeight(1, 1); });

        // Subgraph
        // (1,1)
        //   0

        check_graph(subgraph, Edges{{}}, NodeWeights{{1,1}}, LINE("Subgraph same as parent"));
        
        subgraph = sub_graph(graph, [](auto&& wt) { return wt == NodeWeight(0, 1); });
        // Subgraph
        //   NULL

        check_graph(subgraph, Edges{}, NodeWeights{}, LINE("Null subgraph"));
        
        graph.add_node(1.0, 0.0);

        // Graph: 
        // (1,1) (1,0)
        //   0     0

        check_graph(graph, Edges{{}, {}}, NodeWeights{{1,1}, {1,0}}, LINE(""));

        subgraph = sub_graph(graph, [](auto&& wt) { return wt == NodeWeight(1, 1); });

        // Subgraph
        // (1,1)
        //   0

        check_graph(subgraph, Edges{{}}, NodeWeights{{1,1}}, LINE(""));

        subgraph = sub_graph(graph, [](auto&& wt) { return wt == NodeWeight(1, 0); });

        // Subgraph
        // (1,0)
        //   0

        check_graph(subgraph, Edges{{}}, NodeWeights{{1,0}}, LINE(""));

        graph.join(0, 1, 4);

        // Graph: 
        // (1,1)  4  (1,0)
        //   0---------0

        if constexpr (mutual_info(GraphFlavour))
        {
          check_graph(graph, NP_Edges{{NP_Edge(0,1,0,4)}, {NP_Edge(0,1,0,4)}}, NodeWeights{{1,1}, {1,0}}, LINE(""));
        }
        else
        {
          check_graph(graph, Edges{{Edge(0,1,4)}, {}}, NodeWeights{{1,1}, {1,0}}, LINE(""));
        }

        subgraph = sub_graph(graph, [](auto&& wt) { return wt == NodeWeight(1, 1); });

        // Subgraph
        // (1,1)
        //   0

        check_graph(subgraph, Edges{{}}, NodeWeights{{1,1}}, LINE("Second node removed, leaving first in subgraph"));

        subgraph = sub_graph(graph, [](auto&& wt) { return wt == NodeWeight(1, 0); });

        // Subgraph
        // (1,0)
        //   0

        check_graph(subgraph, Edges{{}}, NodeWeights{{1,0}}, LINE("Node retained"));


        graph.join(0, 0, 2);

        // Graph: 
        // (1,1)  4  (1,0)
        //   0---------0
        //  /\
        //  \/ 2

        check_graph(graph, mutual_info(GraphFlavour) ? Edges{{Edge(0,1,4), Edge(0,0,2), Edge(0,0,2)}, {Edge(0,1,4)}} : Edges{{Edge(0,1,4), Edge(0,0,2)}, {}}, NodeWeights{{1,1}, {1,0}});

        subgraph = sub_graph(graph, [](auto&& wt) { return wt == NodeWeight(1, 1); });

        // Subgraph
        // (1,1)
        //   0
        //  /\
        //  \/ 2

        check_graph(subgraph, mutual_info(GraphFlavour) ? Edges{{Edge(0,0,2), Edge(0,0,2)}} : Edges{{Edge(0,0,2)}}, NodeWeights{{1,1}});

        subgraph = sub_graph(graph, [](auto&& wt) { return wt == NodeWeight(1, 0); });

        // Subgraph
        // (1,0)
        //   0

        check_graph(subgraph, Edges{{}}, NodeWeights{{1,0}});

        graph.add_node(1.0, 1.0);
        graph.join(0, 2, 0);
        graph.join(1, 2, -3);

        // Graph: 
        // (1,1)  4  (1,0)
        //   0---------0
        //  /\\       /
        // 2\/ \0    /
        //      \   /-3
        //       \ /
        //        0
        //       (1,1)

        check_graph(graph, mutual_info(GraphFlavour) ? Edges{{Edge(0,1,4), Edge(0,0,2), Edge(0,0,2), Edge(0,2,0)}, {Edge(0,1,4), Edge(1,2,-3)}, {Edge(0,2,0), Edge(1,2,-3)}}
                                      : Edges{{Edge(0,1,4), Edge(0,0,2), Edge(0,2,0)}, {Edge(1,2,-3)}, {}}, NodeWeights{{1,1}, {1,0}, {1,1}});

        subgraph = sub_graph(graph, [](auto&& wt) { return wt == NodeWeight(1, 1); });
        // subgraph: 
        // (1,1)
        //   0
        //  /\\
        // 2\/ \0
        //      \
        //       \
        //        0
        //       (1,1)

        check_graph(subgraph, mutual_info(GraphFlavour) ? Edges{{Edge(0,0,2), Edge(0,0,2), Edge(0,1,0)}, {Edge(0,1,0)}}
                                         : Edges{{Edge(0,0,2), Edge(0,1,0)}, {}}, NodeWeights{{1,1}, {1,1}});

        subgraph = sub_graph(graph, [](auto&& wt) { return wt == NodeWeight(1, 0); });

        // Subgraph
        // (1,0)
        //   0

        check_graph(subgraph, Edges{{}}, NodeWeights{{1,0}});
      }
    };
 
    template
    <
      maths::graph_flavour GraphFlavour,
      class NodeWeight,
      class EdgeWeight,
      bool ThrowOnError,
      template <class> class NodeWeightStorage,
      template <class> class EdgeWeightStorage,
      template <class, class, bool, template<class...> class> class EdgeStoragePolicy
    >
    class more_generic_weighted_graph_tests
      : public graph_operations<GraphFlavour, NodeWeight, EdgeWeight, ThrowOnError, NodeWeightStorage, EdgeWeightStorage, EdgeStoragePolicy>
    {
    public:

    private:
      using GGraph = typename
        graph_operations<GraphFlavour, NodeWeight, EdgeWeight, ThrowOnError, NodeWeightStorage, EdgeWeightStorage, EdgeStoragePolicy>::graph_type;

      using graph_checker<unit_test_logger<test_mode::standard>>::check_equality;      
      using graph_checker<unit_test_logger<test_mode::standard>>::check_exception_thrown;
      using graph_checker<unit_test_logger<test_mode::standard>>::check_graph;
      
      void execute_operations() override
      {
        GGraph graph;
        using Edge = maths::edge<EdgeWeight, utilities::protective_wrapper<EdgeWeight>>;
        using Edges = std::vector<std::vector<Edge>>;
        using NodeWeights = std::vector<typename GGraph::node_weight_type>;

        graph.add_node(4);
        graph.add_node(2);

        graph.join(0, 1, 1.0, -1.0);

        //      (1,-1)
        //   4---------2

        check_equality<int>(4, *graph.cbegin_node_weights(), "Node 0 has weight 4");
        check_equality<int>(2, *(graph.cbegin_node_weights() + 1), "Node 1 has weight 2");

        check_graph(graph, mutual_info(GraphFlavour) ? Edges{{Edge(0,1,1.0,-1.0)}, {Edge(0,1,1.0,-1.0)}}
                                      : Edges{{Edge(0,1,1.0,-1.0)}, {}}, NodeWeights{4,2});


        graph.set_edge_weight(graph.cbegin_edges(0), -3.0, 2.0);
        //      (-3,2)
        //   4---------2

        check_graph(graph, mutual_info(GraphFlavour) ? Edges{{Edge(0,1,-3.0,2.0)}, {Edge(0,1,-3.0,2.0)}}
                                      : Edges{{Edge(0,1,-3.0,2.0)}, {}}, NodeWeights{4,2});

        graph.set_edge_weight(graph.crbegin_edges(0), 6.8, 3.2);
        //    (6.8,3.2)
        //   4---------2

        check_graph(graph, mutual_info(GraphFlavour) ? Edges{{Edge(0,1,6.8,3.2)}, {Edge(0,1,6.8,3.2)}}
                                      : Edges{{Edge(0,1,6.8,3.2)}, {}}, NodeWeights{4,2});


        graph.join(0, 1, -2.0, -3.0);
        //    (6.8,3.2)
        //   4=========2
        //    (-2, -3)

        check_graph(graph, mutual_info(GraphFlavour) ? Edges{{Edge(0,1,6.8,3.2), Edge(0,1,-2.0,-3.0)}, {Edge(0,1,6.8,3.2), Edge(0,1,-2.0,-3.0)}}
                                      : Edges{{Edge(0,1,6.8,3.2), Edge(0,1,-2.0,-3.0)}, {}}, NodeWeights{4,2});


        graph.set_edge_weight(graph.crbegin_edges(0), -7.9, 0.0);
        //    (6.8,3.2)
        //   4=========2
        //    (-7.9, 0)

        check_graph(graph, mutual_info(GraphFlavour) ? Edges{{Edge(0,1,6.8,3.2), Edge(0,1,-7.9,0.0)}, {Edge(0,1,6.8,3.2), Edge(0,1,-7.9,0.0)}}
                                      : Edges{{Edge(0,1,6.8,3.2), Edge(0,1,-7.9,0.0)}, {}}, NodeWeights{4,2});


        graph.delete_edge(graph.cbegin_edges(0));
        //   4---------2
        //    (-7.9, 0)

        check_graph(graph, mutual_info(GraphFlavour) ? Edges{{Edge(0,1,-7.9,0.0)}, {Edge(0,1,-7.9,0.0)}}
                                      : Edges{{Edge(0,1,-7.9,0.0)}, {}}, NodeWeights{4,2});


        graph.delete_edge(graph.cbegin_edges(0));
        //   4         2

        check_graph(graph, Edges{{}, {}},  NodeWeights{4,2});
      }
    };

    template
    <
      maths::graph_flavour GraphFlavour,
      class NodeWeight,
      class EdgeWeight,
      bool ThrowOnError,
      template <class> class NodeWeightStorage,
      template <class> class EdgeWeightStorage,
      template <class, class, bool, template<class...> class> class EdgeStoragePolicy
    >
    class test_copy_move
      : public graph_operations<GraphFlavour, NodeWeight, EdgeWeight, ThrowOnError, NodeWeightStorage, EdgeWeightStorage, EdgeStoragePolicy>
    {
    private:
      using GGraph = typename
        graph_operations<GraphFlavour, NodeWeight, EdgeWeight, ThrowOnError, NodeWeightStorage, EdgeWeightStorage, EdgeStoragePolicy>::graph_type;
      
      using Edge = maths::edge<EdgeWeight, utilities::protective_wrapper<EdgeWeight>>;
      using Edges = std::vector<std::vector<Edge>>;
      using NodeWeights = std::vector<typename GGraph::node_weight_type>;

      using graph_checker<unit_test_logger<test_mode::standard>>::check_equality;      
      using graph_checker<unit_test_logger<test_mode::standard>>::check_exception_thrown;
      using graph_checker<unit_test_logger<test_mode::standard>>::check_graph;
      
      void execute_operations()
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
          
      GGraph generate_test_graph1(const std::vector<int>& edgeVal)
      {
        GGraph graph;
        //  (-1,-2)
        //   /\
      	//   \/   (5,-6) or (-7,9)
        //    0===========0
        // (1,2) (0,2) (-3,4)

        graph.add_node();
        graph.add_node();
        graph.node_weight(graph.cbegin_node_weights(), std::vector<int>{1, 2});
        graph.node_weight(graph.cbegin_node_weights() + 1, std::vector<int>{-3, 4});

        graph.join(0, 0, std::vector<int>{-1, -2});
        graph.join(0, 1, edgeVal);
        graph.join(1, 0, std::vector<int>{0, 2});

        check_test_graph_1(graph, edgeVal);

        return graph;
      }

      void check_empty_graph(const GGraph& graph, const std::string& message="")
      {
        check_graph(graph, Edges{}, NodeWeights{}, message);

        check_exception_thrown<std::out_of_range>([&graph](){ graph.cbegin_edges(0); }, "Cannot obtain iterator to edges for empty graph");
      }

      void check_test_graph_1(const GGraph& graph, const std::vector<int>& edgeVal, const std::string& message="")
      {
        check_graph(graph, mutual_info(GraphFlavour) ? Edges{{Edge{0,0,-1,-2}, Edge{0,0,-1,-2}, Edge{0,1,edgeVal}, Edge{1,0,0,2}}, {Edge{0,1,edgeVal}, Edge{1,0,0,2}}}
                                      : Edges{{Edge{0,0,-1,-2}, Edge{0,1,edgeVal}}, {Edge{1,0,0,2}}},  NodeWeights{{1,2},{-3,4}}, message);
      }
    };    
  }
}
