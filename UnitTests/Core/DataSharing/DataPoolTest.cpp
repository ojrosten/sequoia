////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "DataPoolTest.hpp"
#include "DataPoolTestingUtilities.hpp"

namespace sequoia::unit_testing
{
  void data_pool_test::run_tests()
  {
    test_pooled();
    test_multi_pools();
    test_unpooled();

    do_move_only_allocation_tests(*this);
  }

  data_sharing::data_pool<int> data_pool_test::make_int_pool(const int val)
  {
    using namespace data_sharing;
    using pool_t = data_pool<int>;
    
    using prediction_t = typename weak_equivalence_checker<pool_t>::prediction_type;
    
    pool_t pool{};

    auto elt = pool.make(val);
    //                   int: val
    // proxy: elt  --->  handle -----> Pool 
    //                      ^-----------               

    check_weak_equivalence(LINE(""), pool, prediction_t{{val, 1}});
    check_equality(LINE(""), elt.get(), val);

    return pool;
  }

  void data_pool_test::test_multi_pools()
  {
    using namespace data_sharing;
    using pool_t = data_pool<int>;
    
    using prediction_t = typename weak_equivalence_checker<pool_t>::prediction_type;
    
    pool_t pool{make_int_pool(5)};
    check_weak_equivalence(LINE("Proxy created in function call goes out of scope, decreasing count to 0"), pool, prediction_t{{5, 0}});

    auto elt = pool.make(5);

    check_weak_equivalence(LINE(""), pool, prediction_t{{5, 1}});
    check_equality(LINE(""), elt.get(), 5);

    auto elt2 = pool.make(6);
    check_weak_equivalence(LINE(""), pool, prediction_t{{5, 1}, {6, 1}});
    check_equality(LINE(""), elt.get(), 5);
    check_equality(LINE(""), elt2.get(), 6);

    data_pool<int> pool2{make_int_pool(4)};
    check_weak_equivalence(LINE("Proxy created in function call goes out of scope, decreasing count to 0"), pool2, prediction_t{{4, 0}});

    auto _2elt = pool2.make(4);

    check_weak_equivalence(LINE(""), pool2, prediction_t{{4, 1}});
    check_equality(LINE(""), _2elt.get(), 4);

    auto _2elt2 = pool2.make(7);

    check_weak_equivalence(LINE(""), pool2, prediction_t{{4, 1}, {7, 1}});
    check_equality(LINE(""), _2elt.get(), 4);
    check_equality(LINE(""), _2elt2.get(), 7);

    auto _2elt3 = pool2.make(-3);
      
    check_weak_equivalence(LINE(""), pool2, prediction_t{{4, 1}, {7, 1}, {-3, 1}});
    check_equality(LINE(""), _2elt.get(), 4);
    check_equality(LINE(""), _2elt2.get(), 7);
    check_equality(LINE(""), _2elt3.get(), -3);

    swap(pool, pool2);

    // Pools are swapped...
    check_weak_equivalence(LINE(""), pool, prediction_t{{4, 1}, {7, 1}, {-3, 1}});
    check_weak_equivalence(LINE(""), pool2, prediction_t{{5, 1}, {6, 1}});

    // But proxies still point to the same things!

    // These are now stored in pool 1
    check_equality(LINE(""), _2elt.get(), 4);
    check_equality(LINE(""), _2elt2.get(), 7);
    check_equality(LINE(""), _2elt3.get(), -3);

    // and these are now stored in pool 2
    check_equality(LINE(""), elt.get(), 5);
    check_equality(LINE(""), elt2.get(), 6);

    elt.set(-6);

    check_weak_equivalence(LINE("Will change if pool pointers haven't been swapped"), pool, prediction_t{{4, 1}, {7, 1}, {-3, 1}});
    check_weak_equivalence(LINE(""), pool2, prediction_t{{5, 0}, {6, 1}, {-6, 1}});

    auto elt3 = pool.make(1);
      
    check_weak_equivalence(LINE("Will change if pool pointers haven't been swapped"), pool, prediction_t{{4, 1}, {7, 1}, {-3, 1}, {1, 1}});
    check_equality(LINE(""), elt3.get(), 1);

    auto _2elt4 = pool2.make(10);

    check_weak_equivalence(LINE(""), pool2, prediction_t{{5, 0}, {6, 1}, {-6, 1}, {10, 1}});
    check_equality(LINE(""), _2elt4.get(), 10);
  }
    
