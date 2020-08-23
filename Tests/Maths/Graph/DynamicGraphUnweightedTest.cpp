////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "DynamicGraphUnweightedTest.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view unweighted_graph_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void unweighted_graph_test::run_tests()
  {
    struct null_weight {};

    graph_test_helper<null_weight, null_weight> helper{concurrent_execution()};
      
    helper.run_tests<generic_graph_operations>(*this);
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
    check_semantics(LINE("Regular semantics"), network, graph_t{});

    check_exception_thrown<std::out_of_range>(LINE("Unable to join zeroth node to non-existant first node"), [&network]() { network.join(0, 1); });

    check_exception_thrown<std::out_of_range>(LINE("Index out of range for swapping nodes"), [&network]() { network.swap_nodes(0,1); });
    
    check_equality(LINE("Index of added node is 1"), network.add_node(), 1ul);
    //    0    1

    check_equality(LINE(""), network, graph_t{{}, {}});
    check_semantics(LINE("Regular semantics"), network, graph_t{{}});

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

    check_semantics(LINE("Regular semantics"), network, graph_t{{}, {}});
    
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

    check_semantics(LINE("Regular semantics"), network, network2);
  }  
}
