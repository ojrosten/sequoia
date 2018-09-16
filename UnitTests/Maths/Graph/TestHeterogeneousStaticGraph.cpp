#include "TestHeterogeneousStaticGraph.hpp"

#include "HeterogeneousStaticGraph.hpp"

namespace sequoia::unit_testing
{
  void test_heterogeneous_static_graph::run_tests()
  {
    test_generic_undirected();
    test_generic_embedded_undirected();
    test_generic_directed();
    test_generic_embedded_directed();
  }

  void test_heterogeneous_static_graph::test_generic_undirected()
  {
    using namespace maths;

    using graph_t = heterogeneous_static_graph<directed_flavour::undirected, 1, 2, float, int, double>;
    using edge = typename graph_t::edge_init_type;

    constexpr graph_t g{{edge{1, 0.5f}}, {edge{0, 0.5f}}};
  }

  void test_heterogeneous_static_graph::test_generic_embedded_undirected()
  {
  }

  void test_heterogeneous_static_graph::test_generic_directed()
  {
  }

  void test_heterogeneous_static_graph::test_generic_embedded_directed()
  {
  }
}
