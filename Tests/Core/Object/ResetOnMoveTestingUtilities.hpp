////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include "sequoia/TestFramework/RegularTestCore.hpp"
#include "sequoia/Core/Object/ResetOnMove.hpp"

namespace sequoia::testing
{
  template<class T, T Reset>
  struct value_tester<object::reset_on_move<T, Reset>>
  {
    using type = object::reset_on_move<T, Reset>;

    template<test_mode Mode>
    static void test(equality_check_t, test_logger<Mode>& logger, const type& actual, const type& prediction)
    {
      check(equality, "Value", logger, actual.value(), prediction.value());
    }

    template<test_mode Mode>
    static void test(equivalence_check_t,
                     test_logger<Mode>& logger,
                     const type& actual,
                     const type::value_type& prediction)
    {
      check(equality, "Value", logger, actual.value(), prediction);
    }
  };
}
