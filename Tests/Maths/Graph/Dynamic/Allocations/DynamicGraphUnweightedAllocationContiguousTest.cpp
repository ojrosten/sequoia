////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "DynamicGraphUnweightedAllocationContiguousTest.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path unweighted_graph_allocation_contiguous_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void unweighted_graph_allocation_contiguous_test::run_tests()
  {
    using namespace maths;

    graph_test_helper<null_weight, null_weight, unweighted_graph_allocation_contiguous_test> helper{*this};
    helper.run_tests<custom_allocator_contiguous_edge_storage_config, node_storage<null_weight>>();
  }

  template
  <
    maths::graph_flavour GraphFlavour,
    class EdgeWeight,
    class NodeWeight,
    class EdgeStorageConfig,
    class NodeWeightStorage
  >
  void unweighted_graph_allocation_contiguous_test::execute_operations()
  {
    using graph_type = graph_type_generator_t<GraphFlavour, EdgeWeight, NodeWeight, EdgeStorageConfig, NodeWeightStorage>;

    contiguous_memory<graph_type>();
  }

  template<maths::dynamic_network Graph>
  void unweighted_graph_allocation_contiguous_test::contiguous_memory()
  {
    using namespace maths;
    using edge_partitions_allocator = decltype(Graph{}.get_edge_allocator(partitions_allocator_tag{}));
    using edge_allocator = typename Graph::edge_allocator_type;
    constexpr auto GraphFlavour{Graph::flavour};

    // null
    Graph g{edge_allocator{}, edge_partitions_allocator{}};

    check(equality, report(""), g.edges_capacity(), 0_sz);
    check(equality, report(""), g.node_capacity(), 0_sz);

    g.reserve_edges(4);
    check(equality, report(""), g.edges_capacity(), 4_sz);

    g.reserve_nodes(4);
    check(equality, report(""), g.node_capacity(), 4_sz);

    g.shrink_to_fit();
    check(equality, report("May fail if stl implementation doesn't actually shrink to fit!"), g.edges_capacity(), 0_sz);
    check(equality, report("May fail if stl implementation doesn't actually shrink to fit!"), g.node_capacity(), 0_sz);

    using edge_init_t = typename Graph::edge_init_type;

    auto nodeMaker{ [](Graph& g) { g.add_node(); } };

    check_semantics(report(""),
                    g,
                    Graph{{{}}, edge_allocator{}, edge_partitions_allocator{}},
                    nodeMaker,
                    allocation_info{edge_alloc_getter<Graph>{}, {0_c, {0_c, 0_mu}, {0_anp, 0_awp}}},
                    allocation_info{edge_partitions_alloc_getter<Graph>{}, {0_c, {1_c, 1_mu}, {1_anp, 1_awp}}});
    Graph g2{};

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_semantics(report(""),
                      g2,
                      Graph{{edge_init_t{1}}, {}},
                      nodeMaker,
                      allocation_info{edge_alloc_getter<Graph>{}, {0_c, {1_c, 0_mu}, {1_anp, 1_awp}}},
                      allocation_info{edge_partitions_alloc_getter<Graph>{}, {0_c, {1_c, 1_mu}, {1_anp, 1_awp}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_semantics(report(""),
                      g2,
                      Graph{{edge_init_t{1}}, {edge_init_t{0}}},
                      nodeMaker,
                      allocation_info{edge_alloc_getter<Graph>{}, {0_c, {1_c, 0_mu}, {1_anp, 1_awp}}},
                      allocation_info{edge_partitions_alloc_getter<Graph>{}, {0_c, {1_c, 1_mu}, {1_anp, 1_awp}}});
    }
    else
    {
      check_semantics(report(""),
                      g2, Graph{{edge_init_t{1,0}}, {edge_init_t{0,0}}},
                      nodeMaker,
                      allocation_info{edge_alloc_getter<Graph>{}, {0_c, {1_c, 0_mu}, {1_anp, 1_awp}}},
                      allocation_info{edge_partitions_alloc_getter<Graph>{}, {0_c, {1_c, 1_mu}, {1_anp, 1_awp}}});
    }
  }
}
