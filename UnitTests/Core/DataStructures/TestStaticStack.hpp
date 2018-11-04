#pragma once

#include "UnitTestUtils.hpp"

namespace sequoia::unit_testing
{
  class test_static_stack : public unit_test
  {
  public:
    using unit_test::unit_test;
  private:
    void run_tests() override;
  };
}
