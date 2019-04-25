////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TestGraph.hpp"

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
      graph_test_helper<null_weight, null_weight> helper{"unweighted"};
      
      helper.run_tests<generic_graph_operations>(*this);
      helper.run_storage_tests<contiguous_edge_storage_traits, graph_contiguous_capacity>(*this);
      helper.run_storage_tests<bucketed_edge_storage_traits, graph_bucketed_capacity>(*this);
    }
      
    {
      graph_test_helper<int, complex<double>>  helper{"int, complex<double>"};
      
      helper.run_tests<generic_weighted_graph_tests>(*this);

      helper.run_storage_tests<contiguous_edge_storage_traits, graph_contiguous_capacity>(*this);
      helper.run_storage_tests<bucketed_edge_storage_traits, graph_bucketed_capacity>(*this);
    }

    {
      graph_test_helper<complex<int>, complex<double>>  helper{"complex<int>, complex<double>"};
      helper.run_tests<generic_weighted_graph_tests>(*this);
    }

    {
      graph_test_helper<complex<double>, int> helper{"complex<double>, int"};
      helper.run_tests<more_generic_weighted_graph_tests>(*this);
    }
      
    {
      graph_test_helper<std::vector<int>, std::vector<int>>  helper{"Weighted (vector<int>, vector<int>)"};
      helper.run_tests<test_copy_move>(*this);
    }
  }

  // Generic Graph Operations
  
  template
  <
    maths::graph_flavour GraphFlavour,
    class EdgeWeight,
    class NodeWeight,      
    template <class> class EdgeWeightPooling,
    template <class> class NodeWeightPooling,
    template <maths::graph_flavour, class, template<class> class> class EdgeStorageTraits,
    template <class, template<class> class, bool> class NodeWeightStorageTraits
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
    using edge_init_t = typename GGraph::edge_init_type;    

    GGraph network;

    check_exception_thrown<std::out_of_range>([&network]() { return network.cbegin_edges(0); }, LINE("cbegin_edges throws for empty graph"));
    check_exception_thrown<std::out_of_range>([&network]() { return network.cend_edges(0); }, LINE("cend_edges throws for empty graph"));

    check_exception_thrown<std::out_of_range>([&network]() { network.swap_nodes(0,0); }, LINE("swapping nodes throws for empty graph"));
          
    check_equality(network.add_node(), 0ul, LINE("Index of added node is 0"));
    //    0
    
    check_equality(network, GGraph{{}}, LINE(""));
    check_regular_semantics(network, GGraph{}, LINE("Regular semantics"));
    
    check_exception_thrown<std::out_of_range>([&network](){ get_edge(network, 0, 0, 0); },  LINE("For network with no edges, trying to obtain a reference to one throws an exception"));

    check_exception_thrown<std::out_of_range>([&network]() { network.join(0, 1); }, LINE("Unable to join zeroth node to non-existant first node"));

    check_exception_thrown<std::out_of_range>([&network]() { network.swap_nodes(0,1); }, LINE("Index out of range for swapping nodes"));
    
    check_equality(network.add_node(), 1ul, LINE("Index of added node is 1"));
    //    0    1

    check_equality(network, GGraph{{}, {}}, LINE(""));
    check_regular_semantics(network, GGraph{{}}, LINE("Regular semantics"));

    network.swap_nodes(0,1);
    
    check_equality(network, GGraph{{}, {}}, LINE(""));
    
    network.join(0, 1);
    //    0----1

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(network, GGraph{{edge_init_t{1}}, {}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_equality(network, GGraph{{edge_init_t{1}}, {edge_init_t{0}}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(network, GGraph{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}}}, LINE(""));
    }
    else
    {
      check_equality(network, GGraph{{edge_init_t{1,0}}, {edge_init_t{0,0}}}, LINE(""));
    }

    check_regular_semantics(network, GGraph{{}, {}}, LINE("Regular semantics"));
    
    network.swap_nodes(0,1);

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(network, GGraph{{}, {edge_init_t{0}}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_equality(network, GGraph{{edge_init_t{1}}, {edge_init_t{0}}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(network, GGraph{{edge_init_t{1,0,0}}, {edge_init_t{1,0,0}}}, LINE(""));
    }
    else
    {
      check_equality(network, GGraph{{edge_init_t{1,0}}, {edge_init_t{0,0}}}, LINE(""));
    }
    
    network.swap_nodes(1,0);

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(network, GGraph{{edge_init_t{1}}, {}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_equality(network, GGraph{{edge_init_t{1}}, {edge_init_t{0}}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(network, GGraph{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}}}, LINE(""));
    }
    else
    {
      check_equality(network, GGraph{{edge_init_t{1,0}}, {edge_init_t{0,0}}}, LINE(""));
    }
    
    auto nodeNum{network.add_node()};
    check_equality(nodeNum, 2ul, LINE("Index of added node is 2"));
    network.join(1, nodeNum);
    //    0----1----2

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(network, GGraph{{edge_init_t{1}}, {edge_init_t{2}}, {}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_equality(network, GGraph{{edge_init_t{1}}, {edge_init_t{0}, edge_init_t{2}}, {edge_init_t{1}}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {      
      check_equality(network, GGraph{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}, edge_init_t{1,2,0}}, {edge_init_t{1,2,1}}}, LINE(""));
    }
    else
    {  
      check_equality(network, GGraph{{edge_init_t{1,0}}, {edge_init_t{0,0}, edge_init_t{2,0}}, {edge_init_t{1,1}}}, LINE(""));
    }

    network.swap_nodes(2,1);

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(network, GGraph{{edge_init_t{2}}, {}, {edge_init_t{1}}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_equality(network, GGraph{{edge_init_t{2}}, {edge_init_t{2}}, {edge_init_t{0}, edge_init_t{1}}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {      
      check_equality(network, GGraph{{edge_init_t{0,2,0}}, {edge_init_t{2,1,1}}, {edge_init_t{0,2,0}, edge_init_t{2,1,0}}}, LINE(""));
    }
    else
    {
      check_equality(network, GGraph{{edge_init_t{2,0}}, {edge_init_t{2,1}}, {edge_init_t{0,0}, edge_init_t{1,0}}}, LINE(""));
    }
    
    network.swap_nodes(1,2);
     //    0----1----2

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(network, GGraph{{edge_init_t{1}}, {edge_init_t{2}}, {}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_equality(network, GGraph{{edge_init_t{1}}, {edge_init_t{0}, edge_init_t{2}}, {edge_init_t{1}}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {      
      check_equality(network, GGraph{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}, edge_init_t{1,2,0}}, {edge_init_t{1,2,1}}}, LINE(""));
    }
    else
    {
      check_equality(network, GGraph{{edge_init_t{1,0}}, {edge_init_t{0,0}, edge_init_t{2,0}}, {edge_init_t{1,1}}}, LINE(""));
    }

    network.swap_nodes(0,2);

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(network, GGraph{{}, {edge_init_t{0}}, {edge_init_t{1}}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      GGraph g{{edge_init_t{1}}, {edge_init_t{0}, edge_init_t{2}}, {edge_init_t{1}}};
      g.sort_edges(g.cbegin_edges(1), g.cend_edges(1), [](const auto& l, const auto& r) {
          return l.target_node() > r.target_node();
        }
      );
      
      check_equality(network, g, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {      
      check_equality(network, GGraph{{edge_init_t{1,0,1}}, {edge_init_t{2,1,0}, edge_init_t{1,0,0}}, {edge_init_t{2,1,0}}}, LINE(""));
    }
    else
    {   
      check_equality(network, GGraph{{edge_init_t{1,1}}, {edge_init_t{2,0}, edge_init_t{0,0}}, {edge_init_t{1,0}}}, LINE(""));
    }

    network.swap_nodes(0,2);

    if constexpr (GraphFlavour == graph_flavour::directed)
    {      
      check_equality(network, GGraph{{edge_init_t{1}}, {edge_init_t{2}}, {}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_equality(network, GGraph{{edge_init_t{1}}, {edge_init_t{0}, edge_init_t{2}}, {edge_init_t{1}}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {      
      check_equality(network, GGraph{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}, edge_init_t{1,2,0}}, {edge_init_t{1,2,1}}}, LINE(""));
    }
    else
    {      
      check_equality(network, GGraph{{edge_init_t{1,0}}, {edge_init_t{0,0}, edge_init_t{2,0}}, {edge_init_t{1,1}}}, LINE(""));
    }
    
    nodeNum = network.add_node();
    check_equality(nodeNum, 3ul, LINE("Index of added node is 3"));
    network.join(0, nodeNum);
    //    0----1----2
    //    |
    //    3

    if constexpr (GraphFlavour == graph_flavour::directed)
    {      
      check_equality(network, GGraph{{edge_init_t{1}, edge_init_t{3}}, {edge_init_t{2}}, {}, {}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_equality(network, GGraph{{edge_init_t{1}, edge_init_t{3}}, {edge_init_t{0}, edge_init_t{2}}, {edge_init_t{1}}, {edge_init_t{0}}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {      
      check_equality(network, GGraph{{edge_init_t{0,1,0}, edge_init_t{0,3,0}}, {edge_init_t{0,1,0}, edge_init_t{1,2,0}}, {edge_init_t{1,2,1}}, {edge_init_t{0,3,1}}}, LINE(""));
    }
    else
    {      
      check_equality(network, GGraph{{edge_init_t{1,0}, edge_init_t{3,0}}, {edge_init_t{0,0}, edge_init_t{2,0}}, {edge_init_t{1,1}}, {edge_init_t{0,1}}}, LINE(""));
    }

    network.erase_node(0);
    //    0----1
    //
    //    2

    if constexpr (GraphFlavour == graph_flavour::directed)
    {      
      check_equality(network, GGraph{{edge_init_t{1}}, {}, {}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_equality(network, GGraph{{edge_init_t{1}}, {edge_init_t{0}}, {}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {      
      check_equality(network, GGraph{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}}, {}}, LINE(""));
    }
    else
    {      
      check_equality(network, GGraph{{edge_init_t{1,0}}, {edge_init_t{0,0}}, {}}, LINE(""));
    }

    network.swap_nodes(0,2);

    if constexpr (GraphFlavour == graph_flavour::directed)
    {      
      check_equality(network, GGraph{{}, {}, {edge_init_t{1}}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_equality(network, GGraph{{}, {edge_init_t{2}}, {edge_init_t{1}}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {      
      check_equality(network, GGraph{{}, {edge_init_t{2,1,0}}, {edge_init_t{2,1,0}}}, LINE(""));
    }
    else
    {      
      check_equality(network, GGraph{{}, {edge_init_t{2,0}}, {edge_init_t{1,0}}}, LINE(""));
    }
    
    network.swap_nodes(2,0);

    if constexpr (GraphFlavour == graph_flavour::directed)
    {      
      check_equality(network, GGraph{{edge_init_t{1}}, {}, {}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_equality(network, GGraph{{edge_init_t{1}}, {edge_init_t{0}}, {}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {      
      check_equality(network, GGraph{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}}, {}}, LINE(""));
    }
    else
    {      
      check_equality(network, GGraph{{edge_init_t{1,0}}, {edge_init_t{0,0}}, {}}, LINE(""));
    }
    
    network.erase_edge(network.cbegin_edges(0));
    //    0    1
    //
    //    2

    check_equality(network, GGraph{{}, {}, {}}, LINE(""));
          
    network.join(0, 1);
    network.join(1, 1);
    network.join(1, 0);
    //    0======1
    //           /\
    //    2      \/

    if constexpr (GraphFlavour == graph_flavour::directed)
    {      
      check_equality(network, GGraph{{edge_init_t{1}}, {edge_init_t{1}, edge_init_t{0}}, {}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      GGraph g{{edge_init_t{1}, edge_init_t{1}}, {edge_init_t{0}, edge_init_t{0}, edge_init_t{1}, edge_init_t{1}}, {}};
      g.sort_edges(g.cbegin_edges(1) + 1, g.cend_edges(1), [](const auto& l, const auto& r) {
          return l.target_node() > r.target_node();
        }
      );
      
      check_equality(network, g, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {      
      check_equality(network, GGraph{{edge_init_t{0,1,0}, edge_init_t{1, 0, 3}}, {edge_init_t{0,1,0}, edge_init_t{1,1,2}, edge_init_t{1,1,1}, edge_init_t{1, 0, 1}}, {}}, LINE(""));
    }
    else
    {      
      check_equality(network, GGraph{{edge_init_t{1,0}, edge_init_t{1,3}}, {edge_init_t{0,0}, edge_init_t{1,2}, edge_init_t{1,1}, edge_init_t{0,1}}, {}}, LINE(""));
    }
        
    network.swap_nodes(1,0);

    if constexpr (GraphFlavour == graph_flavour::directed)
    {      
      check_equality(network, GGraph{{edge_init_t{0}, edge_init_t{1}}, {edge_init_t{0}}, {}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      GGraph g{{edge_init_t{0}, edge_init_t{0}, edge_init_t{1}, edge_init_t{1}}, {edge_init_t{0}, edge_init_t{0}}, {}};
      g.sort_edges(g.cbegin_edges(0), g.cbegin_edges(0)+3, [](const auto& l, const auto& r) {
          return l.target_node() > r.target_node();
        }
      );
      
      check_equality(network, g, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {      
      check_equality(network, GGraph{{edge_init_t{1,0,0}, edge_init_t{0,0,2}, edge_init_t{0,0,1}, edge_init_t{0, 1, 1}}, {edge_init_t{1,0,0}, edge_init_t{0, 1, 3}}, {}}, LINE(""));
    }
    else
    {      
      check_equality(network, GGraph{{edge_init_t{1,0}, edge_init_t{0,2}, edge_init_t{0,1}, edge_init_t{1,1}}, {edge_init_t{0,0}, edge_init_t{0,3}}, {}}, LINE(""));
    }

    network.swap_nodes(0,1);

    if constexpr (GraphFlavour == graph_flavour::directed)
    {      
      check_equality(network, GGraph{{edge_init_t{1}}, {edge_init_t{1}, edge_init_t{0}}, {}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      GGraph g{{edge_init_t{1}, edge_init_t{1}}, {edge_init_t{0}, edge_init_t{0}, edge_init_t{1}, edge_init_t{1}}, {}};
      g.sort_edges(g.cbegin_edges(1) + 1, g.cend_edges(1), [](const auto& l, const auto& r) {
          return l.target_node() > r.target_node();
        }
      );
      
      check_equality(network, g, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {      
      check_equality(network, GGraph{{edge_init_t{0,1,0}, edge_init_t{1, 0, 3}}, {edge_init_t{0,1,0}, edge_init_t{1,1,2}, edge_init_t{1,1,1}, edge_init_t{1, 0, 1}}, {}}, LINE(""));
    }
    else
    {      
      check_equality(network, GGraph{{edge_init_t{1,0}, edge_init_t{1,3}}, {edge_init_t{0,0}, edge_init_t{1,2}, edge_init_t{1,1}, edge_init_t{0,1}}, {}}, LINE(""));
    }

    //    0======1
    //           /\
    //    2      \/

    network.swap_nodes(0,2);

    if constexpr (GraphFlavour == graph_flavour::directed)
    {      
      check_equality(network, GGraph{{}, {edge_init_t{1}, edge_init_t{2}}, {edge_init_t{1}}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      GGraph g{{}, {edge_init_t{1}, edge_init_t{1}, edge_init_t{2}, edge_init_t{2}}, {edge_init_t{1}, edge_init_t{1}}};
      g.sort_edges(g.cbegin_edges(1), g.cbegin_edges(1) + 3, [](const auto& l, const auto& r) {
          return l.target_node() > r.target_node();
        }
      );
      
      check_equality(network, g, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {      
      check_equality(network, GGraph{{}, {edge_init_t{2,1,0}, edge_init_t{1,1,2}, edge_init_t{1,1,1}, edge_init_t{1, 2, 1}}, {edge_init_t{2,1,0}, edge_init_t{1, 2, 3}}}, LINE(""));
    }
    else
    {      
      check_equality(network, GGraph{{}, {edge_init_t{2,0}, edge_init_t{1,2}, edge_init_t{1,1}, edge_init_t{2,1}}, {edge_init_t{1,0}, edge_init_t{1,3}}}, LINE(""));
    }

    network.swap_nodes(2,0);

    if constexpr (GraphFlavour == graph_flavour::directed)
    {      
      check_equality(network, GGraph{{edge_init_t{1}}, {edge_init_t{1}, edge_init_t{0}}, {}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      GGraph g{{edge_init_t{1}, edge_init_t{1}}, {edge_init_t{0}, edge_init_t{0}, edge_init_t{1}, edge_init_t{1}}, {}};
      g.sort_edges(g.cbegin_edges(1) + 1, g.cend_edges(1), [](const auto& l, const auto& r) {
          return l.target_node() > r.target_node();
        }
      );
      
      check_equality(network, g, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {      
      check_equality(network, GGraph{{edge_init_t{0,1,0}, edge_init_t{1, 0, 3}}, {edge_init_t{0,1,0}, edge_init_t{1,1,2}, edge_init_t{1,1,1}, edge_init_t{1, 0, 1}}, {}}, LINE(""));
    }
    else
    {      
      check_equality(network, GGraph{{edge_init_t{1,0}, edge_init_t{1,3}}, {edge_init_t{0,0}, edge_init_t{1,2}, edge_init_t{1,1}, edge_init_t{0,1}}, {}}, LINE(""));
    }

    network.erase_edge(mutual_info(GraphFlavour) ? ++network.cbegin_edges(0) : network.cbegin_edges(0));
    //    0------1
    //           /\
    //    2      \/

    if constexpr (GraphFlavour == graph_flavour::directed)
    {      
      check_equality(network, GGraph{{}, {edge_init_t{1}, edge_init_t{0}}, {}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      GGraph g{{edge_init_t{1}}, { edge_init_t{0}, edge_init_t{1}, edge_init_t{1}}, {}};
      g.sort_edges(g.cbegin_edges(1), g.cend_edges(1), [](const auto& l, const auto& r) {
          return l.target_node() > r.target_node();
        }
      );
      
      check_equality(network, g, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {      
      check_equality(network, GGraph{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}, edge_init_t{1,1,2}, edge_init_t{1,1,1}}, {}}, LINE(""));
    }
    else
    {      
      check_equality(network, GGraph{{edge_init_t{1,0}}, {edge_init_t{0,0}, edge_init_t{1,2}, edge_init_t{1,1}}, {}}, LINE(""));
    }
    
    network.join(2,1);
    //    0------1------2
    //           /\
    //           \/

    if constexpr (GraphFlavour == graph_flavour::directed)
    {      
      check_equality(network, GGraph{{}, {edge_init_t{1}, edge_init_t{0}}, {edge_init_t{1}}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      GGraph g{{edge_init_t{1}}, { edge_init_t{0}, edge_init_t{1}, edge_init_t{1}, edge_init_t{2}}, {edge_init_t{1}}};
      g.sort_edges(g.cbegin_edges(1), g.cend_edges(1) -1, [](const auto& l, const auto& r) {
          return l.target_node() > r.target_node();
        }
      );
      
      check_equality(network, g, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {      
      check_equality(network, GGraph{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}, edge_init_t{1,1,2}, edge_init_t{1,1,1}, edge_init_t{2,1,0}}, {edge_init_t{2,1,3}}}, LINE(""));
    }
    else
    {      
      check_equality(network, GGraph{{edge_init_t{1,0}}, {edge_init_t{0,0}, edge_init_t{1,2}, edge_init_t{1,1}, edge_init_t{2,0}}, {edge_init_t{1,3}}}, LINE(""));
    }
          
    network.erase_edge(mutual_info(GraphFlavour) ? ++network.cbegin_edges(1) : network.cbegin_edges(1));
    //    0------1------2
 
    if constexpr (GraphFlavour == graph_flavour::directed)
    {      
      check_equality(network, GGraph{{}, {edge_init_t{0}}, {edge_init_t{1}}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {      
      check_equality(network, GGraph{{edge_init_t{1}}, { edge_init_t{0}, edge_init_t{2}}, {edge_init_t{1}}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {      
      check_equality(network, GGraph{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}, edge_init_t{2,1,0}}, {edge_init_t{2,1,1}}}, LINE(""));
    }
    else
    {      
      check_equality(network, GGraph{{edge_init_t{1,0}}, {edge_init_t{0,0}, edge_init_t{2,0}}, {edge_init_t{1,1}}}, LINE(""));
    }

    network.erase_edge(network.cbegin_edges(2));
    //    0------1     2
    
    if constexpr (GraphFlavour == graph_flavour::directed)
    {      
      check_equality(network, GGraph{{}, {edge_init_t{0}}, {}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {      
      check_equality(network, GGraph{{edge_init_t{1}}, {edge_init_t{0}}, {}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {      
      check_equality(network, GGraph{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}}, {}}, LINE(""));
    }
    else
    {      
      check_equality(network, GGraph{{edge_init_t{1,0}}, {edge_init_t{0,0}}, {}}, LINE(""));
    }

    network.join(1,1);
    network.join(2,1);

    //    0------1------2
    //           /\
    //           \/

    if constexpr (GraphFlavour == graph_flavour::directed)
    {      
      check_equality(network, GGraph{{}, {edge_init_t{0}, edge_init_t{1}}, {edge_init_t{1}}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {      
      check_equality(network, GGraph{{edge_init_t{1}}, {edge_init_t{0}, edge_init_t{1}, edge_init_t{1}, edge_init_t{2}}, {edge_init_t{1}}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {      
      check_equality(network, GGraph{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}, edge_init_t{1,1,2}, edge_init_t{1,1,1}, edge_init_t{2,1,0}}, {edge_init_t{2,1,3}}}, LINE(""));
    }
    else
    {      
      check_equality(network, GGraph{{edge_init_t{1,0}}, {edge_init_t{0,0}, edge_init_t{1,2}, edge_init_t{1,1}, edge_init_t{2,0}}, {edge_init_t{1,3}}}, LINE(""));
    }
    
    network.erase_edge(mutual_info(GraphFlavour) ? network.cbegin_edges(1)+2 : network.cbegin_edges(1)+1);
    //    0------1------2
 
    if constexpr (GraphFlavour == graph_flavour::directed)
    {      
      check_equality(network, GGraph{{}, {edge_init_t{0}}, {edge_init_t{1}}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {      
      check_equality(network, GGraph{{edge_init_t{1}}, {edge_init_t{0}, edge_init_t{2}}, {edge_init_t{1}}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {      
      check_equality(network, GGraph{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}, edge_init_t{2,1,0}}, {edge_init_t{2,1,1}}}, LINE(""));
    }
    else
    {      
      check_equality(network, GGraph{{edge_init_t{1,0}}, {edge_init_t{0,0}, edge_init_t{2,0}}, {edge_init_t{1,1}}}, LINE(""));
    }
        
    network.erase_edge(network.cbegin_edges(1));
    network.erase_edge(network.cbegin_edges(2));
    network.join(1,1);
    //    0      1
    //           /\
    //    2      \/

    if constexpr (GraphFlavour == graph_flavour::directed)
    {      
      check_equality(network, GGraph{{}, {edge_init_t{1}}, {}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {      
      check_equality(network, GGraph{{}, {edge_init_t{1}, edge_init_t{1}}, {}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {      
      check_equality(network, GGraph{{}, {edge_init_t{1,1,1}, edge_init_t{1,1,0}}, {}}, LINE(""));
    }
    else
    {      
      check_equality(network, GGraph{{}, {edge_init_t{1,1}, edge_init_t{1,0}}, {}}, LINE(""));
    }
 
    network.erase_edge(network.cbegin_edges(1));
    //    0    1
    //
    //    2

    check_equality(network, GGraph{{}, {}, {}}, LINE(""));
          
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
      check_equality(network,
                     GGraph{
                       {edge_init_t{1}, edge_init_t{1}, edge_init_t{2}, edge_init_t{2}},
                       {edge_init_t{2}},
                       {edge_init_t{1}}
                     }, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {      
      check_equality(network,
                     GGraph{
                       {edge_init_t{1}, edge_init_t{1}, edge_init_t{2}, edge_init_t{2}},
                       {edge_init_t{0}, edge_init_t{0}, edge_init_t{2}, edge_init_t{2}},
                       {edge_init_t{0}, edge_init_t{0}, edge_init_t{1}, edge_init_t{1}}
                     }, LINE(""));
    }    
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(network,
                     GGraph{
                       {edge_init_t{0,1,0}, edge_init_t{0,1,1}, edge_init_t{0,2,0}, edge_init_t{0,2,1}},
                       {edge_init_t{0,1,0}, edge_init_t{0,1,1}, edge_init_t{1,2,2}, edge_init_t{2,1,3}},
                       {edge_init_t{0,2,2}, edge_init_t{0,2,3}, edge_init_t{1,2,2}, edge_init_t{2,1,3}}
                     }, LINE(""));
    }
    else 
    { 
      check_equality(network,
                     GGraph{
                       {edge_init_t{1,0}, edge_init_t{1,1}, edge_init_t{2,0}, edge_init_t{2,1}},
                       {edge_init_t{0,0}, edge_init_t{0,1}, edge_init_t{2,2}, edge_init_t{2,3}},
                       {edge_init_t{0,2}, edge_init_t{0,3}, edge_init_t{1,2}, edge_init_t{1,3}}
                     }, LINE(""));
    }
    
    mutual_info(GraphFlavour) ? network.erase_edge(network.cbegin_edges(2)+2) : network.erase_edge(network.cbegin_edges(2));

    //    0=====1
    //    ||  /
    //    ||/
    //     2

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(network,
                     GGraph{
                       {edge_init_t{1}, edge_init_t{1}, edge_init_t{2}, edge_init_t{2}},
                       {edge_init_t{2}},
                       {}
                     }, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {      
      check_equality(network,
                     GGraph{
                       {edge_init_t{1}, edge_init_t{1}, edge_init_t{2}, edge_init_t{2}},
                       {edge_init_t{0}, edge_init_t{0}, edge_init_t{2}},
                       {edge_init_t{0}, edge_init_t{0}, edge_init_t{1}}
                     }, LINE(""));
    }    
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(network,
                     GGraph{
                       {edge_init_t{0,1,0}, edge_init_t{0,1,1}, edge_init_t{0,2,0}, edge_init_t{0,2,1}},
                       {edge_init_t{0,1,0}, edge_init_t{0,1,1}, edge_init_t{2,1,2}},
                       {edge_init_t{0,2,2}, edge_init_t{0,2,3}, edge_init_t{2,1,2}}
                     }, LINE(""));
    }
    else 
    { 
      check_equality(network,
                     GGraph{
                       {edge_init_t{1,0}, edge_init_t{1,1}, edge_init_t{2,0}, edge_init_t{2,1}},
                       {edge_init_t{0,0}, edge_init_t{0,1}, edge_init_t{2,2}},
                       {edge_init_t{0,2}, edge_init_t{0,3}, edge_init_t{1,2}}
                     }, LINE(""));
    }
        
    network.erase_node(0);
    //  0----1

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(network, GGraph{{edge_init_t{1}}, {}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {      
      check_equality(network, GGraph{{edge_init_t{1}}, {edge_init_t{0}}}, LINE(""));
    }    
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(network,  GGraph{{edge_init_t{1,0,0}}, {edge_init_t{1,0,0}}}, LINE(""));
    }
    else 
    { 
      check_equality(network,  GGraph{{edge_init_t{1,0}}, {edge_init_t{0,0}}}, LINE(""));
    }

    network.join(0, 0);
    // /\
    // \/
    //  0---1

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(network, GGraph{{edge_init_t{1}, edge_init_t{0}}, {}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      GGraph g{{edge_init_t{0}, edge_init_t{0}, edge_init_t{1}}, {edge_init_t{0}}};
      g.sort_edges(g.cbegin_edges(0), g.cend_edges(0),
                   [](const auto& l, const auto& r) {return l.target_node() > r.target_node();});
      
      check_equality(network, g, LINE(""));
    }    
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(network,
                     GGraph{
                       {edge_init_t{1,0,0}, edge_init_t{0,0,2}, edge_init_t{0,0,1}},
                       {edge_init_t{1,0,0}}
                     }, LINE(""));
    }
    else 
    { 
      check_equality(network,
                     GGraph{
                       {edge_init_t{1,0}, edge_init_t{0,2}, edge_init_t{0,1}},
                       {edge_init_t{0,0}}
                     }, LINE(""));
    }
          
    network.erase_edge(++network.cbegin_edges(0));
    //  0----1

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(network, GGraph{{edge_init_t{1}}, {}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {      
      check_equality(network, GGraph{{edge_init_t{1}}, {edge_init_t{0}}}, LINE(""));
    }    
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(network,  GGraph{{edge_init_t{1,0,0}}, {edge_init_t{1,0,0}}}, LINE(""));
    }
    else 
    { 
      check_equality(network,  GGraph{{edge_init_t{1,0}}, {edge_init_t{0,0}}}, LINE(""));
    }

    network.erase_node(0);
    // 0

    check_equality(network, GGraph{{}});

    check_graph(network, {{}}, {}, LINE(""));

    network.erase_node(0);
    //

    check_graph(network, {}, {}, LINE(""));

    check_equality(network.add_node(), 0ul, LINE("Node added back to graph"));
    // 0
    
    network.join(0, 0);
    // /\
    // \/
    // 0

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(network, GGraph{{edge_init_t{0}}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {      
      check_equality(network, GGraph{{edge_init_t{0}, edge_init_t{0}}}, LINE(""));
    }    
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(network,  GGraph{{edge_init_t{0,0,1}, edge_init_t{0,0,0}}}, LINE(""));
    }
    else 
    {
      check_equality(network,  GGraph{{edge_init_t{0,1}, edge_init_t{0,0}}}, LINE(""));
    }

    check_equality(network.insert_node(0), 0ul, LINE("Prepend a node to the exising graph"));
    //    /\
    //    \/
    // 0  0

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(network, GGraph{{}, {edge_init_t{1}}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {      
      check_equality(network, GGraph{{}, {edge_init_t{1}, edge_init_t{1}}}, LINE(""));
    }    
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(network,  GGraph{{}, {edge_init_t{1,1,1}, edge_init_t{1,1,0}}}, LINE(""));
    }
    else 
    {
      check_equality(network,  GGraph{{}, {edge_init_t{1,1}, edge_init_t{1,0}}}, LINE(""));
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
      check_equality(network, GGraph{{edge_init_t{2}}, {}, {edge_init_t{2}}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      GGraph g{{edge_init_t{2}}, {}, {edge_init_t{0}, edge_init_t{2}, edge_init_t{2}}};
      g.sort_edges(g.cbegin_edges(2), g.cend_edges(2), [](const auto& l, const auto& r) { return l.target_node() > r.target_node(); });
      check_equality(network, g, LINE(""));
    }    
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(network,  GGraph{{edge_init_t{0,2,2}}, {}, {edge_init_t{2,2,1}, edge_init_t{2,2,0}, edge_init_t{0,2,0}}}, LINE(""));
    }
    else 
    { 
      check_equality(network,  GGraph{{edge_init_t{2,2}}, {}, {edge_init_t{2,1}, edge_init_t{2,0}, edge_init_t{0,0}}}, LINE(""));
    }

    
    network.erase_node(1);
    network.erase_node(0);
    // /\
    // \/
    // 0

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(network, GGraph{{edge_init_t{0}}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {      
      check_equality(network, GGraph{{edge_init_t{0}, edge_init_t{0}}}, LINE(""));
    }    
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(network,  GGraph{{edge_init_t{0,0,1}, edge_init_t{0,0,0}}}, LINE(""));
    }
    else 
    { 
      check_equality(network,  GGraph{{edge_init_t{0,1}, edge_init_t{0,0}}}, LINE(""));
    }
        
    GGraph network2;
      
    check_equality(network2.add_node(), 0ul, LINE("Node added to second network"));
    check_equality(network2.add_node(), 1ul, LINE("Second node added to second network"));
    check_equality(network2.add_node(), 2ul, LINE("Third node added to second network"));
    check_equality(network2.add_node(), 3ul, LINE("Fourth node added to second network"));

    network2.join(0, 1);
    network2.join(3, 2);
    // 0----0    0----0

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(network2, GGraph{{edge_init_t{1}}, {}, {}, {edge_init_t{2}}}, LINE("Check di-component graph"));
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {      
      check_equality(network2, GGraph{{edge_init_t{1}}, {edge_init_t{0}}, {edge_init_t{3}}, {edge_init_t{2}}}, LINE("Check di-component graph"));
    }    
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(network2,  GGraph{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}}, {edge_init_t{3,2,0}}, {edge_init_t{3,2,0}}}, LINE("Check di-component graph"));
    }
    else 
    { 
      check_equality(network2, GGraph{{edge_init_t{1,0}}, {edge_init_t{0,0}}, {edge_init_t{3,0}}, {edge_init_t{2,0}}}, LINE("Check di-component graph"));
    }

    check_regular_semantics(network, network2, LINE("Regular semantics"));
  }

  // Capacity

  template
  <
    maths::graph_flavour GraphFlavour,
    class EdgeWeight,
    class NodeWeight,      
    template <class> class EdgeWeightPooling,
    template <class> class NodeWeightPooling,
    template <maths::graph_flavour, class, template<class> class> class EdgeStorageTraits,
    template <class, template<class> class, bool> class NodeWeightStorageTraits
  >
  void graph_contiguous_capacity<
      GraphFlavour,
      EdgeWeight,
      NodeWeight,
      EdgeWeightPooling,
      NodeWeightPooling,
      EdgeStorageTraits,
      NodeWeightStorageTraits
  >::execute_operations()
  {
    graph_t g{};

    check_equality(g.edges_capacity(), 0ul, LINE(""));
    check_equality(g.node_capacity(), 0ul, LINE(""));

    g.reserve_edges(4);
    check_equality(g.edges_capacity(), 4ul, LINE(""));

    g.reserve_nodes(4);
    check_equality(g.node_capacity(), 4ul, LINE(""));

    g.shrink_to_fit();
    check_equality(g.edges_capacity(), 0ul, LINE("May fail if stl implementation doesn't actually shrink to fit!"));
    check_equality(g.node_capacity(), 0ul, LINE("May fail if stl implementation doesn't actually shrink to fit!"));    
  }

   template
  <
    maths::graph_flavour GraphFlavour,
    class EdgeWeight,
    class NodeWeight,      
    template <class> class EdgeWeightPooling,
    template <class> class NodeWeightPooling,
     template <maths::graph_flavour, class, template<class> class> class EdgeStorageTraits,
    template <class, template<class> class, bool> class NodeWeightStorageTraits
  >
  void graph_bucketed_capacity<
      GraphFlavour,
      EdgeWeight,
      NodeWeight,
      EdgeWeightPooling,
      NodeWeightPooling,
      EdgeStorageTraits,
      NodeWeightStorageTraits
  >::execute_operations()
  {
    graph_t g{};

    check_exception_thrown<std::out_of_range>([&g](){ g.reserve_edges(0, 4);}, LINE(""));
    check_exception_thrown<std::out_of_range>([&g](){ return g.edges_capacity(0);}, LINE(""));
    check_equality(g.node_capacity(), 0ul, LINE(""));

    g.add_node();
    check_equality(g.edges_capacity(0), 0ul, LINE(""));
    check_equality(g.node_capacity(), 1ul, LINE(""));
    g.reserve_edges(0, 4);
    check_equality(g.edges_capacity(0), 4ul, LINE(""));

    g.reserve_nodes(4);
    check_equality(g.node_capacity(), 4ul, LINE(""));

    g.shrink_to_fit();
    check_equality(g.edges_capacity(0), 0ul, LINE("May fail if stl implementation doesn't actually shrink to fit!"));
    check_equality(g.node_capacity(), 1ul, LINE("May fail if stl implementation doesn't actually shrink to fit!"));
  }

  // Generic Weighted
  
  template
  <
    maths::graph_flavour GraphFlavour,
    class EdgeWeight,
    class NodeWeight,      
    template <class> class EdgeWeightPooling,
    template <class> class NodeWeightPooling,
    template <maths::graph_flavour, class, template<class> class> class EdgeStorageTraits,
    template <class, template<class> class, bool> class NodeWeightStorageTraits
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
    using edge_init_t = typename GGraph::edge_init_type;
    using edge_init_list_t = std::initializer_list<std::initializer_list<edge_init_t>>;
    using edge_weight_t = typename GGraph::edge_weight_type; 
    
    GGraph graph;    
    using Edge = maths::edge<EdgeWeight, utilities::protective_wrapper<EdgeWeight>>;

    graph.reserve_nodes(4);
    check_equality(graph.node_capacity(), 4ul, LINE(""));
    graph.shrink_to_fit();
    check_equality(graph.node_capacity(), 0ul, LINE("May fail if stl implementation doesn't actually shrink to fit!"));

    // NULL
    check_exception_thrown<std::out_of_range>([&graph](){ graph.erase_node(0); }, LINE("No nodes to erase"));
    check_exception_thrown<std::out_of_range>([&graph](){ graph.erase_edge(graph.cbegin_edges(0)); }, LINE("No edges to erase"));
    check_exception_thrown<std::out_of_range>([&graph](){ graph.node_weight(graph.cend_node_weights(), 1.0); }, LINE("No nodes to set weight"));
    check_exception_thrown<std::out_of_range>([&graph](){ graph.set_edge_weight(graph.cbegin_edges(0), 1); }, LINE("No edges to set weight"));
    
    check_equality(graph, GGraph{}, LINE(""));
        
    check_equality(graph.add_node(0.0, 1.0), 0ul, "Node added with weight i");
    //   (i)

    check_equality(graph, GGraph{edge_init_list_t{{}}, {{0,1}}}, LINE(""));

    check_exception_thrown<std::out_of_range>([&graph](){ graph.node_weight(graph.cend_node_weights(), 0.0); }, LINE("Throw if node out of range"));
    
    graph.erase_node(0);
    //    NULL

    check_equality(graph, GGraph{}, LINE(""));

    check_equality(graph.add_node(0.0, 1.0), 0ul, "Node of weight i recreated");
    //    (i)

    check_equality(graph, GGraph{edge_init_list_t{{}}, {{0,1}}}, LINE(""));
        
    graph.node_weight(graph.cbegin_node_weights(), 1.1, -4.3);
    // (1.1-i4.3)

    check_equality(graph, GGraph{edge_init_list_t{{}}, {{1.1,-4.3}}}, LINE(""));

    graph.join(0, 0, 1);
    //   /\ 1
    //   \/
    //  (1.1-i4.3)
    
    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(graph, GGraph{{{{edge_init_t{0,1}}}}, {{1.1,-4.3}}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {      
      check_equality(graph, GGraph{{{edge_init_t{0,1}, edge_init_t{0,1}}}, {{1.1,-4.3}}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(graph, GGraph{{{edge_init_t{0,0,1,1}, edge_init_t{0,0,0,1}}}, {{1.1,-4.3}}}, LINE(""));
    }
    else 
    { 
      check_equality(graph, GGraph{{{edge_init_t{0,1,1}, edge_init_t{0,0,1}}}, {{1.1,-4.3}}}, LINE(""));
    }

    graph.set_edge_weight(graph.cbegin_edges(0), -2);
    //   /\ -2
    //   \/
    // (1.1-i4.3)

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(graph, GGraph{{{{edge_init_t{0,-2}}}}, {{1.1,-4.3}}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {      
      check_equality(graph, GGraph{{{edge_init_t{0,-2}, edge_init_t{0,-2}}}, {{1.1,-4.3}}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(graph, GGraph{{{edge_init_t{0,0,1,-2}, edge_init_t{0,0,0,-2}}}, {{1.1,-4.3}}}, LINE(""));
    }
    else 
    { 
      check_equality(graph, GGraph{{{edge_init_t{0,1,-2}, edge_init_t{0,0,-2}}}, {{1.1,-4.3}}}, LINE(""));
    }

    graph.set_edge_weight(graph.cbegin_edges(0), -4);
    //   /\ -4
    //   \/
    // (1.1-i4.3)

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(graph, GGraph{{{{edge_init_t{0,-4}}}}, {{1.1,-4.3}}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {      
      check_equality(graph, GGraph{{{edge_init_t{0,-4}, edge_init_t{0,-4}}}, {{1.1,-4.3}}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(graph, GGraph{{{edge_init_t{0,0,1,-4}, edge_init_t{0,0,0,-4}}}, {{1.1,-4.3}}}, LINE(""));
    }
    else 
    { 
      check_equality(graph, GGraph{{{edge_init_t{0,1,-4}, edge_init_t{0,0,-4}}}, {{1.1,-4.3}}}, LINE(""));
    }

    graph.mutate_edge_weight(graph.cbegin_edges(0), [](auto& val) { val *= 2; });
    //   /\ -8
    //   \/
    // (1.1-i4.3)

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(graph, GGraph{{{{edge_init_t{0,-8}}}}, {{1.1,-4.3}}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {      
      check_equality(graph, GGraph{{{edge_init_t{0,-8}, edge_init_t{0,-8}}}, {{1.1,-4.3}}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(graph, GGraph{{{edge_init_t{0,0,1,-8}, edge_init_t{0,0,0,-8}}}, {{1.1,-4.3}}}, LINE(""));
    }
    else 
    { 
      check_equality(graph, GGraph{{{edge_init_t{0,1,-8}, edge_init_t{0,0,-8}}}, {{1.1,-4.3}}}, LINE(""));
    }

    graph.erase_edge(graph.cbegin_edges(0));
    //  (1.1-i4.3)

    check_equality(graph, GGraph{edge_init_list_t{{}}, {{1.1,-4.3}}}, LINE(""));

    graph.join(0, 0, 1);
    //   /\ 1
    //   \/
    // (1.1-i4.3)

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(graph, GGraph{{{{edge_init_t{0,1}}}}, {{1.1,-4.3}}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {      
      check_equality(graph, GGraph{{{edge_init_t{0,1}, edge_init_t{0,1}}}, {{1.1,-4.3}}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(graph, GGraph{{{edge_init_t{0,0,1,1}, edge_init_t{0,0,0,1}}}, {{1.1,-4.3}}}, LINE(""));
    }
    else 
    { 
      check_equality(graph, GGraph{{{edge_init_t{0,1,1}, edge_init_t{0,0,1}}}, {{1.1,-4.3}}}, LINE(""));
    }
        
    graph.join(0, 0, -1);
    //   /\ 1
    //   \/
    // (1.1-i4.3)
    //   /\
    //   \/ -1

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(graph, GGraph{{{edge_init_t{0,1}, edge_init_t{0,-1}}}, {{1.1,-4.3}}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      GGraph g{{{edge_init_t{0,1}, edge_init_t{0,1}, edge_init_t{0,-1}, edge_init_t{0,-1}}}, {{1.1,-4.3}}};

      if constexpr(is_orderable_v<edge_weight_t>)
      {
        g.sort_edges(g.cbegin_edges(0), g.cend_edges(0),
                   [](const auto& l , const auto& r){ return l.weight() > r.weight(); }
        );
      }
      check_equality(graph, g, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(graph, GGraph{{{edge_init_t{0,0,1,1}, edge_init_t{0,0,0,1}, edge_init_t{0,0,3,-1}, edge_init_t{0,0,2,-1}}}, {{1.1,-4.3}}}, LINE(""));
    }
    else 
    { 
      check_equality(graph, GGraph{{{edge_init_t{0,1,1}, edge_init_t{0,0,1}, edge_init_t{0,3,-1}, edge_init_t{0,2,-1}}}, {{1.1,-4.3}}}, LINE(""));
    }

    mutual_info(GraphFlavour) ? graph.set_edge_weight(graph.cbegin_edges(0) + 2, -4) : graph.set_edge_weight(++graph.cbegin_edges(0), -4);
    //   /\ 1
    //   \/
    // (1.1-i4.3)
    //   /\
    //   \/ -4
         
    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(graph, GGraph{{{edge_init_t{0,1}, edge_init_t{0,-4}}}, {{1.1,-4.3}}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      GGraph g{{{edge_init_t{0,1}, edge_init_t{0,1}, edge_init_t{0,-4}, edge_init_t{0,-4}}}, {{1.1,-4.3}}};

      if constexpr(is_orderable_v<edge_weight_t>)
      {
        g.sort_edges(g.cbegin_edges(0), g.cend_edges(0),
                   [](const auto& l , const auto& r){ return l.weight() > r.weight(); }
        );
      }
      check_equality(graph, g, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(graph, GGraph{{{edge_init_t{0,0,1,1}, edge_init_t{0,0,0,1}, edge_init_t{0,0,3,-4}, edge_init_t{0,0,2,-4}}}, {{1.1,-4.3}}}, LINE(""));
    }
    else 
    { 
      check_equality(graph, GGraph{{{edge_init_t{0,1,1}, edge_init_t{0,0,1}, edge_init_t{0,3,-4}, edge_init_t{0,2,-4}}}, {{1.1,-4.3}}}, LINE(""));
    }


    check_equality(graph.add_node(), 1ul, LINE(""));
    //   /\ 1
    //   \/
    // (1.1-i4.3)   (0)
    //   /\
    //   \/ -4
        
    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(graph, GGraph{{{edge_init_t{0,1}, edge_init_t{0,-4}}, {}}, {{1.1,-4.3}, {}}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      GGraph g{{{edge_init_t{0,1}, edge_init_t{0,1}, edge_init_t{0,-4}, edge_init_t{0,-4}}, {}}, {{1.1,-4.3}, {}}};

      if constexpr(is_orderable_v<edge_weight_t>)
      {
        g.sort_edges(g.cbegin_edges(0), g.cend_edges(0),
                   [](const auto& l , const auto& r){ return l.weight() > r.weight(); }
        );
      }
      check_equality(graph, g, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(graph, GGraph{{{edge_init_t{0,0,1,1}, edge_init_t{0,0,0,1}, edge_init_t{0,0,3,-4}, edge_init_t{0,0,2,-4}}, {}}, {{1.1,-4.3}, {}}}, LINE(""));
    }
    else 
    { 
      check_equality(graph, GGraph{{{edge_init_t{0,1,1}, edge_init_t{0,0,1}, edge_init_t{0,3,-4}, edge_init_t{0,2,-4}}, {}}, {{1.1,-4.3}, {}}}, LINE(""));
    }

    graph.join(0, 1, 6);
    //   /\1
    //   \/  6
    // (1.1-i4.3)------(0)
    //   /\
    //   \/-4
        
    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(graph, GGraph{{{edge_init_t{0,1}, edge_init_t{0,-4}, edge_init_t{1, 6}}, {}}, {{1.1,-4.3}, {}}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      GGraph g{{{edge_init_t{0,1}, edge_init_t{0,1}, edge_init_t{0,-4}, edge_init_t{0,-4}, edge_init_t{1, 6}},
                {edge_init_t{0, 6}}
               }, {{1.1,-4.3}, {}}};

      if constexpr(is_orderable_v<edge_weight_t>)
      {
        g.sort_edges(g.cbegin_edges(0), g.cend_edges(0) - 1,
                   [](const auto& l , const auto& r){ return l.weight() > r.weight(); }
        );
      }
      check_equality(graph, g, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(graph,
                     GGraph{{{edge_init_t{0,0,1,1}, edge_init_t{0,0,0,1}, edge_init_t{0,0,3,-4}, edge_init_t{0,0,2,-4}, edge_init_t{0,1,0,6}},
                             {edge_init_t{0,1,4,6}}
                            }, {{1.1,-4.3}, {}}}, LINE(""));
    }
    else 
    { 
      check_equality(graph,
                     GGraph{{{edge_init_t{0,1,1}, edge_init_t{0,0,1}, edge_init_t{0,3,-4}, edge_init_t{0,2,-4}, edge_init_t{1,0,6}},
                             {edge_init_t{0,4,6}}
                            }, {{1.1,-4.3}, {}}}, LINE(""));
    }
    
    graph.swap_nodes(0,1);

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(graph, GGraph{{{}, {edge_init_t{1,1}, edge_init_t{1,-4}, edge_init_t{0, 6}}}, {{}, {1.1,-4.3}}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      GGraph g{{{edge_init_t{1,6}},
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
      
      check_equality(graph, g, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(graph,
                     GGraph{{{edge_init_t{1,0,4,6}},
                             {edge_init_t{1,1,1,1}, edge_init_t{1,1,0,1}, edge_init_t{1,1,3,-4}, edge_init_t{1,1,2,-4}, edge_init_t{1,0,0,6}}
                            }, {{}, {1.1,-4.3}}}, LINE(""));
    }
    else 
    { 
      check_equality(graph,
                     GGraph{{{edge_init_t{1,4,6}},
                             {edge_init_t{1,1,1}, edge_init_t{1,0,1}, edge_init_t{1,3,-4}, edge_init_t{1,2,-4}, edge_init_t{0,0,6}}
                       }, {{}, {1.1,-4.3}}}, LINE(""));
    }

    graph.swap_nodes(1,0);

    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(graph, GGraph{{{edge_init_t{0,1}, edge_init_t{0,-4}, edge_init_t{1, 6}}, {}}, {{1.1,-4.3}, {}}}, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      GGraph g{{{edge_init_t{0,1}, edge_init_t{0,1}, edge_init_t{0,-4}, edge_init_t{0,-4}, edge_init_t{1, 6}},
                {edge_init_t{0, 6}}
               }, {{1.1,-4.3}, {}}};

      if constexpr(is_orderable_v<edge_weight_t>)
      {
        g.sort_edges(g.cbegin_edges(0), g.cend_edges(0) - 1,
                   [](const auto& l , const auto& r){ return l.weight() > r.weight(); }
        );
      }
      check_equality(graph, g, LINE(""));
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(graph,
                     GGraph{{{edge_init_t{0,0,1,1}, edge_init_t{0,0,0,1}, edge_init_t{0,0,3,-4}, edge_init_t{0,0,2,-4}, edge_init_t{0,1,0,6}},
                             {edge_init_t{0,1,4,6}}
                            }, {{1.1,-4.3}, {}}}, LINE(""));
    }
    else 
    { 
      check_equality(graph,
                     GGraph{{{edge_init_t{0,1,1}, edge_init_t{0,0,1}, edge_init_t{0,3,-4}, edge_init_t{0,2,-4}, edge_init_t{1,0,6}},
                             {edge_init_t{0,4,6}}
                            }, {{1.1,-4.3}, {}}}, LINE(""));
    }

    graph.set_edge_weight(--graph.cend_edges(0), 7);
    //   /\1
    //   \/  7
    // (1.1-i4.3)------(0)
    //   /\
    //   \/-4
        
    if constexpr(mutual_info(GraphFlavour))
    {
      check_graph(graph, {{Edge(0,0,1), Edge(0,0,1), Edge(0,0,-4), Edge(0,0,-4), Edge(0,1,7)}, {Edge(0,1,7)}}, {{1.1,-4.3}, {0,0}}, LINE(""));
    }
    else
    {
      check_graph(graph, {{Edge(0,0,1), Edge(0,0,-4), Edge(0,1,7)}, {}}, {{1.1,-4.3}, {0,0}}, LINE(""));
    }
       
    graph.set_edge_weight(--graph.cend_edges(0), 6);
    //   /\1
    //   \/  6
    // (1.1-i4.3)------(0)
    //   /\
    //   \/-4

    if constexpr(mutual_info(GraphFlavour))
    {
      check_graph(graph, {{Edge(0,0,1), Edge(0,0,1), Edge(0,0,-4), Edge(0,0,-4), Edge(0,1,6)}, {Edge(0,1,6)}}, {{1.1,-4.3}, {0,0}}, LINE(""));
    }
    else
    {
      check_graph(graph, {{Edge(0,0,1), Edge(0,0,-4), Edge(0,1,6)}, {}}, {{1.1,-4.3}, {0,0}}, LINE(""));
    }
        
    graph.join(1, 0, 7);
    //     /\1
    //     \/     6
    // (1.1-i4.3)======(0)
    //     /\     7
    //     \/-4
       
    if constexpr(mutual_info(GraphFlavour))
    {
      check_graph(graph, {{Edge(0,0,1), Edge(0,0,1), Edge(0,0,-4), Edge(0,0,-4), Edge(0,1,6), Edge(1,0,7)}, {Edge(0,1,6), Edge(1,0,7)}}, {{1.1,-4.3}, {0,0}}, LINE(""));
    }
    else
    {
      check_graph(graph, {{Edge(0,0,1), Edge(0,0,-4), Edge(0,1,6)}, {Edge(1,0,7)}}, {{1.1,-4.3}, {0,0}}, LINE(""));
    }
        

    graph.join(0, 1, 8);
    //     /\1   /--\8
    //     \/   /6   \
    // (1.1-i4.3)====(0)
    //     /\     7
    //     \/-4

    if constexpr(mutual_info(GraphFlavour))
    {
      check_graph(graph, {{Edge(0,0,1), Edge(0,0,1), Edge(0,0,-4), Edge(0,0,-4), Edge(0,1,6), Edge(1,0,7), Edge(0,1,8)}, {Edge(0,1,6), Edge(1,0,7), Edge(0,1,8)}}, {{1.1,-4.3}, {0,0}}, LINE(""));
    }
    else
    {
      check_graph(graph, {{Edge(0,0,1), Edge(0,0,-4), Edge(0,1,6), Edge(0,1,8)}, {Edge(1,0,7)}}, {{1.1,-4.3}, {0,0}}, LINE(""));
    }
        
    graph.erase_edge(graph.cbegin_edges(0));
    //            8
    //           /--\
    //          / 6  \
    // (1.1-i4.3)====(0)
    //     /\     7
    //     \/-4
        
    if constexpr(mutual_info(GraphFlavour))
    {
      check_graph(graph, {{Edge(0,0,-4), Edge(0,0,-4), Edge(0,1,6), Edge(1,0,7), Edge(0,1,8)}, {Edge(0,1,6), Edge(1,0,7), Edge(0,1,8)}}, {{1.1,-4.3}, {0,0}}, LINE(""));
    }
    else
    {
      check_graph(graph, {{Edge(0,0,-4), Edge(0,1,6), Edge(0,1,8)}, {Edge(1,0,7)}}, {{1.1,-4.3}, {0,0}}, LINE(""));
    }
        
    graph.mutate_edge_weight(graph.cbegin_edges(0) + (mutual_info(GraphFlavour) ?  3 : 2), [](auto& val){ val = 6; });
    //            8
    //           /--\
    //          / 6  \
    // (1.1-i4.3)====(0)
    //     /\     6
    //     \/-4

    if constexpr(!mutual_info(GraphFlavour))
    {
      check_graph(graph, {{Edge(0,0,-4), Edge(0,1,6), Edge(0,1,6)}, {Edge(1,0,7)}}, {{1.1,-4.3}, {0,0}}, LINE(""));
    }
    else
    {
      check_graph(graph, {{Edge(0,0,-4), Edge(0,0,-4), Edge(0,1,6), Edge(1,0,6), Edge(0,1,8)}, {Edge(0,1,6), Edge(1,0,6), Edge(0,1,8)}}, {{1.1,-4.3}, {0,0}}, LINE(""));
    }

    graph.set_edge_weight(graph.cbegin_edges(0) + (mutual_info(GraphFlavour) ?  3 : 2), 10);
    //            8
    //           /--\
    //          / 10 \
    // (1.1-i4.3)====(0)
    //     /\     6
    //     \/-4

    if constexpr(GraphFlavour == maths::graph_flavour::directed)
    {
      check_graph(graph, {{Edge(0,0,-4), Edge(0,1,6), Edge(0,1,10)}, {Edge(1,0,7)}}, {{1.1,-4.3}, {0,0}}, LINE(""));
    }
    else if constexpr(GraphFlavour == maths::graph_flavour::undirected)
    {
      check_graph(graph, {{Edge(0,0,-4), Edge(0,0,-4), Edge(0,1,6), Edge(1,0,10), Edge(0,1,8)}, {Edge(0,1,10), Edge(1,0,6), Edge(0,1,8)}}, {{1.1,-4.3}, {0,0}}, LINE(""));
    }
    else
    {
      check_graph(graph, {{Edge(0,0,-4), Edge(0,0,-4), Edge(0,1,6), Edge(1,0,10), Edge(0,1,8)}, {Edge(0,1,6), Edge(1,0,10), Edge(0,1,8)}}, {{1.1,-4.3}, {0,0}}, LINE(""));
    }

    graph.mutate_edge_weight(graph.cbegin_edges(0) + (mutual_info(GraphFlavour) ?  4 : 1), [](auto& val){ val = 10; });
    //            10
    //           /--\
    //          / 10 \
    // (1.1-i4.3)====(0)
    //     /\     6
    //     \/-4

    if constexpr(GraphFlavour == maths::graph_flavour::directed)
    {
      check_graph(graph, {{Edge(0,0,-4), Edge(0,1,10), Edge(0,1,10)}, {Edge(1,0,7)}}, {{1.1,-4.3}, {0,0}}, LINE(""));
    }
    else if constexpr(GraphFlavour == maths::graph_flavour::undirected)
    {
      check_graph(graph, {{Edge(0,0,-4), Edge(0,0,-4), Edge(0,1,6), Edge(1,0,10), Edge(0,1,10)}, {Edge(0,1,10), Edge(1,0,6), Edge(0,1,10)}}, {{1.1,-4.3}, {0,0}}, LINE(""));
    }
    else
    {
      check_graph(graph, {{Edge(0,0,-4), Edge(0,0,-4), Edge(0,1,6), Edge(1,0,10), Edge(0,1,10)}, {Edge(0,1,6), Edge(1,0,10), Edge(0,1,10)}}, {{1.1,-4.3}, {0,0}}, LINE(""));
    }

    graph.mutate_edge_weight(graph.cbegin_edges(0) + (mutual_info(GraphFlavour) ?  4 : 2), [](auto& val){ val = 7; });
    //            7
    //           /--\
    //          / 10 \
    // (1.1-i4.3)====(0)
    //     /\     6
    //     \/-4

    if constexpr(GraphFlavour == maths::graph_flavour::directed)
    {
      check_graph(graph, {{Edge(0,0,-4), Edge(0,1,10), Edge(0,1,7)}, {Edge(1,0,7)}}, {{1.1,-4.3}, {0,0}}, LINE(""));
    }
    else if constexpr(GraphFlavour == maths::graph_flavour::undirected)
    {
      check_graph(graph, {{Edge(0,0,-4), Edge(0,0,-4), Edge(0,1,6), Edge(1,0,10), Edge(0,1,7)}, {Edge(0,1,7), Edge(1,0,6), Edge(0,1,10)}}, {{1.1,-4.3}, {0,0}}, LINE(""));
    }
    else
    {
      check_graph(graph, {{Edge(0,0,-4), Edge(0,0,-4), Edge(0,1,6), Edge(1,0,10), Edge(0,1,7)}, {Edge(0,1,6), Edge(1,0,10), Edge(0,1,7)}}, {{1.1,-4.3}, {0,0}}, LINE(""));
    }
        
    graph.erase_node(0);
    //  (0)

    check_graph(graph, {{}}, {{0}});
        

    graph.join(0, 0, 1);
    //   /\ 1
    //   \/
    //   (0)
        
    if constexpr(mutual_info(GraphFlavour))
    {
      check_graph(graph, {{Edge(0, 0, 1), Edge(0, 0, 1)}}, {{0}}, LINE(""));
    }
    else
    {
      check_graph(graph, {{Edge(0, 0, 1)}}, {{0}}, LINE(""));
    }
 

    check_equality(graph.insert_node(0, 1.0, 1.0), 0ul, "Node with weight 1+i inserted into slot 0");
    //       /\ 1
    //       \/
    // (1+i) (0)

    if constexpr(mutual_info(GraphFlavour))
    {
      check_graph(graph, {{}, {Edge(1,1,1), Edge(1,1,1)}}, {{1,1}, {0}}, LINE(""));
    }
    else
    {
      check_graph(graph, {{}, {Edge(1,1,1)}}, {{1,1}, {0}}, LINE(""));
       
    }
  }

  
  // Copy, Move...
  
  template
  <
    maths::graph_flavour GraphFlavour,
    class EdgeWeight,
    class NodeWeight,      
    template <class> class EdgeWeightPooling,
    template <class> class NodeWeightPooling,
    template <maths::graph_flavour, class, template<class> class> class EdgeStorageTraits,
    template <class, template<class> class, bool> class NodeWeightStorageTraits
  >
  void test_copy_move<
      GraphFlavour,
      EdgeWeight,
      NodeWeight,
      EdgeWeightPooling,
      NodeWeightPooling,
      EdgeStorageTraits,
      NodeWeightStorageTraits
  >::execute_operations()
  {
    const std::vector<int> specialWeight{5, -6};

    GGraph graph1{generate_test_graph1(specialWeight)};
    check_test_graph_1(graph1, specialWeight, LINE("Checking rvalue constructor"));
 
    GGraph graph2;
    check_empty_graph(graph2);

    graph2 = graph1;

    check_test_graph_1(graph2, specialWeight, LINE("Checking assignment operator"));
        
    graph1 = GGraph();
    check_empty_graph(graph1, LINE("Checking assignment of empty graph"));

    std::swap(graph1, graph2);

    check_test_graph_1(graph1, specialWeight, LINE("Checking results of swap"));
    check_empty_graph(graph2, LINE("Checking results of swap giving empty graph"));
        
    GGraph graph3{graph1};
    check_test_graph_1(graph3, specialWeight, LINE("Checking lvalue constructor"));
    check_test_graph_1(graph1, specialWeight, LINE("Checking no unexpected side effects of lvalue constructor"));
        
    const std::vector<int> newWeight{-7, 9};
    graph1.set_edge_weight(mutual_info(GraphFlavour) ? graph1.cbegin_edges(0) + 2: ++graph1.cbegin_edges(0), newWeight);
        
    check_test_graph_1(graph1, newWeight, LINE("Checking result of setting weight"));
    check_test_graph_1(graph3, specialWeight, LINE("Checking no unexpected side effects of result of setting weight"));
        
    GGraph graph4{graph1};
    graph4.set_edge_weight(mutual_info(GraphFlavour) ? graph4.cbegin_edges(0) + 2 : ++graph4.cbegin_edges(0), 8, 4);
    check_test_graph_1(graph4, {8,4});
  }
}
