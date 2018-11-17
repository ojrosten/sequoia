#pragma once

#include "UnitTestUtils.hpp"

namespace sequoia::unit_testing
{
  class test_static_stack_false_positives : public false_positive_test
  {
  public:
    using false_positive_test::false_positive_test;

  private:    
    void run_tests() override;
    
    void check_depth_0();
    void check_depth_1();    
  };
}
