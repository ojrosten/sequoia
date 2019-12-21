////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "DynamicGraphCommonTest.hpp"

#include <complex>

namespace sequoia::unit_testing
{
  void test_graph::run_tests()
  {
    using std::complex;
    using namespace maths;
    using namespace data_structures;

    struct null_weight {};
      
    {
      graph_test_helper<null_weight, null_weight> helper{};
      
      helper.run_tests<generic_graph_operations>(*this);
      helper.run_storage_tests<custom_allocator_contiguous_edge_storage_traits, custom_node_weight_storage_traits, graph_contiguous_memory>(*this);
      helper.run_storage_tests<custom_allocator_bucketed_edge_storage_traits, custom_node_weight_storage_traits, graph_bucketed_memory>(*this);
    }
      
    {
      graph_test_helper<int, complex<double>>  helper{};
      
      helper.run_tests<generic_weighted_graph_tests>(*this);
      helper.run_storage_tests<custom_allocator_contiguous_edge_storage_traits, custom_node_weight_storage_traits, graph_contiguous_memory>(*this);
      helper.run_storage_tests<custom_allocator_bucketed_edge_storage_traits, custom_node_weight_storage_traits, graph_bucketed_memory>(*this);
    }

    {
      graph_test_helper<complex<int>, complex<double>>  helper{};
      helper.run_tests<generic_weighted_graph_tests>(*this);
      helper.run_storage_tests<custom_allocator_contiguous_edge_storage_traits, custom_node_weight_storage_traits, graph_contiguous_memory>(*this);
      helper.run_storage_tests<custom_allocator_bucketed_edge_storage_traits, custom_node_weight_storage_traits, graph_bucketed_memory>(*this);
    }

    {
      graph_test_helper<std::vector<int>, std::vector<complex<double>>>  helper{};
      
      helper.run_tests<generic_weighted_graph_tests>(*this);
      helper.run_storage_tests<custom_allocator_contiguous_edge_storage_traits, custom_node_weight_storage_traits, graph_contiguous_memory>(*this);
      helper.run_storage_tests<custom_allocator_bucketed_edge_storage_traits, custom_node_weight_storage_traits, graph_bucketed_memory>(*this);
    }
  }

  // Generic Graph Operations
  
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
  void generic_graph_operations<
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
    using edge_init_t = typename graph_t::edge_init_type;    

    graph_t network;

    check_exception_thrown<std::out_of_range>(LINE("cbegin_edges throws for empty graph"), [&network]() { return network.cbegin_edges(0); });
    check_exception_thrown<std::out_of_range>(LINE("cend_edges throws for empty graph"), [&network]() { return network.cend_edges(0); });

    check_exception_thrown<std::out_of_range>(LINE("swapping nodes throws for empty graph"), [&network]() { network.swap_nodes(0,0); });
          
    check_equality(LINE("Index of added node is 0"), network.add_node(), 0ul);
    //    0
    
    check_equality(LINE(""), network, graph_t{{}});
    check_regular_semantics(LINE("Regular semantics"), network, graph_t{});

    check_exception_thrown<std::out_of_range>(LINE("Unable to join zeroth node to non-existant first node"), [&network]() { network.join(0, 1); });

    check_exception_thrown<std::out_of_range>(LINE("Index out of range for swapping nodes"), [&network]() { network.swap_nodes(0,1); });
    
    check_equality(LINE("Index of added node is 1"), network.add_node(), 1ul);
    //    0    1

    check_equality(LINE(""), network, graph_t{{}, {}});
    check_regular_semantics(LINE("Regular semantics"), network, graph_t{{}});

    network.swap_nodes(0,1);
    
    check_equality(LINE(""), network, graph_t{{}, {}});
    
