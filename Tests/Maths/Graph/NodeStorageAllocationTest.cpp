////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "NodeStorageAllocationTest.hpp"
#include "NodeStorageAllocationTestingUtilities.hpp"

#include "sequoia/Maths/Graph/GraphDetails.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path node_storage_allocation_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void node_storage_allocation_test::run_tests()
  {
    do_allocation_tests(*this);
  }

  template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void node_storage_allocation_test::test_allocation()
  {
    test_allocation_impl<int, PropagateCopy, PropagateMove, PropagateSwap>();
    test_allocation_impl<int, PropagateCopy, PropagateMove, PropagateSwap>();
  }

  template<class Weight, bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void node_storage_allocation_test::test_allocation_impl()
  {
    using namespace maths::graph_impl;

    using storage = node_storage_tester<Weight, PropagateCopy, PropagateMove, PropagateSwap>;
    using allocator = typename storage::allocator_type;

    auto mutator{
      [](storage& s){ s.add_node(); }
    };

    auto[s,t]{check_semantics(report_line(""),
                    [](){ return storage(allocator{}); },
                    [](){ return storage{{1, 1, 0}, allocator{}}; },
                    mutator,
                    allocation_info{node_storage_alloc_getter<storage>{}, {0_c, {1_c,1_mu}, {1_anp,1_awp}}})};

    check(equivalence, report_line(""), s, std::initializer_list<int>{});
    check(equivalence, report_line(""), t, std::initializer_list<int>{1, 1, 0});
  }
}
