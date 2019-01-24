#pragma once

#include "UnitTestUtils.hpp"

namespace sequoia
{
  namespace unit_testing
  {
    class iterator_test : public unit_test
    {
    public:
      using unit_test::unit_test;
    private:
      void run_tests();

      void test_vector_int();
      void test_vector_vector();
      void test_array();
    };
  }
}
