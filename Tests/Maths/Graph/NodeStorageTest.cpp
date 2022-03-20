////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "NodeStorageTest.hpp"
#include "sequoia/Core/Object/UniformWrapper.hpp"
#include "sequoia/Core/Object/DataPool.hpp"
#include "sequoia/Maths/Graph/GraphDetails.hpp"

#include <complex>
#include <list>

namespace sequoia:: testing
{
  [[nodiscard]]
  std::string_view node_storage_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void node_storage_test::run_tests()
  {
    test_dynamic_node_storage<object::spawner<double>>();
    test_dynamic_node_storage<object::data_pool<double>>();

    test_static_node_storage();
  }

  template<class Sharing>
  void node_storage_test::test_dynamic_node_storage()
  {
    using namespace maths::graph_impl;

    using storage = node_storage_tester<Sharing>;

    storage store{};
    check(equivalence, LINE(""), store, std::initializer_list<double>{});

    check(equality, LINE(""), store.capacity(), 0_sz);
    store.reserve(4_sz);
    check(equality, LINE(""), store.capacity(), 4_sz);
    store.shrink_to_fit();
    check(equality, LINE("Check may fail if stl implementation doesn't shrink to fit!"), store.capacity(), 0_sz);

    store.add_node(2.4);
    // 2.4

    check(equality, LINE(""), store, storage{2.4});
    check_semantics(LINE("Regular semantics"), store, storage{});

    store.node_weight(store.cbegin_node_weights(), 1.3);
    // 1.3

    check(equality, LINE(""), store, storage{1.3});

    store.erase_node(store.cbegin_node_weights());
    //

    check(equality, LINE(""), store, storage{});

    store.add_node(-0.4);
    store.add_node(5.6);
    // -0.4, 5.6

    check(equality, LINE(""), store, storage{-0.4, 5.6});
    check_semantics(LINE("Regular semantics"), store, storage{-0.4});

    store.erase_node(store.cbegin_node_weights());
    // 5.6

    check(equality, LINE(""), store, storage{5.6});

    store.add_node(6.4);
    store.add_node(-7.66);
    // 5.6, 6.4, -7.66

    check(equality, LINE(""), store, storage{5.6, 6.4, -7.66});

    store.erase_node(store.cbegin_node_weights()+1);
    // 5.6, -7.66

    check(equality, LINE(""), store, storage{5.6, -7.66});

    auto citer = store.insert_node(store.cbegin_node_weights()+1, 3.0);
    // 5.6, 3.0, -7.66

    check(equality, LINE(""), store, storage{5.6, 3.0, -7.66});
    check(equality, LINE(""), *citer, 3.0);

    store.insert_node(store.cend_node_weights(), 2.2);
    // 5.6, 3.0, -7.66 2.2

    check(equality, LINE(""), store, storage{5.6, 3.0, -7.66, 2.2});

    store.erase_nodes(store.cbegin_node_weights()+1, store.cbegin_node_weights()+3);
    // 5.6, 2.2

    check(equality, LINE(""), store, storage{5.6, 2.2});

    store.erase_nodes(store.cend_node_weights(), store.cend_node_weights());
    // 5.6, 2.2

    check(equality, LINE(""), store, storage{5.6, 2.2});

    check_exception_thrown<std::out_of_range>(LINE(""), [&store]() { store.erase_nodes(store.cend_node_weights(), store.cbegin_node_weights()); });
    check_exception_thrown<std::out_of_range>(LINE(""), [&store]() { store.erase_node(store.cend_node_weights()); });
  }

  void node_storage_test::test_static_node_storage()
  {
    using namespace maths::graph_impl;
    using storage = static_node_storage_tester<object::spawner<int>, 4>;

    constexpr storage store{4, 4, 7, 9};

    check(equivalence, LINE(""), store, std::initializer_list<int>{4, 4, 7, 9});
    check_semantics(LINE("Regular semantics"), store, {4, 4, 9, 7});
  }
}
