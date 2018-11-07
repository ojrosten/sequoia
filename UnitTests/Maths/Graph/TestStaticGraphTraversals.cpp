#include "TestStaticGraphTraversals.hpp"

#include "GraphTraversals.hpp"

namespace sequoia::unit_testing
{
  constexpr auto test_static_graph_traversals::topological_sort()
  {
    using namespace maths;
    using namespace data_structures;

    using g_type = static_graph<directed_flavour::directed, 1, 2, null_weight, null_weight>;
    using edge = typename g_type::edge_init_type;

    std::array<std::size_t, 2> ordering{};
    std::size_t index{};
    auto lateNodeFn{
      [&ordering, &index](const auto nodeIndex){
        ordering[index++] = nodeIndex;
      }
    };
    
    constexpr g_type g{{edge{1}}, {}};
    depth_first_search(g, true, 0, null_functor{}, lateNodeFn);

    return ordering;
  }

  
  void test_static_graph_traversals::run_tests()
  {
    constexpr auto ordering{topological_sort()};
    check_equality<std::size_t>(0, ordering[0], LINE(""));
    check_equality<std::size_t>(1, ordering[1], LINE(""));
  }
}
