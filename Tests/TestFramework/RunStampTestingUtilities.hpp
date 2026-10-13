////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include "sequoia/TestFramework/RegularTestCore.hpp"
#include "sequoia/TestFramework/SumTypeCheckers.hpp"
#include "sequoia/TestFramework/TestExecutionRecord.hpp"

namespace sequoia::testing
{
  template<> struct value_tester<sequoia::testing::run_stamp>
  {
    using type = sequoia::testing::run_stamp;

    template<test_mode Mode>
    static void test(equality_check_t, test_logger<Mode>& logger, const type& actual, const type& prediction)
    {
      check(equality, "Start", logger, actual.start, prediction.start);
    }
  };
}
