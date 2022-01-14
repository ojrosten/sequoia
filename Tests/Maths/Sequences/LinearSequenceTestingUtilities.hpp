////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "sequoia/TestFramework/RegularTestCore.hpp"

#include "sequoia/Maths/Sequences/LinearSequence.hpp"

namespace sequoia::testing
{
  template<class T, std::integral Index>
  struct value_tester<maths::linear_sequence<T, Index>>
  {
    using type = maths::linear_sequence<T, Index>;

    template<test_mode Mode>
    static void test(equality_check_t, test_logger<Mode>& logger, const type& sequence, const type& prediction)
    {
      check(equality, "Start", logger, sequence.start(), prediction.start());
      check(equality, "Step", logger, sequence.step(), prediction.step());
    }

    template<test_mode Mode>
    static void test(equivalence_check_t, test_logger<Mode>& logger, const type& sequence, const T& start, const T& step)
    {
      check(equality, "Start wrong", logger, sequence.start(), start);
      check(equality, "Step wrong", logger, sequence.step(), step);
    }
  };
}
