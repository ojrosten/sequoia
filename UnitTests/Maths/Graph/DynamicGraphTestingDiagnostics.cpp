////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "DynamicGraphTestingDiagnostics.hpp"

namespace sequoia::unit_testing
{
  void test_graph_false_positives::run_tests()
  {    
    graph_test_helper<null_weight, null_weight> helper{concurrent_execution()};
    helper.run_tests<dynamic_graph_false_positives>(*this);
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
  void dynamic_graph_false_positives<
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

    graph_t network{};

    check_equality(LINE("Check false positive: empty graph versus single node"), network, {{}});

    std::string message{"Check false positive: empty graph versus single node with loop"};
    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(message), network, graph_t{{edge_init_t{0}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_equality(LINE(message), network, graph_t{{edge_init_t{0}, edge_init_t{0}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(message), network, graph_t{{edge_init_t{0,0,1}, edge_init_t{0,0,0}}});
    }
    else
    {
      check_equality(LINE(message), network, graph_t{{edge_init_t{0,1}, edge_init_t{0,0}}});
    }
  }
}
