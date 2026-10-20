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
#include <span>
#include <vector>

namespace sequoia::testing
{
  namespace
  {
    constexpr std::chrono::milliseconds spin_unit{2};

    /** \brief Parameters with an odd minimum, so that the number of trials
               in an attempt shows how it is rounded: 11 becomes 12 in attempt
               1, and 16.5 becomes 16 in attempt 2.
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
    test_pass_at_second_attempt_with_baselines();
    test_task_orders_with_baselines();
  }

  void performance_retry_test::test_pass_at_second_attempt()
  {
    const counted_spinner fast{spin_unit},
                          slow{spin_unit, trials_for_minimum_11[0], 2 * spin_unit};

    check_relative_performance("A speed-up of 1, then of 2", fast, slow, retry_parameters);
    check(equality, "Calls of the fast task", fast.calls(), trials_before_attempt(trials_for_minimum_11, 3));
    check(equality, "Calls of the slow task", slow.calls(), trials_before_attempt(trials_for_minimum_11, 3));
  }

  void performance_retry_test::test_pass_at_final_attempt()
  {
    constexpr auto attempts{relative_performance_max_attempts};
    constexpr auto maximumCalls       {retry_parameters.minimum_trials * attempts * (attempts + 3) / 4 + attempts},
                   everyAttemptsTrials{trials_before_attempt(trials_for_minimum_11, attempts + 1)};

    STATIC_CHECK(everyAttemptsTrials <= maximumCalls);

    const counted_spinner fast{spin_unit},
                          slow{spin_unit, trials_before_attempt(trials_for_minimum_11, 3), 2 * spin_unit};

    check_relative_performance("A speed-up of 1 until the final attempt, then of 2", fast, slow, retry_parameters);
    check(equality, "Calls of the fast task, every attempt's trials", fast.calls(), everyAttemptsTrials);
    check(equality, "Calls of the slow task, every attempt's trials", slow.calls(), everyAttemptsTrials);
  }

  void performance_retry_test::test_rising_overlap_multiplier()
  {
    using namespace std::chrono_literals;

    constexpr auto attempts{relative_performance_max_attempts};
    constexpr auto everyAttemptsTrials{trials_before_attempt(trials_for_minimum_10, attempts + 1)};

    constexpr relative_performance_parameters parameters{.prediction{.lower{4.88}, .upper{7.81}}};
    const counted_spinner fast{8ms, trials_before_attempt(trials_for_minimum_10, attempts), 7ms};

    check_relative_performance(
      "The intervals of attempts 1 and 2 lie below the prediction, "
      "and attempt 2's would overlap the prediction only if substantially wider; "
      "at attempt 3 the fast task speeds up, and the wider interval overlaps the prediction, "
      "but would not if substantially narrower; "
      "any multiplier wide enough for attempt 3 would let attempt 1 overlap too",
      fast,
      make_cycling_spinner({8ms, 10ms, 30ms, 40ms, 40ms}),
      parameters
    );
    check(equality, "Calls of the fast task, every attempt's trials", fast.calls(), everyAttemptsTrials);
  }

  void performance_retry_test::test_constant_gate_multiplier()
  {
    using namespace std::chrono_literals;

    constexpr relative_performance_parameters parameters{.prediction{.lower{1.05}, .upper{4}}};
    const counted_spinner fast{2ms, trials_for_minimum_10[0], 1ms};

    check_relative_performance(
      "Attempt 1's interval excludes a speed-up of 1, "
      "but the gate's wider interval does not; "
      "attempt 2 passes",
      fast,
      make_cycling_spinner({2ms, 2ms, 3ms, 4ms, 9ms}),
      parameters
    );
    check(equality, "Calls of the fast task", fast.calls(), trials_before_attempt(trials_for_minimum_10, 3));
  }

  void performance_retry_test::test_trimmed_estimate()
  {
    using namespace std::chrono_literals;

    constexpr relative_performance_parameters parameters{.prediction{.lower{3.2}, .upper{5}}};
    const counted_spinner fast{100ms, 1, 1ms};

    check_relative_performance(
      "The first fast call takes 100 ms. "
      "The trimmed mean gives a speed-up of 4, "
      "and the untrimmed mean gives a speed-up below the prediction at attempt 1",
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
                                                                trials_before_attempt(trials_for_minimum_11, 3),
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
        const auto priorTrials{trials_before_attempt(trials_for_minimum_11, attempt)};

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

  void performance_retry_test::test_pass_at_second_attempt_with_baselines()
  {
    using namespace std::chrono_literals;

    const counted_spinner fast{4ms},
                          fastBaseline{7ms, trials_for_minimum_11[0], 1ms},
                          slow{7ms},
                          slowBaseline{1ms};

    check_relative_performance("The fast task is shorter than its baseline at attempt 1; "
                               "from the net durations, a speed-up of 2 at attempt 2",
                               task_with_baseline{.task = fast, .baseline = fastBaseline},
                               task_with_baseline{.task = slow, .baseline = slowBaseline},
                               retry_parameters);

    constexpr auto callsOfEach{trials_before_attempt(trials_for_minimum_11, 3)};

    check(equality, "Calls of the fast task",            fast.calls(),         callsOfEach);
    check(equality, "Calls of the fast task's baseline", fastBaseline.calls(), callsOfEach);
    check(equality, "Calls of the slow task",            slow.calls(),         callsOfEach);
    check(equality, "Calls of the slow task's baseline", slowBaseline.calls(), callsOfEach);
  }

  void performance_retry_test::test_task_orders_with_baselines()
  {
    using namespace std::chrono_literals;

    enum class task { fast, fast_baseline, slow, slow_baseline };

    constexpr std::size_t callsPerTrial{4},
                          trials{trials_for_minimum_11[0]};

    auto recordTaskCalls{
      [this](const reporter& description) {
        const auto taskCalls{std::make_shared<std::vector<task>>()};
        auto recordedTask{
          [taskCalls](task t, std::chrono::milliseconds spinDuration) {
            return [taskCalls, t, spinDuration]() {
              taskCalls->push_back(t);
              spin_for(spinDuration);
            };
          }
        };

        check_relative_performance(description,
                                   task_with_baseline{.task     = recordedTask(task::fast,          4ms),
                                                      .baseline = recordedTask(task::fast_baseline, 1ms)},
                                   task_with_baseline{.task     = recordedTask(task::slow,          7ms),
                                                      .baseline = recordedTask(task::slow_baseline, 1ms)},
                                   retry_parameters);

        return *taskCalls;
      }
    };

    const auto taskCalls{
      recordTaskCalls("From the net durations, a speed-up of 2, each task and baseline recording its calls")
    };

    if(check(equality, "Calls of the tasks and baselines", taskCalls.size(), callsPerTrial * trials))
    {
      auto isFast{[](task t) { return (t == task::fast) || (t == task::fast_baseline); }};

      auto eachPairIsContiguous{
        [isFast](std::span<const task> trialCalls) {
          return (isFast(trialCalls[0]) == isFast(trialCalls[1])) && (isFast(trialCalls[2]) == isFast(trialCalls[3]));
        }
      };

      auto fastPairRunsFirst{[isFast](std::span<const task> trialCalls) { return isFast(trialCalls[0]); }};

      auto taskRunsBeforeBaseline{
        [](task t, task baseline) {
          return [t, baseline](std::span<const task> trialCalls) {
            return std::ranges::find(trialCalls, t) < std::ranges::find(trialCalls, baseline);
          };
        }
      };

      // TO DO: taskCalls | std::views::chunk(callsPerTrial), once libc++ has
      // chunk (P2442)
      auto callsOfTrial{
        [&taskCalls](std::size_t trial) {
          return std::span<const task>{taskCalls}.subspan(trial * callsPerTrial, callsPerTrial);
        }
      };

      auto trialsWhere{
        [callsOfTrial, numTrials{taskCalls.size() / callsPerTrial}](auto predicate) {
          return static_cast<std::size_t>(
            std::ranges::count_if(std::views::iota(0uz, numTrials) | std::views::transform(callsOfTrial), predicate)
          );
        }
      };

      check(equality, "Trials in which each task runs beside its baseline", trialsWhere(eachPairIsContiguous), trials);
      check(equality,
            "Trials in which the fast task and its baseline run first",
            trialsWhere(fastPairRunsFirst),
            trials / 2);
      check(equality,
            "Trials in which the fast task runs before its baseline",
            trialsWhere(taskRunsBeforeBaseline(task::fast, task::fast_baseline)),
            trials / 2);
      check(equality,
            "Trials in which the slow task runs before its baseline",
            trialsWhere(taskRunsBeforeBaseline(task::slow, task::slow_baseline)),
            trials / 2);
    }

    check("A second check runs the tasks and baselines in another order",
          recordTaskCalls("The same tasks and baselines, checked again") != taskCalls);
  }
}
