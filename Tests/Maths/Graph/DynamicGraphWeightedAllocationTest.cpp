////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "DynamicGraphWeightedAllocationTest.hpp"

#include <complex>

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view weighted_graph_allocation_test::source_file_name() const noexcept
  {
    return __FILE__;
  }

  void weighted_graph_allocation_test::run_tests()
  {
    using std::complex;

    using contig_edge_traits = custom_allocator_contiguous_edge_storage_traits;
    using bucket_edge_traits = custom_allocator_bucketed_edge_storage_traits;

    {
      graph_test_helper<int, complex<double>>  helper{concurrent_execution()};
      
      helper.run_storage_tests<contig_edge_traits, node_traits, graph_contiguous_memory>(*this);
      helper.run_storage_tests<bucket_edge_traits, node_traits, graph_bucketed_memory>(*this);
    }

    {
      graph_test_helper<complex<int>, complex<double>>  helper{concurrent_execution()};

      helper.run_storage_tests<contig_edge_traits, node_traits, graph_contiguous_memory>(*this);
      helper.run_storage_tests<bucket_edge_traits, node_traits, graph_bucketed_memory>(*this);
    }

    {
      graph_test_helper<std::vector<int>, std::vector<complex<double>>>  helper{concurrent_execution()};
      
      helper.run_storage_tests<contig_edge_traits, node_traits, graph_contiguous_memory>(*this);
      helper.run_storage_tests<bucket_edge_traits, node_traits, graph_bucketed_memory>(*this);
    }
  }
}