  void data_pool_test::test_pooled()
  {
    using namespace data_sharing;
    using pool_t = data_pool<int>;
    
    using prediction_t = typename weak_equivalence_checker<pool_t>::prediction_type;
    pool_t pool{};
    check_weak_equivalence(LINE(""), pool, prediction_t{});

    check_equality(LINE(""), pool.size(), 0ul);

    auto elt = pool.make(3);
    // 3(1)
    check_weak_equivalence(LINE(""), pool, prediction_t{{3, 1}});
    check_equality(LINE(""), elt.get(), 3);

    {
      pool_t clonePool{};
      auto e{clonePool.make(3)};
      check_weak_equivalence(LINE(""), clonePool, prediction_t{{3, 1}});
      check_regular_semantics(LINE(""), pool_t{}, std::move(clonePool), pool_t{}, pool);
    }

    elt.set(4);
    // 3(0), 4(1)
    check_weak_equivalence(LINE(""), pool, prediction_t{{3, 0}, {4, 1}});
    check_equality(LINE(""), elt.get(), 4);
    

    auto elt2 = pool.make(4);
    // 3(0), 4(2)
    check_weak_equivalence(LINE(""), pool, prediction_t{{3, 0}, {4, 2}});
    check_equality(LINE(""), elt.get(), 4);
    check_equality(LINE(""), elt2.get(), 4);

    auto elt3 = pool.make(5);
    // 3(0), 4(2), 5(1)
    check_weak_equivalence(LINE(""), pool, prediction_t{{3, 0}, {4, 2}, {5, 1}});
    check_equality(LINE(""), elt.get(), 4);
    check_equality(LINE(""), elt2.get(), 4);
    check_equality(LINE(""), elt3.get(), 5);

    elt2.set(5);
    // 3(0), 4(1), 5(2)
    check_weak_equivalence(LINE(""), pool, prediction_t{{3, 0}, {4, 1}, {5, 2}});
    check_equality(LINE(""), elt.get(), 4);
    check_equality(LINE(""), elt2.get(), 5);
    check_equality(LINE(""), elt3.get(), 5);

    elt3.set(6);      
    // 3(0), 4(1), 5(1), 6(1)
    check_weak_equivalence(LINE(""), pool, prediction_t{{3, 0}, {4, 1}, {5, 1}, {6, 1}});
    check_equality(LINE(""), elt.get(), 4);
    check_equality(LINE(""), elt2.get(), 5);
    check_equality(LINE(""), elt3.get(), 6);

    elt.mutate([](auto& e) { e = 3;});      
    // 3(1), 4(0), 5(1), 6(1)
    check_weak_equivalence(LINE(""), pool, prediction_t{{3, 1}, {4, 0}, {5, 1}, {6, 1}});
    check_equality(LINE(""), elt.get(), 3);
    check_equality(LINE(""), elt2.get(), 5);
    check_equality(LINE(""), elt3.get(), 6);

    elt3.mutate([](auto& e){ e = 7;});
    // 3(1), 4(0), 5(1), 6(0), 7(1)
    check_weak_equivalence(LINE(""), pool, prediction_t{{3, 1}, {4, 0}, {5, 1}, {6, 0}, {7, 1}});
    check_equality(LINE(""), elt.get(), 3);
    check_equality(LINE(""), elt2.get(), 5);
    check_equality(LINE(""), elt3.get(), 7);

    elt2.mutate([](auto& e) { e = 3; });
    // 3(2), 4(0), 5(0), 6(0), 7(1)
    check_weak_equivalence(LINE(""), pool, prediction_t{{3, 2}, {4, 0}, {5, 0}, {6, 0}, {7, 1}});
    check_equality(LINE(""), elt.get(), 3);
    check_equality(LINE(""), elt2.get(), 3);
    check_equality(LINE(""), elt3.get(), 7);
  }

  void data_pool_test::test_unpooled()
  {
    using namespace data_sharing;
    constexpr auto x = unpooled<double>::make(3.0);
    check_equality(LINE(""), x.get(), 3.0);
  }

  template<bool PropagateMove, bool PropagateSwap>
  void data_pool_test::test_move_only_allocation()
  {
    using namespace data_sharing;
    using pool_t = data_pool<int, allocator_generator<PropagateMove, PropagateSwap>::template allocator>;
    using prediction_t = typename weak_equivalence_checker<pool_t>::prediction_type;

    pool_t pool{};
    auto elt{pool.make(-1)};
    check_weak_equivalence(LINE(""), pool, prediction_t{{-1, 1}});
    check_equality(LINE(""), elt.get(), -1);

    pool_t clonePool{};
    auto cloneElt{clonePool.make(-1)};
    check_weak_equivalence(LINE(""), clonePool, prediction_t{{-1, 1}});
    check_equality(LINE(""), cloneElt.get(), -1);

    auto allocGetter{
      [](const pool_t& pool){
        return pool.get_allocator();
      }
    };

    check_regular_semantics(LINE(""), pool_t{}, std::move(clonePool), pool_t{}, pool,
                            move_only_allocation_info{allocGetter, move_only_allocation_predictions{1}});
  }
}
