////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "DynamicUndirectedGraphUnsortableWeightTest.hpp"
#include "DynamicUndirectedGraphUnsortableWeightTestingUtilities.hpp"

#include <complex>

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path dynamic_undirected_graph_unsortable_weight_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void dynamic_undirected_graph_unsortable_weight_test::run_tests()
  {
    using namespace maths;
    using comp_t = std::complex<double>;
    dynamic_undirected_graph_unsortable_weight_operations<comp_t, comp_t, independent_bucketed_edge_storage_config, node_storage<comp_t>>::execute_operations(*this);
  }
}