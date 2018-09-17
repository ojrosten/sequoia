#include "TestSubgraph.hpp"

#include "GraphAlgorithms.hpp"

#include <complex>

namespace sequoia::unit_testing
{
  void test_subgraph::run_tests()
  {
    using std::complex;
    
    {
      graph_test_helper<int, complex<double>>  helper{"int, complex<double>"};
      helper.run_tests<generic_subgraph_tests>(*this);
    }

    {
      graph_test_helper<complex<int>, complex<double>>  helper{"complex<int>, complex<double>"};
      helper.run_tests<generic_subgraph_tests>(*this);
    }
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
  void
  generic_subgraph_tests
  <
    GraphFlavour,
    EdgeWeight,
    NodeWeight,
    EdgeWeightPooling,
    NodeWeightPooling,
    EdgeStorageTraits,
    NodeWeightStorageTraits
  >::test_sub_graph()
  {
    using std::complex;
    GGraph graph;

    using Edge = maths::edge<EdgeWeight, utilities::protective_wrapper<EdgeWeight>>;
    using E_Edge = maths::embedded_edge<EdgeWeight, data_sharing::independent, utilities::protective_wrapper<EdgeWeight>>;

    graph.add_node(1.0, 1.0);

    // Graph: 
    // (1,1)
    //   0

    check_graph(graph, {{}}, {{1,1}}, LINE(""));

    auto subgraph = sub_graph(graph, [](auto&& wt) { return wt == NodeWeight(1, 1); });

    // Subgraph
    // (1,1)
    //   0

    check_graph(subgraph, {{}}, {{1,1}}, LINE("Subgraph same as parent"));
        
    subgraph = sub_graph(graph, [](auto&& wt) { return wt == NodeWeight(0, 1); });
    // Subgraph
    //   NULL

    check_graph(subgraph, {}, {}, LINE("Null subgraph"));
        
    graph.add_node(1.0, 0.0);

    // Graph: 
    // (1,1) (1,0)
    //   0     0

    check_graph(graph, {{}, {}}, {{1,1}, {1,0}}, LINE(""));

    subgraph = sub_graph(graph, [](auto&& wt) { return wt == NodeWeight(1, 1); });

    // Subgraph
    // (1,1)
    //   0

    check_graph(subgraph, {{}}, {{1,1}}, LINE(""));

    subgraph = sub_graph(graph, [](auto&& wt) { return wt == NodeWeight(1, 0); });

    // Subgraph
    // (1,0)
    //   0

    check_graph(subgraph, {{}}, {{1,0}}, LINE(""));

    graph.join(0, 1, 4);

    // Graph: 
    // (1,1)  4  (1,0)
    //   0---------0

    if constexpr (mutual_info(GraphFlavour))
                   {
                     check_graph(graph, {{E_Edge(0,1,0,4)}, {E_Edge(0,1,0,4)}}, {{1,1}, {1,0}}, LINE(""));
                   }
    else
      {
        check_graph(graph, {{Edge(0,1,4)}, {}}, {{1,1}, {1,0}}, LINE(""));
      }

    subgraph = sub_graph(graph, [](auto&& wt) { return wt == NodeWeight(1, 1); });

    // Subgraph
    // (1,1)
    //   0

    check_graph(subgraph, {{}}, {{1,1}}, LINE("Second node removed, leaving first in subgraph"));

    subgraph = sub_graph(graph, [](auto&& wt) { return wt == NodeWeight(1, 0); });

    // Subgraph
    // (1,0)
    //   0

    check_graph(subgraph, {{}}, {{1,0}}, LINE("Node retained"));


    graph.join(0, 0, 2);

    // Graph: 
    // (1,1)  4  (1,0)
    //   0---------0
    //  /\
    //  \/ 2
        
    if constexpr(mutual_info(GraphFlavour))
    {
      check_graph(graph, {{Edge(0,1,4), Edge(0,0,2), Edge(0,0,2)}, {Edge(0,1,4)}}, {{1,1}, {1,0}}, LINE(""));
    }
    else
    {
      check_graph(graph, {{Edge(0,1,4), Edge(0,0,2)}, {}}, {{1,1}, {1,0}}, LINE(""));
    }

    subgraph = sub_graph(graph, [](auto&& wt) { return wt == NodeWeight(1, 1); });

    // Subgraph
    // (1,1)
    //   0
    //  /\
    //  \/ 2

    if constexpr(mutual_info(GraphFlavour))
    {
      check_graph(subgraph, {{Edge(0,0,2), Edge(0,0,2)}}, {{1,1}}, LINE(""));
    }
    else
    {
      check_graph(subgraph, {{Edge(0,0,2)}}, {{1,1}}, LINE(""));
    }

    subgraph = sub_graph(graph, [](auto&& wt) { return wt == NodeWeight(1, 0); });

    // Subgraph
    // (1,0)
    //   0

    check_graph(subgraph, {{}}, {{1,0}});

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
        
    if constexpr(mutual_info(GraphFlavour))
    {
      check_graph(graph, {{Edge(0,1,4), Edge(0,0,2), Edge(0,0,2), Edge(0,2,0)}, {Edge(0,1,4), Edge(1,2,-3)}, {Edge(0,2,0), Edge(1,2,-3)}}, {{1,1}, {1,0}, {1,1}}, LINE(""));
    }
    else
    {
      check_graph(graph, {{Edge(0,1,4), Edge(0,0,2), Edge(0,2,0)}, {Edge(1,2,-3)}, {}}, {{1,1}, {1,0}, {1,1}}, LINE(""));
    }
      
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
        
    if constexpr(mutual_info(GraphFlavour))
    {
      check_graph(subgraph, {{Edge(0,0,2), Edge(0,0,2), Edge(0,1,0)}, {Edge(0,1,0)}}, {{1,1}, {1,1}}, LINE(""));
    }
    else
    {
      check_graph(subgraph, {{Edge(0,0,2), Edge(0,1,0)}, {}}, {{1,1}, {1,1}}, LINE(""));
    }

    subgraph = sub_graph(graph, [](auto&& wt) { return wt == NodeWeight(1, 0); });

    // Subgraph
    // (1,0)
    //   0

    check_graph(subgraph, {{}}, {{1,0}});
  }
}
