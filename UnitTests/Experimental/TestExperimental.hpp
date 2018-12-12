#pragma once

#include "UnitTestUtils.hpp"

namespace sequoia::unit_testing
{    
  class test_experimental : public unit_test
  {      
  public:
    using unit_test::unit_test;
  private:
    void run_tests();
  };
}
