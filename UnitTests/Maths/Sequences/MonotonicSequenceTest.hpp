////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "UnitTestUtils.hpp"

#include "MonotonicSequence.hpp"

namespace sequoia::unit_testing
{
  class monotonic_sequence_test : public unit_test
  {
  public:
    using unit_test::unit_test;

  private:
    template<bool Check>
    constexpr static maths::static_monotonic_sequence<int, 6, std::greater<int>> make_sequence();
    
    void run_tests() override;

    void test_decreasing_sequence();

    void test_static_decreasing_sequence();

    void test_static_increasing_sequence();
  };
}
