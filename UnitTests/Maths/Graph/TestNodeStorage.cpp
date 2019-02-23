#include "TestNodeStorage.hpp"
#include "ProtectiveWrapper.hpp"
#include "DataPool.hpp"
#include "GraphDetails.hpp"

#include <complex>
#include <list>

namespace sequoia
{
  namespace unit_testing
  {
    void test_node_storage::run_tests()
    {
      test_dynamic_node_storage();
      test_static_node_storage();
    }

    void test_node_storage::test_dynamic_node_storage()
    {
      using namespace maths::graph_impl;

      using storage = node_storage_tester<weight_maker<data_sharing::unpooled<double>>>;

      storage store{};
      check_storage(store, std::vector<double>{}, LINE(""));

      check_equality<std::size_t>(0, store.capacity(), LINE(""));
      store.reserve(4ul);
      check_equality<std::size_t>(4, store.capacity(), LINE(""));
      store.shrink_to_fit();
      check_equality<std::size_t>(0, store.capacity(), LINE("Check may fail if stl implementation doesn't shrink to fit!"));
      
      

      store.add_node(2.4);
      // 2.4

      check_storage(store, std::vector<double>{2.4}, LINE(""));

      store.node_weight(store.cbegin_node_weights(), 1.3);
      // 1.3

      check_storage(store, std::vector<double>{1.3}, LINE(""));

      store.erase_node(store.cbegin_node_weights());
      //

      check_storage(store, std::vector<double>{}, LINE(""));

      store.add_node(-0.4);
      store.add_node(5.6);
      // -0.4, 5.6

      check_storage(store, std::vector<double>{-0.4, 5.6}, LINE(""));

      store.erase_node(store.cbegin_node_weights());
      // 5.6

      check_storage(store, std::vector<double>{5.6}, LINE(""));

      store.add_node(6.4);
      store.add_node(-7.66);
      // 5.6, 6.4, -7.66

      check_storage(store, std::vector<double>{5.6, 6.4, -7.66}, LINE(""));

      store.erase_node(store.cbegin_node_weights()+1);
      // 5.6, -7.66

      check_storage(store, std::vector<double>{5.6, -7.66}, LINE(""));

      auto citer = store.insert_node(store.cbegin_node_weights()+1, 3.0);
      // 5.6, 3.0, -7.66

      check_storage(store, std::vector<double>{5.6, 3.0, -7.66}, LINE(""));
      check_equality(3.0, *citer);

      store.insert_node(store.cend_node_weights(), 2.2);
      // 5.6, 3.0, -7.66 2.2

      check_storage(store, std::vector<double>{5.6, 3.0, -7.66, 2.2}, LINE(""));

      store.erase_nodes(store.cbegin_node_weights()+1, store.cbegin_node_weights()+3);
      // 5.6, 2.2

      check_storage(store, std::vector<double>{5.6, 2.2}, LINE(""));

      store.erase_nodes(store.cend_node_weights(), store.cend_node_weights());
      // 5.6, 2.2

      check_storage(store, std::vector<double>{5.6, 2.2}, LINE(""));

      check_exception_thrown<std::out_of_range>([&store]() { store.erase_nodes(store.cend_node_weights(), store.cbegin_node_weights()); });
      check_exception_thrown<std::out_of_range>([&store]() { store.erase_node(store.cend_node_weights()); });

      
      storage store2{1.1, -9.8, 4.3};
      check_storage(store2, std::vector<double>{1.1, -9.8, 4.3}, LINE(""));

      check(store != store2);
      store2 = store;

      check_storage(store, std::vector<double>{5.6, 2.2}, LINE(""));
      check_storage(store2, std::vector<double>{5.6, 2.2}, LINE(""));
      check(store == store2);
    }

    void test_node_storage::test_static_node_storage()
    {
      using namespace maths::graph_impl;

      constexpr static_node_storage_tester<weight_maker<data_sharing::unpooled<int>>, 4>
        store{4, 4, 7, 9};

      check_storage(store, std::vector<int>{4, 4, 7, 9}, LINE(""));
    }
  }
}
