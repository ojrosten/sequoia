#include "TestStaticGraphTraversals.hpp"

#include "StaticGraph.hpp"
#include "GraphTraversals.hpp"

namespace sequoia::unit_testing
{
  constexpr auto test_static_graph_traversals::topological_sort()
  {
    using namespace maths;
    using namespace data_structures;

    static_stack<std::size_t, 2> ordering{};

    using g_type = static_graph<directed_flavour::directed, 1, 2, null_weight, null_weight>;
    using edge = typename g_type::edge_init_type;

    auto lateNodeFn{
      [&ordering](const auto nodeIndex){
        ordering.push(nodeIndex);
      }
    };
    
    constexpr g_type g{{edge{1}}, {}};
    depth_first_search(g, true, 0, null_functor{}, lateNodeFn);

    return ordering;
  }

  
  void test_static_graph_traversals::run_tests()
  {
    constexpr auto ordering{topological_sort()};
    check(!ordering.empty(), LINE(""));
  }
}
