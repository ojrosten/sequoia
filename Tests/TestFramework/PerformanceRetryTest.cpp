////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "PerformanceRetryTest.hpp"
#include "PerformanceTestingUtilities.hpp"

#include <algorithm>
#include <format>
#include <memory>
#include <ranges>
#include <vector>

namespace sequoia::testing
{
  namespace
  {
    constexpr std::chrono::milliseconds spin_unit{2};

    constexpr relative_performance_parameters retry_parameters{
      .min_speedup{1.8}, .max_speedup{2.2}, .trials{10}
    };
  }

  [[nodiscard]]
  std::filesystem::path performance_retry_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void performance_retry_test::run_tests()
  {
    test_pass_at_second_attempt();
    test_pass_at_final_attempt();
    test_rising_confidence_multiplier();
    test_task_orders();
  }

  void performance_retry_test::test_pass_at_second_attempt()
  {
    const counted_spinner fast{spin_unit},
                          slow{spin_unit, retry_parameters.trials, 2 * spin_unit};

    check_relative_performance("A speed-up of 1, then of 2", fast, slow, retry_parameters);
    check(equality, "Calls of the fast task", fast.calls(), retry_parameters.trials * (1 + 2));
    check(equality, "Calls of the slow task", slow.calls(), retry_parameters.trials * (1 + 2));
  }

  void performance_retry_test::test_pass_at_final_attempt()
  {
    constexpr auto attempts{relative_performance_max_attempts};
    const counted_spinner fast{spin_unit},
                          slow{spin_unit, retry_parameters.trials * attempts * (attempts - 1) / 2, 2 * spin_unit};

    check_relative_performance("A speed-up of 1 until the final attempt, then of 2", fast, slow, retry_parameters);
    check(equality, "Calls of the fast task", fast.calls(), retry_parameters.trials * attempts * (attempts + 1) / 2);
    check(equality, "Calls of the slow task", slow.calls(), retry_parameters.trials * attempts * (attempts + 1) / 2);
  }

  void performance_retry_test::test_rising_confidence_multiplier()
  {
    using namespace std::chrono_literals;

    constexpr relative_performance_parameters parameters{.min_speedup{3.2}, .max_speedup{4.8}, .trials{5}};
    const counted_spinner fast{1ms};

    check_relative_performance("An interval of 1 standard error below (3.2, 4.8), then of 2 overlapping it",
                               fast,
                               make_cycling_spinner({1ms, 2ms, 2ms, 3ms, 8ms}),
                               parameters);
    check(equality, "Calls of the fast task", fast.calls(), parameters.trials * (1 + 2));
  }

  void performance_retry_test::test_task_orders()
  {
    enum class task { fast, slow };

    constexpr relative_performance_parameters parameters{.min_speedup{1.8}, .max_speedup{2.2}, .trials{5}};
    constexpr auto attempts{relative_performance_max_attempts};

    const auto taskCalls{std::make_shared<std::vector<task>>()};
    auto recordedTask{
      [taskCalls](task t, counted_spinner spinner) {
        return [taskCalls, t, spinner]() {
          taskCalls->push_back(t);
          spinner();
        };
      }
    };

    check_relative_performance("A speed-up of 1 until the final attempt, then of 2, each task recording its calls",
                               recordedTask(task::fast, counted_spinner{spin_unit}),
                               recordedTask(task::slow,
                                            counted_spinner{spin_unit,
                                                            parameters.trials * attempts * (attempts - 1) / 2,
                                                            2 * spin_unit}),
                               parameters);

    auto firstTasksOfAttempt{
      [&taskCalls, &parameters](std::size_t attempt) {
        const auto priorTrials{parameters.trials * attempt * (attempt - 1) / 2};
        return
            *taskCalls
          | std::views::drop(2 * priorTrials)
          | std::views::take(2 * parameters.trials * attempt)
          | std::views::stride(2)
          | std::ranges::to<std::vector>();
      }
    };

    auto isFast{[](task t) { return t == task::fast; }};

    for(const auto attempt : std::views::iota(1uz, attempts + 1))
    {
      check(equality,
            std::format("Trials of attempt {} in which the fast task runs first", attempt),
            static_cast<std::size_t>(std::ranges::count_if(firstTasksOfAttempt(attempt), isFast)),
            parameters.trials * attempt / 2);
    }

    auto ranFastFirstThenSlowFirst{
      [&firstTasksOfAttempt, &isFast](std::size_t attempt) {
        return std::ranges::is_partitioned(firstTasksOfAttempt(attempt), isFast);
      }
    };

    check("Some attempt runs a slow-first trial before a fast-first one",
          !std::ranges::all_of(std::views::iota(1uz, attempts + 1), ranFastFirstThenSlowFirst));
  }
}
