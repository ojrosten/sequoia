////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TestExecutionRecordTestingDiagnostics.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path test_execution_record_false_negative_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void test_execution_record_false_negative_test::run_tests()
  {
    using namespace std::chrono_literals;

    const test_execution_completion
      completion   {.execution_duration{2us}, .runner_overhead{1us}, .performance_check_attempts{3}},
      otherDuration{.execution_duration{3us}, .runner_overhead{1us}, .performance_check_attempts{3}},
      otherOverhead{.execution_duration{2us}, .runner_overhead{3us}, .performance_check_attempts{3}},
      otherAttempts{.execution_duration{2us}, .runner_overhead{1us}, .performance_check_attempts{3, 1}};

    const test_execution_record unfinished{},
                                finished{.completion{completion}},
                                laterStart{.start_time{test_execution_record::start_time_type{1ms}}};

    check(equality, "Start times differ", unfinished, laterStart);
    check(equality, "Only one record has a completion", unfinished, finished);
    check(equality, "Execution durations differ",        finished, test_execution_record{.completion{otherDuration}});
    check(equality, "Runner overheads differ",           finished, test_execution_record{.completion{otherOverhead}});
    check(equality, "Performance check attempts differ", finished, test_execution_record{.completion{otherAttempts}});
  }
}
