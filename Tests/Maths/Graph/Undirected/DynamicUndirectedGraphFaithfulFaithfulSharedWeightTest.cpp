////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "DynamicUndirectedGraphFaithfulFaithfulSharedWeightTest.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path dynamic_undirected_graph_faithful_faithful_shared_weight_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void dynamic_undirected_graph_faithful_faithful_shared_weight_test::run_tests()
  {
    using namespace maths;
    using namespace object;

    dynamic_undirected_graph_weighted_operations<double, double, faithful_producer<double>, faithful_producer<double>, shared_weight_contiguous_edge_storage_traits, node_weight_storage_traits<double>>::execute_operations(*this);
    dynamic_undirected_graph_weighted_operations<double, double, faithful_producer<double>, faithful_producer<double>, shared_weight_bucketed_edge_storage_traits, node_weight_storage_traits<double>>::execute_operations(*this);
  }
}