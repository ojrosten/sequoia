////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "DynamicUndirectedEmbeddedGraphFundamentalWeightTest.hpp"
#include "DynamicUndirectedEmbeddedGraphWeightedTestingUtilities.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path dynamic_undirected_embedded_graph_fundamental_weight_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void dynamic_undirected_embedded_graph_fundamental_weight_test::run_tests()
  {
    using namespace maths;
    dynamic_undirected_embedded_graph_weighted_operations<double, double, independent_bucketed_edge_storage_config, node_storage<double>>::execute_operations(*this);
  }
}