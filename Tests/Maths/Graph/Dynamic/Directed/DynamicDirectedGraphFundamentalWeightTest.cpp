////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "DynamicDirectedGraphFundamentalWeightTest.hpp"
#include "DynamicDirectedGraphWeightedTestingUtilities.hpp"

#include "Core/DataStructures/ElementTestingUtilities.hpp"

namespace sequoia::testing
{

  [[nodiscard]]
  std::filesystem::path dynamic_directed_graph_fundamental_weight_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void dynamic_directed_graph_fundamental_weight_test::run_tests()
  {
    using namespace maths;
    dynamic_directed_graph_weighted_operations<double, double, independent_bucketed_edge_storage_config, node_storage<double>>::execute_operations(*this);
    test_node_weights_constructible_from_anything();
  }

  void dynamic_directed_graph_fundamental_weight_test::test_node_weights_constructible_from_anything()
  {
    using graph_t        = maths::directed_graph<maths::null_weight, constructible_from_anything>;
    using edge_allocator = graph_t::edge_allocator_type;
    using node_allocator = graph_t::node_weight_allocator_type;
    using edge_init      = graph_t::edge_init_type;

    const graph_t::edges_initializer edges{{edge_init{1}}, {edge_init{2}}, {edge_init{0}}};
    const std::initializer_list<constructible_from_anything> weights{1, 2, 3};

    const graph_t g(edges, weights);
    check(equality, "From edges and node weights", std::ranges::size(g.cnode_weights()), 3uz);
    check(equality, "From edges, node weights and allocators", std::ranges::size(graph_t(edges, edge_allocator{}, weights, node_allocator{}).cnode_weights()), 3uz);

    // Parentheses: braces would select the initializer-list constructor
    check(equality, "Allocator-extended copy", std::ranges::size(graph_t(g, edge_allocator{}, node_allocator{}).cnode_weights()), 3uz);

    graph_t toMove(edges, weights);
    check(equality, "Allocator-extended move", std::ranges::size(graph_t(std::move(toMove), edge_allocator{}, node_allocator{}).cnode_weights()), 3uz);
  }
}