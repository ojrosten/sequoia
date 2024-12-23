////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "DynamicGraphTestingDiagnostics.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path test_graph_false_negatives::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void test_graph_false_negatives::run_tests()
  {
    using namespace maths;

    graph_test_helper<null_weight, null_weight, test_graph_false_negatives> helper{*this};
    helper.run_tests();
  }

  template
  <
    maths::graph_flavour GraphFlavour,
    class EdgeWeight,
    class NodeWeight,
    class EdgeStorageConfig,
    class NodeWeightStorage
  >
  void test_graph_false_negatives::execute_operations()
  {
    using namespace maths;
    using graph_type = typename graph_type_generator<GraphFlavour, EdgeWeight, NodeWeight, EdgeStorageConfig, NodeWeightStorage>::graph_type;

    using edge_init_t = typename graph_type::edge_init_type;

    graph_type g{};

    check(equality, "Check false positive: empty graph versus single node", g, graph_type{{}});

    std::string message{"Check false positive: empty graph versus single node with loop"};
    if constexpr (GraphFlavour == graph_flavour::directed)
    {
      check(equality, message, g, graph_type{{edge_init_t{0}}});
    }
    else if constexpr(GraphFlavour == graph_flavour::undirected)
    {
      check(equality, message, g, graph_type{{edge_init_t{0}, edge_init_t{0}}});
    }
    else
    {
      check(equality, message, g, graph_type{{edge_init_t{0,1}, edge_init_t{0,0}}});
    }
  }
}
