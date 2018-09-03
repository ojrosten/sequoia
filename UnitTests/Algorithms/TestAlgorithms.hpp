#pragma once

#include "UnitTestUtils.hpp"

namespace sequoia
{
  namespace unit_testing
  {
    class test_algorithms : public unit_test
    {
    public:
      using unit_test::unit_test;

    private:
      void run_tests();

      void sort_basic_type();

      void sort_protective_wrapper();

      void sort_partial_edge();

      void cluster_basic_type();
    };
  }
}
