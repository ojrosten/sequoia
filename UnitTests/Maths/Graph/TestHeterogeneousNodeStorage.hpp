#pragma once

#include "UnitTestUtils.hpp"

namespace sequoia::unit_testing
{
  class test_heterogeneous_node_storage : public unit_test
  {
  public:
    using unit_test::unit_test;

  private:
    using unit_test::check_equality;

    void run_tests();
  };
}
