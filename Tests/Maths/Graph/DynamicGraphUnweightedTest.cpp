////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "DynamicGraphUnweightedTest.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view unweighted_graph_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void unweighted_graph_test::run_tests()
  {
    struct null_weight {};

    graph_test_helper<null_weight, null_weight, unweighted_graph_test> helper{*this};

    helper.run_tests();
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
  void unweighted_graph_test::execute_operations()
  {
    using namespace maths;
    using ESTraits = EdgeStorageTraits;
    using NSTraits = NodeWeightStorageTraits;
    using graph_t = graph_type_generator_t<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightCreator, NodeWeightCreator, ESTraits, NSTraits>;
    using edge_init_t = typename graph_t::edge_init_type;

    graph_t g;

    check_exception_thrown<std::out_of_range>(LINE("cbegin_edges throws for empty graph"), [&g]() { return g.cbegin_edges(0); });
    check_exception_thrown<std::out_of_range>(LINE("cend_edges throws for empty graph"), [&g]() { return g.cend_edges(0); });

    check_exception_thrown<std::out_of_range>(LINE("swapping nodes throws for empty graph"), [&g]() { g.swap_nodes(0,0); });

    check_equality(LINE("Index of added node is 0"), g.add_node(), 0_sz);
    //    0

    check_equality(LINE(""), g, graph_t{{}});
    check_semantics(LINE("Regular semantics"), g, graph_t{});

    check_exception_thrown<std::out_of_range>(LINE("Unable to join zeroth node to non-existant first node"), [&g]() { g.join(0, 1); });

    check_exception_thrown<std::out_of_range>(LINE("Index out of range for swapping nodes"), [&g]() { g.swap_nodes(0,1); });

    check_equality(LINE("Index of added node is 1"), g.add_node(), 1_sz);
    //    0    1

    check_equality(LINE(""), g, graph_t{{}, {}});
    check_semantics(LINE("Regular semantics"), g, graph_t{{}});

    g.swap_nodes(0,1);

    check_equality(LINE(""), g, graph_t{{}, {}});

    g.join(0, 1);
    //    0----1

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{1}}, {}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{1}}, {edge_init_t{0}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}}});
    }
    else
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{1,0}}, {edge_init_t{0,0}}});
    }

    check_semantics(LINE("Regular semantics"), g, graph_t{{}, {}});

    g.swap_nodes(0,1);

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), g, graph_t{{}, {edge_init_t{0}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{1}}, {edge_init_t{0}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{1,0,0}}, {edge_init_t{1,0,0}}});
    }
    else
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{1,0}}, {edge_init_t{0,0}}});
    }

    g.swap_nodes(1,0);

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{1}}, {}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{1}}, {edge_init_t{0}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}}});
    }
    else
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{1,0}}, {edge_init_t{0,0}}});
    }

    auto nodeNum{g.add_node()};
    check_equality(LINE("Index of added node is 2"), nodeNum, 2_sz);
    g.join(1, nodeNum);
    //    0----1----2

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{1}}, {edge_init_t{2}}, {}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{1}}, {edge_init_t{0}, edge_init_t{2}}, {edge_init_t{1}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}, edge_init_t{1,2,0}}, {edge_init_t{1,2,1}}});
    }
    else
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{1,0}}, {edge_init_t{0,0}, edge_init_t{2,0}}, {edge_init_t{1,1}}});
    }

    g.swap_nodes(2,1);

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{2}}, {}, {edge_init_t{1}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{2}}, {edge_init_t{2}}, {edge_init_t{0}, edge_init_t{1}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{0,2,0}}, {edge_init_t{2,1,1}}, {edge_init_t{0,2,0}, edge_init_t{2,1,0}}});
    }
    else
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{2,0}}, {edge_init_t{2,1}}, {edge_init_t{0,0}, edge_init_t{1,0}}});
    }

    g.swap_nodes(1,2);
     //    0----1----2

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{1}}, {edge_init_t{2}}, {}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{1}}, {edge_init_t{0}, edge_init_t{2}}, {edge_init_t{1}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}, edge_init_t{1,2,0}}, {edge_init_t{1,2,1}}});
    }
    else
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{1,0}}, {edge_init_t{0,0}, edge_init_t{2,0}}, {edge_init_t{1,1}}});
    }

    g.swap_nodes(0,2);

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), g, graph_t{{}, {edge_init_t{0}}, {edge_init_t{1}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t g2{{edge_init_t{1}}, {edge_init_t{0}, edge_init_t{2}}, {edge_init_t{1}}};
      g2.sort_edges(g2.cbegin_edges(1), g2.cend_edges(1), [](const auto& l, const auto& r) {
          return l.target_node() > r.target_node();
        }
      );

      check_equality(LINE(""), g2, g);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{1,0,1}}, {edge_init_t{2,1,0}, edge_init_t{1,0,0}}, {edge_init_t{2,1,0}}});
    }
    else
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{1,1}}, {edge_init_t{2,0}, edge_init_t{0,0}}, {edge_init_t{1,0}}});
    }

    g.swap_nodes(0,2);

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{1}}, {edge_init_t{2}}, {}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{1}}, {edge_init_t{0}, edge_init_t{2}}, {edge_init_t{1}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}, edge_init_t{1,2,0}}, {edge_init_t{1,2,1}}});
    }
    else
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{1,0}}, {edge_init_t{0,0}, edge_init_t{2,0}}, {edge_init_t{1,1}}});
    }

    nodeNum = g.add_node();
    check_equality(LINE("Index of added node is 3"), nodeNum, 3_sz);
    g.join(0, nodeNum);
    //    0----1----2
    //    |
    //    3

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{1}, edge_init_t{3}}, {edge_init_t{2}}, {}, {}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{1}, edge_init_t{3}}, {edge_init_t{0}, edge_init_t{2}}, {edge_init_t{1}}, {edge_init_t{0}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{0,1,0}, edge_init_t{0,3,0}}, {edge_init_t{0,1,0}, edge_init_t{1,2,0}}, {edge_init_t{1,2,1}}, {edge_init_t{0,3,1}}});
    }
    else
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{1,0}, edge_init_t{3,0}}, {edge_init_t{0,0}, edge_init_t{2,0}}, {edge_init_t{1,1}}, {edge_init_t{0,1}}});
    }

    g.erase_node(0);
    //    0----1
    //
    //    2

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{1}}, {}, {}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{1}}, {edge_init_t{0}}, {}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}}, {}});
    }
    else
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{1,0}}, {edge_init_t{0,0}}, {}});
    }

    g.swap_nodes(0,2);

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), g, graph_t{{}, {}, {edge_init_t{1}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_equality(LINE(""), g, graph_t{{}, {edge_init_t{2}}, {edge_init_t{1}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), g, graph_t{{}, {edge_init_t{2,1,0}}, {edge_init_t{2,1,0}}});
    }
    else
    {
      check_equality(LINE(""), g, graph_t{{}, {edge_init_t{2,0}}, {edge_init_t{1,0}}});
    }

    g.swap_nodes(2,0);

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{1}}, {}, {}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{1}}, {edge_init_t{0}}, {}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}}, {}});
    }
    else
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{1,0}}, {edge_init_t{0,0}}, {}});
    }

    g.erase_edge(g.cbegin_edges(0));
    //    0    1
    //
    //    2

    check_equality(LINE(""), g, graph_t{{}, {}, {}});

    g.join(0, 1);
    g.join(1, 1);
    g.join(1, 0);
    //    0======1
    //           /\
    //    2      \/

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{1}}, {edge_init_t{1}, edge_init_t{0}}, {}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t g2{{edge_init_t{1}, edge_init_t{1}}, {edge_init_t{0}, edge_init_t{0}, edge_init_t{1}, edge_init_t{1}}, {}};
      g2.sort_edges(g2.cbegin_edges(1) + 1, g2.cend_edges(1), [](const auto& l, const auto& r) {
          return l.target_node() > r.target_node();
        }
      );

      check_equality(LINE(""), g2, g);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{0,1,0}, edge_init_t{1, 0, 3}}, {edge_init_t{0,1,0}, edge_init_t{1,1,2}, edge_init_t{1,1,1}, edge_init_t{1, 0, 1}}, {}});
    }
    else
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{1,0}, edge_init_t{1,3}}, {edge_init_t{0,0}, edge_init_t{1,2}, edge_init_t{1,1}, edge_init_t{0,1}}, {}});
    }

    g.swap_nodes(1,0);

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{0}, edge_init_t{1}}, {edge_init_t{0}}, {}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t g2{{edge_init_t{0}, edge_init_t{0}, edge_init_t{1}, edge_init_t{1}}, {edge_init_t{0}, edge_init_t{0}}, {}};
      g2.sort_edges(g2.cbegin_edges(0), g2.cbegin_edges(0)+3, [](const auto& l, const auto& r) {
          return l.target_node() > r.target_node();
        }
      );

      check_equality(LINE(""), g2, g);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{1,0,0}, edge_init_t{0,0,2}, edge_init_t{0,0,1}, edge_init_t{0, 1, 1}}, {edge_init_t{1,0,0}, edge_init_t{0, 1, 3}}, {}});
    }
    else
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{1,0}, edge_init_t{0,2}, edge_init_t{0,1}, edge_init_t{1,1}}, {edge_init_t{0,0}, edge_init_t{0,3}}, {}});
    }

    g.swap_nodes(0,1);

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{1}}, {edge_init_t{1}, edge_init_t{0}}, {}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t g2{{edge_init_t{1}, edge_init_t{1}}, {edge_init_t{0}, edge_init_t{0}, edge_init_t{1}, edge_init_t{1}}, {}};
      g2.sort_edges(g2.cbegin_edges(1) + 1, g2.cend_edges(1), [](const auto& l, const auto& r) {
          return l.target_node() > r.target_node();
        }
      );

      check_equality(LINE(""), g2, g);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{0,1,0}, edge_init_t{1, 0, 3}}, {edge_init_t{0,1,0}, edge_init_t{1,1,2}, edge_init_t{1,1,1}, edge_init_t{1, 0, 1}}, {}});
    }
    else
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{1,0}, edge_init_t{1,3}}, {edge_init_t{0,0}, edge_init_t{1,2}, edge_init_t{1,1}, edge_init_t{0,1}}, {}});
    }

    //    0======1
    //           /\
    //    2      \/

    g.swap_nodes(0,2);

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), g, graph_t{{}, {edge_init_t{1}, edge_init_t{2}}, {edge_init_t{1}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t g2{{}, {edge_init_t{1}, edge_init_t{1}, edge_init_t{2}, edge_init_t{2}}, {edge_init_t{1}, edge_init_t{1}}};
      g2.sort_edges(g2.cbegin_edges(1), g2.cbegin_edges(1) + 3, [](const auto& l, const auto& r) {
          return l.target_node() > r.target_node();
        }
      );

      check_equality(LINE(""), g2, g);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), g, graph_t{{}, {edge_init_t{2,1,0}, edge_init_t{1,1,2}, edge_init_t{1,1,1}, edge_init_t{1, 2, 1}}, {edge_init_t{2,1,0}, edge_init_t{1, 2, 3}}});
    }
    else
    {
      check_equality(LINE(""), g, graph_t{{}, {edge_init_t{2,0}, edge_init_t{1,2}, edge_init_t{1,1}, edge_init_t{2,1}}, {edge_init_t{1,0}, edge_init_t{1,3}}});
    }

    g.swap_nodes(2,0);

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{1}}, {edge_init_t{1}, edge_init_t{0}}, {}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t g2{{edge_init_t{1}, edge_init_t{1}}, {edge_init_t{0}, edge_init_t{0}, edge_init_t{1}, edge_init_t{1}}, {}};
      g2.sort_edges(g2.cbegin_edges(1) + 1, g2.cend_edges(1), [](const auto& l, const auto& r) {
          return l.target_node() > r.target_node();
        }
      );

      check_equality(LINE(""), g2, g);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{0,1,0}, edge_init_t{1, 0, 3}}, {edge_init_t{0,1,0}, edge_init_t{1,1,2}, edge_init_t{1,1,1}, edge_init_t{1, 0, 1}}, {}});
    }
    else
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{1,0}, edge_init_t{1,3}}, {edge_init_t{0,0}, edge_init_t{1,2}, edge_init_t{1,1}, edge_init_t{0,1}}, {}});
    }

    g.erase_edge(mutual_info(GraphFlavour) ? ++g.cbegin_edges(0) : g.cbegin_edges(0));
    //    0------1
    //           /\
    //    2      \/

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), g, graph_t{{}, {edge_init_t{1}, edge_init_t{0}}, {}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t g2{{edge_init_t{1}}, { edge_init_t{0}, edge_init_t{1}, edge_init_t{1}}, {}};
      g2.sort_edges(g2.cbegin_edges(1), g2.cend_edges(1), [](const auto& l, const auto& r) {
          return l.target_node() > r.target_node();
        }
      );

      check_equality(LINE(""), g2, g);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}, edge_init_t{1,1,2}, edge_init_t{1,1,1}}, {}});
    }
    else
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{1,0}}, {edge_init_t{0,0}, edge_init_t{1,2}, edge_init_t{1,1}}, {}});
    }

    g.join(2,1);
    //    0------1------2
    //           /\
    //           \/

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), g, graph_t{{}, {edge_init_t{1}, edge_init_t{0}}, {edge_init_t{1}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t g2{{edge_init_t{1}}, { edge_init_t{0}, edge_init_t{1}, edge_init_t{1}, edge_init_t{2}}, {edge_init_t{1}}};
      g2.sort_edges(g2.cbegin_edges(1), g2.cend_edges(1) -1, [](const auto& l, const auto& r) {
          return l.target_node() > r.target_node();
        }
      );

      check_equality(LINE(""), g2, g);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}, edge_init_t{1,1,2}, edge_init_t{1,1,1}, edge_init_t{2,1,0}}, {edge_init_t{2,1,3}}});
    }
    else
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{1,0}}, {edge_init_t{0,0}, edge_init_t{1,2}, edge_init_t{1,1}, edge_init_t{2,0}}, {edge_init_t{1,3}}});
    }

    g.erase_edge(mutual_info(GraphFlavour) ? ++g.cbegin_edges(1) : g.cbegin_edges(1));
    //    0------1------2

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), g, graph_t{{}, {edge_init_t{0}}, {edge_init_t{1}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{1}}, { edge_init_t{0}, edge_init_t{2}}, {edge_init_t{1}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}, edge_init_t{2,1,0}}, {edge_init_t{2,1,1}}});
    }
    else
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{1,0}}, {edge_init_t{0,0}, edge_init_t{2,0}}, {edge_init_t{1,1}}});
    }

    g.erase_edge(g.cbegin_edges(2));
    //    0------1     2

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), g, graph_t{{}, {edge_init_t{0}}, {}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{1}}, {edge_init_t{0}}, {}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}}, {}});
    }
    else
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{1,0}}, {edge_init_t{0,0}}, {}});
    }

    g.join(1,1);
    g.join(2,1);

    //    0------1------2
    //           /\
    //           \/

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), g, graph_t{{}, {edge_init_t{0}, edge_init_t{1}}, {edge_init_t{1}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{1}}, {edge_init_t{0}, edge_init_t{1}, edge_init_t{1}, edge_init_t{2}}, {edge_init_t{1}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}, edge_init_t{1,1,2}, edge_init_t{1,1,1}, edge_init_t{2,1,0}}, {edge_init_t{2,1,3}}});
    }
    else
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{1,0}}, {edge_init_t{0,0}, edge_init_t{1,2}, edge_init_t{1,1}, edge_init_t{2,0}}, {edge_init_t{1,3}}});
    }

    g.erase_edge(mutual_info(GraphFlavour) ? g.cbegin_edges(1)+2 : g.cbegin_edges(1)+1);
    //    0------1------2

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), g, graph_t{{}, {edge_init_t{0}}, {edge_init_t{1}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{1}}, {edge_init_t{0}, edge_init_t{2}}, {edge_init_t{1}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}, edge_init_t{2,1,0}}, {edge_init_t{2,1,1}}});
    }
    else
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{1,0}}, {edge_init_t{0,0}, edge_init_t{2,0}}, {edge_init_t{1,1}}});
    }

    g.erase_edge(g.cbegin_edges(1));
    g.erase_edge(g.cbegin_edges(2));
    g.join(1,1);
    //    0      1
    //           /\
    //    2      \/

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), g, graph_t{{}, {edge_init_t{1}}, {}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_equality(LINE(""), g, graph_t{{}, {edge_init_t{1}, edge_init_t{1}}, {}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), g, graph_t{{}, {edge_init_t{1,1,1}, edge_init_t{1,1,0}}, {}});
    }
    else
    {
      check_equality(LINE(""), g, graph_t{{}, {edge_init_t{1,1}, edge_init_t{1,0}}, {}});
    }

    g.erase_edge(g.cbegin_edges(1));
    //    0    1
    //
    //    2

    check_equality(LINE(""), g, graph_t{{}, {}, {}});

    g.join(0, 1);
    g.join(0, 1);
    g.join(0, 2);
    g.join(0, 2);
    g.join(1, 2);
    g.join(2, 1);
    //    0=====1
    //    ||  //
    //    ||//
    //     2

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), g,
                     graph_t{
                       {edge_init_t{1}, edge_init_t{1}, edge_init_t{2}, edge_init_t{2}},
                       {edge_init_t{2}},
                       {edge_init_t{1}}
                     });
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_equality(LINE(""), g,
                     graph_t{
                       {edge_init_t{1}, edge_init_t{1}, edge_init_t{2}, edge_init_t{2}},
                       {edge_init_t{0}, edge_init_t{0}, edge_init_t{2}, edge_init_t{2}},
                       {edge_init_t{0}, edge_init_t{0}, edge_init_t{1}, edge_init_t{1}}
                     });
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), g,
                     graph_t{
                       {edge_init_t{0,1,0}, edge_init_t{0,1,1}, edge_init_t{0,2,0}, edge_init_t{0,2,1}},
                       {edge_init_t{0,1,0}, edge_init_t{0,1,1}, edge_init_t{1,2,2}, edge_init_t{2,1,3}},
                       {edge_init_t{0,2,2}, edge_init_t{0,2,3}, edge_init_t{1,2,2}, edge_init_t{2,1,3}}
                     });
    }
    else
    {
      check_equality(LINE(""), g,
                     graph_t{
                       {edge_init_t{1,0}, edge_init_t{1,1}, edge_init_t{2,0}, edge_init_t{2,1}},
                       {edge_init_t{0,0}, edge_init_t{0,1}, edge_init_t{2,2}, edge_init_t{2,3}},
                       {edge_init_t{0,2}, edge_init_t{0,3}, edge_init_t{1,2}, edge_init_t{1,3}}
                     });
    }

    mutual_info(GraphFlavour) ? g.erase_edge(g.cbegin_edges(2)+2) : g.erase_edge(g.cbegin_edges(2));

    //    0=====1
    //    ||  /
    //    ||/
    //     2

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), g,
                     graph_t{
                       {edge_init_t{1}, edge_init_t{1}, edge_init_t{2}, edge_init_t{2}},
                       {edge_init_t{2}},
                       {}
                     });
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_equality(LINE(""), g,
                     graph_t{
                       {edge_init_t{1}, edge_init_t{1}, edge_init_t{2}, edge_init_t{2}},
                       {edge_init_t{0}, edge_init_t{0}, edge_init_t{2}},
                       {edge_init_t{0}, edge_init_t{0}, edge_init_t{1}}
                     });
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), g,
                     graph_t{
                       {edge_init_t{0,1,0}, edge_init_t{0,1,1}, edge_init_t{0,2,0}, edge_init_t{0,2,1}},
                       {edge_init_t{0,1,0}, edge_init_t{0,1,1}, edge_init_t{2,1,2}},
                       {edge_init_t{0,2,2}, edge_init_t{0,2,3}, edge_init_t{2,1,2}}
                     });
    }
    else
    {
      check_equality(LINE(""), g,
                     graph_t{
                       {edge_init_t{1,0}, edge_init_t{1,1}, edge_init_t{2,0}, edge_init_t{2,1}},
                       {edge_init_t{0,0}, edge_init_t{0,1}, edge_init_t{2,2}},
                       {edge_init_t{0,2}, edge_init_t{0,3}, edge_init_t{1,2}}
                     });
    }

    g.erase_node(0);
    //  0----1

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{1}}, {}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{1}}, {edge_init_t{0}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), g,  graph_t{{edge_init_t{1,0,0}}, {edge_init_t{1,0,0}}});
    }
    else
    {
      check_equality(LINE(""), g,  graph_t{{edge_init_t{1,0}}, {edge_init_t{0,0}}});
    }

    g.join(0, 0);
    // /\
    // \/
    //  0---1

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{1}, edge_init_t{0}}, {}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t g2{{edge_init_t{0}, edge_init_t{0}, edge_init_t{1}}, {edge_init_t{0}}};
      g2.sort_edges(g2.cbegin_edges(0), g2.cend_edges(0),
                   [](const auto& l, const auto& r) {return l.target_node() > r.target_node();});

      check_equality(LINE(""), g2, g);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), g,
                     graph_t{
                       {edge_init_t{1,0,0}, edge_init_t{0,0,2}, edge_init_t{0,0,1}},
                       {edge_init_t{1,0,0}}
                     });
    }
    else
    {
      check_equality(LINE(""), g,
                     graph_t{
                       {edge_init_t{1,0}, edge_init_t{0,2}, edge_init_t{0,1}},
                       {edge_init_t{0,0}}
                     });
    }

    g.erase_edge(++g.cbegin_edges(0));
    //  0----1

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{1}}, {}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{1}}, {edge_init_t{0}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), g,  graph_t{{edge_init_t{1,0,0}}, {edge_init_t{1,0,0}}});
    }
    else
    {
      check_equality(LINE(""), g,  graph_t{{edge_init_t{1,0}}, {edge_init_t{0,0}}});
    }

    g.erase_node(0);
    // 0

    check_equality(LINE(""), g, graph_t{{}});

    check_equality(LINE(""), g, {{}});

    g.erase_node(0);
    //

    check_equality(LINE(""), g, {});

    check_equality(LINE("Node added back to graph"), g.add_node(), 0_sz);
    // 0

    g.join(0, 0);
    // /\
    // \/
    // 0

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{0}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{0}, edge_init_t{0}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), g,  graph_t{{edge_init_t{0,0,1}, edge_init_t{0,0,0}}});
    }
    else
    {
      check_equality(LINE(""), g,  graph_t{{edge_init_t{0,1}, edge_init_t{0,0}}});
    }

    check_equality(LINE("Prepend a node to the exising graph"), g.insert_node(0), 0_sz);
    //    /\
    //    \/
    // 0  0

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), g, graph_t{{}, {edge_init_t{1}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_equality(LINE(""), g, graph_t{{}, {edge_init_t{1}, edge_init_t{1}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), g,  graph_t{{}, {edge_init_t{1,1,1}, edge_init_t{1,1,0}}});
    }
    else
    {
      check_equality(LINE(""), g,  graph_t{{}, {edge_init_t{1,1}, edge_init_t{1,0}}});
    }

    g.join(0,1);
    g.insert_node(1);
    //       /\
    //       \/
    // 0  0  0
    // \    /
    //  \  /
    //   \/

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{2}}, {}, {edge_init_t{2}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t g2{{edge_init_t{2}}, {}, {edge_init_t{0}, edge_init_t{2}, edge_init_t{2}}};
      g2.sort_edges(g2.cbegin_edges(2), g2.cend_edges(2), [](const auto& l, const auto& r) { return l.target_node() > r.target_node(); });
      check_equality(LINE(""), g2, g);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), g,  graph_t{{edge_init_t{0,2,2}}, {}, {edge_init_t{2,2,1}, edge_init_t{2,2,0}, edge_init_t{0,2,0}}});
    }
    else
    {
      check_equality(LINE(""), g,  graph_t{{edge_init_t{2,2}}, {}, {edge_init_t{2,1}, edge_init_t{2,0}, edge_init_t{0,0}}});
    }


    g.erase_node(1);
    g.erase_node(0);
    // /\
    // \/
    // 0

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{0}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_equality(LINE(""), g, graph_t{{edge_init_t{0}, edge_init_t{0}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), g,  graph_t{{edge_init_t{0,0,1}, edge_init_t{0,0,0}}});
    }
    else
    {
      check_equality(LINE(""), g,  graph_t{{edge_init_t{0,1}, edge_init_t{0,0}}});
    }

    graph_t network2;

    check_equality(LINE("Node added to second g"), network2.add_node(), 0_sz);
    check_equality(LINE("Second node added to second g"), network2.add_node(), 1_sz);
    check_equality(LINE("Third node added to second g"), network2.add_node(), 2_sz);
    check_equality(LINE("Fourth node added to second g"), network2.add_node(), 3_sz);

    network2.join(0, 1);
    network2.join(3, 2);
    // 0----0    0----0

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE("Check di-component graph"), network2, graph_t{{edge_init_t{1}}, {}, {}, {edge_init_t{2}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_equality(LINE("Check di-component graph"), network2, graph_t{{edge_init_t{1}}, {edge_init_t{0}}, {edge_init_t{3}}, {edge_init_t{2}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE("Check di-component graph"), network2,  graph_t{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}}, {edge_init_t{3,2,0}}, {edge_init_t{3,2,0}}});
    }
    else
    {
      check_equality(LINE("Check di-component graph"), network2, graph_t{{edge_init_t{1,0}}, {edge_init_t{0,0}}, {edge_init_t{3,0}}, {edge_init_t{2,0}}});
    }

    check_semantics(LINE("Regular semantics"), g, network2);
  }
}
