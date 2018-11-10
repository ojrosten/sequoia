#include "TestStaticGraphTraversals.hpp"

#include "GraphTraversals.hpp"

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
  }
}
