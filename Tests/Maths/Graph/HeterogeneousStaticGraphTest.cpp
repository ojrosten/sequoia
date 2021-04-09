////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "HeterogeneousStaticGraphTest.hpp"

#include "sequoia/Maths/Graph/HeterogeneousStaticGraph.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view test_heterogeneous_static_graph::source_file() const noexcept
  {
    return __FILE__;
  }

  void test_heterogeneous_static_graph::run_tests()
  {
    test_generic_undirected();
    test_generic_embedded_undirected();
    test_generic_directed();
    test_generic_embedded_directed();
  }


  constexpr auto test_heterogeneous_static_graph::make_undirected_graph()
  {
    using namespace maths;

    using graph_t = heterogeneous_static_graph<directed_flavour::undirected, 1, 3, float, float, int, double>;
    using edge = typename graph_t::edge_init_type;

    graph_t g{{{edge{1, 0.5f}}, {edge{0, 0.5f}}, {}}, 6.6f, -5, 10.2};

    g.node_weight<float>(9.9f);
    g.mutate_node_weight<double>([](double& d) { d += 1.0; });
    g.node_weight<1>(-6);

    g.set_edge_weight(g.cbegin_edges(0), -0.3f);

    return g;
  }

  constexpr auto test_heterogeneous_static_graph::make_undirected_embedded_graph()
  {
    using namespace maths;

    using graph_t = heterogeneous_embedded_static_graph<directed_flavour::undirected, 1, 3, float, float, int, double>;
    using edge = typename graph_t::edge_init_type;

    graph_t g{{{edge{1, 0, 0.5f}}, {edge{0, 0, 0.5f}}, {}}, 6.6f, -5, 10.2};

    g.node_weight<float>(9.9f);
    g.mutate_node_weight<double>([](double& d) { d += 1.0; });
    g.node_weight<1>(-6);

    g.set_edge_weight(g.cbegin_edges(0), -0.3f);

    return g;
  }

  constexpr auto test_heterogeneous_static_graph::make_directed_graph()
  {
    using namespace maths;

    using graph_t = heterogeneous_static_graph<directed_flavour::directed, 1, 3, float, float, int, double>;
    using edge = typename graph_t::edge_init_type;

    graph_t g{{{edge{1, 0.5f}}, {}, {}}, 6.6f, -5, 10.2};

    g.node_weight<float>(9.9f);
    g.mutate_node_weight<double>([](double& d) { d += 1.0; });
    g.node_weight<1>(-6);

    g.set_edge_weight(g.cbegin_edges(0), -0.3f);

    return g;
  }

  constexpr auto test_heterogeneous_static_graph::make_directed_embedded_graph()
  {
    using namespace maths;

    using graph_t = heterogeneous_embedded_static_graph<directed_flavour::directed, 1, 3, float, float, int, double>;
    using edge = typename graph_t::edge_init_type;

    graph_t g{{{edge{0, 1, 0, 0.5f}}, {edge{0, 1, 0, 0.5f}}, {}}, 6.6f, -5, 10.2};

    g.node_weight<float>(9.9f);
    g.mutate_node_weight<double>([](double& d) { d += 1.0; });
    g.node_weight<1>(-6);

    g.set_edge_weight(g.cbegin_edges(0), -0.3f);

    return g;
  }


  void test_heterogeneous_static_graph::test_generic_undirected()
  {
    using namespace maths;

    {
      using graph_t = heterogeneous_static_graph<directed_flavour::undirected, 1, 2, float, int, double>;
      using edge = typename graph_t::edge_init_type;

      constexpr graph_t g{{edge{1, 0.5f}}, {edge{0, 0.5f}}};

      check_graph(LINE(""), g, {{edge{1, 0.5f}}, {edge{0, 0.5f}}}, std::tuple<int, double>{0, 0.0});
    }

    {
      constexpr auto g{make_undirected_graph()};
      using edge = typename decltype(g)::edge_init_type;
      check_graph(LINE(""), g, {{edge{1, -0.3f}}, {edge{0, -0.3f}}, {}}, std::tuple<float, int, double>{9.9f, -6, 11.2});
    }

    {
      using graph_t = heterogeneous_static_graph<directed_flavour::undirected, 1, 1, float, function_object>;
      using edge = typename graph_t::edge_init_type;

      constexpr graph_t g{ {{edge{0, 0.2f}, edge{0, 0.2f}}}, function_object{}};
      check_equality(LINE(""), 4, g.node_weight<0>()(2));
    }
  }

  void test_heterogeneous_static_graph::test_generic_embedded_undirected()
  {
    using namespace maths;

    {
      using graph_t = heterogeneous_embedded_static_graph<directed_flavour::undirected, 1, 2, float, int, double>;
      using edge = typename graph_t::edge_init_type;

      constexpr graph_t g{{edge{1, 0, 0.5f}}, {edge{0, 0, 0.5f}}};

      check_graph(LINE(""), g, {{edge{1, 0, 0.5f}}, {edge{0, 0, 0.5f}}}, std::tuple<int, double>{0, 0.0});
    }

    {
      constexpr auto g{make_undirected_embedded_graph()};
      using edge = typename decltype(g)::edge_init_type;
      check_graph(LINE(""), g, {{edge{1, 0, -0.3f}}, {edge{0, 0, -0.3f}}, {}}, std::tuple<float, int, double>{9.9f, -6, 11.2});
    }
  }

  void test_heterogeneous_static_graph::test_generic_directed()
  {
    using namespace maths;

    {
      using graph_t = heterogeneous_static_graph<directed_flavour::directed, 1, 2, float, int, double>;
      using edge = typename graph_t::edge_init_type;

      constexpr graph_t g{{edge{1, 0.5f}}, {}};

      check_graph(LINE(""), g, {{edge{1, 0.5f}}, {}}, std::tuple<int, double>{0, 0.0});
    }

    {
      constexpr auto g{make_directed_graph()};
      using edge = typename decltype(g)::edge_init_type;
      check_graph(LINE(""), g, {{edge{1, -0.3f}}, {}, {}}, std::tuple<float, int, double>{9.9f, -6, 11.2});
    }
  }

  void test_heterogeneous_static_graph::test_generic_embedded_directed()
  {
    using namespace maths;

    {
      using graph_t = heterogeneous_embedded_static_graph<directed_flavour::directed, 1, 2, float, int, double>;
      using edge = typename graph_t::edge_init_type;

      constexpr graph_t g{{edge{0, 1, 0, 0.5f}}, {edge{0, 1, 0, 0.5f}}};

      check_graph(LINE(""), g, {{edge{0, 1, 0, 0.5f}}, {edge{0, 1, 0, 0.5f}}}, std::tuple<int, double>{0, 0.0});
    }

    {
      constexpr auto g{make_directed_embedded_graph()};
      using edge = typename decltype(g)::edge_init_type;
      check_graph(LINE(""), g, {{edge{0, 1, 0, -0.3f}}, {edge{0, 1, 0, -0.3f}}, {}}, std::tuple<float, int, double>{9.9f, -6, 11.2});
    }
  }
}
