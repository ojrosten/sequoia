////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "DynamicDirectedEmbeddedGraphFundamentalWeightSharedEdgeTest.hpp"

namespace sequoia::testing
{

  [[nodiscard]]
  std::filesystem::path dynamic_directed_embedded_graph_fundamental_weight_shared_edge_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void dynamic_directed_embedded_graph_fundamental_weight_shared_edge_test::run_tests()
  {
    using namespace maths;
    using namespace object;

    dynamic_directed_embedded_graph_weighted_operations<double, double, faithful_producer<double>, faithful_producer<double>, shared_edge_contiguous_edge_storage_traits, node_weight_storage_traits<double>>::execute_operations(*this);
    dynamic_directed_embedded_graph_weighted_operations<double, double, faithful_producer<double>, faithful_producer<double>, shared_edge_bucketed_edge_storage_traits, node_weight_storage_traits<double>>::execute_operations(*this);
  }
}