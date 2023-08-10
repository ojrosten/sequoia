////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "DynamicUndirectedGraphSharedUnsortableWeightTest.hpp"
#include <complex>

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path dynamic_undirected_graph_shared_unsortable_weight_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void dynamic_undirected_graph_shared_unsortable_weight_test::run_tests()
  {
    using namespace maths;
    using comp_t = std::complex<double>;
    dynamic_undirected_graph_operations<comp_t, comp_t, shared_weight_bucketed_edge_storage_traits, node_weight_storage_traits<comp_t>>::execute_operations(*this);
  }
}