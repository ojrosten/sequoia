////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include "sequoia/TestFramework/ChronoCheckers.hpp"
#include "sequoia/TestFramework/RegularTestCore.hpp"
#include "sequoia/TestFramework/SumTypeCheckers.hpp"
#include "sequoia/TestFramework/TestExecutionRecord.hpp"

namespace sequoia::testing
{
  template<> struct value_tester<sequoia::testing::test_execution_completion>
  {
    using type = sequoia::testing::test_execution_completion;

    template<test_mode Mode>
    static void test(equality_check_t, test_logger<Mode>& logger, const type& actual, const type& prediction)
    {
      check(equality,
            "Execution duration, in microseconds",
            logger,
            actual.execution_duration.count(),
            prediction.execution_duration.count());

      check(equality,
            "Runner overhead, in microseconds",
            logger,
            actual.runner_overhead.count(),
            prediction.runner_overhead.count());

      check(equality,
            "Performance check attempts",
            logger,
            actual.performance_check_attempts,
            prediction.performance_check_attempts);
    }
  };

  template<> struct value_tester<sequoia::testing::test_execution_record>
  {
    using type = sequoia::testing::test_execution_record;

    template<test_mode Mode>
    static void test(equality_check_t, test_logger<Mode>& logger, const type& actual, const type& prediction)
    {
      check(equality, "Start time", logger, actual.start_time, prediction.start_time);
      check(equality, "Completion", logger, actual.completion, prediction.completion);
    }
  };
}
