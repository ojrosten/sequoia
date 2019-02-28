////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "UnitTestUtils.hpp"

#include "DataPool.hpp"

namespace sequoia::unit_testing
{
  class data_pool_test : public unit_test
  {
  public:
    using unit_test::unit_test;
  private:
    void run_tests();
    void test_pooled();
    void test_multi_pools();
    void test_unpooled();

    data_sharing::data_pool<int> make_int_pool(const int val);
  };
}
