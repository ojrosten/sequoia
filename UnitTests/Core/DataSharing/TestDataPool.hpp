#pragma once

#include "UnitTestUtils.hpp"

#include "DataPool.hpp"

namespace sequoia
{
  namespace unit_testing
  {
    class test_data_pool : public unit_test
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
}
