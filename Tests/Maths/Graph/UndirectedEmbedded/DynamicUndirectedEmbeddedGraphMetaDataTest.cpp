////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "DynamicUndirectedEmbeddedGraphMetaDataTest.hpp"
#include "DynamicUndirectedEmbeddedGraphMetaDataTestingUtilities.hpp"

#include <complex>

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path dynamic_undirected_embedded_graph_meta_data_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void dynamic_undirected_embedded_graph_meta_data_test::run_tests()
  {
    using namespace maths;
    dynamic_undirected_embedded_graph_meta_data_operations<null_weight, null_weight, float, independent_bucketed_edge_storage_config, node_weight_storage_config<null_weight>>::execute_operations(*this);
    dynamic_undirected_embedded_graph_meta_data_operations<null_weight, null_weight, std::complex<float>, independent_bucketed_edge_storage_config, node_weight_storage_config<null_weight>>::execute_operations(*this);
  }
}