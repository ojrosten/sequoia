////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "NodeStorageAllocationTest.hpp"
#include "DataPool.hpp"
#include "GraphDetails.hpp"

namespace sequoia:: unit_testing
{
  [[nodiscard]]
  std::string_view node_storage_allocation_test::source_file_name() const noexcept
  {
    return __FILE__;
  }

  void node_storage_allocation_test::run_tests()
  {
    do_allocation_tests(*this);
  }

  
  template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void node_storage_allocation_test::test_allocation()
  {   
    test_allocation_impl<data_sharing::unpooled<int>, PropagateCopy, PropagateMove, PropagateSwap>();
    test_allocation_impl<data_sharing::data_pool<int>, PropagateCopy, PropagateMove, PropagateSwap>();
  }

  template<class Sharing, bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void node_storage_allocation_test::test_allocation_impl()
  {
    
    using namespace maths::graph_impl;
    
    using storage = node_storage_tester<weight_maker<Sharing>, PropagateCopy, PropagateMove, PropagateSwap>;
    using allocator = typename storage::allocator_type; 

    storage s(allocator{});
    check_equivalence(LINE(""), s, std::initializer_list<int>{});
    check_equality(LINE(""), s.get_node_allocator().allocs(), 0);

    storage t{{1, 1, 0}, allocator{}};
    check_equivalence(LINE(""), t, std::initializer_list<int>{1, 1, 0});
    check_equality(LINE(""), t.get_node_allocator().allocs(), 1);

    auto getter{
      [](const storage& s) {
        return s.get_node_allocator();
      }
    };
    
    auto mutator{
      [](storage& s){
        s.add_node();
      }
    };

    check_semantics(LINE(""), s, t, mutator, allocation_info<storage, allocator>{getter, {0_c, {1_c,1_mu}, {1_awp,1_anp}}});
  }
}
