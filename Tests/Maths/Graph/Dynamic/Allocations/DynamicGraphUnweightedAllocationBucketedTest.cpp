////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "DynamicGraphUnweightedAllocationBucketedTest.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path unweighted_graph_allocation_bucketed_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void unweighted_graph_allocation_bucketed_test::run_tests()
  {
    using namespace maths;

    graph_test_helper<null_weight, null_weight, unweighted_graph_allocation_bucketed_test> helper{*this};
    helper.run_tests<custom_allocator_bucketed_edge_storage_config, node_storage<null_weight>>();
  }

  template
  <
    maths::graph_flavour GraphFlavour,
    class EdgeWeight,
    class NodeWeight,
    class EdgeStorageConfig,
    class NodeWeightStorage
  >
  void unweighted_graph_allocation_bucketed_test::execute_operations()
  {
    using graph_type = graph_type_generator_t<GraphFlavour, EdgeWeight, NodeWeight, EdgeStorageConfig, NodeWeightStorage>;
    
    bucketed_memory<graph_type>();
  }

  template<maths::dynamic_network Graph>
  void unweighted_graph_allocation_bucketed_test::bucketed_memory()
  {
    using namespace maths;
    using edge_allocator = typename Graph::edge_allocator_type;
    constexpr auto GraphFlavour{Graph::flavour};

    Graph g{edge_allocator{}};

    //this->template check_exception_thrown<std::out_of_range>("", [&g](){ g.reserve_edges(0, 4);});
    //this->template check_exception_thrown<std::out_of_range>("", [&g](){ return g.edges_capacity(0);});
    check(equality, "", g.node_capacity(), 0uz);

    g.add_node();
    check(equality, "", g, Graph{{{}}, edge_allocator{}});

    check(equality, "", g.edges_capacity(0), 0uz);
    check(equality, "", g.node_capacity(), 1uz);
    g.reserve_edges(0, 4);
    check(equality, "", g.edges_capacity(0), 4uz);

    g.reserve_nodes(4);
    check(equality, "", g.node_capacity(), 4uz);

    g.shrink_to_fit();
    check(equality, "May fail if stl implementation doesn't actually shrink to fit!", g.edges_capacity(0), 0uz);
    check(equality, "May fail if stl implementation doesn't actually shrink to fit!", g.node_capacity(), 1uz);

    g.insert_node(0u);
    check(equality, "", g, Graph{{{}, {}}, edge_allocator{}});

    using edge_init_t = typename Graph::edge_init_type;
    using edge_allocator = typename Graph::edge_allocator_type;

    auto nodeMaker{
      [](Graph& g) { g.add_node(); }
    };

    Graph g2{};

    check_semantics("",
                    g2,
                    Graph{{{}}, edge_allocator{}},
                    nodeMaker,
                    allocation_info{edge_alloc_getter<Graph>{},
                                    {0_c, {1_c, 1_mu}, {1_anp, 1_awp}},
                                    { {0_c, {0_c, 0_mu}, {0_anp, 0_awp}, {0_containers, 1_containers, 2_postmutation}} }
                    });

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_semantics("",
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
      check_semantics("",
                      g2,
                      Graph{{edge_init_t{1}}, {edge_init_t{0}}},
                      nodeMaker,
                      allocation_info{edge_alloc_getter<Graph>{},
                                      {0_c, {1_c, 1_mu}, {1_anp, 1_awp}},
                                      { {0_c, {2_c, 0_mu}, {2_anp, 2_awp}, {0_containers, 2_containers, 3_postmutation}} }
                      });
    }
  }
}
