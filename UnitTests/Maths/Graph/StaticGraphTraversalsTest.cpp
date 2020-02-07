////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "StaticGraphTraversalsTest.hpp"

#include "StaticGraph.hpp"
#include "StaticGraphTraversals.hpp"

namespace sequoia::unit_testing
{
  [[nodiscard]]
  std::string_view test_static_graph_traversals::source_file_name() const noexcept
  {
    return __FILE__;
  }

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
      check_equality(LINE(""), ordering[0], 0ul);
      check_equality(LINE(""), ordering[1], 1ul);
    }

    {
      constexpr auto data{bfs()};
      check_equality(LINE(""), data[0], 3ul);
      check_equality(LINE(""), data[1], 0ul);
    }

    {
      constexpr auto weights{priority_search()};
      check_equality(LINE(""), weights[0], 0);
      check_equality(LINE(""), weights[1], 8);
      check_equality(LINE(""), weights[2], 6);
      check_equality(LINE(""), weights[3], 2);
    }
  }
}
