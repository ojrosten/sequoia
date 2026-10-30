////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "DynamicGraphWeightedAllocationContiguousTest.hpp"
#include "DynamicGraphAllocationTestingUtilities.hpp"

#include "Core/DataStructures/ElementTestingUtilities.hpp"

import std;

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path weighted_graph_allocation_contiguous_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void weighted_graph_allocation_contiguous_test::run_tests()
  {
    using std::complex;

    using contig_edge_config = custom_allocator_contiguous_edge_storage_config;

    {
      graph_test_helper<int, complex<double>, weighted_graph_allocation_contiguous_test>  helper{*this};

      using nstorage = node_storage_generator_t<complex<double>>;

      helper.run_tests<contig_edge_config, nstorage>();
    }

    {
      graph_test_helper<complex<float>, complex<double>, weighted_graph_allocation_contiguous_test>  helper{*this};

      using nstorage = node_storage_generator_t<complex<double>>;

      helper.run_tests<contig_edge_config, nstorage>();
    }

    {
      graph_test_helper<std::vector<int>, std::vector<complex<double>>, weighted_graph_allocation_contiguous_test>  helper{*this};

      using nstorage = node_storage_generator_t<std::vector<complex<double>>>;

      helper.run_tests<contig_edge_config, nstorage>();
    }

    test_node_weights_constructible_from_anything();
  }

  template
  <
    maths::graph_flavour GraphFlavour,
    class EdgeWeight,
    class NodeWeight,
    class EdgeStorageConfig,
    class NodeWeightStorage
  >
  void weighted_graph_allocation_contiguous_test::execute_operations()
  {
    using graph_t = graph_type_generator_t<GraphFlavour, EdgeWeight, NodeWeight, EdgeStorageConfig, NodeWeightStorage>;

    contiguous_memory<graph_t>();
  }

  template<maths::dynamic_network Graph>
  void weighted_graph_allocation_contiguous_test::contiguous_memory()
  {
    using namespace maths;
    using edge_partitions_allocator = decltype(Graph{}.get_edge_allocator(partitions_allocator_tag{}));
    using edge_allocator = Graph::edge_allocator_type;
    using node_allocator = Graph::node_weight_allocator_type;

    // null
    Graph g{edge_allocator{}, edge_partitions_allocator{}, node_allocator{}};

    check(equality, "", g.edges_capacity(), 0uz);
    check(equality, "", g.node_capacity(), 0uz);

    g.reserve_edges(4);
    check(equality, "", g.edges_capacity(), 4uz);

    g.reserve_nodes(4);
    check(equality, "", g.node_capacity(), 4uz);

    g.shrink_to_fit();
    check(equality, "May fail if stl implementation doesn't actually shrink to fit!", g.edges_capacity(), 0uz);
    check(equality, "May fail if stl implementation doesn't actually shrink to fit!", g.node_capacity(), 0uz);

    // x----x
    using edge_init_t = Graph::edge_init_type;

    auto nodeMaker{
      [](Graph& gr) { gr.add_node(); }
    };

    constexpr auto GraphFlavour{Graph::flavour};

    check_semantics("",
                    g,
                    Graph{{{}},edge_allocator{}, edge_partitions_allocator{}, node_allocator{}},
                    nodeMaker,
                    allocation_info{edge_alloc_getter<Graph>{}, {0_c, {0_c, 0_mu}, {0_anp, 0_awp}}},
                    allocation_info{edge_partitions_alloc_getter<Graph>{}, {0_c, {1_c, 1_mu}, {1_anp, 1_awp}}},
                    allocation_info{node_alloc_getter<Graph>{}, {0_c, {1_c, 1_mu}, {1_anp, 1_awp}}});

    check_semantics("",
                    g,
                    Graph{{{}}, edge_allocator{}, edge_partitions_allocator{}, {{1.0, -1.0}}, node_allocator{}},
                    nodeMaker,
                    allocation_info{edge_alloc_getter<Graph>{}, {0_c, {0_c, 0_mu}, {0_anp, 0_awp}}},
                    allocation_info{edge_partitions_alloc_getter<Graph>{}, {0_c, {1_c, 1_mu}, {1_anp, 1_awp}}},
                    allocation_info{node_alloc_getter<Graph>{}, {0_c, {1_c, 1_mu}, {1_anp, 1_awp}}});

    Graph g2{};

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_semantics("", g2, Graph{{edge_init_t{1}}, {}}, nodeMaker, allocation_info{edge_alloc_getter<Graph>{}, {0_c, {1_c, 0_mu}, {1_anp, 1_awp}}}, allocation_info{edge_partitions_alloc_getter<Graph>{}, {0_c, {1_c, 1_mu}, {1_anp, 1_awp}}}, allocation_info{node_alloc_getter<Graph>{}, {0_c, {1_c, 1_mu}, {1_anp, 1_awp}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_semantics("", g2, Graph{{edge_init_t{1}}, {edge_init_t{0}}}, nodeMaker, allocation_info{edge_alloc_getter<Graph>{}, {0_c, {1_c, 0_mu}, {1_anp, 1_awp}}}, allocation_info{edge_partitions_alloc_getter<Graph>{}, {0_c, {1_c, 1_mu}, {1_anp, 1_awp}}}, allocation_info{node_alloc_getter<Graph>{}, {0_c, {1_c, 1_mu}, {1_anp, 1_awp}}});
    }
    else
    {
      check_semantics("", g2, Graph{{edge_init_t{1,0}}, {edge_init_t{0,0}}}, nodeMaker, allocation_info{edge_alloc_getter<Graph>{}, {0_c, {1_c, 0_mu}, {1_anp, 1_awp}}}, allocation_info{edge_partitions_alloc_getter<Graph>{}, {0_c, {1_c, 1_mu}, {1_anp, 1_awp}}}, allocation_info{node_alloc_getter<Graph>{}, {0_c, {1_c, 1_mu}, {1_anp, 1_awp}}});
    }
  }

  void weighted_graph_allocation_contiguous_test::test_node_weights_constructible_from_anything()
  {
    using graph_t                   = maths::directed_graph<maths::null_weight,
                                                            constructible_from_anything,
                                                            custom_allocator_contiguous_edge_storage_config,
                                                            node_storage_generator_t<constructible_from_anything>>;
    using edge_allocator            = graph_t::edge_allocator_type;
    using edge_partitions_allocator = decltype(graph_t{}.get_edge_allocator(maths::partitions_allocator_tag{}));
    using node_allocator            = graph_t::node_weight_allocator_type;
    using prediction_t              = std::pair<std::initializer_list<std::initializer_list<graph_t::edge_init_type>>,
                                                std::initializer_list<constructible_from_anything>>;

    const graph_t::edges_initializer edges{{}};
    const std::initializer_list<constructible_from_anything> weights{1};

    auto nodeMaker{
      [](graph_t& graph) { graph.add_node(); }
    };

    const auto[x,y]{check_semantics("",
                                    [](){ return graph_t(edge_allocator{}, edge_partitions_allocator{}, node_allocator{}); },
                                    [&](){ return graph_t(edges, edge_allocator{}, edge_partitions_allocator{}, weights, node_allocator{}); },
                                    nodeMaker,
                                    allocation_info{edge_alloc_getter<graph_t>{}, {0_c, {0_c, 0_mu}, {0_anp, 0_awp}}},
                                    allocation_info{edge_partitions_alloc_getter<graph_t>{}, {0_c, {1_c, 1_mu}, {1_anp, 1_awp}}},
                                    allocation_info{node_alloc_getter<graph_t>{}, {0_c, {1_c, 1_mu}, {1_anp, 1_awp}}})};

    check(equivalence, "", x, prediction_t{});
    check(equivalence, "", y, prediction_t{{{}}, {1}});
    check(equivalence, "From edges and node weights, without allocators", graph_t(edges, weights), prediction_t{{{}}, {1}});
  }
}