    network.join(0, 1);
    //    0----1

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), network, graph_t{{edge_init_t{1}}, {}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_equality(LINE(""), network, graph_t{{edge_init_t{1}}, {edge_init_t{0}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), network, graph_t{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}}});
    }
    else
    {
      check_equality(LINE(""), network, graph_t{{edge_init_t{1,0}}, {edge_init_t{0,0}}});
    }

    check_regular_semantics(LINE("Regular semantics"), network, graph_t{{}, {}});
    
    network.swap_nodes(0,1);

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), network, graph_t{{}, {edge_init_t{0}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_equality(LINE(""), network, graph_t{{edge_init_t{1}}, {edge_init_t{0}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), network, graph_t{{edge_init_t{1,0,0}}, {edge_init_t{1,0,0}}});
    }
    else
    {
      check_equality(LINE(""), network, graph_t{{edge_init_t{1,0}}, {edge_init_t{0,0}}});
    }
    
    network.swap_nodes(1,0);

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), network, graph_t{{edge_init_t{1}}, {}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_equality(LINE(""), network, graph_t{{edge_init_t{1}}, {edge_init_t{0}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), network, graph_t{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}}});
    }
    else
    {
      check_equality(LINE(""), network, graph_t{{edge_init_t{1,0}}, {edge_init_t{0,0}}});
    }
    
    auto nodeNum{network.add_node()};
    check_equality(LINE("Index of added node is 2"), nodeNum, 2ul);
    network.join(1, nodeNum);
    //    0----1----2

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), network, graph_t{{edge_init_t{1}}, {edge_init_t{2}}, {}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_equality(LINE(""), network, graph_t{{edge_init_t{1}}, {edge_init_t{0}, edge_init_t{2}}, {edge_init_t{1}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {      
      check_equality(LINE(""), network, graph_t{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}, edge_init_t{1,2,0}}, {edge_init_t{1,2,1}}});
    }
    else
    {  
      check_equality(LINE(""), network, graph_t{{edge_init_t{1,0}}, {edge_init_t{0,0}, edge_init_t{2,0}}, {edge_init_t{1,1}}});
    }

    network.swap_nodes(2,1);

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), network, graph_t{{edge_init_t{2}}, {}, {edge_init_t{1}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_equality(LINE(""), network, graph_t{{edge_init_t{2}}, {edge_init_t{2}}, {edge_init_t{0}, edge_init_t{1}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {      
      check_equality(LINE(""), network, graph_t{{edge_init_t{0,2,0}}, {edge_init_t{2,1,1}}, {edge_init_t{0,2,0}, edge_init_t{2,1,0}}});
    }
    else
    {
      check_equality(LINE(""), network, graph_t{{edge_init_t{2,0}}, {edge_init_t{2,1}}, {edge_init_t{0,0}, edge_init_t{1,0}}});
    }
    
    network.swap_nodes(1,2);
     //    0----1----2

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), network, graph_t{{edge_init_t{1}}, {edge_init_t{2}}, {}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_equality(LINE(""), network, graph_t{{edge_init_t{1}}, {edge_init_t{0}, edge_init_t{2}}, {edge_init_t{1}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {      
      check_equality(LINE(""), network, graph_t{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}, edge_init_t{1,2,0}}, {edge_init_t{1,2,1}}});
    }
    else
    {
      check_equality(LINE(""), network, graph_t{{edge_init_t{1,0}}, {edge_init_t{0,0}, edge_init_t{2,0}}, {edge_init_t{1,1}}});
    }

    network.swap_nodes(0,2);

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), network, graph_t{{}, {edge_init_t{0}}, {edge_init_t{1}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t g{{edge_init_t{1}}, {edge_init_t{0}, edge_init_t{2}}, {edge_init_t{1}}};
      g.sort_edges(g.cbegin_edges(1), g.cend_edges(1), [](const auto& l, const auto& r) {
          return l.target_node() > r.target_node();
        }
      );
      
      check_equality(LINE(""), network, g);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {      
      check_equality(LINE(""), network, graph_t{{edge_init_t{1,0,1}}, {edge_init_t{2,1,0}, edge_init_t{1,0,0}}, {edge_init_t{2,1,0}}});
    }
    else
    {   
      check_equality(LINE(""), network, graph_t{{edge_init_t{1,1}}, {edge_init_t{2,0}, edge_init_t{0,0}}, {edge_init_t{1,0}}});
    }

    network.swap_nodes(0,2);

    if constexpr (GraphFlavour == graph_flavour::directed)
    {      
      check_equality(LINE(""), network, graph_t{{edge_init_t{1}}, {edge_init_t{2}}, {}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_equality(LINE(""), network, graph_t{{edge_init_t{1}}, {edge_init_t{0}, edge_init_t{2}}, {edge_init_t{1}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {      
      check_equality(LINE(""), network, graph_t{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}, edge_init_t{1,2,0}}, {edge_init_t{1,2,1}}});
    }
    else
    {      
      check_equality(LINE(""), network, graph_t{{edge_init_t{1,0}}, {edge_init_t{0,0}, edge_init_t{2,0}}, {edge_init_t{1,1}}});
    }
    
    nodeNum = network.add_node();
    check_equality(LINE("Index of added node is 3"), nodeNum, 3ul);
    network.join(0, nodeNum);
    //    0----1----2
    //    |
    //    3

    if constexpr (GraphFlavour == graph_flavour::directed)
    {      
      check_equality(LINE(""), network, graph_t{{edge_init_t{1}, edge_init_t{3}}, {edge_init_t{2}}, {}, {}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_equality(LINE(""), network, graph_t{{edge_init_t{1}, edge_init_t{3}}, {edge_init_t{0}, edge_init_t{2}}, {edge_init_t{1}}, {edge_init_t{0}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {      
      check_equality(LINE(""), network, graph_t{{edge_init_t{0,1,0}, edge_init_t{0,3,0}}, {edge_init_t{0,1,0}, edge_init_t{1,2,0}}, {edge_init_t{1,2,1}}, {edge_init_t{0,3,1}}});
    }
    else
    {      
      check_equality(LINE(""), network, graph_t{{edge_init_t{1,0}, edge_init_t{3,0}}, {edge_init_t{0,0}, edge_init_t{2,0}}, {edge_init_t{1,1}}, {edge_init_t{0,1}}});
    }

    network.erase_node(0);
    //    0----1
    //
    //    2

    if constexpr (GraphFlavour == graph_flavour::directed)
    {      
      check_equality(LINE(""), network, graph_t{{edge_init_t{1}}, {}, {}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_equality(LINE(""), network, graph_t{{edge_init_t{1}}, {edge_init_t{0}}, {}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {      
      check_equality(LINE(""), network, graph_t{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}}, {}});
    }
    else
    {      
      check_equality(LINE(""), network, graph_t{{edge_init_t{1,0}}, {edge_init_t{0,0}}, {}});
    }

    network.swap_nodes(0,2);

    if constexpr (GraphFlavour == graph_flavour::directed)
    {      
      check_equality(LINE(""), network, graph_t{{}, {}, {edge_init_t{1}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_equality(LINE(""), network, graph_t{{}, {edge_init_t{2}}, {edge_init_t{1}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {      
      check_equality(LINE(""), network, graph_t{{}, {edge_init_t{2,1,0}}, {edge_init_t{2,1,0}}});
    }
    else
    {      
      check_equality(LINE(""), network, graph_t{{}, {edge_init_t{2,0}}, {edge_init_t{1,0}}});
    }
    
    network.swap_nodes(2,0);

    if constexpr (GraphFlavour == graph_flavour::directed)
    {      
      check_equality(LINE(""), network, graph_t{{edge_init_t{1}}, {}, {}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_equality(LINE(""), network, graph_t{{edge_init_t{1}}, {edge_init_t{0}}, {}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {      
      check_equality(LINE(""), network, graph_t{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}}, {}});
    }
    else
    {      
      check_equality(LINE(""), network, graph_t{{edge_init_t{1,0}}, {edge_init_t{0,0}}, {}});
    }
    
    network.erase_edge(network.cbegin_edges(0));
    //    0    1
    //
    //    2

    check_equality(LINE(""), network, graph_t{{}, {}, {}});
          
    network.join(0, 1);
    network.join(1, 1);
    network.join(1, 0);
    //    0======1
    //           /\
    //    2      \/

    if constexpr (GraphFlavour == graph_flavour::directed)
    {      
      check_equality(LINE(""), network, graph_t{{edge_init_t{1}}, {edge_init_t{1}, edge_init_t{0}}, {}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t g{{edge_init_t{1}, edge_init_t{1}}, {edge_init_t{0}, edge_init_t{0}, edge_init_t{1}, edge_init_t{1}}, {}};
      g.sort_edges(g.cbegin_edges(1) + 1, g.cend_edges(1), [](const auto& l, const auto& r) {
          return l.target_node() > r.target_node();
        }
      );
      
      check_equality(LINE(""), network, g);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {      
      check_equality(LINE(""), network, graph_t{{edge_init_t{0,1,0}, edge_init_t{1, 0, 3}}, {edge_init_t{0,1,0}, edge_init_t{1,1,2}, edge_init_t{1,1,1}, edge_init_t{1, 0, 1}}, {}});
    }
    else
    {      
      check_equality(LINE(""), network, graph_t{{edge_init_t{1,0}, edge_init_t{1,3}}, {edge_init_t{0,0}, edge_init_t{1,2}, edge_init_t{1,1}, edge_init_t{0,1}}, {}});
    }
        
    network.swap_nodes(1,0);

    if constexpr (GraphFlavour == graph_flavour::directed)
    {      
      check_equality(LINE(""), network, graph_t{{edge_init_t{0}, edge_init_t{1}}, {edge_init_t{0}}, {}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t g{{edge_init_t{0}, edge_init_t{0}, edge_init_t{1}, edge_init_t{1}}, {edge_init_t{0}, edge_init_t{0}}, {}};
      g.sort_edges(g.cbegin_edges(0), g.cbegin_edges(0)+3, [](const auto& l, const auto& r) {
          return l.target_node() > r.target_node();
        }
      );
      
      check_equality(LINE(""), network, g);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {      
      check_equality(LINE(""), network, graph_t{{edge_init_t{1,0,0}, edge_init_t{0,0,2}, edge_init_t{0,0,1}, edge_init_t{0, 1, 1}}, {edge_init_t{1,0,0}, edge_init_t{0, 1, 3}}, {}});
    }
    else
    {      
      check_equality(LINE(""), network, graph_t{{edge_init_t{1,0}, edge_init_t{0,2}, edge_init_t{0,1}, edge_init_t{1,1}}, {edge_init_t{0,0}, edge_init_t{0,3}}, {}});
    }

    network.swap_nodes(0,1);

    if constexpr (GraphFlavour == graph_flavour::directed)
    {      
      check_equality(LINE(""), network, graph_t{{edge_init_t{1}}, {edge_init_t{1}, edge_init_t{0}}, {}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t g{{edge_init_t{1}, edge_init_t{1}}, {edge_init_t{0}, edge_init_t{0}, edge_init_t{1}, edge_init_t{1}}, {}};
      g.sort_edges(g.cbegin_edges(1) + 1, g.cend_edges(1), [](const auto& l, const auto& r) {
          return l.target_node() > r.target_node();
        }
      );
      
      check_equality(LINE(""), network, g);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {      
      check_equality(LINE(""), network, graph_t{{edge_init_t{0,1,0}, edge_init_t{1, 0, 3}}, {edge_init_t{0,1,0}, edge_init_t{1,1,2}, edge_init_t{1,1,1}, edge_init_t{1, 0, 1}}, {}});
    }
    else
    {      
      check_equality(LINE(""), network, graph_t{{edge_init_t{1,0}, edge_init_t{1,3}}, {edge_init_t{0,0}, edge_init_t{1,2}, edge_init_t{1,1}, edge_init_t{0,1}}, {}});
    }

    //    0======1
    //           /\
    //    2      \/

    network.swap_nodes(0,2);

    if constexpr (GraphFlavour == graph_flavour::directed)
    {      
      check_equality(LINE(""), network, graph_t{{}, {edge_init_t{1}, edge_init_t{2}}, {edge_init_t{1}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t g{{}, {edge_init_t{1}, edge_init_t{1}, edge_init_t{2}, edge_init_t{2}}, {edge_init_t{1}, edge_init_t{1}}};
      g.sort_edges(g.cbegin_edges(1), g.cbegin_edges(1) + 3, [](const auto& l, const auto& r) {
          return l.target_node() > r.target_node();
        }
      );
      
      check_equality(LINE(""), network, g);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {      
      check_equality(LINE(""), network, graph_t{{}, {edge_init_t{2,1,0}, edge_init_t{1,1,2}, edge_init_t{1,1,1}, edge_init_t{1, 2, 1}}, {edge_init_t{2,1,0}, edge_init_t{1, 2, 3}}});
    }
    else
    {      
      check_equality(LINE(""), network, graph_t{{}, {edge_init_t{2,0}, edge_init_t{1,2}, edge_init_t{1,1}, edge_init_t{2,1}}, {edge_init_t{1,0}, edge_init_t{1,3}}});
    }

    network.swap_nodes(2,0);

    if constexpr (GraphFlavour == graph_flavour::directed)
    {      
      check_equality(LINE(""), network, graph_t{{edge_init_t{1}}, {edge_init_t{1}, edge_init_t{0}}, {}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t g{{edge_init_t{1}, edge_init_t{1}}, {edge_init_t{0}, edge_init_t{0}, edge_init_t{1}, edge_init_t{1}}, {}};
      g.sort_edges(g.cbegin_edges(1) + 1, g.cend_edges(1), [](const auto& l, const auto& r) {
          return l.target_node() > r.target_node();
        }
      );
      
      check_equality(LINE(""), network, g);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {      
      check_equality(LINE(""), network, graph_t{{edge_init_t{0,1,0}, edge_init_t{1, 0, 3}}, {edge_init_t{0,1,0}, edge_init_t{1,1,2}, edge_init_t{1,1,1}, edge_init_t{1, 0, 1}}, {}});
    }
    else
    {      
      check_equality(LINE(""), network, graph_t{{edge_init_t{1,0}, edge_init_t{1,3}}, {edge_init_t{0,0}, edge_init_t{1,2}, edge_init_t{1,1}, edge_init_t{0,1}}, {}});
    }

    network.erase_edge(mutual_info(GraphFlavour) ? ++network.cbegin_edges(0) : network.cbegin_edges(0));
    //    0------1
    //           /\
    //    2      \/

    if constexpr (GraphFlavour == graph_flavour::directed)
    {      
      check_equality(LINE(""), network, graph_t{{}, {edge_init_t{1}, edge_init_t{0}}, {}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t g{{edge_init_t{1}}, { edge_init_t{0}, edge_init_t{1}, edge_init_t{1}}, {}};
      g.sort_edges(g.cbegin_edges(1), g.cend_edges(1), [](const auto& l, const auto& r) {
          return l.target_node() > r.target_node();
        }
      );
      
      check_equality(LINE(""), network, g);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {      
      check_equality(LINE(""), network, graph_t{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}, edge_init_t{1,1,2}, edge_init_t{1,1,1}}, {}});
    }
    else
    {      
      check_equality(LINE(""), network, graph_t{{edge_init_t{1,0}}, {edge_init_t{0,0}, edge_init_t{1,2}, edge_init_t{1,1}}, {}});
    }
    
    network.join(2,1);
    //    0------1------2
    //           /\
    //           \/

    if constexpr (GraphFlavour == graph_flavour::directed)
    {      
      check_equality(LINE(""), network, graph_t{{}, {edge_init_t{1}, edge_init_t{0}}, {edge_init_t{1}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t g{{edge_init_t{1}}, { edge_init_t{0}, edge_init_t{1}, edge_init_t{1}, edge_init_t{2}}, {edge_init_t{1}}};
      g.sort_edges(g.cbegin_edges(1), g.cend_edges(1) -1, [](const auto& l, const auto& r) {
          return l.target_node() > r.target_node();
        }
      );
      
      check_equality(LINE(""), network, g);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {      
      check_equality(LINE(""), network, graph_t{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}, edge_init_t{1,1,2}, edge_init_t{1,1,1}, edge_init_t{2,1,0}}, {edge_init_t{2,1,3}}});
    }
    else
    {      
      check_equality(LINE(""), network, graph_t{{edge_init_t{1,0}}, {edge_init_t{0,0}, edge_init_t{1,2}, edge_init_t{1,1}, edge_init_t{2,0}}, {edge_init_t{1,3}}});
    }
          
    network.erase_edge(mutual_info(GraphFlavour) ? ++network.cbegin_edges(1) : network.cbegin_edges(1));
    //    0------1------2
 
    if constexpr (GraphFlavour == graph_flavour::directed)
    {      
      check_equality(LINE(""), network, graph_t{{}, {edge_init_t{0}}, {edge_init_t{1}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {      
      check_equality(LINE(""), network, graph_t{{edge_init_t{1}}, { edge_init_t{0}, edge_init_t{2}}, {edge_init_t{1}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {      
      check_equality(LINE(""), network, graph_t{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}, edge_init_t{2,1,0}}, {edge_init_t{2,1,1}}});
    }
    else
    {      
      check_equality(LINE(""), network, graph_t{{edge_init_t{1,0}}, {edge_init_t{0,0}, edge_init_t{2,0}}, {edge_init_t{1,1}}});
    }

    network.erase_edge(network.cbegin_edges(2));
    //    0------1     2
    
    if constexpr (GraphFlavour == graph_flavour::directed)
    {      
      check_equality(LINE(""), network, graph_t{{}, {edge_init_t{0}}, {}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {      
      check_equality(LINE(""), network, graph_t{{edge_init_t{1}}, {edge_init_t{0}}, {}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {      
      check_equality(LINE(""), network, graph_t{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}}, {}});
    }
    else
    {      
      check_equality(LINE(""), network, graph_t{{edge_init_t{1,0}}, {edge_init_t{0,0}}, {}});
    }

    network.join(1,1);
    network.join(2,1);

    //    0------1------2
    //           /\
    //           \/

    if constexpr (GraphFlavour == graph_flavour::directed)
    {      
      check_equality(LINE(""), network, graph_t{{}, {edge_init_t{0}, edge_init_t{1}}, {edge_init_t{1}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {      
      check_equality(LINE(""), network, graph_t{{edge_init_t{1}}, {edge_init_t{0}, edge_init_t{1}, edge_init_t{1}, edge_init_t{2}}, {edge_init_t{1}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {      
      check_equality(LINE(""), network, graph_t{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}, edge_init_t{1,1,2}, edge_init_t{1,1,1}, edge_init_t{2,1,0}}, {edge_init_t{2,1,3}}});
    }
    else
    {      
      check_equality(LINE(""), network, graph_t{{edge_init_t{1,0}}, {edge_init_t{0,0}, edge_init_t{1,2}, edge_init_t{1,1}, edge_init_t{2,0}}, {edge_init_t{1,3}}});
    }
    
    network.erase_edge(mutual_info(GraphFlavour) ? network.cbegin_edges(1)+2 : network.cbegin_edges(1)+1);
    //    0------1------2
 
    if constexpr (GraphFlavour == graph_flavour::directed)
    {      
      check_equality(LINE(""), network, graph_t{{}, {edge_init_t{0}}, {edge_init_t{1}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {      
      check_equality(LINE(""), network, graph_t{{edge_init_t{1}}, {edge_init_t{0}, edge_init_t{2}}, {edge_init_t{1}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {      
      check_equality(LINE(""), network, graph_t{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}, edge_init_t{2,1,0}}, {edge_init_t{2,1,1}}});
    }
    else
    {      
      check_equality(LINE(""), network, graph_t{{edge_init_t{1,0}}, {edge_init_t{0,0}, edge_init_t{2,0}}, {edge_init_t{1,1}}});
    }
        
    network.erase_edge(network.cbegin_edges(1));
    network.erase_edge(network.cbegin_edges(2));
    network.join(1,1);
    //    0      1
    //           /\
    //    2      \/

    if constexpr (GraphFlavour == graph_flavour::directed)
    {      
      check_equality(LINE(""), network, graph_t{{}, {edge_init_t{1}}, {}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {      
      check_equality(LINE(""), network, graph_t{{}, {edge_init_t{1}, edge_init_t{1}}, {}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {      
      check_equality(LINE(""), network, graph_t{{}, {edge_init_t{1,1,1}, edge_init_t{1,1,0}}, {}});
    }
    else
    {      
      check_equality(LINE(""), network, graph_t{{}, {edge_init_t{1,1}, edge_init_t{1,0}}, {}});
    }
 
    network.erase_edge(network.cbegin_edges(1));
    //    0    1
    //
    //    2

    check_equality(LINE(""), network, graph_t{{}, {}, {}});
          
    network.join(0, 1);
    network.join(0, 1);
    network.join(0, 2);
    network.join(0, 2);
    network.join(1, 2);
    network.join(2, 1);
    //    0=====1
    //    ||  //
    //    ||//
    //     2

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), network,
                     graph_t{
                       {edge_init_t{1}, edge_init_t{1}, edge_init_t{2}, edge_init_t{2}},
                       {edge_init_t{2}},
                       {edge_init_t{1}}
                     });
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {      
      check_equality(LINE(""), network,
                     graph_t{
                       {edge_init_t{1}, edge_init_t{1}, edge_init_t{2}, edge_init_t{2}},
                       {edge_init_t{0}, edge_init_t{0}, edge_init_t{2}, edge_init_t{2}},
                       {edge_init_t{0}, edge_init_t{0}, edge_init_t{1}, edge_init_t{1}}
                     });
    }    
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), network,
                     graph_t{
                       {edge_init_t{0,1,0}, edge_init_t{0,1,1}, edge_init_t{0,2,0}, edge_init_t{0,2,1}},
                       {edge_init_t{0,1,0}, edge_init_t{0,1,1}, edge_init_t{1,2,2}, edge_init_t{2,1,3}},
                       {edge_init_t{0,2,2}, edge_init_t{0,2,3}, edge_init_t{1,2,2}, edge_init_t{2,1,3}}
                     });
    }
    else 
    { 
      check_equality(LINE(""), network,
                     graph_t{
                       {edge_init_t{1,0}, edge_init_t{1,1}, edge_init_t{2,0}, edge_init_t{2,1}},
                       {edge_init_t{0,0}, edge_init_t{0,1}, edge_init_t{2,2}, edge_init_t{2,3}},
                       {edge_init_t{0,2}, edge_init_t{0,3}, edge_init_t{1,2}, edge_init_t{1,3}}
                     });
    }
    
    mutual_info(GraphFlavour) ? network.erase_edge(network.cbegin_edges(2)+2) : network.erase_edge(network.cbegin_edges(2));

    //    0=====1
    //    ||  /
    //    ||/
    //     2

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), network,
                     graph_t{
                       {edge_init_t{1}, edge_init_t{1}, edge_init_t{2}, edge_init_t{2}},
                       {edge_init_t{2}},
                       {}
                     });
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {      
      check_equality(LINE(""), network,
                     graph_t{
                       {edge_init_t{1}, edge_init_t{1}, edge_init_t{2}, edge_init_t{2}},
                       {edge_init_t{0}, edge_init_t{0}, edge_init_t{2}},
                       {edge_init_t{0}, edge_init_t{0}, edge_init_t{1}}
                     });
    }    
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), network,
                     graph_t{
                       {edge_init_t{0,1,0}, edge_init_t{0,1,1}, edge_init_t{0,2,0}, edge_init_t{0,2,1}},
                       {edge_init_t{0,1,0}, edge_init_t{0,1,1}, edge_init_t{2,1,2}},
                       {edge_init_t{0,2,2}, edge_init_t{0,2,3}, edge_init_t{2,1,2}}
                     });
    }
    else 
    { 
      check_equality(LINE(""), network,
                     graph_t{
                       {edge_init_t{1,0}, edge_init_t{1,1}, edge_init_t{2,0}, edge_init_t{2,1}},
                       {edge_init_t{0,0}, edge_init_t{0,1}, edge_init_t{2,2}},
                       {edge_init_t{0,2}, edge_init_t{0,3}, edge_init_t{1,2}}
                     });
    }
        
    network.erase_node(0);
    //  0----1

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), network, graph_t{{edge_init_t{1}}, {}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {      
      check_equality(LINE(""), network, graph_t{{edge_init_t{1}}, {edge_init_t{0}}});
    }    
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), network,  graph_t{{edge_init_t{1,0,0}}, {edge_init_t{1,0,0}}});
    }
    else 
    { 
      check_equality(LINE(""), network,  graph_t{{edge_init_t{1,0}}, {edge_init_t{0,0}}});
    }

    network.join(0, 0);
    // /\
    // \/
    //  0---1

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), network, graph_t{{edge_init_t{1}, edge_init_t{0}}, {}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t g{{edge_init_t{0}, edge_init_t{0}, edge_init_t{1}}, {edge_init_t{0}}};
      g.sort_edges(g.cbegin_edges(0), g.cend_edges(0),
                   [](const auto& l, const auto& r) {return l.target_node() > r.target_node();});
      
      check_equality(LINE(""), network, g);
    }    
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), network,
                     graph_t{
                       {edge_init_t{1,0,0}, edge_init_t{0,0,2}, edge_init_t{0,0,1}},
                       {edge_init_t{1,0,0}}
                     });
    }
    else 
    { 
      check_equality(LINE(""), network,
                     graph_t{
                       {edge_init_t{1,0}, edge_init_t{0,2}, edge_init_t{0,1}},
                       {edge_init_t{0,0}}
                     });
    }
          
    network.erase_edge(++network.cbegin_edges(0));
    //  0----1

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), network, graph_t{{edge_init_t{1}}, {}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {      
      check_equality(LINE(""), network, graph_t{{edge_init_t{1}}, {edge_init_t{0}}});
    }    
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), network,  graph_t{{edge_init_t{1,0,0}}, {edge_init_t{1,0,0}}});
    }
    else 
    { 
      check_equality(LINE(""), network,  graph_t{{edge_init_t{1,0}}, {edge_init_t{0,0}}});
    }

    network.erase_node(0);
    // 0

    check_equality(LINE(""), network, graph_t{{}});

    check_equality(LINE(""), network, {{}});

    network.erase_node(0);
    //

    check_equality(LINE(""), network, {});

    check_equality(LINE("Node added back to graph"), network.add_node(), 0ul);
    // 0
    
    network.join(0, 0);
    // /\
    // \/
    // 0

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), network, graph_t{{edge_init_t{0}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {      
      check_equality(LINE(""), network, graph_t{{edge_init_t{0}, edge_init_t{0}}});
    }    
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), network,  graph_t{{edge_init_t{0,0,1}, edge_init_t{0,0,0}}});
    }
    else 
    {
      check_equality(LINE(""), network,  graph_t{{edge_init_t{0,1}, edge_init_t{0,0}}});
    }

    check_equality(LINE("Prepend a node to the exising graph"), network.insert_node(0), 0ul);
    //    /\
    //    \/
    // 0  0

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), network, graph_t{{}, {edge_init_t{1}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {      
      check_equality(LINE(""), network, graph_t{{}, {edge_init_t{1}, edge_init_t{1}}});
    }    
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), network,  graph_t{{}, {edge_init_t{1,1,1}, edge_init_t{1,1,0}}});
    }
    else 
    {
      check_equality(LINE(""), network,  graph_t{{}, {edge_init_t{1,1}, edge_init_t{1,0}}});
    }

    network.join(0,1);
    network.insert_node(1);
    //       /\
    //       \/
    // 0  0  0
    // \    /
    //  \  /
    //   \/

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), network, graph_t{{edge_init_t{2}}, {}, {edge_init_t{2}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t g{{edge_init_t{2}}, {}, {edge_init_t{0}, edge_init_t{2}, edge_init_t{2}}};
      g.sort_edges(g.cbegin_edges(2), g.cend_edges(2), [](const auto& l, const auto& r) { return l.target_node() > r.target_node(); });
      check_equality(LINE(""), network, g);
    }    
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), network,  graph_t{{edge_init_t{0,2,2}}, {}, {edge_init_t{2,2,1}, edge_init_t{2,2,0}, edge_init_t{0,2,0}}});
    }
    else 
    { 
      check_equality(LINE(""), network,  graph_t{{edge_init_t{2,2}}, {}, {edge_init_t{2,1}, edge_init_t{2,0}, edge_init_t{0,0}}});
    }

    
    network.erase_node(1);
    network.erase_node(0);
    // /\
    // \/
    // 0

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), network, graph_t{{edge_init_t{0}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {      
      check_equality(LINE(""), network, graph_t{{edge_init_t{0}, edge_init_t{0}}});
    }    
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), network,  graph_t{{edge_init_t{0,0,1}, edge_init_t{0,0,0}}});
    }
    else 
    { 
      check_equality(LINE(""), network,  graph_t{{edge_init_t{0,1}, edge_init_t{0,0}}});
    }
        
    graph_t network2;
      
    check_equality(LINE("Node added to second network"), network2.add_node(), 0ul);
    check_equality(LINE("Second node added to second network"), network2.add_node(), 1ul);
    check_equality(LINE("Third node added to second network"), network2.add_node(), 2ul);
    check_equality(LINE("Fourth node added to second network"), network2.add_node(), 3ul);

    network2.join(0, 1);
    network2.join(3, 2);
    // 0----0    0----0

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE("Check di-component graph"), network2, graph_t{{edge_init_t{1}}, {}, {}, {edge_init_t{2}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {      
      check_equality(LINE("Check di-component graph"), network2, graph_t{{edge_init_t{1}}, {edge_init_t{0}}, {edge_init_t{3}}, {edge_init_t{2}}});
    }    
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE("Check di-component graph"), network2,  graph_t{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}}, {edge_init_t{3,2,0}}, {edge_init_t{3,2,0}}});
    }
    else 
    { 
      check_equality(LINE("Check di-component graph"), network2, graph_t{{edge_init_t{1,0}}, {edge_init_t{0,0}}, {edge_init_t{3,0}}, {edge_init_t{2,0}}});
    }

    check_regular_semantics(LINE("Regular semantics"), network, network2);
  }

  // Capacity

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

    auto maker{
      [](){
        if constexpr(std::is_empty_v<NodeWeight>)
        {
          return graph_t(edge_allocator{}, edge_partitions_allocator{});
        }
        else
        {
          using node_allocator = typename graph_t::node_weight_container_type::allocator_type;
          return graph_t{edge_allocator{}, edge_partitions_allocator{}, node_allocator{}};
        }
      }
    };

    // null
    graph_t g{maker()};

    check_equality(LINE(""), g.edges_capacity(), 0ul);
    check_equality(LINE(""), g.node_capacity(), 0ul);

    g.reserve_edges(4);
    check_equality(LINE(""), g.edges_capacity(), 4ul);

    g.reserve_nodes(4);
    check_equality(LINE(""), g.node_capacity(), 4ul);

    g.shrink_to_fit();
    check_equality(LINE("May fail if stl implementation doesn't actually shrink to fit!"), g.edges_capacity(), 0ul);
    check_equality(LINE("May fail if stl implementation doesn't actually shrink to fit!"), g.node_capacity(), 0ul);
      
    // x----x
    using edge_init_t = typename graph_t::edge_init_type;

    if constexpr(std::is_empty_v<NodeWeight>)
    {
      check_regular_semantics(LINE("Regular Semantics"), g, graph_t{{{}}, edge_allocator{}, edge_partitions_allocator{}});
      graph_t g2{};

      auto nodeMaker{
        [](graph_t& g) { g.add_node(); }
      };

      auto allocGetter{
        [](const graph_t& g) { return g.get_edge_allocator(); }
      };

      auto partitionAllocGetter{
        [](const graph_t& g) { return g.get_edge_allocator(partitions_allocator_tag{}); }
      };

      if constexpr (GraphFlavour == graph_flavour::directed)
      {
        check_regular_semantics(LINE("Regular Semantics"), g2, graph_t{{edge_init_t{1}}, {}}, nodeMaker, allocation_info{allocGetter, {0, {1, 0}, {1, 1}}}, allocation_info{partitionAllocGetter, {0, {1, 1}, {1, 1}}});
      }
      else if constexpr(GraphFlavour == graph_flavour::undirected)
      {
        check_regular_semantics(LINE("Regular Semantics"), g2, graph_t{{edge_init_t{1}}, {edge_init_t{0}}}, nodeMaker, allocation_info{allocGetter, {0, {1, 0}, {1, 1}}}, allocation_info{partitionAllocGetter, {0, {1, 1}, {1, 1}}});
      }
      else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
      {
        check_regular_semantics(LINE("Regular Semantics"), g2, graph_t{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}}}, nodeMaker, allocation_info{allocGetter, {0, {1, 0}, {1, 1}}}, allocation_info{partitionAllocGetter, {0, {1, 1}, {1, 1}}});
      }
      else
      {
        check_regular_semantics(LINE("Regular Semantics"), g2, graph_t{{edge_init_t{1,0}}, {edge_init_t{0,0}}}, nodeMaker, allocation_info{allocGetter, {0, {1, 0}, {1, 1}}}, allocation_info{partitionAllocGetter, {0, {1, 1}, {1, 1}}});
      }
    }
    else
    {
      using node_allocator = typename graph_t::node_weight_container_type::allocator_type;
      check_regular_semantics(LINE("Regular Semantics"), g, graph_t{{{}}, edge_allocator{}, edge_partitions_allocator{}, node_allocator{}});

      check_regular_semantics(LINE("Regular Semantics"), g, graph_t{{{}}, edge_allocator{}, edge_partitions_allocator{}, {{1.0, -1.0}}, node_allocator{}});

      graph_t g2{};

      auto nodeMaker{
        [](graph_t& g) { g.add_node(); }
      };

      auto allocGetter{
        [](const graph_t& g) { return g.get_edge_allocator(); }
      };

      auto partitionAllocGetter{
        [](const graph_t& g) { return g.get_edge_allocator(partitions_allocator_tag{}); }
      };

      auto nodeAllocGetter{
        [](const graph_t& g) { return g.get_node_allocator(); }
      };

      if constexpr (GraphFlavour == graph_flavour::directed)
      {
        check_regular_semantics(LINE("Regular Semantics"), g2, graph_t{{edge_init_t{1}}, {}}, nodeMaker, allocation_info{allocGetter, {0, {1, 0}, {1, 1}}}, allocation_info{partitionAllocGetter, {0, {1, 1}, {1, 1}}}, allocation_info{nodeAllocGetter, {0, {1, 1}, {1, 1}}});
      }
      else if constexpr(GraphFlavour == graph_flavour::undirected)
      {
        check_regular_semantics(LINE("Regular Semantics"), g2, graph_t{{edge_init_t{1}}, {edge_init_t{0}}}, nodeMaker, allocation_info{allocGetter, {0, {1, 0}, {1, 1}}}, allocation_info{partitionAllocGetter, {0, {1, 1}, {1, 1}}}, allocation_info{nodeAllocGetter, {0, {1, 1}, {1, 1}}});
      }
      else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
      {
        check_regular_semantics(LINE("Regular Semantics"), g2, graph_t{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}}}, nodeMaker, allocation_info{allocGetter, {0, {1, 0}, {1, 1}}}, allocation_info{partitionAllocGetter, {0, {1, 1}, {1, 1}}}, allocation_info{nodeAllocGetter, {0, {1, 1}, {1, 1}}});
      }
      else
      {
        check_regular_semantics(LINE("Regular Semantics"), g2, graph_t{{edge_init_t{1,0}}, {edge_init_t{0,0}}}, nodeMaker, allocation_info{allocGetter, {0, {1, 0}, {1, 1}}}, allocation_info{partitionAllocGetter, {0, {1, 1}, {1, 1}}}, allocation_info{nodeAllocGetter, {0, {1, 1}, {1, 1}}});
      }
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

    auto maker{
      [](){
        if constexpr(std::is_empty_v<NodeWeight>)
        {
          return graph_t(edge_allocator{});
        }
        else
        {
          using node_allocator = typename graph_t::node_weight_container_type::allocator_type;
          return graph_t{edge_allocator{}, node_allocator{}};
        }
      }
    };
    
    graph_t g{maker()};

    check_exception_thrown<std::out_of_range>(LINE(""), [&g](){ g.reserve_edges(0, 4);});
    check_exception_thrown<std::out_of_range>(LINE(""), [&g](){ return g.edges_capacity(0);});
    check_equality(LINE(""), g.node_capacity(), 0ul);

    if constexpr(std::is_empty_v<NodeWeight>)
    {
      check_regular_semantics(LINE("Regular Semantics"), g, graph_t{{{}}, edge_allocator{}});
    }
    else
    {
      using node_allocator = typename graph_t::node_weight_container_type::allocator_type;
      check_regular_semantics(LINE("Regular Semantics"), g, graph_t{{{}}, edge_allocator{}, node_allocator{}});

      check_regular_semantics(LINE("Regular Semantics"), g, graph_t{{{}}, edge_allocator{}, {{1.0, -1.0}}, node_allocator{}});
    }

    g.add_node();
    if constexpr(std::is_empty_v<NodeWeight>)
    {
      check_equality(LINE(""), g, graph_t{{{}}, edge_allocator{}});
    }
    else
    {
      using node_allocator = typename graph_t::node_weight_container_type::allocator_type;
      check_equality(LINE(""), g, graph_t{{{}}, edge_allocator{}, node_allocator{}});
    }

    check_equality(LINE(""), g.edges_capacity(0), 0ul);
    check_equality(LINE(""), g.node_capacity(), 1ul);
    g.reserve_edges(0, 4);
    check_equality(LINE(""), g.edges_capacity(0), 4ul);

    g.reserve_nodes(4);
    check_equality(LINE(""), g.node_capacity(), 4ul);

    g.shrink_to_fit();
    check_equality(LINE("May fail if stl implementation doesn't actually shrink to fit!"), g.edges_capacity(0), 0ul);
    check_equality(LINE("May fail if stl implementation doesn't actually shrink to fit!"), g.node_capacity(), 1ul);

    g.insert_node(0u);
    if constexpr(std::is_empty_v<NodeWeight>)
    {
      check_equality(LINE(""), g, graph_t{{{}, {}}, edge_allocator{}});
    }
    else
    {
      using node_allocator = typename graph_t::node_weight_container_type::allocator_type;
      check_equality(LINE(""), g, graph_t{{{}, {}}, edge_allocator{}, node_allocator{}});
    }

    // x----x
    using edge_init_t = typename graph_t::edge_init_type;
    using namespace maths;
    using edge_allocator = typename graph_t::edge_allocator_type;

    if constexpr(std::is_empty_v<NodeWeight>)
    {
      graph_t g2{};

      auto nodeMaker{
        [](graph_t& g) { g.add_node(); }
      };

      auto allocGetter{
        [](const graph_t& g) { return g.get_edge_allocator(); }
      };

      if constexpr (GraphFlavour == graph_flavour::directed)
      {
        check_regular_semantics(LINE("Regular Semantics"), g2, graph_t{{edge_init_t{1}}, {}}, nodeMaker, allocation_info{allocGetter, {{0, {1, 1}, {1, 1}}, {0, {1, 0}, {1, 1}}}});
      }
      else if constexpr(GraphFlavour == graph_flavour::undirected)
      {
        check_regular_semantics(LINE("Regular Semantics"), g2, graph_t{{edge_init_t{1}}, {edge_init_t{0}}}, nodeMaker, allocation_info{allocGetter, {{0, {1, 1}, {1, 1}}, {0, {2, 0}, {2, 2}}}});
      }
      else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
      {
        check_regular_semantics(LINE("Regular Semantics"), g2, graph_t{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}}}, nodeMaker, allocation_info{allocGetter, {{0, {1, 1}, {1, 1}}, {0, {2, 0}, {2, 2}}}});
      }
      else
      {
        check_regular_semantics(LINE("Regular Semantics"), g2, graph_t{{edge_init_t{1,0}}, {edge_init_t{0,0}}}, nodeMaker, allocation_info{allocGetter, {{0, {1, 1}, {1, 1}}, {0, {2, 0}, {2, 2}}}});
      }
    }
    else
    {
      graph_t g2{};

      auto nodeMaker{
        [](graph_t& g) { g.add_node(); }
      };

      auto allocGetter{
        [](const graph_t& g) { return g.get_edge_allocator(); }
      };

      auto nodeAllocGetter{
        [](const graph_t& g) { return g.get_node_allocator(); }
      };

      if constexpr (GraphFlavour == graph_flavour::directed)
      {
        check_regular_semantics(LINE("Regular Semantics"), g2, graph_t{{edge_init_t{1}}, {}}, nodeMaker, allocation_info{allocGetter, {{0, {1, 1}, {1, 1}}, {0, {1, 0}, {1, 1}}}}, allocation_info{nodeAllocGetter, {0, {1, 1}, {1, 1}}});
      }
      else if constexpr(GraphFlavour == graph_flavour::undirected)
      {
        check_regular_semantics(LINE("Regular Semantics"), g2, graph_t{{edge_init_t{1}}, {edge_init_t{0}}}, nodeMaker, allocation_info{allocGetter, {{0, {1, 1}, {1, 1}}, {0, {2, 0}, {2, 2}}}}, allocation_info{nodeAllocGetter, {0, {1, 1}, {1, 1}}});
      }
      else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
      {
        check_regular_semantics(LINE("Regular Semantics"), g2, graph_t{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}}}, nodeMaker, allocation_info{allocGetter, {{0, {1, 1}, {1, 1}}, {0, {2, 0}, {2, 2}}}}, allocation_info{nodeAllocGetter, {0, {1, 1}, {1, 1}}});
      }
      else
      {
        check_regular_semantics(LINE("Regular Semantics"), g2, graph_t{{edge_init_t{1,0}}, {edge_init_t{0,0}}}, nodeMaker, allocation_info{allocGetter, {{0, {1, 1}, {1, 1}}, {0, {2, 0}, {2, 2}}}}, allocation_info{nodeAllocGetter, {0, {1, 1}, {1, 1}}});
      }
    }
  }

  // Generic Weighted
  
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
  void generic_weighted_graph_tests<
      GraphFlavour,
      EdgeWeight,
      NodeWeight,
      EdgeWeightPooling,
      NodeWeightPooling,
      EdgeStorageTraits,
      NodeWeightStorageTraits
  >::test_basic_operations()
  {
    using namespace maths;
    using edge_init_t      = typename graph_t::edge_init_type;
    using edge_t           = typename graph_t::edge_type;
    using edge_init_list_t = std::initializer_list<std::initializer_list<edge_init_t>>;
    using edge_weight_t    = typename graph_t::edge_weight_type;
    using node_weight_t    = typename graph_t::node_weight_type;

    constexpr bool edgeDataPool{std::is_same_v<EdgeWeightPooling, data_sharing::data_pool<EdgeWeight>>};
    
    graph_t graph;

    graph.reserve_nodes(4);
    check_equality(LINE(""), graph.node_capacity(), 4ul);
    graph.shrink_to_fit();
    check_equality(LINE("May fail if stl implementation doesn't actually shrink to fit!"), graph.node_capacity(), 0ul);

    // NULL
    check_exception_thrown<std::out_of_range>(LINE("No nodes to erase"), [&graph](){ graph.erase_node(0); });
    check_exception_thrown<std::out_of_range>(LINE("No edges to erase"), [&graph](){ graph.erase_edge(graph.cbegin_edges(0)); });
    check_exception_thrown<std::out_of_range>(LINE("No nodes to set weight"), [&graph](){ graph.node_weight(graph.cend_node_weights(), 1.0); });
    check_exception_thrown<std::out_of_range>(LINE("No edges to set weight"), [&graph](){ graph.set_edge_weight(graph.cbegin_edges(0), 1); });
    
    check_equality(LINE(""), graph, graph_t{});
        
    check_equality(LINE("Node added with weight i"), graph.add_node(0.0, 1.0), 0ul);
    //   (i)

    check_equality(LINE(""), graph, graph_t{edge_init_list_t{{}}, {{0,1}}});
    check_regular_semantics(LINE("Regular semantics"), graph, graph_t{});
    check_regular_semantics(LINE("Regular semantics"), graph, graph_t{edge_init_list_t{{}}, {{1,0}}});

    check_exception_thrown<std::out_of_range>(LINE("Throw if node out of range"), [&graph](){ graph.node_weight(graph.cend_node_weights(), 0.0); });
    
    graph.erase_node(0);
    //    NULL

    check_equality(LINE(""), graph, graph_t{});

    check_equality(LINE("Node of weight i recreated"), graph.add_node(0.0, 1.0), 0ul);
    //    (i)

    check_equality(LINE(""), graph, graph_t{edge_init_list_t{{}}, {{0,1}}});
        
    graph.node_weight(graph.cbegin_node_weights(), 1.1, -4.3);
    // (1.1-i4.3)

    check_equality(LINE(""), graph, graph_t{edge_init_list_t{{}}, {{1.1,-4.3}}});

    graph.join(0, 0, 1);
    //   /\ 1
    //   \/
    //  (1.1-i4.3)
    
    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), graph, graph_t{{{{edge_init_t{0,1}}}}, {{1.1,-4.3}}});
      check_regular_semantics(LINE("Regular semantics"), graph, graph_t{{{{edge_init_t{0,2}}}}, {{1.1,-4.3}}});
      check_regular_semantics(LINE("Regular semantics"), graph, graph_t{{{{edge_init_t{0,1}}}}, {{-4.3, 1.1}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {      
      check_equality(LINE(""), graph, graph_t{{{edge_init_t{0,1}, edge_init_t{0,1}}}, {{1.1,-4.3}}});
      check_regular_semantics(LINE("Regular semantics"), graph, graph_t{{{edge_init_t{0,2}, edge_init_t{0,2}}}, {{1.1,-4.3}}});
      check_regular_semantics(LINE("Regular semantics"), graph, graph_t{{{edge_init_t{0,1}, edge_init_t{0,1}}}, {{-4.3,1.1}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), graph, graph_t{{{edge_init_t{0,0,1,1}, edge_init_t{0,0,0,1}}}, {{1.1,-4.3}}});
      check_regular_semantics(LINE(""), graph, graph_t{{{edge_init_t{0,0,1,2}, edge_init_t{0,0,0,2}}}, {{1.1,-4.3}}});
      check_regular_semantics(LINE(""), graph, graph_t{{{edge_init_t{0,0,1,1}, edge_init_t{0,0,0,1}}}, {{-4.3,1.1}}});      
    }
    else 
    { 
      check_equality(LINE(""), graph, graph_t{{{edge_init_t{0,1,1}, edge_init_t{0,0,1}}}, {{1.1,-4.3}}});
      check_regular_semantics(LINE(""), graph, graph_t{{{edge_init_t{0,1,2}, edge_init_t{0,0,2}}}, {{1.1,-4.3}}});
      check_regular_semantics(LINE(""), graph, graph_t{{{edge_init_t{0,1,1}, edge_init_t{0,0,1}}}, {{-4.3,1.1}}});
    }

    check_regular_semantics(LINE("Regular semantics"), graph, graph_t{});

    graph.set_edge_weight(graph.cbegin_edges(0), -2);
    //   /\ -2
    //   \/
    // (1.1-i4.3)

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), graph, graph_t{{{{edge_init_t{0,-2}}}}, {{1.1,-4.3}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {      
      check_equality(LINE(""), graph, graph_t{{{edge_init_t{0,-2}, edge_init_t{0,-2}}}, {{1.1,-4.3}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), graph, graph_t{{{edge_init_t{0,0,1,-2}, edge_init_t{0,0,0,-2}}}, {{1.1,-4.3}}});
    }
    else 
    { 
      check_equality(LINE(""), graph, graph_t{{{edge_init_t{0,1,-2}, edge_init_t{0,0,-2}}}, {{1.1,-4.3}}});
    }

    graph.set_edge_weight(graph.crbegin_edges(0), -4);
    //   /\ -4
    //   \/
    // (1.1-i4.3)

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), graph, graph_t{{{{edge_init_t{0,-4}}}}, {{1.1,-4.3}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {      
      check_equality(LINE(""), graph, graph_t{{{edge_init_t{0,-4}, edge_init_t{0,-4}}}, {{1.1,-4.3}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), graph, graph_t{{{edge_init_t{0,0,1,-4}, edge_init_t{0,0,0,-4}}}, {{1.1,-4.3}}});
    }
    else 
    { 
      check_equality(LINE(""), graph, graph_t{{{edge_init_t{0,1,-4}, edge_init_t{0,0,-4}}}, {{1.1,-4.3}}});
    }

    graph.mutate_edge_weight(graph.cbegin_edges(0),
        [](auto& val) {
        if constexpr(is_container_v<edge_weight_t>)
        {
          val[0] *= 2;
        }
        else
        {
          val *= 2;
        }
      }
    );

    //   /\ -8
    //   \/
    // (1.1-i4.3)

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), graph, graph_t{{{{edge_init_t{0,-8}}}}, {{1.1,-4.3}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {      
      check_equality(LINE(""), graph, graph_t{{{edge_init_t{0,-8}, edge_init_t{0,-8}}}, {{1.1,-4.3}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), graph, graph_t{{{edge_init_t{0,0,1,-8}, edge_init_t{0,0,0,-8}}}, {{1.1,-4.3}}});
    }
    else 
    { 
      check_equality(LINE(""), graph, graph_t{{{edge_init_t{0,1,-8}, edge_init_t{0,0,-8}}}, {{1.1,-4.3}}});
    }

    graph.erase_edge(graph.cbegin_edges(0));
    //  (1.1-i4.3)

    check_equality(LINE(""), graph, graph_t{edge_init_list_t{{}}, {{1.1,-4.3}}});

    graph.join(0, 0, 1);
    //   /\ 1
    //   \/
    // (1.1-i4.3)

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), graph, graph_t{{{{edge_init_t{0,1}}}}, {{1.1,-4.3}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {      
      check_equality(LINE(""), graph, graph_t{{{edge_init_t{0,1}, edge_init_t{0,1}}}, {{1.1,-4.3}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), graph, graph_t{{{edge_init_t{0,0,1,1}, edge_init_t{0,0,0,1}}}, {{1.1,-4.3}}});
    }
    else 
    { 
      check_equality(LINE(""), graph, graph_t{{{edge_init_t{0,1,1}, edge_init_t{0,0,1}}}, {{1.1,-4.3}}});
    }
        
    graph.join(0, 0, -1);
    //   /\ 1
    //   \/
    // (1.1-i4.3)
    //   /\
    //   \/ -1

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), graph, graph_t{{{edge_init_t{0,1}, edge_init_t{0,-1}}}, {{1.1,-4.3}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t g{{{edge_init_t{0,1}, edge_init_t{0,1}, edge_init_t{0,-1}, edge_init_t{0,-1}}}, {{1.1,-4.3}}};

      if constexpr(is_orderable_v<edge_weight_t>)
      {
        g.sort_edges(g.cbegin_edges(0), g.cend_edges(0),
                   [](const auto& l , const auto& r){ return l.weight() > r.weight(); }
        );
      }
      check_equality(LINE(""), graph, g);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), graph, graph_t{{{edge_init_t{0,0,1,1}, edge_init_t{0,0,0,1}, edge_init_t{0,0,3,-1}, edge_init_t{0,0,2,-1}}}, {{1.1,-4.3}}});
    }
    else 
    { 
      check_equality(LINE(""), graph, graph_t{{{edge_init_t{0,1,1}, edge_init_t{0,0,1}, edge_init_t{0,3,-1}, edge_init_t{0,2,-1}}}, {{1.1,-4.3}}});
    }

    mutual_info(GraphFlavour) ? graph.set_edge_weight(graph.cbegin_edges(0) + 2, -4) : graph.set_edge_weight(++graph.cbegin_edges(0), -4);
    //   /\ 1
    //   \/
    // (1.1-i4.3)
    //   /\
    //   \/ -4
         
    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), graph, graph_t{{{edge_init_t{0,1}, edge_init_t{0,-4}}}, {{1.1,-4.3}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t g{{{edge_init_t{0,1}, edge_init_t{0,1}, edge_init_t{0,-4}, edge_init_t{0,-4}}}, {{1.1,-4.3}}};

      if constexpr(is_orderable_v<edge_weight_t>)
      {
        g.sort_edges(g.cbegin_edges(0), g.cend_edges(0),
                   [](const auto& l , const auto& r){ return l.weight() > r.weight(); }
        );
      }
      check_equality(LINE(""), graph, g);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), graph, graph_t{{{edge_init_t{0,0,1,1}, edge_init_t{0,0,0,1}, edge_init_t{0,0,3,-4}, edge_init_t{0,0,2,-4}}}, {{1.1,-4.3}}});
    }
    else 
    { 
      check_equality(LINE(""), graph, graph_t{{{edge_init_t{0,1,1}, edge_init_t{0,0,1}, edge_init_t{0,3,-4}, edge_init_t{0,2,-4}}}, {{1.1,-4.3}}});
    }


    check_equality(LINE(""), graph.add_node(), 1ul);
    //   /\ 1
    //   \/
    // (1.1-i4.3)   (0)
    //   /\
    //   \/ -4
        
    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), graph, graph_t{{{edge_init_t{0,1}, edge_init_t{0,-4}}, {}}, {{1.1,-4.3}, {}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t g{{{edge_init_t{0,1}, edge_init_t{0,1}, edge_init_t{0,-4}, edge_init_t{0,-4}}, {}}, {{1.1,-4.3}, {}}};

      if constexpr(is_orderable_v<edge_weight_t>)
      {
        g.sort_edges(g.cbegin_edges(0), g.cend_edges(0),
                   [](const auto& l , const auto& r){ return l.weight() > r.weight(); }
        );
      }
      check_equality(LINE(""), graph, g);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), graph, graph_t{{{edge_init_t{0,0,1,1}, edge_init_t{0,0,0,1}, edge_init_t{0,0,3,-4}, edge_init_t{0,0,2,-4}}, {}}, {{1.1,-4.3}, {}}});
    }
    else 
    { 
      check_equality(LINE(""), graph, graph_t{{{edge_init_t{0,1,1}, edge_init_t{0,0,1}, edge_init_t{0,3,-4}, edge_init_t{0,2,-4}}, {}}, {{1.1,-4.3}, {}}});
    }

    graph.join(0, 1, 6);
    //   /\1
    //   \/  6
    // (1.1-i4.3)------(0)
    //   /\
    //   \/-4
        
    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), graph, graph_t{{{edge_init_t{0,1}, edge_init_t{0,-4}, edge_init_t{1, 6}}, {}}, {{1.1,-4.3}, {}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t g{{{edge_init_t{0,1}, edge_init_t{0,1}, edge_init_t{0,-4}, edge_init_t{0,-4}, edge_init_t{1, 6}},
                {edge_init_t{0, 6}}
               }, {{1.1,-4.3}, {}}};

      if constexpr(is_orderable_v<edge_weight_t>)
      {
        g.sort_edges(g.cbegin_edges(0), g.cend_edges(0) - 1,
                   [](const auto& l , const auto& r){ return l.weight() > r.weight(); }
        );
      }
      check_equality(LINE(""), graph, g);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), graph,
                     graph_t{{{edge_init_t{0,0,1,1}, edge_init_t{0,0,0,1}, edge_init_t{0,0,3,-4}, edge_init_t{0,0,2,-4}, edge_init_t{0,1,0,6}},
                             {edge_init_t{0,1,4,6}}
                              }, {{1.1,-4.3}, {}}});
    }
    else 
    { 
      check_equality(LINE(""), graph,
                     graph_t{{{edge_init_t{0,1,1}, edge_init_t{0,0,1}, edge_init_t{0,3,-4}, edge_init_t{0,2,-4}, edge_init_t{1,0,6}},
                             {edge_init_t{0,4,6}}
                               }, {{1.1,-4.3}, {}}});
    }
    
    graph.swap_nodes(0,1);

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), graph, graph_t{{{}, {edge_init_t{1,1}, edge_init_t{1,-4}, edge_init_t{0, 6}}}, {{}, {1.1,-4.3}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t g{{{edge_init_t{1,6}},
                {edge_init_t{1,1}, edge_init_t{1,1}, edge_init_t{1,-4}, edge_init_t{1,-4}, edge_init_t{0, 6}}
               }, {{}, {1.1,-4.3}}};
      
      g.sort_edges(g.cbegin_edges(1), g.cend_edges(1),
                   [](const auto& l , const auto& r){
                       if constexpr(is_orderable_v<edge_weight_t>)
                       {
                         return (l.target_node() == r.target_node()) ? (l.weight() > r.weight()) : (l.target_node() > r.target_node());
                       }
                       else
                       {
                         return l.target_node() > r.target_node();
                       }
                   }
      );
      
      check_equality(LINE(""), graph, g);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), graph,
                     graph_t{{{edge_init_t{1,0,4,6}},
                             {edge_init_t{1,1,1,1}, edge_init_t{1,1,0,1}, edge_init_t{1,1,3,-4}, edge_init_t{1,1,2,-4}, edge_init_t{1,0,0,6}}
                            }, {{}, {1.1,-4.3}}});
    }
    else 
    { 
      check_equality(LINE(""), graph,
                     graph_t{{{edge_init_t{1,4,6}},
                             {edge_init_t{1,1,1}, edge_init_t{1,0,1}, edge_init_t{1,3,-4}, edge_init_t{1,2,-4}, edge_init_t{0,0,6}}
                       }, {{}, {1.1,-4.3}}});
    }

    graph.swap_nodes(1,0);

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), graph, graph_t{{{edge_init_t{0,1}, edge_init_t{0,-4}, edge_init_t{1, 6}}, {}}, {{1.1,-4.3}, {}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t g{{{edge_init_t{0,1}, edge_init_t{0,1}, edge_init_t{0,-4}, edge_init_t{0,-4}, edge_init_t{1, 6}},
                 {edge_init_t{0, 6}}
                }, {{1.1,-4.3}, {}}};

      if constexpr(is_orderable_v<edge_weight_t>)
      {
        g.sort_edges(g.cbegin_edges(0), g.cend_edges(0) - 1,
                   [](const auto& l , const auto& r){ return l.weight() > r.weight(); }
        );
      }
      check_equality(LINE(""), graph, g);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), graph,
                     graph_t{{{edge_init_t{0,0,1,1}, edge_init_t{0,0,0,1}, edge_init_t{0,0,3,-4}, edge_init_t{0,0,2,-4}, edge_init_t{0,1,0,6}},
                             {edge_init_t{0,1,4,6}}
                            }, {{1.1,-4.3}, {}}});
    }
    else 
    { 
      check_equality(LINE(""), graph,
                     graph_t{{{edge_init_t{0,1,1}, edge_init_t{0,0,1}, edge_init_t{0,3,-4}, edge_init_t{0,2,-4}, edge_init_t{1,0,6}},
                             {edge_init_t{0,4,6}}
                            }, {{1.1,-4.3}, {}}});
    }

    graph.set_edge_weight(--graph.cend_edges(0), 7);
    //   /\1
    //   \/  7
    // (1.1-i4.3)------(0)
    //   /\
    //   \/-4
        
    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), graph, graph_t{{{edge_init_t{0,1}, edge_init_t{0,-4}, edge_init_t{1, 7}}, {}}, {{1.1,-4.3}, {}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t g{{{edge_init_t{0,1}, edge_init_t{0,1}, edge_init_t{0,-4}, edge_init_t{0,-4}, edge_init_t{1, 7}},
                {edge_init_t{0, 7}}
               }, {{1.1,-4.3}, {}}};

      if constexpr(is_orderable_v<edge_weight_t>)
      {
        g.sort_edges(g.cbegin_edges(0), g.cend_edges(0) - 1,
                   [](const auto& l , const auto& r){ return l.weight() > r.weight(); }
        );
      }
      check_equality(LINE(""), graph, g);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), graph,
                     graph_t{{{edge_init_t{0,0,1,1}, edge_init_t{0,0,0,1}, edge_init_t{0,0,3,-4}, edge_init_t{0,0,2,-4}, edge_init_t{0,1,0,7}},
                             {edge_init_t{0,1,4,7}}
                            }, {{1.1,-4.3}, {}}});
    }
    else 
    { 
      check_equality(LINE(""), graph,
                     graph_t{{{edge_init_t{0,1,1}, edge_init_t{0,0,1}, edge_init_t{0,3,-4}, edge_init_t{0,2,-4}, edge_init_t{1,0,7}},
                             {edge_init_t{0,4,7}}
                            }, {{1.1,-4.3}, {}}});
    }
       
    graph.set_edge_weight(--graph.cend_edges(0), 6);
    //   /\1
    //   \/  6
    // (1.1-i4.3)------(0)
    //   /\
    //   \/-4

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), graph, graph_t{{{edge_init_t{0,1}, edge_init_t{0,-4}, edge_init_t{1, 6}}, {}}, {{1.1,-4.3}, {}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t g{{{edge_init_t{0,1}, edge_init_t{0,1}, edge_init_t{0,-4}, edge_init_t{0,-4}, edge_init_t{1, 6}},
                {edge_init_t{0, 6}}
               }, {{1.1,-4.3}, {}}};

      if constexpr(is_orderable_v<edge_weight_t>)
      {
        g.sort_edges(g.cbegin_edges(0), g.cend_edges(0) - 1,
                   [](const auto& l , const auto& r){ return l.weight() > r.weight(); }
        );
      }
      check_equality(LINE(""), graph, g);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), graph,
                     graph_t{{{edge_init_t{0,0,1,1}, edge_init_t{0,0,0,1}, edge_init_t{0,0,3,-4}, edge_init_t{0,0,2,-4}, edge_init_t{0,1,0,6}},
                             {edge_init_t{0,1,4,6}}
                            }, {{1.1,-4.3}, {}}});
    }
    else 
    { 
      check_equality(LINE(""), graph,
                     graph_t{{{edge_init_t{0,1,1}, edge_init_t{0,0,1}, edge_init_t{0,3,-4}, edge_init_t{0,2,-4}, edge_init_t{1,0,6}},
                             {edge_init_t{0,4,6}}
                            }, {{1.1,-4.3}, {}}});
    }
        
    graph.join(1, 0, 7);
    //     /\1
    //     \/     6
    // (1.1-i4.3)======(0)
    //     /\     7
    //     \/-4
       
    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), graph, graph_t{{{edge_init_t{0,1}, edge_init_t{0,-4}, edge_init_t{1, 6}}, {edge_init_t{0, 7}}}, {{1.1,-4.3}, {}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t g{{{edge_init_t{0,1}, edge_init_t{0,1}, edge_init_t{0,-4}, edge_init_t{0,-4}, edge_init_t{1, 7}, edge_init_t{1, 6}},
                {edge_init_t{0,7}, edge_init_t{0,6}}
               }, {{1.1,-4.3}, {}}};

      if constexpr(is_orderable_v<edge_weight_t>)
      {
        g.sort_edges(g.cbegin_edges(0), g.cend_edges(0) - 2,
                   [](const auto& l , const auto& r){ return l.weight() > r.weight(); }
        );
      }      
      
      check_equality(LINE(""), graph, g);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), graph,
                     graph_t{{{edge_init_t{0,0,1,1}, edge_init_t{0,0,0,1}, edge_init_t{0,0,3,-4}, edge_init_t{0,0,2,-4}, edge_init_t{0,1,0,6}, edge_init_t{1,0,1,7}},
                             {edge_init_t{0,1,4,6}, edge_init_t{1,0,5,7}}
                            }, {{1.1,-4.3}, {}}});
    }
    else 
    { 
      check_equality(LINE(""), graph,
                     graph_t{{{edge_init_t{0,1,1}, edge_init_t{0,0,1}, edge_init_t{0,3,-4}, edge_init_t{0,2,-4}, edge_init_t{1,0,6}, edge_init_t{1,1,7}},
                             {edge_init_t{0,4,6}, edge_init_t{0,5,7}}
                            }, {{1.1,-4.3}, {}}});
    }
        

    graph.join(0, 1, 8);
    //     /\1   /--\8
    //     \/   /6   \
    // (1.1-i4.3)====(0)
    //     /\     7
    //     \/-4

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), graph,
                     graph_t{{{edge_init_t{0,1}, edge_init_t{0,-4}, edge_init_t{1, 6}, edge_init_t{1,8}},
                              {edge_init_t{0, 7}}
                             }, {{1.1,-4.3}, {}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t g{{{edge_init_t{0,1}, edge_init_t{0,1}, edge_init_t{0,-4}, edge_init_t{0,-4}, edge_init_t{1, 8}, edge_init_t{1, 6}, edge_init_t{1,7}},
                 {edge_init_t{0,8}, edge_init_t{0,6}, edge_init_t{0,7}}
               }, {{1.1,-4.3}, {}}};

      if constexpr(is_orderable_v<edge_weight_t>)
      {
        g.sort_edges(g.cbegin_edges(0), g.cend_edges(0) - 3,
                   [](const auto& l , const auto& r){ return l.weight() > r.weight(); }
        );
      }      
      
      check_equality(LINE(""), graph, g);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), graph,
                     graph_t{{{edge_init_t{0,0,1,1}, edge_init_t{0,0,0,1}, edge_init_t{0,0,3,-4}, edge_init_t{0,0,2,-4}, edge_init_t{0,1,0,6}, edge_init_t{1,0,1,7}, edge_init_t{0,1,2,8}},
                           {edge_init_t{0,1,4,6}, edge_init_t{1,0,5,7}, edge_init_t{0,1,6,8}}
                            }, {{1.1,-4.3}, {}}});
    }
    else 
    { 
      check_equality(LINE(""), graph,
                     graph_t{{{edge_init_t{0,1,1}, edge_init_t{0,0,1}, edge_init_t{0,3,-4}, edge_init_t{0,2,-4}, edge_init_t{1,0,6}, edge_init_t{1,1,7}, edge_init_t{1,2,8}},
                             {edge_init_t{0,4,6}, edge_init_t{0,5,7}, edge_init_t{0,6,8}}
                            }, {{1.1,-4.3}, {}}});
    }
        
    graph.erase_edge(graph.cbegin_edges(0));
    //            8
    //           /--\
    //          / 6  \
    // (1.1-i4.3)====(0)
    //     /\     7
    //     \/-4
        
    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), graph,
                     graph_t{{{edge_init_t{0,-4}, edge_init_t{1, 6}, edge_init_t{1,8}},
                             {edge_init_t{0, 7}}
                           }, {{1.1,-4.3}, {}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t g{{{edge_init_t{0,-4}, edge_init_t{0,-4}, edge_init_t{1, 8}, edge_init_t{1, 6}, edge_init_t{1,7}},
                {edge_init_t{0,8}, edge_init_t{0,6}, edge_init_t{0,7}}
               }, {{1.1,-4.3}, {}}};
      
      check_equality(LINE(""), graph, g);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), graph,
                     graph_t{{{edge_init_t{0,0,1,-4}, edge_init_t{0,0,0,-4}, edge_init_t{0,1,0,6}, edge_init_t{1,0,1,7}, edge_init_t{0,1,2,8}},
                             {edge_init_t{0,1,2,6}, edge_init_t{1,0,3,7}, edge_init_t{0,1,4,8}}
                            }, {{1.1,-4.3}, {}}});
    }
    else 
    { 
      check_equality(LINE(""), graph,
                     graph_t{{{edge_init_t{0,1,-4}, edge_init_t{0,0,-4}, edge_init_t{1,0,6}, edge_init_t{1,1,7}, edge_init_t{1,2,8}},
                             {edge_init_t{0,2,6}, edge_init_t{0,3,7}, edge_init_t{0,4,8}}
                            }, {{1.1,-4.3}, {}}});
    }
        
    graph.mutate_edge_weight(graph.cbegin_edges(0) + (mutual_info(GraphFlavour) ?  3 : 2),
      [](auto& val){
        if constexpr(is_container_v<edge_weight_t>)
        {
          val[0] = 6;
        }
        else
        {
          val = 6;
        }
      }
    );
    
    //            8 (6, directed)
    //           /--\
    //          /6(7)\
    // (1.1-i4.3)====(0)
    //     /\     6
    //     \/-4

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), graph,
                     graph_t{{{edge_init_t{0,-4}, edge_init_t{1, 6}, edge_init_t{1,6}},
                             {edge_init_t{0, 7}}
                           }, {{1.1,-4.3}, {}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t g{{{edge_init_t{0,-4}, edge_init_t{0,-4}, edge_init_t{1, 8}, edge_init_t{1, 6}, edge_init_t{1,6}},
                {edge_init_t{0,8}, edge_init_t{0,6}, edge_init_t{0,6}}
               }, {{1.1,-4.3}, {}}};
      
      check_equality(LINE(""), graph, g);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), graph,
                     graph_t{{{edge_init_t{0,0,1,-4}, edge_init_t{0,0,0,-4}, edge_init_t{0,1,0,6}, edge_init_t{1,0,1,6}, edge_init_t{0,1,2,8}},
                             {edge_init_t{0,1,2,6}, edge_init_t{1,0,3,6}, edge_init_t{0,1,4,8}}
                            }, {{1.1,-4.3}, {}}});
    }
    else 
    { 
      check_equality(LINE(""), graph,
                     graph_t{{{edge_init_t{0,1,-4}, edge_init_t{0,0,-4}, edge_init_t{1,0,6}, edge_init_t{1,1,6}, edge_init_t{1,2,8}},
                             {edge_init_t{0,2,6}, edge_init_t{0,3,6}, edge_init_t{0,4,8}}
                            }, {{1.1,-4.3}, {}}});
    }

    graph.set_edge_weight(graph.cbegin_edges(0) + (mutual_info(GraphFlavour) ?  3 : 2), 10);
    //            8 (10, directed)
    //           /--\
    //          /10(7)\
    // (1.1-i4.3)====(0)
    //     /\     6
    //     \/-4

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), graph,
                     graph_t{{{edge_init_t{0,-4}, edge_init_t{1, 6}, edge_init_t{1,10}},
                             {edge_init_t{0, 7}}
                           }, {{1.1,-4.3}, {}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t g{{{edge_init_t{0,-4}, edge_init_t{0,-4}, edge_init_t{1, 8}, edge_init_t{1, 6}, edge_init_t{1,10}},
                {edge_init_t{0,8}, edge_init_t{0,10}, edge_init_t{0,6}}
               }, {{1.1,-4.3}, {}}};

      if constexpr(is_orderable_v<edge_weight_t>)
      {
        g.sort_edges(g.cbegin_edges(0) + 3, g.cend_edges(0),
                   [](const auto& l , const auto& r){ return l.weight() > r.weight(); }
        );

        g.sort_edges(g.cbegin_edges(1), g.cend_edges(1),
                   [](const auto& l , const auto& r){ return l.weight() > r.weight(); }
        );

        g.sort_edges(g.cbegin_edges(1) + 1, g.cend_edges(1),
                   [](const auto& l , const auto& r){ return l.weight() < r.weight(); }
        );
      }

      if constexpr(!edgeDataPool && !std::is_same_v<edge_init_t, edge_t>)
      {
        g.swap_edges(1, 0, 1);
      }
      
      check_equality(LINE(""), graph, g);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), graph,
                     graph_t{{{edge_init_t{0,0,1,-4}, edge_init_t{0,0,0,-4}, edge_init_t{0,1,0,6}, edge_init_t{1,0,1,10}, edge_init_t{0,1,2,8}},
                             {edge_init_t{0,1,2,6}, edge_init_t{1,0,3,10}, edge_init_t{0,1,4,8}}
                            }, {{1.1,-4.3}, {}}});
    }
    else 
    { 
      check_equality(LINE(""), graph,
                     graph_t{{{edge_init_t{0,1,-4}, edge_init_t{0,0,-4}, edge_init_t{1,0,6}, edge_init_t{1,1,10}, edge_init_t{1,2,8}},
                             {edge_init_t{0,2,6}, edge_init_t{0,3,10}, edge_init_t{0,4,8}}
                            }, {{1.1,-4.3}, {}}});
    }

    graph.mutate_edge_weight(graph.cbegin_edges(0) + (mutual_info(GraphFlavour) ?  4 : 1),
      [](auto& val){
        if constexpr(is_container_v<edge_weight_t>)
        {
          val[0] = 10;
        }
        else
        {
          val = 10;
        }
      }
    );
    
    //            10
    //           /--\
    //          / 10 \
    // (1.1-i4.3)====(0)
    //     /\     6 (10)
    //     \/-4

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), graph,
                     graph_t{{{edge_init_t{0,-4}, edge_init_t{1, 10}, edge_init_t{1,10}},
                             {edge_init_t{0, 7}}
                           }, {{1.1,-4.3}, {}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t g{{{edge_init_t{0,-4}, edge_init_t{0,-4}, edge_init_t{1,10}, edge_init_t{1,6}, edge_init_t{1,10}},
                {edge_init_t{0,6}, edge_init_t{0,10}, edge_init_t{0,10}}
               }, {{1.1,-4.3}, {}}};

      if constexpr(is_orderable_v<edge_weight_t>)
      {
        g.sort_edges(g.cbegin_edges(1), g.cbegin_edges(1) + 2,
                   [](const auto& l , const auto& r){ return l.weight() > r.weight(); }
        );

        if constexpr(!edgeDataPool && !std::is_same_v<edge_init_t, edge_t>)
        {
          g.swap_edges(1, 0, 1);
        }
      }
      else
      {
        g.swap_edges(1, 1, 2);
      }

      check_equality(LINE(""), graph, g);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), graph,
                     graph_t{{{edge_init_t{0,0,1,-4}, edge_init_t{0,0,0,-4}, edge_init_t{0,1,0,6}, edge_init_t{1,0,1,10}, edge_init_t{0,1,2,10}},
                             {edge_init_t{0,1,2,6}, edge_init_t{1,0,3,10}, edge_init_t{0,1,4,10}}
                            }, {{1.1,-4.3}, {}}});
    }
    else 
    { 
      check_equality(LINE(""), graph,
                     graph_t{{{edge_init_t{0,1,-4}, edge_init_t{0,0,-4}, edge_init_t{1,0,6}, edge_init_t{1,1,10}, edge_init_t{1,2,10}},
                             {edge_init_t{0,2,6}, edge_init_t{0,3,10}, edge_init_t{0,4,10}}
                            }, {{1.1,-4.3}, {}}});
    }

    graph.mutate_edge_weight(graph.cbegin_edges(0) + (mutual_info(GraphFlavour) ?  4 : 2),
      [](auto& val){
        if constexpr(is_container_v<edge_weight_t>)
        {
          val[0] = 7;
        }
        else
        {
          val = 7;
        }
      }
    );
    //            7
    //           /--\
    //          / 10 \
    // (1.1-i4.3)====(0)
    //     /\     6(7)
    //     \/-4

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), graph,
                     graph_t{{{edge_init_t{0,-4}, edge_init_t{1, 10}, edge_init_t{1,7}},
                             {edge_init_t{0, 7}}
                           }, {{1.1,-4.3}, {}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      graph_t g{{{edge_init_t{0,-4}, edge_init_t{0,-4}, edge_init_t{1, 6}, edge_init_t{1, 10}, edge_init_t{1,7}},
                {edge_init_t{0,6}, edge_init_t{0,10}, edge_init_t{0,7}}
               }, {{1.1,-4.3}, {}}};

      if constexpr(is_orderable_v<edge_weight_t>)
      {
        g.sort_edges(g.cbegin_edges(0) + 3, g.cend_edges(0),
                   [](const auto& l , const auto& r){ return l.weight() > r.weight(); }
        );
        
        g.sort_edges(g.cbegin_edges(1), g.cbegin_edges(1) + 2,
                   [](const auto& l , const auto& r){ return l.weight() > r.weight(); }
        );

        if constexpr(!edgeDataPool && !std::is_same_v<edge_init_t, edge_t>)
        {
          g.swap_edges(1, 0, 1);
          g.swap_edges(1, 1, 2);
        }
      }
      else
      {
        g.swap_edges(0, 2, 4);
        g.swap_edges(0, 3, 4);
        g.swap_edges(1, 0, 1);
        g.swap_edges(1, 1, 2);
      }

      
      check_equality(LINE(""), graph, g);
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), graph,
                     graph_t{{{edge_init_t{0,0,1,-4}, edge_init_t{0,0,0,-4}, edge_init_t{0,1,0,6}, edge_init_t{1,0,1,10}, edge_init_t{0,1,2,7}},
                             {edge_init_t{0,1,2,6}, edge_init_t{1,0,3,10}, edge_init_t{0,1,4,7}}
                            }, {{1.1,-4.3}, {}}});
    }
    else 
    { 
      check_equality(LINE(""), graph,
                     graph_t{{{edge_init_t{0,1,-4}, edge_init_t{0,0,-4}, edge_init_t{1,0,6}, edge_init_t{1,1,10}, edge_init_t{1,2,7}},
                             {edge_init_t{0,2,6}, edge_init_t{0,3,10}, edge_init_t{0,4,7}}
                            }, {{1.1,-4.3}, {}}});
    }
    
    graph.erase_node(0);
    //  (0)

    check_equality(LINE(""), graph, graph_t{edge_init_list_t{{}}, {{}}});
        

    graph.join(0, 0, 1);
    //   /\ 1
    //   \/
    //   (0)
        
    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), graph, graph_t{edge_init_list_t{{edge_init_t{0,1}}}, {{}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {      
      check_equality(LINE(""), graph, graph_t{{{edge_init_t{0,1}, edge_init_t{0,1}}}, {{}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), graph, graph_t{{{edge_init_t{0,0,1,1}, edge_init_t{0,0,0,1}}}, {{}}});
    }
    else 
    { 
      check_equality(LINE(""), graph, graph_t{{{edge_init_t{0,1,1}, edge_init_t{0,0,1}}}, {{}}});
    }

    check_equality(LINE("Node with weight 1+i inserted into slot 0"), graph.insert_node(0, 1.0, 1.0), 0ul);
    //       /\ 1
    //       \/
    // (1+i) (0)

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), graph, graph_t{{{}, {edge_init_t{1,1}}}, {{1, 1}, {}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {      
      check_equality(LINE(""), graph, graph_t{{{}, {edge_init_t{1,1}, edge_init_t{1,1}}}, {{1, 1}, {}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), graph, graph_t{{{}, {edge_init_t{1,1,1,1}, edge_init_t{1,1,0,1}}}, {{1, 1}, {}}});
    }
    else 
    { 
      check_equality(LINE(""), graph, graph_t{{{}, {edge_init_t{1,1,1}, edge_init_t{1,0,1}}}, {{1, 1}, {}}});
    }

    graph.mutate_node_weight(graph.cbegin_node_weights(), [](auto& val){
        if constexpr(is_container_v<node_weight_t>)
        {
          val[0] *= 2;
          val[1] *= 2;
        }
        else
        {
          val *= 2;
        }
      }
    );

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(""), graph, graph_t{{{}, {edge_init_t{1,1}}}, {{2, 2}, {}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {      
      check_equality(LINE(""), graph, graph_t{{{}, {edge_init_t{1,1}, edge_init_t{1,1}}}, {{2, 2}, {}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(""), graph, graph_t{{{}, {edge_init_t{1,1,1,1}, edge_init_t{1,1,0,1}}}, {{2, 2}, {}}});
    }
    else 
    { 
      check_equality(LINE(""), graph, graph_t{{{}, {edge_init_t{1,1,1}, edge_init_t{1,0,1}}}, {{2, 2}, {}}});
    }
  }  
}
