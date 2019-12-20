////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "UnitTestCore.hpp"

#include "DataPool.hpp"

namespace sequoia::unit_testing
{
  class data_pool_test : public unit_test
  {
  public:
    using unit_test::unit_test;
  private:
    template<class Test>
    friend void do_move_only_allocation_tests(Test&); 

    void run_tests() override;

    void test_pooled();
    void test_multi_pools();
    void test_unpooled();

    data_sharing::data_pool<int> make_int_pool(const int val);

    template<bool PropagateMove, bool PropagateSwap>
    void test_move_only_allocation();
  };
}
