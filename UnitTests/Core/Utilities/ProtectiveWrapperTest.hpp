////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "UnitTestCore.hpp"

namespace sequoia::unit_testing
{
  class protective_wrapper_test : public unit_test
  {
  public:
    using unit_test::unit_test;

  private:
    void run_tests();

    void test_basic_type();

    void test_container_type();

    void test_aggregate_type();
  };
}
