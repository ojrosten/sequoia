////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "DynamicGraphWeightedAllocationTest.hpp"

#include <complex>

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view weighted_graph_allocation_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void weighted_graph_allocation_test::run_tests()
  {
    using std::complex;

    using contig_edge_traits = custom_allocator_contiguous_edge_storage_traits;
    using bucket_edge_traits = custom_allocator_bucketed_edge_storage_traits;

    {
      graph_test_helper<int, complex<double>>  helper{concurrent_execution()};

      using ntraits = node_traits<complex<double>>;
      
      helper.run_storage_tests<contig_edge_traits, ntraits, graph_contiguous_memory>(*this);
      helper.run_storage_tests<bucket_edge_traits, ntraits, graph_bucketed_memory>(*this);
    }

    {
      graph_test_helper<complex<int>, complex<double>>  helper{concurrent_execution()};

      using ntraits = node_traits<complex<double>>;
      
      helper.run_storage_tests<contig_edge_traits, ntraits, graph_contiguous_memory>(*this);
      helper.run_storage_tests<bucket_edge_traits, ntraits, graph_bucketed_memory>(*this);
    }

    {
      graph_test_helper<std::vector<int>, std::vector<complex<double>>>  helper{concurrent_execution()};

      using ntraits = node_traits<std::vector<complex<double>>>;
      
      helper.run_storage_tests<contig_edge_traits, ntraits, graph_contiguous_memory>(*this);
      helper.run_storage_tests<bucket_edge_traits, ntraits, graph_bucketed_memory>(*this);
    }
  }

  template
  <
    maths::graph_flavour GraphFlavour,
    class EdgeWeight,
    class NodeWeight,
    class EdgeWeightPooling,
    class NodeWeightPooling,
    class EdgeStorageTraits,
    class NodeWeightStorageTraits
  >
  void graph_contiguous_memory<
      GraphFlavour,
      EdgeWeight,
      NodeWeight,
      EdgeWeightPooling,
      NodeWeightPooling,
      EdgeStorageTraits,
      NodeWeightStorageTraits
  >::execute_operations()
  {
    using namespace maths;
    using edge_partitions_allocator = decltype(graph_t{}.get_edge_allocator(partitions_allocator_tag{}));
    using edge_allocator = typename graph_t::edge_allocator_type;
    using node_allocator = typename graph_t::node_weight_container_type::allocator_type;

    // null
    graph_t g{edge_allocator{}, edge_partitions_allocator{}, node_allocator{}};

    check_equality(LINE(""), g.edges_capacity(), 0_sz);
    check_equality(LINE(""), g.node_capacity(), 0_sz);

    g.reserve_edges(4);
    check_equality(LINE(""), g.edges_capacity(), 4_sz);

    g.reserve_nodes(4);
    check_equality(LINE(""), g.node_capacity(), 4_sz);

    g.shrink_to_fit();
    check_equality(LINE("May fail if stl implementation doesn't actually shrink to fit!"), g.edges_capacity(), 0_sz);
    check_equality(LINE("May fail if stl implementation doesn't actually shrink to fit!"), g.node_capacity(), 0_sz);
      
    // x----x
    using edge_init_t = typename graph_t::edge_init_type;
    
    auto nodeMaker{
      [](graph_t& g) { g.add_node(); }
    };

    using node_allocator = typename graph_t::node_weight_container_type::allocator_type;
    check_semantics(LINE(""),
                    g,
                    graph_t{{{}},edge_allocator{}, edge_partitions_allocator{}, node_allocator{}},
                    nodeMaker,
                    allocation_info{edge_alloc_getter<graph_t>{}, {0_c, {0_c, 0_mu}, {0_awp, 0_anp}}},
                    allocation_info{partitions_alloc_getter<graph_t>{}, {0_c, {1_c, 1_mu}, {1_awp, 1_anp}}},
                    allocation_info{node_alloc_getter<graph_t>{}, {0_c, {1_c, 1_mu}, {1_awp, 1_anp}}});

    check_semantics(LINE(""),
                    g,
                    graph_t{{{}}, edge_allocator{}, edge_partitions_allocator{}, {{1.0, -1.0}}, node_allocator{}},
                    nodeMaker,
                    allocation_info{edge_alloc_getter<graph_t>{}, {0_c, {0_c, 0_mu}, {0_awp, 0_anp}}},
                    allocation_info{partitions_alloc_getter<graph_t>{}, {0_c, {1_c, 1_mu}, {1_awp, 1_anp}}},
                    allocation_info{node_alloc_getter<graph_t>{}, {0_c, {1_c, 1_mu}, {1_awp, 1_anp}}});

    graph_t g2{};

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_semantics(LINE(""), g2, graph_t{{edge_init_t{1}}, {}}, nodeMaker, allocation_info{edge_alloc_getter<graph_t>{}, {0_c, {1_c, 0_mu}, {1_awp, 1_anp}}}, allocation_info{partitions_alloc_getter<graph_t>{}, {0_c, {1_c, 1_mu}, {1_awp, 1_anp}}}, allocation_info{node_alloc_getter<graph_t>{}, {0_c, {1_c, 1_mu}, {1_awp, 1_anp}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_semantics(LINE(""), g2, graph_t{{edge_init_t{1}}, {edge_init_t{0}}}, nodeMaker, allocation_info{edge_alloc_getter<graph_t>{}, {0_c, {1_c, 0_mu}, {1_awp, 1_anp}}}, allocation_info{partitions_alloc_getter<graph_t>{}, {0_c, {1_c, 1_mu}, {1_awp, 1_anp}}}, allocation_info{node_alloc_getter<graph_t>{}, {0_c, {1_c, 1_mu}, {1_awp, 1_anp}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_semantics(LINE(""), g2, graph_t{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}}}, nodeMaker, allocation_info{edge_alloc_getter<graph_t>{}, {0_c, {1_c, 0_mu}, {1_awp, 1_anp}}}, allocation_info{partitions_alloc_getter<graph_t>{}, {0_c, {1_c, 1_mu}, {1_awp, 1_anp}}}, allocation_info{node_alloc_getter<graph_t>{}, {0_c, {1_c, 1_mu}, {1_awp, 1_anp}}});
    }
    else
    {
      check_semantics(LINE(""), g2, graph_t{{edge_init_t{1,0}}, {edge_init_t{0,0}}}, nodeMaker, allocation_info{edge_alloc_getter<graph_t>{}, {0_c, {1_c, 0_mu}, {1_awp, 1_anp}}}, allocation_info{partitions_alloc_getter<graph_t>{}, {0_c, {1_c, 1_mu}, {1_awp, 1_anp}}}, allocation_info{node_alloc_getter<graph_t>{}, {0_c, {1_c, 1_mu}, {1_awp, 1_anp}}});
    }
  }

  template
  <
    maths::graph_flavour GraphFlavour,
    class EdgeWeight,
    class NodeWeight,
    class EdgeWeightPooling,
    class NodeWeightPooling,
    class EdgeStorageTraits,
    class NodeWeightStorageTraits
  >
  void graph_bucketed_memory<
      GraphFlavour,
      EdgeWeight,
      NodeWeight,
      EdgeWeightPooling,
      NodeWeightPooling,
      EdgeStorageTraits,
      NodeWeightStorageTraits
  >::execute_operations()
  {
    using edge_allocator = typename graph_t::edge_allocator_type;
    using node_allocator = typename graph_t::node_weight_container_type::allocator_type;
    
    graph_t g{edge_allocator{}, node_allocator{}};

    this->template check_exception_thrown<std::out_of_range>(LINE(""), [&g](){ g.reserve_edges(0, 4);});
    this->template check_exception_thrown<std::out_of_range>(LINE(""), [&g](){ return g.edges_capacity(0);});
    check_equality(LINE(""), g.node_capacity(), 0_sz);

    g.add_node();
    check_equality(LINE(""), g, graph_t{{{}}, edge_allocator{}, node_allocator{}});

    check_equality(LINE(""), g.edges_capacity(0), 0_sz);
    check_equality(LINE(""), g.node_capacity(), 1_sz);
    g.reserve_edges(0, 4);
    check_equality(LINE(""), g.edges_capacity(0), 4_sz);

    g.reserve_nodes(4);
    check_equality(LINE(""), g.node_capacity(), 4_sz);

    g.shrink_to_fit();
    check_equality(LINE("May fail if stl implementation doesn't actually shrink to fit!"), g.edges_capacity(0), 0_sz);
    check_equality(LINE("May fail if stl implementation doesn't actually shrink to fit!"), g.node_capacity(), 1_sz);

    g.insert_node(0u);
    check_equality(LINE(""), g, graph_t{{{}, {}}, edge_allocator{}, node_allocator{}});

    // x----x
    using edge_init_t = typename graph_t::edge_init_type;
    using namespace maths;
    using edge_allocator = typename graph_t::edge_allocator_type;

    auto nodeMaker{
      [](graph_t& g) { g.add_node(); }
    };

    graph_t g2{};

    using node_allocator = typename graph_t::node_weight_container_type::allocator_type;

    check_semantics(LINE(""),
                    g2,
                    graph_t{{{}}, edge_allocator{}, node_allocator{}},
                    nodeMaker,
                    allocation_info{edge_alloc_getter<graph_t>{},
                                      {0_c, {1_c, 1_mu}, {1_awp, 1_anp}},
                                      { {0_c, {0_c, 0_mu}, {0_awp, 0_anp}, {0_containers, 1_containers, 2_postmutation}} }
                    },
                    allocation_info{node_alloc_getter<graph_t>{}, {0_c, {1_c, 1_mu}, {1_awp, 1_anp}}});
      
    check_semantics(LINE(""),
                    g2,
                    graph_t{{{}}, edge_allocator{}, {{1.0, -1.0}}, node_allocator{}},
                    nodeMaker,
                    allocation_info{edge_alloc_getter<graph_t>{},
                                    {0_c, {1_c, 1_mu}, {1_awp, 1_anp}},
                                    { {0_c, {0_c, 0_mu}, {0_awp, 0_anp}, {0_containers, 1_containers, 2_postmutation}} }

                    },
                    allocation_info{node_alloc_getter<graph_t>{}, {0_c, {1_c, 1_mu}, {1_awp, 1_anp}}});

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_semantics(LINE(""),
                      g2,
                      graph_t{{edge_init_t{1}}, {}},
                      nodeMaker,
                      allocation_info{edge_alloc_getter<graph_t>{},
                                      {0_c, {1_c, 1_mu}, {1_awp, 1_anp}},
                                      { {0_c, {1_c, 0_mu}, {1_awp, 1_anp}, {0_containers, 2_containers, 3_postmutation}} }
                      },
                      allocation_info{node_alloc_getter<graph_t>{}, {0_c, {1_c, 1_mu}, {1_awp, 1_anp}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_semantics(LINE(""),
                      g2,
                      graph_t{{edge_init_t{1}}, {edge_init_t{0}}},
                      nodeMaker,
                      allocation_info{edge_alloc_getter<graph_t>{},
                                      {0_c, {1_c, 1_mu}, {1_awp, 1_anp}},
                                      { {0_c, {2_c, 0_mu}, {2_awp, 2_anp}, {0_containers, 2_containers, 3_postmutation}} }
                      },
                      allocation_info{node_alloc_getter<graph_t>{}, {0_c, {1_c, 1_mu}, {1_awp, 1_anp}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_semantics(LINE(""),
                      g2,
                      graph_t{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}}},
                      nodeMaker,
                      allocation_info{edge_alloc_getter<graph_t>{},
                                      {0_c, {1_c, 1_mu}, {1_awp, 1_anp}},
                                      { {0_c, {2_c, 0_mu}, {2_awp, 2_anp}, {0_containers, 2_containers, 3_postmutation}} }
                      },
                      allocation_info{node_alloc_getter<graph_t>{}, {0_c, {1_c, 1_mu}, {1_awp, 1_anp}}});
    }
    else
    {
      check_semantics(LINE(""),
                      g2,
                      graph_t{{edge_init_t{1,0}}, {edge_init_t{0,0}}},
                      nodeMaker,
                      allocation_info{edge_alloc_getter<graph_t>{},
                                      {0_c, {1_c, 1_mu}, {1_awp, 1_anp}},
                                      { {0_c, {2_c, 0_mu}, {2_awp, 2_anp}, {0_containers, 2_containers, 3_postmutation}} }
                      },
                      allocation_info{node_alloc_getter<graph_t>{}, {0_c, {1_c, 1_mu}, {1_awp, 1_anp}}});
    }
  }
}
