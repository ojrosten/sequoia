////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

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
    template <maths::graph_flavour, class, template<class> class> class EdgeStorageTraits,
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
    using namespace maths;
    GGraph graph;

    using Edge = maths::edge<EdgeWeight, utilities::protective_wrapper<EdgeWeight>>;

    using edge_init_t = typename GGraph::edge_init_type;
    using edge_init_list_t = std::initializer_list<std::initializer_list<edge_init_t>>;

    graph.add_node(1.0, 1.0);

    // Graph: 
    // (1,1)
    //   X

    check_equality(graph, {edge_init_list_t{{}}, {{1,1}}}, LINE(""));

    auto subgraph = sub_graph(graph, [](auto&& wt) { return wt == NodeWeight(1, 1); });

    // Subgraph
    // (1,1)
    //   X

    check_equality(subgraph, {edge_init_list_t{{}}, {{1,1}}}, LINE("Subgraph same as parent"));
        
    subgraph = sub_graph(graph, [](auto&& wt) { return wt == NodeWeight(0, 1); });
    // Subgraph
    //   NULL

    check_equality(subgraph, {}, LINE("Null subgraph"));
        
    graph.add_node(1.0, 0.0);

    // Graph: 
    // (1,1) (1,0)
    //   X     X

    check_equality(graph, {edge_init_list_t{{}, {}}, {{1,1}, {1,0}}}, LINE(""));

    subgraph = sub_graph(graph, [](auto&& wt) { return wt == NodeWeight(1, 1); });

    // Subgraph
    // (1,1)
    //   X

    check_equality(subgraph, {edge_init_list_t{{}}, {{1,1}}}, LINE(""));

    subgraph = sub_graph(graph, [](auto&& wt) { return wt == NodeWeight(1, 0); });

    // Subgraph
    // (1,0)
    //   X

    check_equality(subgraph, {edge_init_list_t{{}}, {{1,0}}}, LINE(""));

    graph.join(0, 1, 4);

    // Graph: 
    // (1,1)  4  (1,0)
    //   X---------X

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(graph, {{{edge_init_t{1, 4}}, {}}, {{1,1}, {1,0}}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_equality(graph, {{{edge_init_t{1,4}}, {edge_init_t{0,4}}}, {{1,1}, {1,0}}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(graph, {{{edge_init_t{0,1,0,4}}, {edge_init_t{0,1,0,4}}}, {{1,1}, {1,0}}}, LINE(""));
    }
    else
    {
      check_equality(graph, {edge_init_list_t{{edge_init_t{1,0,4}}, {edge_init_t{0,0,4}}}, {{1,1}, {1,0}}}, LINE(""));
    }

    subgraph = sub_graph(graph, [](auto&& wt) { return wt == NodeWeight(1, 1); });

    // Subgraph
    // (1,1)
    //   X

    check_equality(subgraph, {edge_init_list_t{{}}, {{1,1}}}, LINE("Second node removed, leaving first in subgraph"));

    subgraph = sub_graph(graph, [](auto&& wt) { return wt == NodeWeight(1, 0); });

    // Subgraph
    // (1,0)
    //   X

    check_equality(subgraph, {edge_init_list_t{{}}, {{1,0}}}, LINE("Node retained"));


    graph.join(0, 0, 2);

    // Graph: 
    // (1,1)  4  (1,0)
    //   0---------0
    //  /\
    //  \/ 2

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(graph, {{{edge_init_t{1, 4}, edge_init_t{0, 2}}, {}}, {{1,1}, {1,0}}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      GGraph g{{{edge_init_t{0, 2}, edge_init_t{0, 2}, edge_init_t{1,4}},
              {edge_init_t{0,4}}}, {{1,1}, {1,0}}};

      g.swap_edges(0, 0, 2);
      
      check_equality(graph, g, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(graph, {{{edge_init_t{0,1,0,4}, edge_init_t{0,0,2,2}, edge_init_t{0,0,1,2}},
                              {edge_init_t{0,1,0,4}}}, {{1,1}, {1,0}}}, LINE(""));
    }
    else
    {
      check_equality(graph, {{{edge_init_t{1,0,4}, edge_init_t{0,2,2}, edge_init_t{0,1,2}},
                              {edge_init_t{0,0,4}}}, {{1,1}, {1,0}}}, LINE(""));
    }

    subgraph = sub_graph(graph, [](auto&& wt) { return wt == NodeWeight(1, 1); });

    // Subgraph
    // (1,1)
    //   0
    //  /\
    //  \/ 2

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(subgraph, {edge_init_list_t{{edge_init_t{0, 2}}}, {{1,1}}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {      
      check_equality(subgraph, {edge_init_list_t{{edge_init_t{0, 2}, edge_init_t{0, 2}}}, {{1,1}}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(subgraph, {{{edge_init_t{0,0,1,2}, edge_init_t{0,0,0,2}}}, {{1,1}}}, LINE(""));
    }
    else
    {
      check_equality(subgraph, {{{edge_init_t{0,1,2}, edge_init_t{0,0,2}}}, {{1,1}}}, LINE(""));
    }
    
    subgraph = sub_graph(graph, [](auto&& wt) { return wt == NodeWeight(1, 0); });

    // Subgraph
    // (1,0)
    //   0

    check_equality(subgraph, {edge_init_list_t{{}}, {{1,0}}}, LINE(""));

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
