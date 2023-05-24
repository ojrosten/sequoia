////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "DynamicGraphUnweightedAllocationTest.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path unweighted_graph_allocation_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void unweighted_graph_allocation_test::run_tests()
  {
    using namespace maths;
    using contig_edge_traits = custom_allocator_contiguous_edge_storage_traits;
    using bucket_edge_traits = custom_allocator_bucketed_edge_storage_traits;

    graph_test_helper<null_weight, null_weight, unweighted_graph_allocation_test> helper{*this};

    helper.run_tests<contig_edge_traits, node_traits<null_weight>>();
    helper.run_tests<bucket_edge_traits, node_traits<null_weight>>();
  }

  template
  <
    maths::graph_flavour GraphFlavour,
    class EdgeWeight,
    class NodeWeight,
    class EdgeWeightCreator,
    class NodeWeightCreator,
    class EdgeStorageTraits,
    class NodeWeightStorageTraits
  >
  void unweighted_graph_allocation_test::execute_operations()
  {
    using ESTraits = EdgeStorageTraits;
    using NSTraits = NodeWeightStorageTraits;
    using graph_type = graph_type_generator_t<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightCreator, NodeWeightCreator, ESTraits, NSTraits>;

    if constexpr(std::is_same_v<ESTraits, custom_allocator_contiguous_edge_storage_traits>)
    {
      contiguous_memory<graph_type>();
    }
    else
    {
      bucketed_memory<graph_type>();
    }
  }

  template<class Graph>
  void unweighted_graph_allocation_test::contiguous_memory()
  {
    using namespace maths;
    using edge_partitions_allocator = decltype(Graph{}.get_edge_allocator(partitions_allocator_tag{}));
    using edge_allocator = typename Graph::edge_allocator_type;
    constexpr auto GraphFlavour{Graph::flavour};

    // null
    Graph g{edge_allocator{}, edge_partitions_allocator{}};

    check(equality, report_line(""), g.edges_capacity(), 0_sz);
    check(equality, report_line(""), g.node_capacity(), 0_sz);

    g.reserve_edges(4);
    check(equality, report_line(""), g.edges_capacity(), 4_sz);

    g.reserve_nodes(4);
    check(equality, report_line(""), g.node_capacity(), 4_sz);

    g.shrink_to_fit();
    check(equality, report_line("May fail if stl implementation doesn't actually shrink to fit!"), g.edges_capacity(), 0_sz);
    check(equality, report_line("May fail if stl implementation doesn't actually shrink to fit!"), g.node_capacity(), 0_sz);

    using edge_init_t = typename Graph::edge_init_type;

    auto nodeMaker{ [](Graph& g) { g.add_node(); } };

    check_semantics(report_line(""),
                    g,
                    Graph{{{}}, edge_allocator{}, edge_partitions_allocator{}},
                    nodeMaker,
                    allocation_info{edge_alloc_getter<Graph>{}, {0_c, {0_c, 0_mu}, {0_anp, 0_awp}}},
                    allocation_info{edge_partitions_alloc_getter<Graph>{}, {0_c, {1_c, 1_mu}, {1_anp, 1_awp}}});
    Graph g2{};

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_semantics(report_line(""),
                      g2,
                      Graph{{edge_init_t{1}}, {}},
                      nodeMaker,
                      allocation_info{edge_alloc_getter<Graph>{}, {0_c, {1_c, 0_mu}, {1_anp, 1_awp}}},
                      allocation_info{edge_partitions_alloc_getter<Graph>{}, {0_c, {1_c, 1_mu}, {1_anp, 1_awp}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_semantics(report_line(""),
                      g2,
                      Graph{{edge_init_t{1}}, {edge_init_t{0}}},
                      nodeMaker,
                      allocation_info{edge_alloc_getter<Graph>{}, {0_c, {1_c, 0_mu}, {1_anp, 1_awp}}},
                      allocation_info{edge_partitions_alloc_getter<Graph>{}, {0_c, {1_c, 1_mu}, {1_anp, 1_awp}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_semantics(report_line(""),
                      g2,
                      Graph{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}}},
                      nodeMaker, allocation_info{edge_alloc_getter<Graph>{}, {0_c, {1_c, 0_mu}, {1_anp, 1_awp}}},
                      allocation_info{edge_partitions_alloc_getter<Graph>{}, {0_c, {1_c, 1_mu}, {1_anp, 1_awp}}});
    }
    else
    {
      check_semantics(report_line(""),
                      g2, Graph{{edge_init_t{1,0}}, {edge_init_t{0,0}}},
                      nodeMaker,
                      allocation_info{edge_alloc_getter<Graph>{}, {0_c, {1_c, 0_mu}, {1_anp, 1_awp}}},
                      allocation_info{edge_partitions_alloc_getter<Graph>{}, {0_c, {1_c, 1_mu}, {1_anp, 1_awp}}});
    }
  }

  template<class Graph>
  void unweighted_graph_allocation_test::bucketed_memory()
  {
    using namespace maths;
    using edge_allocator = typename Graph::edge_allocator_type;
    constexpr auto GraphFlavour{Graph::flavour};

    Graph g{edge_allocator{}};

    this->template check_exception_thrown<std::out_of_range>(report_line(""), [&g](){ g.reserve_edges(0, 4);});
    this->template check_exception_thrown<std::out_of_range>(report_line(""), [&g](){ return g.edges_capacity(0);});
    check(equality, report_line(""), g.node_capacity(), 0_sz);

    g.add_node();
    check(equality, report_line(""), g, Graph{{{}}, edge_allocator{}});

    check(equality, report_line(""), g.edges_capacity(0), 0_sz);
    check(equality, report_line(""), g.node_capacity(), 1_sz);
    g.reserve_edges(0, 4);
    check(equality, report_line(""), g.edges_capacity(0), 4_sz);

    g.reserve_nodes(4);
    check(equality, report_line(""), g.node_capacity(), 4_sz);

    g.shrink_to_fit();
    check(equality, report_line("May fail if stl implementation doesn't actually shrink to fit!"), g.edges_capacity(0), 0_sz);
    check(equality, report_line("May fail if stl implementation doesn't actually shrink to fit!"), g.node_capacity(), 1_sz);

    g.insert_node(0u);
    check(equality, report_line(""), g, Graph{{{}, {}}, edge_allocator{}});

    using edge_init_t = typename Graph::edge_init_type;
    using edge_allocator = typename Graph::edge_allocator_type;

    auto nodeMaker{
      [](Graph& g) { g.add_node(); }
    };

    Graph g2{};

    check_semantics(report_line(""),
                    g2,
                    Graph{{{}}, edge_allocator{}},
                    nodeMaker,
                    allocation_info{edge_alloc_getter<Graph>{},
                                    {0_c, {1_c, 1_mu}, {1_anp, 1_awp}},
                                    { {0_c, {0_c, 0_mu}, {0_anp, 0_awp}, {0_containers, 1_containers, 2_postmutation}} }
                    });

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_semantics(report_line(""),
                      g2,
                      Graph{{edge_init_t{1}}, {}},
                      nodeMaker,
                      allocation_info{edge_alloc_getter<Graph>{},
                                      {0_c, {1_c, 1_mu}, {1_anp, 1_awp}},
                                      { {0_c, {1_c, 0_mu}, {1_anp, 1_awp}, {0_containers, 2_containers, 3_postmutation}} }
                      });
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_semantics(report_line(""),
                      g2,
                      Graph{{edge_init_t{1}}, {edge_init_t{0}}},
                      nodeMaker,
                      allocation_info{edge_alloc_getter<Graph>{},
                                      {0_c, {1_c, 1_mu}, {1_anp, 1_awp}},
                                      { {0_c, {2_c, 0_mu}, {2_anp, 2_awp}, {0_containers, 2_containers, 3_postmutation}} }
                      });
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_semantics(report_line(""),
                      g2,
                      Graph{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}}},
                      nodeMaker,
                      allocation_info{edge_alloc_getter<Graph>{},
                                      {0_c, {1_c, 1_mu}, {1_anp, 1_awp}},
                                      { {0_c, {2_c, 0_mu}, {2_anp, 2_awp}, {0_containers, 2_containers, 3_postmutation}} }
                      });
    }
  }
}
