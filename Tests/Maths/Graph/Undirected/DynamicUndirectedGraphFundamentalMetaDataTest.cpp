////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "DynamicUndirectedGraphFundamentalMetaDataTest.hpp"
#include "DynamicUndirectedGraphMetaDataTestingUtilities.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path dynamic_undirected_graph_fundamental_meta_data_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void dynamic_undirected_graph_fundamental_meta_data_test::run_tests()
  {
    using namespace maths;
    dynamic_undirected_graph_meta_data_operations<null_weight, null_weight, float, independent_bucketed_edge_storage_traits, node_weight_storage_traits<null_weight>>::execute_operations(*this);
  }
}