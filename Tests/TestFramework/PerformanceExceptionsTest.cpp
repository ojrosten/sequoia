////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "PerformanceExceptionsTest.hpp"

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
                                                  relativePerformanceCheck({.prediction{.lower{1.0}, .upper{2.0}},
                                                                            .minimum_trials{10}}));
    check_exception_thrown<std::invalid_argument>("Maximum speed-up of 1",
                                                  relativePerformanceCheck({.prediction{.lower{1.5}, .upper{1.0}},
                                                                            .minimum_trials{10}}));
    check_exception_thrown<std::invalid_argument>("Minimum speed-up of NaN",
                                                  relativePerformanceCheck({.prediction{.lower{nan}, .upper{2.0}},
                                                                            .minimum_trials{10}}));
    check_exception_thrown<std::invalid_argument>("Minimum speed-up exceeding the maximum",
                                                  relativePerformanceCheck({.prediction{.lower{2.5}, .upper{2.0}},
                                                                            .minimum_trials{10}}));
    check_exception_thrown<std::invalid_argument>("Too few trials",
                                                  relativePerformanceCheck({.prediction{.lower{2.0}, .upper{3.0}},
                                                                            .minimum_trials{tooFewTrials}}));
  }

  void performance_exceptions_test::test_checks_made_by_a_task()
  {
    const relative_performance_parameters parameters{.prediction{.lower{2.0}, .upper{3.0}}, .minimum_trials{10}};

    auto checkWithFastTask{
      [this, parameters](auto fast) {
        return [this, parameters, fast]() {
          return check_relative_performance("Relative performance with a task which makes a check",
                                            fast,
                                            []() {},
                                            parameters);
        };
      }
    };

    check_exception_thrown<std::logic_error>(
      "Fast task makes a check",
      checkWithFastTask([this]() { check("A check within a performance check", true); })
    );

    check_exception_thrown<std::logic_error>(
      "Fast task makes a performance check",
      checkWithFastTask(
        [this, parameters]() {
          check_relative_performance("A performance check within a performance check", []() {}, []() {}, parameters);
        }
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
                                          {.prediction{.lower{2.0}, .upper{3.0}},
                                           .minimum_trials{10}});
      }
    };

    check_exception_thrown<std::runtime_error>("Fast task throws", checkWithThrowingFastTask);
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
  }

  void performance_exceptions_test::test_non_positive_durations()
  {
    using namespace std::chrono_literals;

    auto judgeAttemptContaining{
      [](relative_performance_durations trial) {
        return [trial]() {
          // As many trials as the floor, so only a duration can be refused.
          const auto trialDurations{
            [trial]() {
              std::array<relative_performance_durations, relative_performance_minimum_trials_floor> durations{};
              durations.fill({.fast{1s}, .slow{1s}});
              durations[durations.size() / 2] = trial;
              return durations;
            }()
          };

          return impl::judge_attempt(trialDurations, 1.0, {.lower{2.0}, .upper{3.0}});
        };
      }
    };

    check_exception_thrown<std::runtime_error>("A fast task timed as zero",
                                               judgeAttemptContaining({.fast{0s}, .slow{1s}}));
    check_exception_thrown<std::runtime_error>("A slow task timed as zero",
                                               judgeAttemptContaining({.fast{1s}, .slow{0s}}));
    check_exception_thrown<std::runtime_error>("A negative duration",
                                               judgeAttemptContaining({.fast{1s}, .slow{-1s}}));
  }
}
