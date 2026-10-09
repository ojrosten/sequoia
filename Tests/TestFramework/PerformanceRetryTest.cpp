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
      .prediction{.lower{1.8}, .upper{2.2}}, .minimum_trials{10}
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
    test_rising_overlap_multiplier();
    test_constant_gate_multiplier();
    test_trimmed_estimate();
    test_task_orders();
  }

  void performance_retry_test::test_pass_at_second_attempt()
  {
    const counted_spinner fast{spin_unit},
                          slow{spin_unit, retry_parameters.minimum_trials, 2 * spin_unit};

    check_relative_performance("A speed-up of 1, then of 2", fast, slow, retry_parameters);
    check(equality, "Calls of the fast task", fast.calls(), retry_parameters.minimum_trials * (1 + 2));
    check(equality, "Calls of the slow task", slow.calls(), retry_parameters.minimum_trials * (1 + 2));
  }

  void performance_retry_test::test_pass_at_final_attempt()
  {
    constexpr auto attempts{relative_performance_max_attempts};
    const counted_spinner fast{spin_unit},
                          slow{spin_unit, retry_parameters.minimum_trials * attempts * (attempts - 1) / 2, 2 * spin_unit};

    check_relative_performance("A speed-up of 1 until the final attempt, then of 2", fast, slow, retry_parameters);
    check(equality, "Calls of the fast task", fast.calls(), retry_parameters.minimum_trials * attempts * (attempts + 1) / 2);
    check(equality, "Calls of the slow task", slow.calls(), retry_parameters.minimum_trials * attempts * (attempts + 1) / 2);
  }

  void performance_retry_test::test_rising_overlap_multiplier()
  {
    using namespace std::chrono_literals;

    constexpr relative_performance_parameters parameters{.prediction{.lower{3.82}, .upper{5.7}}, .minimum_trials{10}};
    const counted_spinner fast{1ms};

    check_relative_performance(
      "Overlapping [3.82, 5.7] needs 1.25 standard errors: the first attempt's 1 falls short, the second's 2 suffices",
      fast,
      make_cycling_spinner({1ms, 3ms, 3ms, 4ms, 4ms}),
      parameters
    );
    check(equality, "Calls of the fast task", fast.calls(), parameters.minimum_trials * (1 + 2));
  }

  void performance_retry_test::test_constant_gate_multiplier()
  {
    using namespace std::chrono_literals;

    constexpr relative_performance_parameters parameters{.prediction{.lower{1.05}, .upper{4}}, .minimum_trials{10}};
    const counted_spinner fast{1ms};

    check_relative_performance(
      "The log speed-up is 1.95 standard errors at the first attempt, short of the gate's 3, and 4.09 at the second",
      fast,
      make_cycling_spinner({2ms, 2ms, 2ms, 9ms, 9ms}),
      parameters
    );
    check(equality, "Calls of the fast task", fast.calls(), parameters.minimum_trials * (1 + 2));
  }

  void performance_retry_test::test_trimmed_estimate()
  {
    using namespace std::chrono_literals;

    constexpr relative_performance_parameters parameters{.prediction{.lower{3.2}, .upper{5}}, .minimum_trials{10}};

    check_relative_performance(
      "One fast call in five takes 12 ms. The trimmed mean gives a speed-up of 4, and the untrimmed mean gives 2.43",
      make_cycling_spinner({1ms, 1ms, 1ms, 1ms, 12ms}),
      counted_spinner{4ms},
      parameters
    );
  }

  void performance_retry_test::test_task_orders()
  {
    enum class task { fast, slow };

    constexpr relative_performance_parameters parameters{.prediction{.lower{1.8}, .upper{2.2}}, .minimum_trials{10}};
    constexpr auto attempts{relative_performance_max_attempts};

    auto recordTaskCalls{
      [this, &parameters](const reporter& description) {
        const auto taskCalls{std::make_shared<std::vector<task>>()};
        auto recordedTask{
          [taskCalls](task t, counted_spinner spinner) {
            return [taskCalls, t, spinner]() {
              taskCalls->push_back(t);
              spinner();
            };
          }
        };

        check_relative_performance(description,
                                   recordedTask(task::fast, counted_spinner{spin_unit}),
                                   recordedTask(task::slow,
                                                counted_spinner{spin_unit,
                                                                parameters.minimum_trials * attempts * (attempts - 1) / 2,
                                                                2 * spin_unit}),
                                   parameters);

        return *taskCalls;
      }
    };

    const auto taskCalls{
      recordTaskCalls("A speed-up of 1 until the final attempt, then of 2, each task recording its calls")
    };

    auto tasksRunFirstInAttempt{
      [&taskCalls, &parameters](std::size_t attempt) {
        const auto priorTrials{parameters.minimum_trials * attempt * (attempt - 1) / 2};
        return
            taskCalls
          | std::views::drop(2 * priorTrials)
          | std::views::take(2 * parameters.minimum_trials * attempt)
          | std::views::stride(2)
          | std::ranges::to<std::vector>();
      }
    };

    auto isFast{[](task t) { return t == task::fast; }};

    for(const auto attempt : std::views::iota(1uz, attempts + 1))
    {
      check(equality,
            std::format("Trials of attempt {} in which the fast task runs first", attempt),
            static_cast<std::size_t>(std::ranges::count_if(tasksRunFirstInAttempt(attempt), isFast)),
            parameters.minimum_trials * attempt / 2);
    }

    check("A second check runs the tasks in another order",
          recordTaskCalls("The same tasks, checked again") != taskCalls);
  }
}
