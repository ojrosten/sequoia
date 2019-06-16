////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "UnitTestCore.hpp"

namespace sequoia::unit_testing
{
  class test_static_queue : public unit_test
  {
  public:
    using unit_test::unit_test;
  private:
    void run_tests() override;

    void check_depth_0();
    void check_depth_1();
    void check_depth_2();

    constexpr static auto make_static_queue_2();
  };
}
