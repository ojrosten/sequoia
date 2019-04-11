////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "DataPoolTest.hpp"

namespace sequoia
{
  namespace unit_testing
  {
    void data_pool_test::run_tests()
    {
      test_pooled();
      test_multi_pools();
      test_unpooled();      
    }

    data_sharing::data_pool<int> data_pool_test::make_int_pool(const int val)
    {
      using namespace data_sharing;
      data_pool<int> pool{};

      auto elt = pool.make(val);
      //                   int: val
      // proxy: elt  --->  handle -----> Pool 
      //                      ^-----------               

      check_equality(pool.size(), 1ul, LINE(""));
      check_equality(elt.get(), val);

      return pool;
    }

    void data_pool_test::test_multi_pools()
    {
      using namespace data_sharing;
      data_pool<int> pool{make_int_pool(5)};           

      check_equality(pool.size(), 1ul, LINE(""));

      auto elt = pool.make(5);

      check_equality(pool.size(), 1ul, LINE(""));
      check_equality(elt.get(), 5, LINE(""));

      auto elt2 = pool.make(6);
      check_equality(pool.size(), 2ul, LINE(""));
      check_equality(elt.get(), 5, LINE(""));
      check_equality(elt2.get(), 6, LINE(""));

      
      data_pool<int> pool2{make_int_pool(4)};

      auto _2elt = pool2.make(4);

      check_equality(pool2.size(), 1ul, LINE(""));
      check_equality(_2elt.get(), 4, LINE(""));

      auto _2elt2 = pool2.make(7);

      check_equality(pool2.size(), 2ul, LINE(""));
      check_equality(_2elt.get(), 4, LINE(""));
      check_equality(_2elt2.get(), 7, LINE(""));

      auto _2elt3 = pool2.make(-3);
      
      check_equality(pool2.size(), 3ul, LINE(""));
      check_equality(_2elt.get(), 4, LINE(""));
      check_equality(_2elt2.get(), 7, LINE(""));
      check_equality(_2elt3.get(), -3, LINE(""));

      swap(pool, pool2);

      // Pools are swapped...
      check_equality(pool.size(), 3ul, LINE(""));
      check_equality(pool2.size(), 2ul, LINE(""));

      // But proxies still point to the same things!

      // These are now stored in pool 1
      check_equality(_2elt.get(), 4, LINE(""));
      check_equality(_2elt2.get(), 7, LINE(""));
      check_equality(_2elt3.get(), -3, LINE(""));

      // and these are now stored in pool 2
      check_equality(elt.get(), 5, LINE(""));
      check_equality(elt2.get(), 6, LINE(""));

      elt.set(-6);

      check_equality(pool.size(), 3ul, LINE("Will increase if pool pointers haven't ben swapped"));
      check_equality(pool2.size(), 3ul, LINE(""));

      auto elt3 = pool.make(1);
      
      check_equality(pool.size(), 4ul, LINE(""));
      check_equality(elt3.get(), 1, LINE(""));

      auto _2elt4 = pool2.make(10);

      check_equality(pool2.size(), 4ul, LINE(""));
      check_equality(_2elt4.get(), 10, LINE(""));
    }
    
    void data_pool_test::test_pooled()
    {
      using namespace data_sharing;
      data_pool<int> pool{};
      check_equality(pool.size(), 0ul, LINE(""));

      auto elt = pool.make(3);
      // 3(1)

      check_equality(pool.size(), 1ul, LINE(""));
      check_equality(elt.get(), 3, LINE(""));

      elt.set(4);
      // 3(0), 4(1)
      check_equality(pool.size(), 2ul, LINE(""));
      check_equality(elt.get(), 4, LINE(""));

      auto elt2 = pool.make(4);
      // 3(0), 4(2)
      check_equality(pool.size(), 2ul, LINE(""));
      check_equality(elt.get(), 4, LINE(""));
      check_equality(elt2.get(), 4, LINE(""));

      auto elt3 = pool.make(5);
      // 3(0), 4(2), 5(1)
      check_equality(pool.size(), 3ul, LINE(""));
      check_equality(elt.get(), 4, LINE(""));
      check_equality(elt2.get(), 4, LINE(""));
      check_equality(elt3.get(), 5, LINE(""));

      elt2.set(5);
      // 3(0), 4(1), 5(2)
      check_equality(pool.size(), 3ul, LINE(""));
      check_equality(elt.get(), 4, LINE(""));
      check_equality(elt2.get(), 5, LINE(""));
      check_equality(elt3.get(), 5, LINE(""));

      elt3.set(6);      
      // 3(0), 4(1), 5(1), 6(1)
      check_equality(pool.size(), 4ul, LINE(""));
      check_equality(elt.get(), 4, LINE(""));
      check_equality(elt2.get(), 5, LINE(""));
      check_equality(elt3.get(), 6, LINE(""));

      elt.mutate([](auto& e) { e = 3;});      
      // 3(1), 4(0), 5(1), 6(1)
      check_equality(pool.size(), 4ul, LINE(""));
      check_equality(elt.get(), 3, LINE(""));
      check_equality(elt2.get(), 5, LINE(""));
      check_equality(elt3.get(), 6, LINE(""));

      elt3.mutate([](auto& e){ e = 7;});
      // 3(1), 4(0), 5(1), 6(0), 7(1)
      check_equality(pool.size(), 5ul, LINE(""));
      check_equality(elt.get(), 3, LINE(""));
      check_equality(elt2.get(), 5, LINE(""));
      check_equality(elt3.get(), 7, LINE(""));

      elt2.mutate([](auto& e) { e = 3; });
      // 3(2), 4(0), 5(0), 6(0), 7(1)
      check_equality(pool.size(), 5ul, LINE(""));
      check_equality(elt.get(), 3, LINE(""));
      check_equality(elt2.get(), 3, LINE(""));
      check_equality(elt3.get(), 7, LINE(""));
    }

    void data_pool_test::test_unpooled()
    {
      using namespace data_sharing;
      constexpr auto x = unpooled<double>::make(3.0);
      check_equality(x.get(), 3.0);
    }
  }
}
