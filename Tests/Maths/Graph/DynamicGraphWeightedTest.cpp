////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "DynamicGraphWeightedTest.hpp"

#include <complex>

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view weighted_graph_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void weighted_graph_test::run_tests()
  {
    using std::complex;
    using namespace maths;

    {
      graph_test_helper<int, complex<double>, weighted_graph_test> helper{*this};
      helper.run_tests();
    }

    {
      graph_test_helper<std::vector<int>, std::vector<complex<double>>, weighted_graph_test> helper{*this};
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
  void weighted_graph_test::execute_operations()
  {
    using namespace maths;
    using ESTraits = EdgeStorageTraits;
    using NSTraits = NodeWeightStorageTraits;
    using graph_t = graph_type_generator_t<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightCreator, NodeWeightCreator, ESTraits, NSTraits>;
    using edge_init_t      = typename graph_t::edge_init_type;
    using edge_t           = typename graph_t::edge_type;
    using edge_init_list_t = std::initializer_list<std::initializer_list<edge_init_t>>;
    using edge_weight_t    = typename graph_t::edge_weight_type;
    using node_weight_t    = typename graph_t::node_weight_type;

    constexpr bool edgeDataPool{std::is_same_v<EdgeWeightCreator, object::data_pool<EdgeWeight>>};

    graph_t graph;

    graph.reserve_nodes(4);
    check(equality, LINE(""), graph.node_capacity(), 4_sz);
    graph.shrink_to_fit();
    check(equality, LINE("May fail if stl implementation doesn't actually shrink to fit!"), graph.node_capacity(), 0_sz);

    // NULL
    check_exception_thrown<std::out_of_range>(LINE("No nodes to erase"), [&graph](){ graph.erase_node(0); });
    check_exception_thrown<std::out_of_range>(LINE("No edges to erase"), [&graph](){ graph.erase_edge(graph.cbegin_edges(0)); });
    check_exception_thrown<std::out_of_range>(LINE("No nodes to set weight"), [&graph](){ graph.node_weight(graph.cend_node_weights(), 1.0); });
    check_exception_thrown<std::out_of_range>(LINE("No edges to set weight"), [&graph]() { graph.set_edge_weight(graph.cbegin_edges(0), EdgeWeight{1}); });

    check(equality, LINE(""), graph, graph_t{});

    check(equality, LINE("Node added with weight i"), graph.add_node(0.0, 1.0), 0_sz);
    //   (i)

    check(equality, LINE(""), graph, graph_t{edge_init_list_t{{}}, {{0,1}}});
    check_semantics(LINE("Regular semantics"), graph, graph_t{});
    check_semantics(LINE("Regular semantics"), graph, graph_t{edge_init_list_t{{}}, {{1,0}}});

    check_exception_thrown<std::out_of_range>(LINE("Throw if node out of range"), [&graph](){ graph.node_weight(graph.cend_node_weights(), 0.0); });

    graph.erase_node(0);
    //    NULL

    check(equality, LINE(""), graph, graph_t{});

    check(equality, LINE("Node of weight i recreated"), graph.add_node(0.0, 1.0), 0_sz);
    //    (i)

    check(equality, LINE(""), graph, graph_t{edge_init_list_t{{}}, {{0,1}}});

    graph.node_weight(graph.cbegin_node_weights(), 1.1, -4.3);
    // (1.1-i4.3)

    check(equality, LINE(""), graph, graph_t{edge_init_list_t{{}}, {{1.1,-4.3}}});

    graph.join(0, 0, EdgeWeight{1});
    //   /\ 1
    //   \/
    //  (1.1-i4.3)

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check(equality, LINE(""), graph, graph_t{edge_init_list_t{{{edge_init_t{0,1}}}}, {{1.1,-4.3}}});
      check_semantics(LINE("Regular semantics"), graph, graph_t{edge_init_list_t{{{edge_init_t{0,2}}}}, {{1.1,-4.3}}});
      check_semantics(LINE("Regular semantics"), graph, graph_t{edge_init_list_t{{{edge_init_t{0,1}}}}, {{-4.3, 1.1}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check(equality, LINE(""), graph, graph_t{{{edge_init_t{0,1}, edge_init_t{0,1}}}, {{1.1,-4.3}}});
      check_semantics(LINE("Regular semantics"), graph, graph_t{{{edge_init_t{0,2}, edge_init_t{0,2}}}, {{1.1,-4.3}}});
      check_semantics(LINE("Regular semantics"), graph, graph_t{{{edge_init_t{0,1}, edge_init_t{0,1}}}, {{-4.3,1.1}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check(equality, LINE(""), graph, graph_t{{{edge_init_t{0,0,1,1}, edge_init_t{0,0,0,1}}}, {{1.1,-4.3}}});
      check_semantics(LINE(""), graph, graph_t{{{edge_init_t{0,0,1,2}, edge_init_t{0,0,0,2}}}, {{1.1,-4.3}}});
      check_semantics(LINE(""), graph, graph_t{{{edge_init_t{0,0,1,1}, edge_init_t{0,0,0,1}}}, {{-4.3,1.1}}});
    }
    else
    {
      check(equality, LINE(""), graph, graph_t{{{edge_init_t{0,1,1}, edge_init_t{0,0,1}}}, {{1.1,-4.3}}});
      check_semantics(LINE(""), graph, graph_t{{{edge_init_t{0,1,2}, edge_init_t{0,0,2}}}, {{1.1,-4.3}}});
      check_semantics(LINE(""), graph, graph_t{{{edge_init_t{0,1,1}, edge_init_t{0,0,1}}}, {{-4.3,1.1}}});
    }

    check_semantics(LINE("Regular semantics"), graph, graph_t{});

    graph.set_edge_weight(graph.cbegin_edges(0), EdgeWeight{-2});
    //   /\ -2
    //   \/
    // (1.1-i4.3)

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check(equality, LINE(""), graph, graph_t{edge_init_list_t{{{edge_init_t{0,-2}}}}, {{1.1,-4.3}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check(equality, LINE(""), graph, graph_t{edge_init_list_t{{edge_init_t{0,-2}, edge_init_t{0,-2}}}, {{1.1,-4.3}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check(equality, LINE(""), graph, graph_t{edge_init_list_t{{edge_init_t{0,0,1,-2}, edge_init_t{0,0,0,-2}}}, {{1.1,-4.3}}});
    }
    else
    {
      check(equality, LINE(""), graph, graph_t{edge_init_list_t{{edge_init_t{0,1,-2}, edge_init_t{0,0,-2}}}, {{1.1,-4.3}}});
    }

    graph.set_edge_weight(graph.crbegin_edges(0), EdgeWeight{-4});
    //   /\ -4
    //   \/
    // (1.1-i4.3)

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check(equality, LINE(""), graph, graph_t{edge_init_list_t{{{edge_init_t{0,-4}}}}, {{1.1,-4.3}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check(equality, LINE(""), graph, graph_t{edge_init_list_t{{edge_init_t{0,-4}, edge_init_t{0,-4}}}, {{1.1,-4.3}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check(equality, LINE(""), graph, graph_t{edge_init_list_t{{edge_init_t{0,0,1,-4}, edge_init_t{0,0,0,-4}}}, {{1.1,-4.3}}});
    }
    else
    {
      check(equality, LINE(""), graph, graph_t{edge_init_list_t{{edge_init_t{0,1,-4}, edge_init_t{0,0,-4}}}, {{1.1,-4.3}}});
    }

    check(equality,
          LINE(""),
          graph.mutate_edge_weight(graph.cbegin_edges(0),
              [](auto& val) -> int {
                if constexpr(faithful_range<edge_weight_t>)
                {
                  val[0] *= 2;
                }
                else
                {
                  val *= 2;
                }

                return 42;
            }
          ),
          42
    );

    //   /\ -8
    //   \/
    // (1.1-i4.3)

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check(equality, LINE(""), graph, graph_t{edge_init_list_t{{{edge_init_t{0,-8}}}}, {{1.1,-4.3}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check(equality, LINE(""), graph, graph_t{edge_init_list_t{{edge_init_t{0,-8}, edge_init_t{0,-8}}}, {{1.1,-4.3}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check(equality, LINE(""), graph, graph_t{edge_init_list_t{{edge_init_t{0,0,1,-8}, edge_init_t{0,0,0,-8}}}, {{1.1,-4.3}}});
    }
    else
    {
      check(equality, LINE(""), graph, graph_t{edge_init_list_t{{edge_init_t{0,1,-8}, edge_init_t{0,0,-8}}}, {{1.1,-4.3}}});
    }

    graph.erase_edge(graph.cbegin_edges(0));
    //  (1.1-i4.3)

    check(equality, LINE(""), graph, graph_t{edge_init_list_t{{}}, {{1.1,-4.3}}});

    graph.join(0, 0, EdgeWeight{1});
    //   /\ 1
    //   \/
    // (1.1-i4.3)

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check(equality, LINE(""), graph, graph_t{edge_init_list_t{{{edge_init_t{0,1}}}}, {{1.1,-4.3}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check(equality, LINE(""), graph, graph_t{{{edge_init_t{0,1}, edge_init_t{0,1}}}, {{1.1,-4.3}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check(equality, LINE(""), graph, graph_t{{{edge_init_t{0,0,1,1}, edge_init_t{0,0,0,1}}}, {{1.1,-4.3}}});
    }
    else
    {
      check(equality, LINE(""), graph, graph_t{{{edge_init_t{0,1,1}, edge_init_t{0,0,1}}}, {{1.1,-4.3}}});
    }

    graph.join(0, 0, EdgeWeight{-1});
    //   /\ 1
    //   \/
    // (1.1-i4.3)
    //   /\
    //   \/ -1

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check(equality, LINE(""), graph, graph_t{{{edge_init_t{0,1}, edge_init_t{0,-1}}}, {{1.1,-4.3}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t g{{{edge_init_t{0,1}, edge_init_t{0,1}, edge_init_t{0,-1}, edge_init_t{0,-1}}}, {{1.1,-4.3}}};

      if constexpr(std::totally_ordered<edge_weight_t>)
      {
        g.sort_edges(g.cbegin_edges(0), g.cend_edges(0),
                   [](const auto& l , const auto& r){ return l.weight() > r.weight(); }
        );
      }
      check(equality, LINE(""), graph, g);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check(equality, LINE(""), graph, graph_t{{{edge_init_t{0,0,1,1}, edge_init_t{0,0,0,1}, edge_init_t{0,0,3,-1}, edge_init_t{0,0,2,-1}}}, {{1.1,-4.3}}});
    }
    else
    {
      check(equality, LINE(""), graph, graph_t{{{edge_init_t{0,1,1}, edge_init_t{0,0,1}, edge_init_t{0,3,-1}, edge_init_t{0,2,-1}}}, {{1.1,-4.3}}});
    }

    mutual_info(GraphFlavour) ? graph.set_edge_weight(graph.cbegin_edges(0) + 2, EdgeWeight{-4}) : graph.set_edge_weight(++graph.cbegin_edges(0), EdgeWeight{-4});
    //   /\ 1
    //   \/
    // (1.1-i4.3)
    //   /\
    //   \/ -4

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check(equality, LINE(""), graph, graph_t{{{edge_init_t{0,1}, edge_init_t{0,-4}}}, {{1.1,-4.3}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t g{{{edge_init_t{0,1}, edge_init_t{0,1}, edge_init_t{0,-4}, edge_init_t{0,-4}}}, {{1.1,-4.3}}};

      if constexpr(std::totally_ordered<edge_weight_t>)
      {
        g.sort_edges(g.cbegin_edges(0), g.cend_edges(0),
                   [](const auto& l , const auto& r){ return l.weight() > r.weight(); }
        );
      }
      check(equality, LINE(""), graph, g);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check(equality, LINE(""), graph, graph_t{{{edge_init_t{0,0,1,1}, edge_init_t{0,0,0,1}, edge_init_t{0,0,3,-4}, edge_init_t{0,0,2,-4}}}, {{1.1,-4.3}}});
    }
    else
    {
      check(equality, LINE(""), graph, graph_t{{{edge_init_t{0,1,1}, edge_init_t{0,0,1}, edge_init_t{0,3,-4}, edge_init_t{0,2,-4}}}, {{1.1,-4.3}}});
    }


    check(equality, LINE(""), graph.add_node(), 1_sz);
    //   /\ 1
    //   \/
    // (1.1-i4.3)   (0)
    //   /\
    //   \/ -4

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check(equality, LINE(""), graph, graph_t{{{edge_init_t{0,1}, edge_init_t{0,-4}}, {}}, {{1.1,-4.3}, {}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t g{{{edge_init_t{0,1}, edge_init_t{0,1}, edge_init_t{0,-4}, edge_init_t{0,-4}}, {}}, {{1.1,-4.3}, {}}};

      if constexpr(std::totally_ordered<edge_weight_t>)
      {
        g.sort_edges(g.cbegin_edges(0), g.cend_edges(0),
                   [](const auto& l , const auto& r){ return l.weight() > r.weight(); }
        );
      }
      check(equality, LINE(""), graph, g);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check(equality, LINE(""), graph, graph_t{{{edge_init_t{0,0,1,1}, edge_init_t{0,0,0,1}, edge_init_t{0,0,3,-4}, edge_init_t{0,0,2,-4}}, {}}, {{1.1,-4.3}, {}}});
    }
    else
    {
      check(equality, LINE(""), graph, graph_t{{{edge_init_t{0,1,1}, edge_init_t{0,0,1}, edge_init_t{0,3,-4}, edge_init_t{0,2,-4}}, {}}, {{1.1,-4.3}, {}}});
    }

    graph.join(0, 1, EdgeWeight{6});
    //   /\1
    //   \/  6
    // (1.1-i4.3)------(0)
    //   /\
    //   \/-4

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check(equality, LINE(""), graph, graph_t{{{edge_init_t{0,1}, edge_init_t{0,-4}, edge_init_t{1, 6}}, {}}, {{1.1,-4.3}, {}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t g{{{edge_init_t{0,1}, edge_init_t{0,1}, edge_init_t{0,-4}, edge_init_t{0,-4}, edge_init_t{1, 6}},
                {edge_init_t{0, 6}}
               }, {{1.1,-4.3}, {}}};

      if constexpr(std::totally_ordered<edge_weight_t>)
      {
        g.sort_edges(g.cbegin_edges(0), g.cend_edges(0) - 1,
                   [](const auto& l , const auto& r){ return l.weight() > r.weight(); }
        );
      }
      check(equality, LINE(""), graph, g);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check(equality, LINE(""), graph,
                     graph_t{{{edge_init_t{0,0,1,1}, edge_init_t{0,0,0,1}, edge_init_t{0,0,3,-4}, edge_init_t{0,0,2,-4}, edge_init_t{0,1,0,6}},
                             {edge_init_t{0,1,4,6}}
                              }, {{1.1,-4.3}, {}}});
    }
    else
    {
      check(equality, LINE(""), graph,
                     graph_t{{{edge_init_t{0,1,1}, edge_init_t{0,0,1}, edge_init_t{0,3,-4}, edge_init_t{0,2,-4}, edge_init_t{1,0,6}},
                             {edge_init_t{0,4,6}}
                               }, {{1.1,-4.3}, {}}});
    }

    graph.swap_nodes(0,1);

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check(equality, LINE(""), graph, graph_t{{{}, {edge_init_t{1,1}, edge_init_t{1,-4}, edge_init_t{0, 6}}}, {{}, {1.1,-4.3}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t g{{{edge_init_t{1,6}},
                {edge_init_t{1,1}, edge_init_t{1,1}, edge_init_t{1,-4}, edge_init_t{1,-4}, edge_init_t{0, 6}}
               }, {{}, {1.1,-4.3}}};

      g.sort_edges(g.cbegin_edges(1), g.cend_edges(1),
                   [](const auto& l , const auto& r){
                       if constexpr(std::totally_ordered<edge_weight_t>)
                       {
                         return (l.target_node() == r.target_node()) ? (l.weight() > r.weight()) : (l.target_node() > r.target_node());
                       }
                       else
                       {
                         return l.target_node() > r.target_node();
                       }
                   }
      );

      check(equality, LINE(""), graph, g);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check(equality, LINE(""), graph,
                     graph_t{{{edge_init_t{1,0,4,6}},
                             {edge_init_t{1,1,1,1}, edge_init_t{1,1,0,1}, edge_init_t{1,1,3,-4}, edge_init_t{1,1,2,-4}, edge_init_t{1,0,0,6}}
                            }, {{}, {1.1,-4.3}}});
    }
    else
    {
      check(equality, LINE(""), graph,
                     graph_t{{{edge_init_t{1,4,6}},
                             {edge_init_t{1,1,1}, edge_init_t{1,0,1}, edge_init_t{1,3,-4}, edge_init_t{1,2,-4}, edge_init_t{0,0,6}}
                       }, {{}, {1.1,-4.3}}});
    }

    graph.swap_nodes(1,0);

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check(equality, LINE(""), graph, graph_t{{{edge_init_t{0,1}, edge_init_t{0,-4}, edge_init_t{1, 6}}, {}}, {{1.1,-4.3}, {}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t g{{{edge_init_t{0,1}, edge_init_t{0,1}, edge_init_t{0,-4}, edge_init_t{0,-4}, edge_init_t{1, 6}},
                 {edge_init_t{0, 6}}
                }, {{1.1,-4.3}, {}}};

      if constexpr(std::totally_ordered<edge_weight_t>)
      {
        g.sort_edges(g.cbegin_edges(0), g.cend_edges(0) - 1,
                   [](const auto& l , const auto& r){ return l.weight() > r.weight(); }
        );
      }
      check(equality, LINE(""), graph, g);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check(equality, LINE(""), graph,
                     graph_t{{{edge_init_t{0,0,1,1}, edge_init_t{0,0,0,1}, edge_init_t{0,0,3,-4}, edge_init_t{0,0,2,-4}, edge_init_t{0,1,0,6}},
                             {edge_init_t{0,1,4,6}}
                            }, {{1.1,-4.3}, {}}});
    }
    else
    {
      check(equality, LINE(""), graph,
                     graph_t{{{edge_init_t{0,1,1}, edge_init_t{0,0,1}, edge_init_t{0,3,-4}, edge_init_t{0,2,-4}, edge_init_t{1,0,6}},
                             {edge_init_t{0,4,6}}
                            }, {{1.1,-4.3}, {}}});
    }

    graph.set_edge_weight(--graph.cend_edges(0), EdgeWeight{7});
    //   /\1
    //   \/  7
    // (1.1-i4.3)------(0)
    //   /\
    //   \/-4

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check(equality, LINE(""), graph, graph_t{{{edge_init_t{0,1}, edge_init_t{0,-4}, edge_init_t{1, 7}}, {}}, {{1.1,-4.3}, {}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t g{{{edge_init_t{0,1}, edge_init_t{0,1}, edge_init_t{0,-4}, edge_init_t{0,-4}, edge_init_t{1, 7}},
                {edge_init_t{0, 7}}
               }, {{1.1,-4.3}, {}}};

      if constexpr(std::totally_ordered<edge_weight_t>)
      {
        g.sort_edges(g.cbegin_edges(0), g.cend_edges(0) - 1,
                   [](const auto& l , const auto& r){ return l.weight() > r.weight(); }
        );
      }
      check(equality, LINE(""), graph, g);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check(equality, LINE(""), graph,
                     graph_t{{{edge_init_t{0,0,1,1}, edge_init_t{0,0,0,1}, edge_init_t{0,0,3,-4}, edge_init_t{0,0,2,-4}, edge_init_t{0,1,0,7}},
                             {edge_init_t{0,1,4,7}}
                            }, {{1.1,-4.3}, {}}});
    }
    else
    {
      check(equality, LINE(""), graph,
                     graph_t{{{edge_init_t{0,1,1}, edge_init_t{0,0,1}, edge_init_t{0,3,-4}, edge_init_t{0,2,-4}, edge_init_t{1,0,7}},
                             {edge_init_t{0,4,7}}
                            }, {{1.1,-4.3}, {}}});
    }

    graph.set_edge_weight(--graph.cend_edges(0), EdgeWeight{6});
    //   /\1
    //   \/  6
    // (1.1-i4.3)------(0)
    //   /\
    //   \/-4

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check(equality, LINE(""), graph, graph_t{{{edge_init_t{0,1}, edge_init_t{0,-4}, edge_init_t{1, 6}}, {}}, {{1.1,-4.3}, {}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t g{{{edge_init_t{0,1}, edge_init_t{0,1}, edge_init_t{0,-4}, edge_init_t{0,-4}, edge_init_t{1, 6}},
                {edge_init_t{0, 6}}
               }, {{1.1,-4.3}, {}}};

      if constexpr(std::totally_ordered<edge_weight_t>)
      {
        g.sort_edges(g.cbegin_edges(0), g.cend_edges(0) - 1,
                   [](const auto& l , const auto& r){ return l.weight() > r.weight(); }
        );
      }
      check(equality, LINE(""), graph, g);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check(equality, LINE(""), graph,
                     graph_t{{{edge_init_t{0,0,1,1}, edge_init_t{0,0,0,1}, edge_init_t{0,0,3,-4}, edge_init_t{0,0,2,-4}, edge_init_t{0,1,0,6}},
                             {edge_init_t{0,1,4,6}}
                            }, {{1.1,-4.3}, {}}});
    }
    else
    {
      check(equality, LINE(""), graph,
                     graph_t{{{edge_init_t{0,1,1}, edge_init_t{0,0,1}, edge_init_t{0,3,-4}, edge_init_t{0,2,-4}, edge_init_t{1,0,6}},
                             {edge_init_t{0,4,6}}
                            }, {{1.1,-4.3}, {}}});
    }

    graph.join(1, 0, EdgeWeight{7});
    //     /\1
    //     \/     6
    // (1.1-i4.3)======(0)
    //     /\     7
    //     \/-4

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check(equality, LINE(""), graph, graph_t{{{edge_init_t{0,1}, edge_init_t{0,-4}, edge_init_t{1, 6}}, {edge_init_t{0, 7}}}, {{1.1,-4.3}, {}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t g{{{edge_init_t{0,1}, edge_init_t{0,1}, edge_init_t{0,-4}, edge_init_t{0,-4}, edge_init_t{1, 7}, edge_init_t{1, 6}},
                {edge_init_t{0,7}, edge_init_t{0,6}}
               }, {{1.1,-4.3}, {}}};

      if constexpr(std::totally_ordered<edge_weight_t>)
      {
        g.sort_edges(g.cbegin_edges(0), g.cend_edges(0) - 2,
                   [](const auto& l , const auto& r){ return l.weight() > r.weight(); }
        );
      }

      check(equality, LINE(""), graph, g);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check(equality, LINE(""), graph,
                     graph_t{{{edge_init_t{0,0,1,1}, edge_init_t{0,0,0,1}, edge_init_t{0,0,3,-4}, edge_init_t{0,0,2,-4}, edge_init_t{0,1,0,6}, edge_init_t{1,0,1,7}},
                             {edge_init_t{0,1,4,6}, edge_init_t{1,0,5,7}}
                            }, {{1.1,-4.3}, {}}});
    }
    else
    {
      check(equality, LINE(""), graph,
                     graph_t{{{edge_init_t{0,1,1}, edge_init_t{0,0,1}, edge_init_t{0,3,-4}, edge_init_t{0,2,-4}, edge_init_t{1,0,6}, edge_init_t{1,1,7}},
                             {edge_init_t{0,4,6}, edge_init_t{0,5,7}}
                            }, {{1.1,-4.3}, {}}});
    }


    graph.join(0, 1, EdgeWeight{8});
    //     /\1   /--\8
    //     \/   /6   \
    // (1.1-i4.3)====(0)
    //     /\     7
    //     \/-4

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check(equality, LINE(""), graph,
                     graph_t{{{edge_init_t{0,1}, edge_init_t{0,-4}, edge_init_t{1, 6}, edge_init_t{1,8}},
                              {edge_init_t{0, 7}}
                             }, {{1.1,-4.3}, {}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t g{{{edge_init_t{0,1}, edge_init_t{0,1}, edge_init_t{0,-4}, edge_init_t{0,-4}, edge_init_t{1, 8}, edge_init_t{1, 6}, edge_init_t{1,7}},
                 {edge_init_t{0,8}, edge_init_t{0,6}, edge_init_t{0,7}}
               }, {{1.1,-4.3}, {}}};

      if constexpr(std::totally_ordered<edge_weight_t>)
      {
        g.sort_edges(g.cbegin_edges(0), g.cend_edges(0) - 3,
                   [](const auto& l , const auto& r){ return l.weight() > r.weight(); }
        );
      }

      check(equality, LINE(""), graph, g);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check(equality, LINE(""), graph,
                     graph_t{{{edge_init_t{0,0,1,1}, edge_init_t{0,0,0,1}, edge_init_t{0,0,3,-4}, edge_init_t{0,0,2,-4}, edge_init_t{0,1,0,6}, edge_init_t{1,0,1,7}, edge_init_t{0,1,2,8}},
                           {edge_init_t{0,1,4,6}, edge_init_t{1,0,5,7}, edge_init_t{0,1,6,8}}
                            }, {{1.1,-4.3}, {}}});
    }
    else
    {
      check(equality, LINE(""), graph,
                     graph_t{{{edge_init_t{0,1,1}, edge_init_t{0,0,1}, edge_init_t{0,3,-4}, edge_init_t{0,2,-4}, edge_init_t{1,0,6}, edge_init_t{1,1,7}, edge_init_t{1,2,8}},
                             {edge_init_t{0,4,6}, edge_init_t{0,5,7}, edge_init_t{0,6,8}}
                            }, {{1.1,-4.3}, {}}});
    }

    graph.erase_edge(graph.cbegin_edges(0));
    //            8
    //           /--\
    //          / 6  \
    // (1.1-i4.3)====(0)
    //     /\     7
    //     \/-4

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check(equality, LINE(""), graph,
                     graph_t{{{edge_init_t{0,-4}, edge_init_t{1, 6}, edge_init_t{1,8}},
                             {edge_init_t{0, 7}}
                           }, {{1.1,-4.3}, {}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t g{{{edge_init_t{0,-4}, edge_init_t{0,-4}, edge_init_t{1, 8}, edge_init_t{1, 6}, edge_init_t{1,7}},
                {edge_init_t{0,8}, edge_init_t{0,6}, edge_init_t{0,7}}
               }, {{1.1,-4.3}, {}}};

      check(equality, LINE(""), graph, g);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check(equality, LINE(""), graph,
                     graph_t{{{edge_init_t{0,0,1,-4}, edge_init_t{0,0,0,-4}, edge_init_t{0,1,0,6}, edge_init_t{1,0,1,7}, edge_init_t{0,1,2,8}},
                             {edge_init_t{0,1,2,6}, edge_init_t{1,0,3,7}, edge_init_t{0,1,4,8}}
                            }, {{1.1,-4.3}, {}}});
    }
    else
    {
      check(equality, LINE(""), graph,
                     graph_t{{{edge_init_t{0,1,-4}, edge_init_t{0,0,-4}, edge_init_t{1,0,6}, edge_init_t{1,1,7}, edge_init_t{1,2,8}},
                             {edge_init_t{0,2,6}, edge_init_t{0,3,7}, edge_init_t{0,4,8}}
                            }, {{1.1,-4.3}, {}}});
    }

    graph.mutate_edge_weight(graph.cbegin_edges(0) + (mutual_info(GraphFlavour) ?  3 : 2),
      [](auto& val){
        if constexpr(faithful_range<edge_weight_t>)
        {
          val[0] = 6;
        }
        else
        {
          val = 6;
        }
      }
    );

    //            8 (6, directed)
    //           /--\
    //          /6(7)\
    // (1.1-i4.3)====(0)
    //     /\     6
    //     \/-4

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check(equality, LINE(""), graph,
                     graph_t{{{edge_init_t{0,-4}, edge_init_t{1, 6}, edge_init_t{1,6}},
                             {edge_init_t{0, 7}}
                           }, {{1.1,-4.3}, {}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t g{{{edge_init_t{0,-4}, edge_init_t{0,-4}, edge_init_t{1, 8}, edge_init_t{1, 6}, edge_init_t{1,6}},
                {edge_init_t{0,8}, edge_init_t{0,6}, edge_init_t{0,6}}
               }, {{1.1,-4.3}, {}}};

      check(equality, LINE(""), graph, g);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check(equality, LINE(""), graph,
                     graph_t{{{edge_init_t{0,0,1,-4}, edge_init_t{0,0,0,-4}, edge_init_t{0,1,0,6}, edge_init_t{1,0,1,6}, edge_init_t{0,1,2,8}},
                             {edge_init_t{0,1,2,6}, edge_init_t{1,0,3,6}, edge_init_t{0,1,4,8}}
                            }, {{1.1,-4.3}, {}}});
    }
    else
    {
      check(equality, LINE(""), graph,
                     graph_t{{{edge_init_t{0,1,-4}, edge_init_t{0,0,-4}, edge_init_t{1,0,6}, edge_init_t{1,1,6}, edge_init_t{1,2,8}},
                             {edge_init_t{0,2,6}, edge_init_t{0,3,6}, edge_init_t{0,4,8}}
                            }, {{1.1,-4.3}, {}}});
    }

    graph.set_edge_weight(graph.cbegin_edges(0) + (mutual_info(GraphFlavour) ?  3 : 2), EdgeWeight{10});
    //            8 (10, directed)
    //           /--\
    //          /10(7)\
    // (1.1-i4.3)====(0)
    //     /\     6
    //     \/-4

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check(equality, LINE(""), graph,
                     graph_t{{{edge_init_t{0,-4}, edge_init_t{1, 6}, edge_init_t{1,10}},
                             {edge_init_t{0, 7}}
                           }, {{1.1,-4.3}, {}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t g{{{edge_init_t{0,-4}, edge_init_t{0,-4}, edge_init_t{1, 8}, edge_init_t{1, 6}, edge_init_t{1,10}},
                {edge_init_t{0,8}, edge_init_t{0,10}, edge_init_t{0,6}}
               }, {{1.1,-4.3}, {}}};

      if constexpr(std::totally_ordered<edge_weight_t>)
      {
        g.sort_edges(g.cbegin_edges(0) + 3, g.cend_edges(0),
                   [](const auto& l , const auto& r){ return l.weight() > r.weight(); }
        );

        g.sort_edges(g.cbegin_edges(1), g.cend_edges(1),
                   [](const auto& l , const auto& r){ return l.weight() > r.weight(); }
        );

        g.sort_edges(g.cbegin_edges(1) + 1, g.cend_edges(1),
                   [](const auto& l , const auto& r){ return l.weight() < r.weight(); }
        );
      }

      if constexpr(!edgeDataPool && !std::is_same_v<edge_init_t, edge_t>)
      {
        g.swap_edges(1, 0, 1);
      }

      check(equality, LINE(""), graph, g);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check(equality, LINE(""), graph,
                     graph_t{{{edge_init_t{0,0,1,-4}, edge_init_t{0,0,0,-4}, edge_init_t{0,1,0,6}, edge_init_t{1,0,1,10}, edge_init_t{0,1,2,8}},
                             {edge_init_t{0,1,2,6}, edge_init_t{1,0,3,10}, edge_init_t{0,1,4,8}}
                            }, {{1.1,-4.3}, {}}});
    }
    else
    {
      check(equality, LINE(""), graph,
                     graph_t{{{edge_init_t{0,1,-4}, edge_init_t{0,0,-4}, edge_init_t{1,0,6}, edge_init_t{1,1,10}, edge_init_t{1,2,8}},
                             {edge_init_t{0,2,6}, edge_init_t{0,3,10}, edge_init_t{0,4,8}}
                            }, {{1.1,-4.3}, {}}});
    }

    graph.mutate_edge_weight(graph.cbegin_edges(0) + (mutual_info(GraphFlavour) ?  4 : 1),
      [](auto& val){
        if constexpr(faithful_range<edge_weight_t>)
        {
          val[0] = 10;
        }
        else
        {
          val = 10;
        }
      }
    );

    //            10
    //           /--\
    //          / 10 \
    // (1.1-i4.3)====(0)
    //     /\     6 (10)
    //     \/-4

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check(equality, LINE(""), graph,
                     graph_t{{{edge_init_t{0,-4}, edge_init_t{1, 10}, edge_init_t{1,10}},
                             {edge_init_t{0, 7}}
                           }, {{1.1,-4.3}, {}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t g{{{edge_init_t{0,-4}, edge_init_t{0,-4}, edge_init_t{1,10}, edge_init_t{1,6}, edge_init_t{1,10}},
                {edge_init_t{0,6}, edge_init_t{0,10}, edge_init_t{0,10}}
               }, {{1.1,-4.3}, {}}};

      if constexpr(std::totally_ordered<edge_weight_t>)
      {
        g.sort_edges(g.cbegin_edges(1), g.cbegin_edges(1) + 2,
                   [](const auto& l , const auto& r){ return l.weight() > r.weight(); }
        );

        if constexpr(!edgeDataPool && !std::is_same_v<edge_init_t, edge_t>)
        {
          g.swap_edges(1, 0, 1);
        }
      }
      else
      {
        g.swap_edges(1, 1, 2);
      }

      check(equality, LINE(""), graph, g);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check(equality, LINE(""), graph,
                     graph_t{{{edge_init_t{0,0,1,-4}, edge_init_t{0,0,0,-4}, edge_init_t{0,1,0,6}, edge_init_t{1,0,1,10}, edge_init_t{0,1,2,10}},
                             {edge_init_t{0,1,2,6}, edge_init_t{1,0,3,10}, edge_init_t{0,1,4,10}}
                            }, {{1.1,-4.3}, {}}});
    }
    else
    {
      check(equality, LINE(""), graph,
                     graph_t{{{edge_init_t{0,1,-4}, edge_init_t{0,0,-4}, edge_init_t{1,0,6}, edge_init_t{1,1,10}, edge_init_t{1,2,10}},
                             {edge_init_t{0,2,6}, edge_init_t{0,3,10}, edge_init_t{0,4,10}}
                            }, {{1.1,-4.3}, {}}});
    }

    graph.mutate_edge_weight(graph.cbegin_edges(0) + (mutual_info(GraphFlavour) ?  4 : 2),
      [](auto& val){
        if constexpr(faithful_range<edge_weight_t>)
        {
          val[0] = 7;
        }
        else
        {
          val = 7;
        }
      }
    );
    //            7
    //           /--\
    //          / 10 \
    // (1.1-i4.3)====(0)
    //     /\     6(7)
    //     \/-4

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check(equality, LINE(""), graph,
                     graph_t{{{edge_init_t{0,-4}, edge_init_t{1, 10}, edge_init_t{1,7}},
                             {edge_init_t{0, 7}}
                           }, {{1.1,-4.3}, {}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t g{{{edge_init_t{0,-4}, edge_init_t{0,-4}, edge_init_t{1, 6}, edge_init_t{1, 10}, edge_init_t{1,7}},
                {edge_init_t{0,6}, edge_init_t{0,10}, edge_init_t{0,7}}
               }, {{1.1,-4.3}, {}}};

      if constexpr(std::totally_ordered<edge_weight_t>)
      {
        g.sort_edges(g.cbegin_edges(0) + 3, g.cend_edges(0),
                   [](const auto& l , const auto& r){ return l.weight() > r.weight(); }
        );

        g.sort_edges(g.cbegin_edges(1), g.cbegin_edges(1) + 2,
                   [](const auto& l , const auto& r){ return l.weight() > r.weight(); }
        );

        if constexpr(!edgeDataPool && !std::is_same_v<edge_init_t, edge_t>)
        {
          g.swap_edges(1, 0, 1);
          g.swap_edges(1, 1, 2);
        }
      }
      else
      {
        g.swap_edges(0, 2, 4);
        g.swap_edges(0, 3, 4);
        g.swap_edges(1, 0, 1);
        g.swap_edges(1, 1, 2);
      }


      check(equality, LINE(""), graph, g);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check(equality, LINE(""), graph,
                     graph_t{{{edge_init_t{0,0,1,-4}, edge_init_t{0,0,0,-4}, edge_init_t{0,1,0,6}, edge_init_t{1,0,1,10}, edge_init_t{0,1,2,7}},
                             {edge_init_t{0,1,2,6}, edge_init_t{1,0,3,10}, edge_init_t{0,1,4,7}}
                            }, {{1.1,-4.3}, {}}});
    }
    else
    {
      check(equality, LINE(""), graph,
                     graph_t{{{edge_init_t{0,1,-4}, edge_init_t{0,0,-4}, edge_init_t{1,0,6}, edge_init_t{1,1,10}, edge_init_t{1,2,7}},
                             {edge_init_t{0,2,6}, edge_init_t{0,3,10}, edge_init_t{0,4,7}}
                            }, {{1.1,-4.3}, {}}});
    }

    graph.erase_node(0);
    //  (0)

    check(equality, LINE(""), graph, graph_t{edge_init_list_t{{}}, {{}}});


    graph.join(0, 0, EdgeWeight{1});
    //   /\ 1
    //   \/
    //   (0)

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check(equality, LINE(""), graph, graph_t{edge_init_list_t{{edge_init_t{0,1}}}, {{}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check(equality, LINE(""), graph, graph_t{{{edge_init_t{0,1}, edge_init_t{0,1}}}, {{}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check(equality, LINE(""), graph, graph_t{{{edge_init_t{0,0,1,1}, edge_init_t{0,0,0,1}}}, {{}}});
    }
    else
    {
      check(equality, LINE(""), graph, graph_t{{{edge_init_t{0,1,1}, edge_init_t{0,0,1}}}, {{}}});
    }

    check(equality, LINE("Node with weight 1+i inserted into slot 0"), graph.insert_node(0, 1.0, 1.0), 0_sz);
    //       /\ 1
    //       \/
    // (1+i) (0)

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check(equality, LINE(""), graph, graph_t{edge_init_list_t{{}, {edge_init_t{1,1}}}, {{1, 1}, {}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check(equality, LINE(""), graph, graph_t{edge_init_list_t{{}, {edge_init_t{1,1}, edge_init_t{1,1}}}, {{1, 1}, {}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check(equality, LINE(""), graph, graph_t{edge_init_list_t{{}, {edge_init_t{1,1,1,1}, edge_init_t{1,1,0,1}}}, {{1, 1}, {}}});
    }
    else
    {
      check(equality, LINE(""), graph, graph_t{edge_init_list_t{{}, {edge_init_t{1,1,1}, edge_init_t{1,0,1}}}, {{1, 1}, {}}});
    }

    check(equality,
          LINE(""),
          graph.mutate_node_weight(graph.cbegin_node_weights(), [](auto& val) -> int {
              if constexpr(faithful_range<node_weight_t>)
              {
                val[0] *= 2;
                val[1] *= 2;
              }
              else
              {
                val *= 2;
              }

              return 42;
            }
          ),
          42
    );

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check(equality, LINE(""), graph, graph_t{edge_init_list_t{{}, {edge_init_t{1,1}}}, {{2, 2}, {}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check(equality, LINE(""), graph, graph_t{edge_init_list_t{{}, {edge_init_t{1,1}, edge_init_t{1,1}}}, {{2, 2}, {}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check(equality, LINE(""), graph, graph_t{edge_init_list_t{{}, {edge_init_t{1,1,1,1}, edge_init_t{1,1,0,1}}}, {{2, 2}, {}}});
    }
    else
    {
      check(equality, LINE(""), graph, graph_t{edge_init_list_t{{}, {edge_init_t{1,1,1}, edge_init_t{1,0,1}}}, {{2, 2}, {}}});
    }
  }
}
