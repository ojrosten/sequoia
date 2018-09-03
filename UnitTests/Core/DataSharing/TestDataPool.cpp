#include "TestDataPool.hpp"

namespace sequoia
{
  namespace unit_testing
  {
    void test_data_pool::run_tests()
    {
      test_pooled();
      test_multi_pools();
      test_unpooled();      
    }

    data_sharing::data_pool<int> test_data_pool::make_int_pool(const int val)
    {
      using namespace data_sharing;
      data_pool<int> pool{};

      auto elt = pool.make(val);
      //                   int: val
      // proxy: elt  --->  handle -----> Pool 
      //                      ^-----------               

      check_equality<size_t>(1, pool.size());
      check_equality<int>(val, elt.get());

      return pool;
    }

    void test_data_pool::test_multi_pools()
    {
      using namespace data_sharing;
      data_pool<int> pool{make_int_pool(5)};           

      check_equality<size_t>(1, pool.size());

      auto elt = pool.make(5);

      check_equality<size_t>(1, pool.size());
      check_equality<int>(5, elt.get());

      auto elt2 = pool.make(6);
      check_equality<size_t>(2, pool.size());
      check_equality<int>(5, elt.get());
      check_equality<int>(6, elt2.get());

      
      data_pool<int> pool2{make_int_pool(4)};

      auto _2elt = pool2.make(4);

      check_equality<size_t>(1, pool2.size());
      check_equality<int>(4, _2elt.get());

      auto _2elt2 = pool2.make(7);

      check_equality<size_t>(2, pool2.size());
      check_equality<int>(4, _2elt.get());
      check_equality<int>(7, _2elt2.get());

      auto _2elt3 = pool2.make(-3);
      
      check_equality<size_t>(3, pool2.size());
      check_equality<int>(4, _2elt.get());
      check_equality<int>(7, _2elt2.get());
      check_equality<int>(-3, _2elt3.get());

      swap(pool, pool2);

      // Pools are swapped...
      check_equality<size_t>(3, pool.size());
      check_equality<size_t>(2, pool2.size());

      // But proxies still point to the same things!

      // These are now stored in pool 1
      check_equality<int>(4, _2elt.get());
      check_equality<int>(7, _2elt2.get());
      check_equality<int>(-3, _2elt3.get());

      // and these are now stored in pool 2
      check_equality<int>(5, elt.get());
      check_equality<int>(6, elt2.get());

      elt.set(-6);

      check_equality<size_t>(3, pool.size(), LINE("Will increase if pool pointers haven't ben swapped"));
      check_equality<size_t>(3, pool2.size(), LINE(""));

      auto elt3 = pool.make(1);
      
      check_equality<size_t>(4, pool.size(), LINE(""));
      check_equality<int>(1, elt3.get(), LINE(""));

      auto _2elt4 = pool2.make(10);

      check_equality<size_t>(4, pool2.size(), LINE(""));
      check_equality<size_t>(10, _2elt4.get(), LINE(""));
    }
    
    void test_data_pool::test_pooled()
    {
      using namespace data_sharing;
      data_pool<int> pool{};
      check_equality<size_t>(0, pool.size());

      auto elt = pool.make(3);
      // 3(1)

      check_equality<size_t>(1, pool.size(), LINE(""));
      check_equality<int>(3, elt.get(), LINE(""));

      elt.set(4);
      // 3(0), 4(1)
      check_equality<size_t>(2, pool.size(), LINE(""));
      check_equality(4, elt.get(), LINE(""));

      auto elt2 = pool.make(4);
      // 3(0), 4(2)
      check_equality<size_t>(2, pool.size(), LINE(""));
      check_equality(4, elt.get(), LINE(""));
      check_equality(4, elt2.get(), LINE(""));

      auto elt3 = pool.make(5);
      // 3(0), 4(2), 5(1)
      check_equality<size_t>(3, pool.size(), LINE(""));
      check_equality(4, elt.get(), LINE(""));
      check_equality(4, elt2.get(), LINE(""));
      check_equality(5, elt3.get(), LINE(""));

      elt2.set(5);
      // 3(0), 4(1), 5(2)
      check_equality<size_t>(3, pool.size(), LINE(""));
      check_equality(4, elt.get(), LINE(""));
      check_equality(5, elt2.get(), LINE(""));
      check_equality(5, elt3.get(), LINE(""));

      elt3.set(6);      
      // 3(0), 4(1), 5(1), 6(1)
      check_equality<size_t>(4, pool.size(), LINE(""));
      check_equality(4, elt.get(), LINE(""));
      check_equality(5, elt2.get(), LINE(""));
      check_equality(6, elt3.get(), LINE(""));

      elt.mutate([](auto& e) { e = 3;});      
      // 3(1), 4(0), 5(1), 6(1)
      check_equality<size_t>(4, pool.size(), LINE(""));
      check_equality(3, elt.get(), LINE(""));
      check_equality(5, elt2.get(), LINE(""));
      check_equality(6, elt3.get(), LINE(""));

      elt3.mutate([](auto& e){ e = 7;});
      // 3(1), 4(0), 5(1), 6(0), 7(1)
      check_equality<size_t>(5, pool.size(), LINE(""));
      check_equality(3, elt.get(), LINE(""));
      check_equality(5, elt2.get(), LINE(""));
      check_equality(7, elt3.get(), LINE(""));

      elt2.mutate([](auto& e) { e = 3; });
      // 3(2), 4(0), 5(0), 6(0), 7(1)
      check_equality<size_t>(5, pool.size(), LINE(""));
      check_equality(3, elt.get(), LINE(""));
      check_equality(3, elt2.get(), LINE(""));
      check_equality(7, elt3.get(), LINE(""));
    }

    void test_data_pool::test_unpooled()
    {
      using namespace data_sharing;
      constexpr auto x = unpooled<double>::make(3.0);
      check_equality<double>(3.0, x.get());
    }
  }
}
