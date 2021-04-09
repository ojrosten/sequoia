////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "DynamicGraphTestingDiagnostics.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view test_graph_false_positives::source_file() const noexcept
  {
    return __FILE__;
  }

  void test_graph_false_positives::run_tests()
  {    
    graph_test_helper<null_weight, null_weight, test_graph_false_positives> helper{*this};
    helper.run_tests();
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
  void test_graph_false_positives::execute_operations()
  {
    using namespace maths;
    using graph_type = typename graph_type_generator<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>::graph_type;
    
    using edge_init_t = typename graph_type::edge_init_type;

    graph_type network{};

    check_equality(LINE("Check false positive: empty graph versus single node"), network, graph_type{{}});

    std::string message{"Check false positive: empty graph versus single node with loop"};
    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check_equality(LINE(message), network, graph_type{{edge_init_t{0}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check_equality(LINE(message), network, graph_type{{edge_init_t{0}, edge_init_t{0}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
    {
      check_equality(LINE(message), network, graph_type{{edge_init_t{0,0,1}, edge_init_t{0,0,0}}});
    }
    else
    {
      check_equality(LINE(message), network, graph_type{{edge_init_t{0,1}, edge_init_t{0,0}}});
    }
  }
}
