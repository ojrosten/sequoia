#pragma once

#include "UnitTestUtils.hpp"

namespace sequoia
{
  namespace unit_testing
  {
    class false_positive_diagnostics : public false_positive_test
    {
    public:
      using false_positive_test::false_positive_test;
    private:
      void run_tests() override;

      void test_relative_performance();
      void test_container_checks();
      void test_mixed();
    };

    class false_negative_diagnostics : public false_negative_test
    {
    public:
      using false_negative_test::false_negative_test;
    private:
      void run_tests() override;

      void test_relative_performance();
      void test_container_checks();
      void test_mixed();
      
    };
  }
}
