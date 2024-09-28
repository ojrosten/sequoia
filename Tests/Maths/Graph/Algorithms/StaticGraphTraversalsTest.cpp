////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "StaticGraphTraversalsTest.hpp"

#include "Maths/Graph/GraphTestingUtilities.hpp"
#include "sequoia/Maths/Graph/StaticGraph.hpp"
#include "sequoia/Maths/Graph/StaticGraphTraversals.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path test_static_graph_traversals::source_file() const
  {
    return std::source_location::current().file_name();
  }

  constexpr auto test_static_graph_traversals::arrage()
  {
    using namespace maths;

    using g_type = static_directed_graph<1, 2, null_weight, null_weight>;
    using edge_t = typename g_type::edge_init_type;

    std::array<std::size_t, 2> ordering{};
    std::size_t index{};
    auto lateNodeFn{
      [&ordering, &index](const auto nodeIndex){
        ordering[index++] = nodeIndex;
      }
    };

    constexpr g_type g{{edge_t{1}}, {}};
    traverse(pseudo_depth_first, g, find_disconnected_t{}, null_func_obj{}, lateNodeFn);

    return ordering;
  }

  //constexpr auto test_static_graph_traversals::bfs()
  //{
  //  using namespace maths;
  //  using g_type = static_embedded_graph<directed_flavour::directed, 2, 1, null_weight, null_weight>;
  //  using edge_t = typename g_type::edge_init_type;

  //  //   /--<--\ ->-\
  //  //   \   / /    /
  //  //    \ / /    /

  //  constexpr g_type g{
  //    {
  //      edge_t{0, inversion_constant<true>{}, 2},
  //      edge_t{0, 0, 3},
  //      edge_t{0, inversion_constant<true>{}, 0},
  //      edge_t{0, 0, 1}
  //    },
  //  };

  //  std::array<std::size_t, 2> edgeData{};
  //  std::size_t index{};
  //  auto edgeFn{
  //    [&edgeData, &index](const auto edgeIter){
  //      edgeData[index++] = edgeIter->complementary_index();
  //    }
  //  };

  //  traverse(breadth_first, g, find_disconnected_t{}, null_func_obj{}, null_func_obj{}, edgeFn);

  //  return edgeData;
  //}

  constexpr auto test_static_graph_traversals::priority_search()
  {
    using namespace maths;
    using g_type = static_undirected_graph<3, 4, null_weight, int>;
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

    maths::traverse(priority_first, g, find_disconnected_t{}, nodeEarlyFn);

    return orderedNodeWeights;
  }


  void test_static_graph_traversals::run_tests()
  {
    {
      constexpr auto ordering{arrage()};
      check(equality, report(""), ordering[0], 0_sz);
      check(equality, report(""), ordering[1], 1_sz);
    }

    /*{
      constexpr auto data{bfs()};
      check(equality, report(""), data[0], 3_sz);
      check(equality, report(""), data[1], 0_sz);
    }*/

    {
      constexpr auto weights{priority_search()};
      check(equality, report(""), weights[0], 0);
      check(equality, report(""), weights[1], 8);
      check(equality, report(""), weights[2], 6);
      check(equality, report(""), weights[3], 2);
    }
  }
}
