////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "UnitTestCore.hpp"

#include "MonotonicSequence.hpp"

namespace sequoia::unit_testing
{
  template<class Test>
  class allocation_tester
  {
  public:
    allocation_tester(Test& test) : m_Test{test}
    {
      m_Test.template test_allocation<false, false, false>();
      m_Test.template test_allocation<false, false, true>();
      m_Test.template test_allocation<false, true, false>();
      m_Test.template test_allocation<false, true, true>();
      m_Test.template test_allocation<true, false, false>();
      m_Test.template test_allocation<true, false, true>();
      m_Test.template test_allocation<true, true, false>();
      m_Test.template test_allocation<true, true, true>();
    }
  private:
    Test& m_Test;
  };

  class monotonic_sequence_test : public unit_test
  {
  public:
    using unit_test::unit_test;

    template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
    void test_allocation();

  private:
    template<bool Check>
    constexpr static maths::static_monotonic_sequence<int, 6, std::greater<int>> make_sequence();
    
    void run_tests() override;

    void test_decreasing_sequence();

    void test_static_decreasing_sequence();

    void test_static_increasing_sequence();
  };
}
