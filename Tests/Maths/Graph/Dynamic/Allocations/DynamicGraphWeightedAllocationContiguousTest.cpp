////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "DynamicGraphWeightedAllocationContiguousTest.hpp"
#include "DynamicGraphAllocationTestingUtilities.hpp"

#include <complex>

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path weighted_graph_allocation_contiguous_test::source_file() const
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
    using graph_type = graph_type_generator_t<GraphFlavour, EdgeWeight, NodeWeight, EdgeStorageConfig, NodeWeightStorage>;

    contiguous_memory<graph_type>();
  }

  template<maths::dynamic_network Graph>
  void weighted_graph_allocation_contiguous_test::contiguous_memory()
  {
    using namespace maths;
    using edge_partitions_allocator = decltype(Graph{}.get_edge_allocator(partitions_allocator_tag{}));
    using edge_allocator = typename Graph::edge_allocator_type;
    using node_allocator = typename Graph::node_weight_allocator_type;

    // null
    Graph g{edge_allocator{}, edge_partitions_allocator{}, node_allocator{}};

    check(equality, report(""), g.edges_capacity(), 0_sz);
    check(equality, report(""), g.node_capacity(), 0_sz);

    g.reserve_edges(4);
    check(equality, report(""), g.edges_capacity(), 4_sz);

    g.reserve_nodes(4);
    check(equality, report(""), g.node_capacity(), 4_sz);

    g.shrink_to_fit();
    check(equality, report("May fail if stl implementation doesn't actually shrink to fit!"), g.edges_capacity(), 0_sz);
    check(equality, report("May fail if stl implementation doesn't actually shrink to fit!"), g.node_capacity(), 0_sz);

    // x----x
    using edge_init_t = typename Graph::edge_init_type;

    auto nodeMaker{
      [](Graph& g) { g.add_node(); }
    };

    constexpr auto GraphFlavour{Graph::flavour};

    check_semantics(report(""),
                    g,
                    Graph{{{}},edge_allocator{}, edge_partitions_allocator{}, node_allocator{}},
                    nodeMaker,
                    allocation_info{edge_alloc_getter<Graph>{}, {0_c, {0_c, 0_mu}, {0_anp, 0_awp}}},
                    allocation_info{edge_partitions_alloc_getter<Graph>{}, {0_c, {1_c, 1_mu}, {1_anp, 1_awp}}},
                    allocation_info{node_alloc_getter<Graph>{}, {0_c, {1_c, 1_mu}, {1_anp, 1_awp}}});

    check_semantics(report(""),
                    g,
                    Graph{{{}}, edge_allocator{}, edge_partitions_allocator{}, {{1.0, -1.0}}, node_allocator{}},
                    nodeMaker,
                    allocation_info{edge_alloc_getter<Graph>{}, {0_c, {0_c, 0_mu}, {0_anp, 0_awp}}},
                    allocation_info{edge_partitions_alloc_getter<Graph>{}, {0_c, {1_c, 1_mu}, {1_anp, 1_awp}}},
                    allocation_info{node_alloc_getter<Graph>{}, {0_c, {1_c, 1_mu}, {1_anp, 1_awp}}});

    Graph g2{};

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_semantics(report(""), g2, Graph{{edge_init_t{1}}, {}}, nodeMaker, allocation_info{edge_alloc_getter<Graph>{}, {0_c, {1_c, 0_mu}, {1_anp, 1_awp}}}, allocation_info{edge_partitions_alloc_getter<Graph>{}, {0_c, {1_c, 1_mu}, {1_anp, 1_awp}}}, allocation_info{node_alloc_getter<Graph>{}, {0_c, {1_c, 1_mu}, {1_anp, 1_awp}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_semantics(report(""), g2, Graph{{edge_init_t{1}}, {edge_init_t{0}}}, nodeMaker, allocation_info{edge_alloc_getter<Graph>{}, {0_c, {1_c, 0_mu}, {1_anp, 1_awp}}}, allocation_info{edge_partitions_alloc_getter<Graph>{}, {0_c, {1_c, 1_mu}, {1_anp, 1_awp}}}, allocation_info{node_alloc_getter<Graph>{}, {0_c, {1_c, 1_mu}, {1_anp, 1_awp}}});
    }
    else
    {
      check_semantics(report(""), g2, Graph{{edge_init_t{1,0}}, {edge_init_t{0,0}}}, nodeMaker, allocation_info{edge_alloc_getter<Graph>{}, {0_c, {1_c, 0_mu}, {1_anp, 1_awp}}}, allocation_info{edge_partitions_alloc_getter<Graph>{}, {0_c, {1_c, 1_mu}, {1_anp, 1_awp}}}, allocation_info{node_alloc_getter<Graph>{}, {0_c, {1_c, 1_mu}, {1_anp, 1_awp}}});
    }
  }
}
