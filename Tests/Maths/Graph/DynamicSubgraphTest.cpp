////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "DynamicSubgraphTest.hpp"

#include "sequoia/Maths/Graph/GraphAlgorithms.hpp"

#include <complex>

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view test_subgraph::source_file() const noexcept
  {
    return __FILE__;
  }

  void test_subgraph::run_tests()
  {
    using std::complex;

    {
      graph_test_helper<int, complex<double>, test_subgraph>  helper{*this};
      helper.run_tests();
    }

    {
      graph_test_helper<complex<int>, complex<double>, test_subgraph>  helper{*this};
      helper.run_tests();
    }
  }

  template
  <
    maths::graph_flavour GraphFlavour,
    class EdgeWeight,
    class NodeWeight,
    class EdgeWeightCreator,
    class NodeWeightCreator,
    class EdgeStorageTraits,
    class NodeWeightStorageTraits
  >
  void test_subgraph::execute_operations()
  {
    using std::complex;
    using namespace maths;
    using ESTraits = EdgeStorageTraits;
    using NSTraits = NodeWeightStorageTraits;
    using graph_type = graph_type_generator_t<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightCreator, NodeWeightCreator, ESTraits, NSTraits>;
    using edge_init_t = typename graph_type::edge_init_type;
    using edge_init_list_t = std::initializer_list<std::initializer_list<edge_init_t>>;

    graph_type graph{};
    graph.add_node(1.0, 1.0);

    // Graph:
    // (1,1)
    //   X

    check(equality, LINE(""), graph, {edge_init_list_t{{}}, {{1,1}}});

    auto subgraph = sub_graph(graph, [](auto&& wt) { return wt == NodeWeight(1, 1); });

    // Subgraph
    // (1,1)
    //   X

    check(equality, LINE("Subgraph same as parent"), subgraph, {edge_init_list_t{{}}, {{1,1}}});

    subgraph = sub_graph(graph, [](auto&& wt) { return wt == NodeWeight(0, 1); });
    // Subgraph
    //   NULL

    check(equality, LINE("Null subgraph"), subgraph, {});

    graph.add_node(1.0, 0.0);

    // Graph:
    // (1,1) (1,0)
    //   X     X

    check(equality, LINE(""), graph, {edge_init_list_t{{}, {}}, {{1,1}, {1,0}}});

    subgraph = sub_graph(graph, [](auto&& wt) { return wt == NodeWeight(1, 1); });

    // Subgraph
    // (1,1)
    //   X

    check(equality, LINE(""), subgraph, {edge_init_list_t{{}}, {{1,1}}});

    subgraph = sub_graph(graph, [](auto&& wt) { return wt == NodeWeight(1, 0); });

    // Subgraph
    // (1,0)
    //   X

    check(equality, LINE(""), subgraph, {edge_init_list_t{{}}, {{1,0}}});

    graph.join(0, 1, 4);

    // Graph:
    // (1,1)  4  (1,0)
    //   X---------X

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check(equality, LINE(""), graph, {edge_init_list_t{{edge_init_t{1, 4}}, {}}, {{1,1}, {1,0}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check(equality, LINE(""), graph, {edge_init_list_t{{edge_init_t{1,4}}, {edge_init_t{0,4}}}, {{1,1}, {1,0}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check(equality, LINE(""), graph, {edge_init_list_t{{edge_init_t{0,1,0,4}}, {edge_init_t{0,1,0,4}}}, {{1,1}, {1,0}}});
    }
    else
    {
      check(equality, LINE(""), graph, {edge_init_list_t{{edge_init_t{1,0,4}}, {edge_init_t{0,0,4}}}, {{1,1}, {1,0}}});
    }

    subgraph = sub_graph(graph, [](auto&& wt) { return wt == NodeWeight(1, 1); });

    // Subgraph
    // (1,1)
    //   X

    check(equality, LINE("Second node removed, leaving first in subgraph"), subgraph, {edge_init_list_t{{}}, {{1,1}}});

    subgraph = sub_graph(graph, [](auto&& wt) { return wt == NodeWeight(1, 0); });

    // Subgraph
    // (1,0)
    //   X

    check(equality, LINE("Node retained"), subgraph, {edge_init_list_t{{}}, {{1,0}}});


    graph.join(0, 0, 2);

    // Graph:
    // (1,1)  4  (1,0)
    //   X---------X
    //  /\
    //  \/ 2

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check(equality, LINE(""), graph, {{{edge_init_t{1, 4}, edge_init_t{0, 2}}, {}}, {{1,1}, {1,0}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_type g{{{edge_init_t{0, 2}, edge_init_t{0, 2}, edge_init_t{1,4}},
              {edge_init_t{0,4}}}, {{1,1}, {1,0}}};

      g.swap_edges(0, 0, 2);

      check(equality, LINE(""), graph, g);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check(equality, LINE(""), graph, {{{edge_init_t{0,1,0,4}, edge_init_t{0,0,2,2}, edge_init_t{0,0,1,2}},
                              {edge_init_t{0,1,0,4}}}, {{1,1}, {1,0}}});
    }
    else
    {
      check(equality, LINE(""), graph, {{{edge_init_t{1,0,4}, edge_init_t{0,2,2}, edge_init_t{0,1,2}},
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
      check(equality, LINE(""), subgraph, {edge_init_list_t{{edge_init_t{0, 2}}}, {{1,1}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check(equality, LINE(""), subgraph, {edge_init_list_t{{edge_init_t{0, 2}, edge_init_t{0, 2}}}, {{1,1}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check(equality, LINE(""), subgraph, {edge_init_list_t{{edge_init_t{0,0,1,2}, edge_init_t{0,0,0,2}}}, {{1,1}}});
    }
    else
    {
      check(equality, LINE(""), subgraph, {edge_init_list_t{{edge_init_t{0,1,2}, edge_init_t{0,0,2}}}, {{1,1}}});
    }

    subgraph = sub_graph(graph, [](auto&& wt) { return wt == NodeWeight(1, 0); });

    // Subgraph
    // (1,0)
    //   X

    check(equality, LINE(""), subgraph, {edge_init_list_t{{}}, {{1,0}}});

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
      check(equality, LINE(""), graph, {{{edge_init_t{1, 4}, edge_init_t{0, 2}, edge_init_t{2,0}},
                              {edge_init_t{2,-3}},
                              {}}, {{1,1}, {1,0}, {1,1}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_type g{{{edge_init_t{0, 2}, edge_init_t{0, 2}, edge_init_t{1,4}, edge_init_t{2,0}},
                {edge_init_t{0,4}, edge_init_t{2,-3}},
                {edge_init_t{0,0}, edge_init_t{1,-3}}}, {{1,1}, {1,0}, {1,1}}};

      g.swap_edges(0, 0, 2);

      check(equality, LINE(""), graph, g);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check(equality, LINE(""), graph, {{{edge_init_t{0,1,0,4}, edge_init_t{0,0,2,2}, edge_init_t{0,0,1,2}, edge_init_t{0,2,0,0}},
                              {edge_init_t{0,1,0,4}, edge_init_t{1,2,1,-3}},
                              {edge_init_t{0,2,3,0}, edge_init_t{1,2,1,-3}}}, {{1,1}, {1,0}, {1,1}}});
    }
    else
    {
      check(equality, LINE(""), graph, {{{edge_init_t{1,0,4}, edge_init_t{0,2,2}, edge_init_t{0,1,2}, edge_init_t{2,0,0}},
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
      check(equality, LINE(""), subgraph, {{{edge_init_t{0, 2}, edge_init_t{1,0}},
                                 {}}, {{1,1}, {1,1}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check(equality, LINE(""),subgraph, {{{edge_init_t{0, 2}, edge_init_t{0, 2}, edge_init_t{1,0}},
                                 {edge_init_t{0,0}}}, {{1,1}, {1,1}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check(equality, LINE(""),subgraph, {{{edge_init_t{0,0,1,2}, edge_init_t{0,0,0,2}, edge_init_t{0,1,0,0}},
                                 {edge_init_t{0,1,2,0}}}, {{1,1}, {1,1}}});
    }
    else
    {
      check(equality, LINE(""),subgraph, {{{edge_init_t{0,1,2}, edge_init_t{0,0,2}, edge_init_t{1,0,0}},
                                 {edge_init_t{0,2,0}}}, {{1,1}, {1,1}}});
    }

    subgraph = sub_graph(graph, [](auto&& wt) { return wt == NodeWeight(1, 0); });

    // Subgraph
    // (1,0)
    //   X

    check(equality, LINE(""), subgraph, {edge_init_list_t{{}}, {{1,0}}});
  }
}
