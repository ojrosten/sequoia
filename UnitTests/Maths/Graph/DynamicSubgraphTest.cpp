////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "DynamicSubgraphTest.hpp"

#include "GraphAlgorithms.hpp"

#include <complex>

namespace sequoia::unit_testing
{
  void test_subgraph::run_tests()
  {
    using std::complex;
    
    {
      graph_test_helper<int, complex<double>>  helper{};
      helper.run_tests<generic_subgraph_tests>(*this);
    }

    {
      graph_test_helper<complex<int>, complex<double>>  helper{};
      helper.run_tests<generic_subgraph_tests>(*this);
    }
  }

  template
  <
    maths::graph_flavour GraphFlavour,
    class EdgeWeight,
    class NodeWeight,      
    template <class, template<class> class...> class EdgeWeightPooling,
    template <class, template<class> class...> class NodeWeightPooling,
    template <maths::graph_flavour, class, template<class, template<class> class...> class> class EdgeStorageTraits,
    template <class, template<class, template<class> class...> class, bool> class NodeWeightStorageTraits
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
    graph_t graph;

    using edge_init_t = typename graph_t::edge_init_type;
    using edge_init_list_t = std::initializer_list<std::initializer_list<edge_init_t>>;

    graph.add_node(1.0, 1.0);

    // Graph: 
    // (1,1)
    //   X

    check_equality(LINE(""), graph, {edge_init_list_t{{}}, {{1,1}}});

    auto subgraph = sub_graph(graph, [](auto&& wt) { return wt == NodeWeight(1, 1); });

    // Subgraph
    // (1,1)
    //   X

    check_equality(LINE("Subgraph same as parent"), subgraph, {edge_init_list_t{{}}, {{1,1}}});
        
    subgraph = sub_graph(graph, [](auto&& wt) { return wt == NodeWeight(0, 1); });
    // Subgraph
    //   NULL

    check_equality(LINE("Null subgraph"), subgraph, {});
        
    graph.add_node(1.0, 0.0);

    // Graph: 
    // (1,1) (1,0)
    //   X     X

    check_equality(LINE(""), graph, {edge_init_list_t{{}, {}}, {{1,1}, {1,0}}});

    subgraph = sub_graph(graph, [](auto&& wt) { return wt == NodeWeight(1, 1); });

    // Subgraph
    // (1,1)
    //   X

    check_equality(LINE(""), subgraph, {edge_init_list_t{{}}, {{1,1}}});

    subgraph = sub_graph(graph, [](auto&& wt) { return wt == NodeWeight(1, 0); });

    // Subgraph
    // (1,0)
    //   X

    check_equality(LINE(""), subgraph, {edge_init_list_t{{}}, {{1,0}}});

    graph.join(0, 1, 4);

