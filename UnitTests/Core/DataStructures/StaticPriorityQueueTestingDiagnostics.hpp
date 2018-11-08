#pragma once

#include "UnitTestUtils.hpp"

namespace sequoia::unit_testing
{
  class test_static_priority_queue_false_positives : public false_positive_test
  {
  public:
    using false_positive_test::false_positive_test;

  private:    
    void run_tests() override;

    void check_depth_1();    
  };
}
