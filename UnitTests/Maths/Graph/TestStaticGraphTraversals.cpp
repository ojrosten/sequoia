#include "TestStaticGraphTraversals.hpp"

#include "StaticGraph.hpp"
#include "StaticGraphTraversals.hpp"

namespace sequoia::unit_testing
{
  constexpr auto test_static_graph_traversals::topological_sort()
  {
    using namespace maths;

    using g_type = static_graph<directed_flavour::directed, 1, 2, null_weight, null_weight>;
    using edge_t = typename g_type::edge_init_type;

    std::array<std::size_t, 2> ordering{};
    std::size_t index{};
    auto lateNodeFn{
      [&ordering, &index](const auto nodeIndex){
        ordering[index++] = nodeIndex;
      }
    };
    
    constexpr g_type g{{edge_t{1}}, {}};
    depth_first_search(g, true, 0, null_functor{}, lateNodeFn);

    return ordering;
  }

  constexpr auto test_static_graph_traversals::bfs()
  {
    using namespace maths;
    using g_type = static_embedded_graph<directed_flavour::directed, 2, 1, null_weight, null_weight>;
    using edge_t = typename g_type::edge_init_type;

    //   /--<--\ ->-\
    //   \   / /    /
    //    \ / /    /

    constexpr g_type g{
      {
        edge_t{0, inversion_constant<true>{}, 2},
        edge_t{0, 0, 3},
        edge_t{0, inversion_constant<true>{}, 0},
        edge_t{0, 0, 1}
      },
    };
    
    std::array<std::size_t, 2> edgeData{};
    std::size_t index{};
    auto edgeFn{
      [&edgeData, &index](const auto edgeIter){
        edgeData[index++] = edgeIter->complementary_index();
      }
    };

    breadth_first_search(g, true, 0, null_functor{}, null_functor{}, edgeFn);
    
    return edgeData;
  }

  constexpr auto test_static_graph_traversals::priority_search()
  {
    using namespace maths;
    using g_type = static_graph<directed_flavour::undirected, 3, 4, null_weight, int>;
    using edge_t = typename g_type::edge_init_type;

    // 6  4  2
    //  \ | /
    //   \|/
    //    0
    
    constexpr g_type g{
      {
        {edge_t{1}, edge_t{2}, edge_t{3}},
        {edge_t{0}},
        {edge_t{0}},
        {edge_t{0}}
      },
      {
        0, 6, 2, 8
      }
    };

    std::array<int, 4> orderedNodeWeights{};
    std::size_t index{};
    auto nodeEarlyFn{
      [&orderedNodeWeights, &index, &g](const auto node) {
        orderedNodeWeights[index++] = *(g.cbegin_node_weights() + node);
      }
    };
    
    maths::priority_search(g, true, 0, nodeEarlyFn);

    return orderedNodeWeights;
  }

  
  void test_static_graph_traversals::run_tests()
  {
    {
      constexpr auto ordering{topological_sort()};
      check_equality<std::size_t>(0, ordering[0], LINE(""));
      check_equality<std::size_t>(1, ordering[1], LINE(""));
    }

    {
      constexpr auto data{bfs()};
      check_equality<std::size_t>(3, data[0], LINE(""));
      check_equality<std::size_t>(0, data[1], LINE(""));
    }

    {
      constexpr auto weights{priority_search()};
      check_equality<std::size_t>(0, weights[0], LINE(""));
      check_equality<std::size_t>(8, weights[1], LINE(""));
      check_equality<std::size_t>(6, weights[2], LINE(""));
      check_equality<std::size_t>(2, weights[3], LINE(""));
    }
  }
}
