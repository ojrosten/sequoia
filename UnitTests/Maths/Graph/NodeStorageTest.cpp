////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "NodeStorageTest.hpp"
#include "ProtectiveWrapper.hpp"
#include "DataPool.hpp"
#include "GraphDetails.hpp"

#include <complex>
#include <list>

namespace sequoia:: unit_testing
{
  void test_node_storage::run_tests()
  {
    test_dynamic_node_storage<data_sharing::unpooled<double>>();
    test_dynamic_node_storage<data_sharing::data_pool<double>>();
    
    test_static_node_storage();

    allocation_tester t{*this};
  }

  template<class Sharing>
  void test_node_storage::test_dynamic_node_storage()
  {
    using namespace maths::graph_impl;

    using storage = node_storage_tester<weight_maker<Sharing>>;

    storage store{};
    check_equivalence(LINE(""), store, std::initializer_list<double>{});
      
    check_equality(LINE(""), store.capacity(), 0ul);
    store.reserve(4ul);
    check_equality(LINE(""), store.capacity(), 4ul);
    store.shrink_to_fit();
    check_equality(LINE("Check may fail if stl implementation doesn't shrink to fit!"), store.capacity(), 0ul);
      
    store.add_node(2.4);
    // 2.4

    check_equality(LINE(""), store, storage{2.4});
    check_regular_semantics(LINE("Regular semantics"), store, storage{});

    store.node_weight(store.cbegin_node_weights(), 1.3);
    // 1.3

    check_equality(LINE(""), store, storage{1.3});

    store.erase_node(store.cbegin_node_weights());
    //

    check_equality(LINE(""), store, storage{});

    store.add_node(-0.4);
    store.add_node(5.6);
    // -0.4, 5.6

    check_equality(LINE(""), store, storage{-0.4, 5.6});
    check_regular_semantics(LINE("Regular semantics"), store, storage{-0.4});

    store.erase_node(store.cbegin_node_weights());
    // 5.6

    check_equality(LINE(""), store, storage{5.6});

    store.add_node(6.4);
    store.add_node(-7.66);
    // 5.6, 6.4, -7.66

    check_equality(LINE(""), store, storage{5.6, 6.4, -7.66});

    store.erase_node(store.cbegin_node_weights()+1);
    // 5.6, -7.66

    check_equality(LINE(""), store, storage{5.6, -7.66});

    auto citer = store.insert_node(store.cbegin_node_weights()+1, 3.0);
    // 5.6, 3.0, -7.66

    check_equality(LINE(""), store, storage{5.6, 3.0, -7.66});
    check_equality(LINE(""), *citer, 3.0);

    store.insert_node(store.cend_node_weights(), 2.2);
    // 5.6, 3.0, -7.66 2.2

    check_equality(LINE(""), store, storage{5.6, 3.0, -7.66, 2.2});

    store.erase_nodes(store.cbegin_node_weights()+1, store.cbegin_node_weights()+3);
    // 5.6, 2.2

    check_equality(LINE(""), store, storage{5.6, 2.2});

    store.erase_nodes(store.cend_node_weights(), store.cend_node_weights());
    // 5.6, 2.2

    check_equality(LINE(""), store, storage{5.6, 2.2});

    check_exception_thrown<std::out_of_range>(LINE(""), [&store]() { store.erase_nodes(store.cend_node_weights(), store.cbegin_node_weights()); });
    check_exception_thrown<std::out_of_range>(LINE(""), [&store]() { store.erase_node(store.cend_node_weights()); });
  }

  void test_node_storage::test_static_node_storage()
  {
    using namespace maths::graph_impl;
    using storage = static_node_storage_tester<weight_maker<data_sharing::unpooled<int>>, 4>;

    constexpr storage store{4, 4, 7, 9};

    check_equivalence(LINE(""), store, std::initializer_list<int>{4, 4, 7, 9});
    check_regular_semantics(LINE("Regular semantics"), store, {4, 4, 9, 7});
  }

  template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void test_node_storage::test_allocation()
  {   
    test_allocation_impl<data_sharing::unpooled<int>, PropagateCopy, PropagateMove, PropagateSwap>();
    test_allocation_impl<data_sharing::data_pool<int>, PropagateCopy, PropagateMove, PropagateSwap>();
  }

  template<class Sharing, bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void test_node_storage::test_allocation_impl()
  {
    if constexpr(!(std::is_same_v<Sharing, data_sharing::data_pool<int>> && PropagateCopy &&!PropagateMove && !PropagateSwap))
    {
    
      using namespace maths::graph_impl;
    
      using storage = node_storage_tester<weight_maker<Sharing>, PropagateCopy, PropagateMove, PropagateSwap>;
      using allocator = typename storage::allocator_type;

      int
        sAllocCount{}, sDeallocCount{},
        tAllocCount{}, tDeallocCount{};    

      allocator sAlloc{sAllocCount, sDeallocCount};
      
      storage s(sAlloc);
      check_equivalence(LINE(""), s, std::initializer_list<int>{});
      check_equality(LINE(""), sAllocCount, 0);

      allocator tAlloc{tAllocCount, tDeallocCount};
      storage t{{1, 1, 0}, tAlloc};
      check_equivalence(LINE(""), t, std::initializer_list<int>{1, 1, 0});
      check_equality(LINE(""), tAllocCount, 1);

      auto mutator{
        [](storage& s){
          s.add_node();
        }
      };

      check_allocations(LINE(""), s, t, mutator, allocation_info<allocator>{sAlloc, tAlloc, {0, 1, 1, 1}});
    }
  }
}
