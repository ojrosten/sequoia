////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "DynamicGraphUnweightedAllocationTest.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view unweighted_graph_allocation_test::source_file_name() const noexcept
  {
    return __FILE__;
  }

  void unweighted_graph_allocation_test::run_tests()
  {
    struct null_weight {};

    graph_test_helper<null_weight, null_weight> helper{concurrent_execution()};

    {
      using edge_traits = custom_allocator_contiguous_edge_storage_traits;
      helper.run_storage_tests<edge_traits, node_traits, graph_contiguous_memory>(*this);
    }

    {
      using edge_traits = custom_allocator_bucketed_edge_storage_traits;
      helper.run_storage_tests<edge_traits, node_traits, graph_bucketed_memory>(*this);
    }      
  }
}
