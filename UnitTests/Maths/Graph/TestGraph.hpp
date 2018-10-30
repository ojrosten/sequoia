#pragma once

#include "DynamicGraphTestingUtils.hpp"

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
      class EdgeWeight,
      class NodeWeight,      
      template <class> class EdgeWeightPooling,
      template <class> class NodeWeightPooling,
      template <maths::graph_flavour, class, template<class> class> class EdgeStorageTraits,
      template <class, template<class> class, bool> class NodeWeightStorageTraits
    >
    class generic_graph_operations
      : public graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>
    {
    public:
      
    private:
      using base_t = graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>;
      
      using GGraph = typename base_t::graph_type;

      using graph_checker<unit_test_logger<test_mode::standard>>::check_equality;      
      using graph_checker<unit_test_logger<test_mode::standard>>::check_exception_thrown;
      using graph_checker<unit_test_logger<test_mode::standard>>::check_graph;

      void execute_operations() override;
    };

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
    class generic_weighted_graph_tests
      : public graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>
    {
    public:
    private:
      using GGraph = typename
        graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>::graph_type;

      using graph_checker<unit_test_logger<test_mode::standard>>::check_equality;      
      using graph_checker<unit_test_logger<test_mode::standard>>::check_exception_thrown;
      using graph_checker<unit_test_logger<test_mode::standard>>::check_graph;
      
      void execute_operations() override
      {
        test_basic_operations();
      }

      void test_basic_operations();
    };


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
    class graph_contiguous_capacity
      : public graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>
    {
    public:
      
    private:
      using base_t = graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>;
      
      using graph_t = typename base_t::graph_type;

      using graph_checker<unit_test_logger<test_mode::standard>>::check_equality;      
      using graph_checker<unit_test_logger<test_mode::standard>>::check_exception_thrown;
      using graph_checker<unit_test_logger<test_mode::standard>>::check_graph;

      void execute_operations() override;
    };

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
    class graph_bucketed_capacity
      : public graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>
    {
    public:
      
    private:
      using base_t = graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>;
      
      using graph_t = typename base_t::graph_type;

      using graph_checker<unit_test_logger<test_mode::standard>>::check_equality;      
      using graph_checker<unit_test_logger<test_mode::standard>>::check_exception_thrown;
      using graph_checker<unit_test_logger<test_mode::standard>>::check_graph;

      void execute_operations() override;
    };

    
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
    class more_generic_weighted_graph_tests
      : public graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>
    {
    public:

    private:
      using GGraph = typename
        graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>::graph_type;

      using graph_checker<unit_test_logger<test_mode::standard>>::check_equality;      
      using graph_checker<unit_test_logger<test_mode::standard>>::check_exception_thrown;
      using graph_checker<unit_test_logger<test_mode::standard>>::check_graph;
      
      void execute_operations() override
      {
        GGraph graph;
        using Edge = maths::edge<EdgeWeight, utilities::protective_wrapper<EdgeWeight>>;

        graph.add_node(4);
        graph.add_node(2);

        graph.join(0, 1, 1.0, -1.0);

        //      (1,-1)
        //   4---------2

        check_equality<int>(4, *graph.cbegin_node_weights(), "Node 0 has weight 4");
        check_equality<int>(2, *(graph.cbegin_node_weights() + 1), "Node 1 has weight 2");
        
        if constexpr(mutual_info(GraphFlavour))
        {
          check_graph(graph, {{Edge(0,1,1.0,-1.0)}, {Edge(0,1,1.0,-1.0)}}, {4,2}, LINE(""));
        }
        else
        {
          check_graph(graph, {{Edge(0,1,1.0,-1.0)}, {}}, {4,2}, LINE(""));
        }


        graph.set_edge_weight(graph.cbegin_edges(0), -3.0, 2.0);
        //      (-3,2)
        //   4---------2

        if constexpr(mutual_info(GraphFlavour))
        {
          check_graph(graph, {{Edge(0,1,-3.0,2.0)}, {Edge(0,1,-3.0,2.0)}}, {4,2}, LINE(""));
        }
        else
        {
          check_graph(graph, {{Edge(0,1,-3.0,2.0)}, {}}, {4,2}, LINE(""));
        }        

        graph.set_edge_weight(graph.crbegin_edges(0), 6.8, 3.2);
        //    (6.8,3.2)
        //   4---------2
        
        if constexpr(mutual_info(GraphFlavour))
        {
          check_graph(graph, {{Edge(0,1,6.8,3.2)}, {Edge(0,1,6.8,3.2)}}, {4,2}, LINE(""));
        }
        else
        {
          check_graph(graph, {{Edge(0,1,6.8,3.2)}, {}}, {4,2}, LINE(""));
        }


        graph.join(0, 1, -2.0, -3.0);
        //    (6.8,3.2)
        //   4=========2
        //    (-2, -3)
        
        if constexpr(mutual_info(GraphFlavour))
        {
          check_graph(graph,{{Edge(0,1,6.8,3.2), Edge(0,1,-2.0,-3.0)}, {Edge(0,1,6.8,3.2), Edge(0,1,-2.0,-3.0)}}, {4,2}, LINE(""));
        }
        else
        {
          check_graph(graph, {{Edge(0,1,6.8,3.2), Edge(0,1,-2.0,-3.0)}, {}}, {4,2}, LINE(""));
        }


        graph.set_edge_weight(graph.crbegin_edges(0), -7.9, 0.0);
        //    (6.8,3.2)
        //   4=========2
        //    (-7.9, 0)

       
        if constexpr(mutual_info(GraphFlavour))
        {
          check_graph(graph, {{Edge(0,1,6.8,3.2), Edge(0,1,-7.9,0.0)}, {Edge(0,1,6.8,3.2), Edge(0,1,-7.9,0.0)}}, {4,2}, LINE(""));
        }
        else
        {
          check_graph(graph, {{Edge(0,1,6.8,3.2), Edge(0,1,-7.9,0.0)}, {}}, {4,2}, LINE(""));
        }


        graph.erase_edge(graph.cbegin_edges(0));
        //   4---------2
        //    (-7.9, 0)
        
        if constexpr(mutual_info(GraphFlavour))
        {
          check_graph(graph, {{Edge(0,1,-7.9,0.0)}, {Edge(0,1,-7.9,0.0)}}, {4,2}, LINE(""));
        }
        else
        {
          check_graph(graph, {{Edge(0,1,-7.9,0.0)}, {}}, {4,2}, LINE(""));
        }


        graph.erase_edge(graph.cbegin_edges(0));
        //   4         2

        check_graph(graph, {{}, {}},  {4,2});
      }
    };

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
    class test_copy_move
      : public graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>
    {
    private:
      using GGraph = typename
        graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>::graph_type;
      
      using Edge = maths::edge<EdgeWeight, utilities::protective_wrapper<EdgeWeight>>;
      using Edges = std::vector<std::vector<Edge>>;
      using NodeWeights = std::vector<typename GGraph::node_weight_type>;

      using graph_checker<unit_test_logger<test_mode::standard>>::check_equality;      
      using graph_checker<unit_test_logger<test_mode::standard>>::check_exception_thrown;
      using graph_checker<unit_test_logger<test_mode::standard>>::check_graph;
      
      void execute_operations();
      
          
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
        check_graph(graph, {}, {}, message);

        check_exception_thrown<std::out_of_range>([&graph](){ graph.cbegin_edges(0); }, "Cannot obtain iterator to edges for empty graph");
      }

      void check_test_graph_1(const GGraph& graph, const std::vector<int>& edgeVal, const std::string& message="")
      {        
        if constexpr(mutual_info(GraphFlavour))
        {
          check_graph(graph, {{Edge{0,0,-1,-2}, Edge{0,0,-1,-2}, Edge{0,1,edgeVal}, Edge{1,0,0,2}}, {Edge{0,1,edgeVal}, Edge{1,0,0,2}}},  {{1,2},{-3,4}}, message);
        }
        else
        {
          check_graph(graph, {{Edge{0,0,-1,-2}, Edge{0,1,edgeVal}}, {Edge{1,0,0,2}}}, {{1,2},{-3,4}}, message);
        }
      }
    };    
  }
}