    // Graph: 
    // (1,1)  4  (1,0)
    //   X---------X

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), graph, {{{edge_init_t{1, 4}}, {}}, {{1,1}, {1,0}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_equality(LINE(""), graph, {{{edge_init_t{1,4}}, {edge_init_t{0,4}}}, {{1,1}, {1,0}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), graph, {{{edge_init_t{0,1,0,4}}, {edge_init_t{0,1,0,4}}}, {{1,1}, {1,0}}});
    }
    else
    {
      check_equality(LINE(""), graph, {edge_init_list_t{{edge_init_t{1,0,4}}, {edge_init_t{0,0,4}}}, {{1,1}, {1,0}}});
    }

    subgraph = sub_graph(graph, [](auto&& wt) { return wt == NodeWeight(1, 1); });

    // Subgraph
    // (1,1)
    //   X

    check_equality(LINE("Second node removed, leaving first in subgraph"), subgraph, {edge_init_list_t{{}}, {{1,1}}});

    subgraph = sub_graph(graph, [](auto&& wt) { return wt == NodeWeight(1, 0); });

    // Subgraph
    // (1,0)
    //   X

    check_equality(LINE("Node retained"), subgraph, {edge_init_list_t{{}}, {{1,0}}});


    graph.join(0, 0, 2);

    // Graph: 
    // (1,1)  4  (1,0)
    //   X---------X
    //  /\
    //  \/ 2

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), graph, {{{edge_init_t{1, 4}, edge_init_t{0, 2}}, {}}, {{1,1}, {1,0}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t g{{{edge_init_t{0, 2}, edge_init_t{0, 2}, edge_init_t{1,4}},
              {edge_init_t{0,4}}}, {{1,1}, {1,0}}};

      g.swap_edges(0, 0, 2);
      
      check_equality(LINE(""), graph, g);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), graph, {{{edge_init_t{0,1,0,4}, edge_init_t{0,0,2,2}, edge_init_t{0,0,1,2}},
                              {edge_init_t{0,1,0,4}}}, {{1,1}, {1,0}}});
    }
    else
    {
      check_equality(LINE(""), graph, {{{edge_init_t{1,0,4}, edge_init_t{0,2,2}, edge_init_t{0,1,2}},
                              {edge_init_t{0,0,4}}}, {{1,1}, {1,0}}});
    }

    subgraph = sub_graph(graph, [](auto&& wt) { return wt == NodeWeight(1, 1); });

    // Subgraph
    // (1,1)
    //   X
    //  /\
    //  \/ 2

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), subgraph, {{{edge_init_t{0, 2}}}, {{1,1}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {      
      check_equality(LINE(""), subgraph, {{{edge_init_t{0, 2}, edge_init_t{0, 2}}}, {{1,1}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), subgraph, {{{edge_init_t{0,0,1,2}, edge_init_t{0,0,0,2}}}, {{1,1}}});
    }
    else
    {
      check_equality(LINE(""), subgraph, {{{edge_init_t{0,1,2}, edge_init_t{0,0,2}}}, {{1,1}}});
    }
    
    subgraph = sub_graph(graph, [](auto&& wt) { return wt == NodeWeight(1, 0); });

    // Subgraph
    // (1,0)
    //   X

    check_equality(LINE(""), subgraph, {edge_init_list_t{{}}, {{1,0}}});

    graph.add_node(1.0, 1.0);
    graph.join(0, 2, 0);
    graph.join(1, 2, -3);

    // Graph: 
    // (1,1)  4  (1,0)
    //   X---------X
    //  /\\       /
    // 2\/ \0    /
    //      \   /-3
    //       \ /
    //        X
    //       (1,1)
        
    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), graph, {{{edge_init_t{1, 4}, edge_init_t{0, 2}, edge_init_t{2,0}},
                              {edge_init_t{2,-3}},
                              {}}, {{1,1}, {1,0}, {1,1}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t g{{{edge_init_t{0, 2}, edge_init_t{0, 2}, edge_init_t{1,4}, edge_init_t{2,0}},
                {edge_init_t{0,4}, edge_init_t{2,-3}},
                {edge_init_t{0,0}, edge_init_t{1,-3}}}, {{1,1}, {1,0}, {1,1}}};

      g.swap_edges(0, 0, 2);
      
      check_equality(LINE(""), graph, g);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), graph, {{{edge_init_t{0,1,0,4}, edge_init_t{0,0,2,2}, edge_init_t{0,0,1,2}, edge_init_t{0,2,0,0}},
                              {edge_init_t{0,1,0,4}, edge_init_t{1,2,1,-3}},
                              {edge_init_t{0,2,3,0}, edge_init_t{1,2,1,-3}}}, {{1,1}, {1,0}, {1,1}}});
    }
    else
    {
      check_equality(LINE(""), graph, {{{edge_init_t{1,0,4}, edge_init_t{0,2,2}, edge_init_t{0,1,2}, edge_init_t{2,0,0}},
                              {edge_init_t{0,0,4}, edge_init_t{2,1,-3}},
                              {edge_init_t{0,3,0}, edge_init_t{1,1,-3}}}, {{1,1}, {1,0}, {1,1}}});
    }
      
    subgraph = sub_graph(graph, [](auto&& wt) { return wt == NodeWeight(1, 1); });
    // subgraph: 
    // (1,1)
    //   X
    //  /\\
    // 2\/ \0
    //      \
    //       \
    //        X
    //       (1,1)
        
    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), subgraph, {{{edge_init_t{0, 2}, edge_init_t{1,0}},
                                 {}}, {{1,1}, {1,1}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {      
      check_equality(LINE(""),subgraph, {{{edge_init_t{0, 2}, edge_init_t{0, 2}, edge_init_t{1,0}},
                                 {edge_init_t{0,0}}}, {{1,1}, {1,1}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""),subgraph, {{{edge_init_t{0,0,1,2}, edge_init_t{0,0,0,2}, edge_init_t{0,1,0,0}},
                                 {edge_init_t{0,1,2,0}}}, {{1,1}, {1,1}}});
    }
    else
    {
      check_equality(LINE(""),subgraph, {{{edge_init_t{0,1,2}, edge_init_t{0,0,2}, edge_init_t{1,0,0}},
                                 {edge_init_t{0,2,0}}}, {{1,1}, {1,1}}});
    }

    subgraph = sub_graph(graph, [](auto&& wt) { return wt == NodeWeight(1, 0); });

    // Subgraph
    // (1,0)
    //   X

    check_equality(LINE(""), subgraph, {edge_init_list_t{{}}, {{1,0}}});
  }
}
