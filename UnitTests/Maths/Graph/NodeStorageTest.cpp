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
    
    test_allocator<data_sharing::unpooled<int>>();
    test_allocator<data_sharing::data_pool<int>>();
  }

  template<class Sharing>
  void test_node_storage::test_dynamic_node_storage()
  {
    using namespace maths::graph_impl;

    using storage = node_storage_tester<weight_maker<Sharing>>;

    storage store{};
    check_equivalence(store, std::initializer_list<double>{}, LINE(""));
      
    check_equality(store.capacity(), 0ul, LINE(""));
    store.reserve(4ul);
    check_equality(store.capacity(), 4ul, LINE(""));
    store.shrink_to_fit();
    check_equality(store.capacity(), 0ul, LINE("Check may fail if stl implementation doesn't shrink to fit!"));
      
    store.add_node(2.4);
    // 2.4

    check_equality(store, storage{2.4}, LINE(""));
    check_regular_semantics(store, storage{}, LINE("Regular semantics"));

    store.node_weight(store.cbegin_node_weights(), 1.3);
    // 1.3

    check_equality(store, storage{1.3}, LINE(""));

    store.erase_node(store.cbegin_node_weights());
    //

    check_equality(store, storage{}, LINE(""));

    store.add_node(-0.4);
    store.add_node(5.6);
    // -0.4, 5.6

    check_equality(store, storage{-0.4, 5.6}, LINE(""));
    check_regular_semantics(store, storage{-0.4}, LINE("Regular semantics"));

    store.erase_node(store.cbegin_node_weights());
    // 5.6

    check_equality(store, storage{5.6}, LINE(""));

    store.add_node(6.4);
    store.add_node(-7.66);
    // 5.6, 6.4, -7.66

    check_equality(store, storage{5.6, 6.4, -7.66}, LINE(""));

    store.erase_node(store.cbegin_node_weights()+1);
    // 5.6, -7.66

    check_equality(store, storage{5.6, -7.66}, LINE(""));

    auto citer = store.insert_node(store.cbegin_node_weights()+1, 3.0);
    // 5.6, 3.0, -7.66

    check_equality(store, storage{5.6, 3.0, -7.66}, LINE(""));
    check_equality(3.0, *citer, LINE(""));

    store.insert_node(store.cend_node_weights(), 2.2);
    // 5.6, 3.0, -7.66 2.2

    check_equality(store, storage{5.6, 3.0, -7.66, 2.2}, LINE(""));

    store.erase_nodes(store.cbegin_node_weights()+1, store.cbegin_node_weights()+3);
    // 5.6, 2.2

    check_equality(store, storage{5.6, 2.2}, LINE(""));

    store.erase_nodes(store.cend_node_weights(), store.cend_node_weights());
    // 5.6, 2.2

    check_equality(store, storage{5.6, 2.2}, LINE(""));

    check_exception_thrown<std::out_of_range>([&store]() { store.erase_nodes(store.cend_node_weights(), store.cbegin_node_weights()); }, LINE(""));
    check_exception_thrown<std::out_of_range>([&store]() { store.erase_node(store.cend_node_weights()); }, LINE(""));
  }

  void test_node_storage::test_static_node_storage()
  {
    using namespace maths::graph_impl;
    using storage = static_node_storage_tester<weight_maker<data_sharing::unpooled<int>>, 4>;

    constexpr storage store{4, 4, 7, 9};

    check_equivalence(store, std::initializer_list<int>{4, 4, 7, 9}, LINE(""));
    check_regular_semantics(store, {4, 4, 9, 7}, LINE("Regular semantics"));
  }

  template<class Sharing>
  void test_node_storage::test_allocator()
  {
    using namespace maths::graph_impl;
    
    using storage_t = node_storage_tester<weight_maker<Sharing>>;
    using allocator_t = typename storage_t::allocator_type;

    storage_t s{allocator_t{}}, t{{1, 1, 0}, allocator_t{}};

    check_regular_semantics(s, t, allocator_t{}, LINE("Regular Semantics"));
  }
}
