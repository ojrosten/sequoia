////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "PerformanceExceptionsTest.hpp"
#include "PerformanceTestingUtilities.hpp"

#include <array>
#include <chrono>
#include <limits>
#include <stdexcept>

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path performance_exceptions_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void performance_exceptions_test::run_tests()
  {
    test_invalid_arguments();
    test_checks_made_by_a_task();
    test_throwing_task();
    test_attempt_with_too_few_trials();
    test_non_positive_durations();
  }

  void performance_exceptions_test::test_invalid_arguments()
  {
    constexpr double      nan{std::numeric_limits<double>::quiet_NaN()};
    constexpr std::size_t tooFewTrials{relative_performance_minimum_trials_floor - 1};

    auto relativePerformanceCheck{
      [this](const relative_performance_parameters& parameters) {
        return [this, parameters]() {
          return check_relative_performance("Relative performance with invalid arguments",
                                            []() {},
                                            []() {},
                                            parameters);
        };
      }
    };

    check_exception_thrown<std::invalid_argument>("Minimum speed-up of 1",
                                                  relativePerformanceCheck({.prediction{.lower{1.0}, .upper{2.0}}}));
    check_exception_thrown<std::invalid_argument>("Maximum speed-up of 1",
                                                  relativePerformanceCheck({.prediction{.lower{1.5}, .upper{1.0}}}));
    check_exception_thrown<std::invalid_argument>("Minimum speed-up of NaN",
                                                  relativePerformanceCheck({.prediction{.lower{nan}, .upper{2.0}}}));
    check_exception_thrown<std::invalid_argument>("Minimum speed-up exceeding the maximum",
                                                  relativePerformanceCheck({.prediction{.lower{2.5}, .upper{2.0}}}));
    check_exception_thrown<std::invalid_argument>("Too few trials",
                                                  relativePerformanceCheck({.prediction{.lower{2.0}, .upper{3.0}},
                                                                            .minimum_trials{tooFewTrials}}));

    auto relativePerformanceCheckWithBaselines{
      [this](const relative_performance_parameters& parameters) {
        return [this, parameters]() {
          return check_relative_performance("Relative performance with baselines and invalid arguments",
                                            task_with_baseline{.task = []() {}, .baseline = []() {}},
                                            task_with_baseline{.task = []() {}, .baseline = []() {}},
                                            parameters);
        };
      }
    };

    check_exception_thrown<std::invalid_argument>(
      "Minimum speed-up of 1, with baselines",
      relativePerformanceCheckWithBaselines({.prediction{.lower{1.0}, .upper{2.0}}})
    );
    check_exception_thrown<std::invalid_argument>(
      "Maximum speed-up of 1, with baselines",
      relativePerformanceCheckWithBaselines({.prediction{.lower{1.5}, .upper{1.0}}})
    );
    check_exception_thrown<std::invalid_argument>(
      "Minimum speed-up of NaN, with baselines",
      relativePerformanceCheckWithBaselines({.prediction{.lower{nan}, .upper{2.0}}})
    );
    check_exception_thrown<std::invalid_argument>(
      "Minimum speed-up exceeding the maximum, with baselines",
      relativePerformanceCheckWithBaselines({.prediction{.lower{2.5}, .upper{2.0}}})
    );
    check_exception_thrown<std::invalid_argument>(
      "Too few trials, with baselines",
      relativePerformanceCheckWithBaselines({.prediction{.lower{2.0}, .upper{3.0}}, .minimum_trials{tooFewTrials}})
    );
  }

  void performance_exceptions_test::test_checks_made_by_a_task()
  {
    const relative_performance_parameters parameters{.prediction{.lower{2.0}, .upper{3.0}}};

    auto checkWithTasks{
      [this, &parameters](auto fast, auto slow) {
        return [this, &parameters, fast, slow]() {
          return check_relative_performance("Relative performance with a task which performs a check",
                                            fast,
                                            slow,
                                            parameters);
        };
      }
    };

    check_exception_thrown<std::logic_error>(
      "Fast task performs a check",
      checkWithTasks([this]() { check("A check within a performance check", true); }, []() {})
    );

    check_exception_thrown<std::logic_error>(
      "Slow task performs a performance check",
      checkWithTasks(
        []() {},
        [this, &parameters]() {
          check_relative_performance("A performance check within a performance check", []() {}, []() {}, parameters);
        }
      )
    );

    check_exception_thrown<std::logic_error>(
      "Fast task's baseline performs a check",
      checkWithTasks(
        task_with_baseline{.task = []() {}, .baseline = [this]() { check("A check within a performance check", true); }},
        task_with_baseline{.task = []() {}, .baseline = []() {}}
      )
    );

    check_exception_thrown<std::logic_error>(
      "Slow task's baseline performs a check",
      checkWithTasks(
        task_with_baseline{.task = []() {}, .baseline = []() {}},
        task_with_baseline{.task = []() {}, .baseline = [this]() { check("A check within a performance check", true); }}
      )
    );
  }

  void performance_exceptions_test::test_throwing_task()
  {
    auto checkWithThrowingFastTask{
      [this]() {
        return check_relative_performance("Relative performance with a throwing task",
                                          []() { throw std::runtime_error{"Fast task failure"}; },
                                          []() {},
                                          {.prediction{.lower{2.0}, .upper{3.0}}});
      }
    };

    check_exception_thrown<std::runtime_error>("Fast task throws", checkWithThrowingFastTask);

    auto checkWithThrowingSlowBaseline{
      [this]() {
        return check_relative_performance(
          "Relative performance with a throwing baseline",
          task_with_baseline{.task = []() {}, .baseline = []() {}},
          task_with_baseline{.task = []() {}, .baseline = []() { throw std::runtime_error{"Slow baseline failure"}; }},
          {.prediction{.lower{2.0}, .upper{3.0}}}
        );
      }
    };

    check_exception_thrown<std::runtime_error>("Slow task's baseline throws", checkWithThrowingSlowBaseline);
  }

  void performance_exceptions_test::test_attempt_with_too_few_trials()
  {
    using namespace std::chrono_literals;

    constexpr std::size_t tooFewTrials{relative_performance_minimum_trials_floor - 1};

    auto judgeAttemptWithTooFewTrials{
      []() {
        // Every duration is positive, so only the number of trials can be
        // refused.
        const auto trialDurations{
          []() {
            std::array<relative_performance_durations, tooFewTrials> durations{};
            durations.fill({.fast{1s}, .slow{1s}});
            return durations;
          }()
        };

        return impl::judge_attempt(trialDurations, 1.0, {.lower{2.0}, .upper{3.0}});
      }
    };

    check_exception_thrown<std::invalid_argument>("An attempt with one trial fewer than the floor",
                                                  judgeAttemptWithTooFewTrials);

    auto judgeAttemptWithBaselinesAndTooFewTrials{
      []() {
        // Every duration exceeds its baseline's, so only the number of trials
        // can be refused.
        const auto trialDurations{
          []() {
            std::array<impl::relative_performance_durations_with_baselines, tooFewTrials> durations{};
            durations.fill({.fast{.task{2s}, .baseline{1s}}, .slow{.task{2s}, .baseline{1s}}});
            return durations;
          }()
        };

        return impl::judge_attempt(trialDurations, 1.0, {.lower{2.0}, .upper{3.0}});
      }
    };

    check_exception_thrown<std::invalid_argument>("An attempt with baselines and one trial fewer than the floor",
                                                  judgeAttemptWithBaselinesAndTooFewTrials);
  }

  void performance_exceptions_test::test_non_positive_durations()
  {
    using namespace std::chrono_literals;

    auto judgeAttemptContaining{
      [](relative_performance_durations trial) {
        return [trial]() {
          // As many trials as the floor, so only a duration can be refused.
          constexpr relative_performance_durations typical{.fast{1s}, .slow{1s}};
          return impl::judge_attempt(trials_with_one_atypical(typical, trial), 1.0, {.lower{2.0}, .upper{3.0}});
        };
      }
    };

    auto judgeAttemptWithBaselinesContaining{
      [](impl::relative_performance_durations_with_baselines trial) {
        return [trial]() {
          // As many trials as the floor, and every other task's duration
          // exceeds its baseline's, so only a duration can be refused.
          constexpr impl::relative_performance_durations_with_baselines
            typical{.fast{.task{2s}, .baseline{1s}}, .slow{.task{2s}, .baseline{1s}}};

          return impl::judge_attempt(trials_with_one_atypical(typical, trial), 1.0, {.lower{2.0}, .upper{3.0}});
        };
      }
    };

    check_exception_thrown<std::runtime_error>("A fast task timed as zero",
                                               judgeAttemptContaining({.fast{0s}, .slow{1s}}));
    check_exception_thrown<std::runtime_error>("A slow task timed as zero",
                                               judgeAttemptContaining({.fast{1s}, .slow{0s}}));
    check_exception_thrown<std::runtime_error>("A negative duration",
                                               judgeAttemptContaining({.fast{1s}, .slow{-1s}}));

    check_exception_thrown<std::runtime_error>(
      "A fast task's baseline timed as zero",
      judgeAttemptWithBaselinesContaining({.fast{.task{2s}, .baseline{0s}}, .slow{.task{2s}, .baseline{1s}}})
    );
    check_exception_thrown<std::runtime_error>(
      "A slow task's baseline of negative duration",
      judgeAttemptWithBaselinesContaining({.fast{.task{2s}, .baseline{1s}}, .slow{.task{2s}, .baseline{-1s}}})
    );
    check_exception_thrown<std::runtime_error>(
      "A slow task timed as zero, beside its baseline",
      judgeAttemptWithBaselinesContaining({.fast{.task{2s}, .baseline{1s}}, .slow{.task{0s}, .baseline{1s}}})
    );
  }
}
