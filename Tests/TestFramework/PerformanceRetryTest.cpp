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
#include <functional>
#include <memory>
#include <ranges>
#include <vector>

namespace sequoia::testing
{
  namespace
  {
    constexpr std::chrono::milliseconds spin_unit{2};

    /** \brief Parameters with an odd minimum, so that every rounding down of
               a number of trials shows.
     */
    constexpr relative_performance_parameters retry_parameters{
      .prediction{.lower{1.8}, .upper{2.2}}, .minimum_trials{11}
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
                          slow{spin_unit, trials_for_minimum_11[0], 2 * spin_unit};

    check_relative_performance("A speed-up of 1, then of 2", fast, slow, retry_parameters);
    check(equality, "Calls of the fast task", fast.calls(), trials_for_minimum_11[0] + trials_for_minimum_11[1]);
    check(equality, "Calls of the slow task", slow.calls(), trials_for_minimum_11[0] + trials_for_minimum_11[1]);
  }

  void performance_retry_test::test_pass_at_final_attempt()
  {
    constexpr auto attempts{relative_performance_max_attempts};
    constexpr auto maximumCalls{retry_parameters.minimum_trials * attempts * (attempts + 3) / 4};

    const counted_spinner fast{spin_unit},
                          slow{spin_unit, trials_for_minimum_11[0] + trials_for_minimum_11[1], 2 * spin_unit};

    check_relative_performance("A speed-up of 1 until the final attempt, then of 2", fast, slow, retry_parameters);
    check(equality, "Calls of the fast task, the contract's maximum", fast.calls(), maximumCalls);
    check(equality, "Calls of the slow task, the contract's maximum", slow.calls(), maximumCalls);
  }

  void performance_retry_test::test_rising_overlap_multiplier()
  {
    using namespace std::chrono_literals;

    constexpr relative_performance_parameters parameters{.prediction{.lower{7.35}, .upper{11.8}}, .minimum_trials{10}};
    const counted_spinner fast{1ms};

    check_relative_performance(
      "Attempt 1 takes 1 standard error and needs 1.43 to overlap [7.35, 11.8]; attempt 2 takes 2 and needs 1.51",
      fast,
      make_cycling_spinner({1ms, 2ms, 8ms, 8ms, 8ms}),
      parameters
    );
    check(equality, "Calls of the fast task", fast.calls(), trials_for_minimum_10[0] + trials_for_minimum_10[1]);
  }

  void performance_retry_test::test_constant_gate_multiplier()
  {
    using namespace std::chrono_literals;

    constexpr relative_performance_parameters parameters{.prediction{.lower{1.05}, .upper{4}}, .minimum_trials{10}};
    const counted_spinner fast{2ms, trials_for_minimum_10[0], 1ms};

    check_relative_performance(
      "The log speed-up is 1.99 standard errors at the first attempt, short of the gate's 3, and 5.56 at the second",
      fast,
      make_cycling_spinner({2ms, 2ms, 3ms, 4ms, 9ms}),
      parameters
    );
    check(equality, "Calls of the fast task", fast.calls(), trials_for_minimum_10[0] + trials_for_minimum_10[1]);
  }

  void performance_retry_test::test_trimmed_estimate()
  {
    using namespace std::chrono_literals;

    constexpr relative_performance_parameters parameters{.prediction{.lower{3.2}, .upper{5}}, .minimum_trials{10}};
    const counted_spinner fast{100ms, 1, 1ms};

    check_relative_performance(
      "The first fast call takes 100 ms. The trimmed mean gives a speed-up of 4, and the untrimmed mean gives 2.52",
      fast,
      counted_spinner{4ms},
      parameters
    );
    check(equality, "Calls of the fast task", fast.calls(), trials_for_minimum_10[0]);
  }

  void performance_retry_test::test_task_orders()
  {
    enum class task { fast, slow };

    constexpr auto attempts{relative_performance_max_attempts};

    auto recordTaskCalls{
      [this](const reporter& description) {
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
                                                                trials_for_minimum_11[0] + trials_for_minimum_11[1],
                                                                2 * spin_unit}),
                                   retry_parameters);

        return *taskCalls;
      }
    };

    const auto taskCalls{
      recordTaskCalls("A speed-up of 1 until the final attempt, then of 2, each task recording its calls")
    };

    auto tasksRunFirstInAttempt{
      [&taskCalls](std::size_t attempt) {
        const auto priorTrials{
          std::ranges::fold_left(trials_for_minimum_11 | std::views::take(attempt - 1), 0uz, std::plus<>{})
        };

        return
            taskCalls
          | std::views::drop(2 * priorTrials)
          | std::views::take(2 * trials_for_minimum_11[attempt - 1])
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
            trials_for_minimum_11[attempt - 1] / 2);
    }

    check("A second check runs the tasks in another order",
          recordTaskCalls("The same tasks, checked again") != taskCalls);
  }
}
