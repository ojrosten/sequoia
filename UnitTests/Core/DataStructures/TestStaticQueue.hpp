#pragma once

#include "UnitTestUtils.hpp"

namespace sequoia::unit_testing
{
  class test_static_queue : public unit_test
  {
  public:
    using unit_test::unit_test;
  private:
    void run_tests() override;

    void check_depth_1();
    void check_constexpr_queue();

    constexpr static auto make_static_queue();
  };
}
