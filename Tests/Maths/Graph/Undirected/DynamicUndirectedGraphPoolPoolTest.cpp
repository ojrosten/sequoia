////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "DynamicUndirectedGraphPoolPoolTest.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path dynamic_undirected_graph_pool_pool_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void dynamic_undirected_graph_pool_pool_test::run_tests()
  {
    using namespace maths;
    using namespace object;

    dynamic_undirected_graph_weighted_operations<double, double, data_pool<double>, data_pool<double>, independent_contiguous_edge_storage_traits, node_weight_storage_traits<double>>::execute_operations(*this);
    dynamic_undirected_graph_weighted_operations<double, double, data_pool<double>, data_pool<double>, independent_bucketed_edge_storage_traits, node_weight_storage_traits<double>>::execute_operations(*this);
  }
}