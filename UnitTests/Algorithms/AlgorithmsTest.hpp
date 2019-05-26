////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "UnitTestUtils.hpp"

namespace sequoia::unit_testing
{
  class algorithms_test : public unit_test
  {
  public:
    using unit_test::unit_test;

  private:
    void run_tests();

    void sort_basic_type();
    void sort_protective_wrapper();
    void sort_partial_edge();

    void cluster_basic_type();

    void lower_bound_basic_type();
    void lower_bound_protective_wrapper();
    void lower_bound_partial_edge();

    void upper_bound_basic_type();

    void equal_range_basic_type();

    void equality();

    void test_rotate();
  };
}
