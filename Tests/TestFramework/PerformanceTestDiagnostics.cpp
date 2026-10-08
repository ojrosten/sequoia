////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "PerformanceTestDiagnostics.hpp"
#include "PerformanceTestingUtilities.hpp"

#include "sequoia/Streaming/Streaming.hpp"

#include <array>
#include <chrono>
#include <limits>
#include <memory>
#include <utility>

namespace sequoia::testing
{
  namespace
  {
    struct copyable_task
    {
      void operator()() const {}
    };

    struct move_only_task
    {
      std::unique_ptr<int> resource{};

      void operator()() const {}
    };

    struct rvalue_only_task
    {
      void operator()() && {}
    };

    template<class Task>
    concept profilable = requires(Task task) { profile(std::move(task)); };

    template<class Fast, class Slow>
    inline constexpr bool checkable_tasks_v{
      requires(test_logger<test_mode::standard>& logger,
               Fast fast,
               Slow slow,
               const relative_performance_parameters& parameters) {
        check_relative_performance("", logger, std::move(fast), std::move(slow), parameters);
      }
    };

    template<class Fast, class Slow>
    inline constexpr bool checkable_tasks_by_extender_v{
      requires(performance_extender<test_mode::standard>& extender,
               Fast fast,
               Slow slow,
               const relative_performance_parameters& parameters) {
        extender.check_relative_performance("", std::move(fast), std::move(slow), parameters);
      }
    };
  }

  [[nodiscard]]
  std::filesystem::path performance_false_negative_diagnostics::source_file()
  {
    return std::source_location::current().file_name();
  }

  void performance_false_negative_diagnostics::run_tests()
  {
    test_relative_performance();
    test_confidence_multiplier();
    test_significance_gate();
  }

  void performance_false_negative_diagnostics::test_relative_performance()
  {
    using namespace std::chrono_literals;

    check_relative_performance("Fast task 4 ms, slow task cycling through (2, 3, 4, 5, 6) ms, predicted (1.2, 1.5)",
                               []() { spin_for(4ms); },
                               make_cycling_spinner({2ms, 3ms, 4ms, 5ms, 6ms}),
                               {.min_speedup{1.2}, .max_speedup{1.5}, .trials{5}});

    check_relative_performance("Fast task 2 ms, slow task 1 ms, predicted (1.5, 2.0)",
                               []() { spin_for(2ms); },
                               []() { spin_for(1ms); },
                               {.min_speedup{1.5}, .max_speedup{2.0}, .trials{5}});

    check_relative_performance("Speed-up of 2, predicted (3.0, 3.0)",
                               []() { spin_for(1ms); },
                               []() { spin_for(2ms); },
                               {.min_speedup{3.0}, .max_speedup{3.0}, .trials{5}});

    check_relative_performance("Speed-up of 4, predicted (2.0, 2.5)",
                               []() { spin_for(1ms); },
                               []() { spin_for(4ms); },
                               {.min_speedup{2.0}, .max_speedup{2.5}, .trials{5}});
  }

  void performance_false_negative_diagnostics::test_confidence_multiplier()
  {
    using namespace std::chrono_literals;

    check_relative_performance("An interval of 3 standard errors ends below the predicted (11.2, 14.5)",
                               []() { spin_for(1ms); },
                               make_cycling_spinner({2ms, 2ms, 4ms, 5ms, 5ms}),
                               {.min_speedup{11.2}, .max_speedup{14.5}, .trials{5}});

    check_relative_performance("An interval of 3 standard errors starts above the predicted (1.15, 1.5)",
                               []() { spin_for(1ms); },
                               make_cycling_spinner({2ms, 2ms, 3ms, 3ms, 8ms}),
                               {.min_speedup{1.15}, .max_speedup{1.5}, .trials{5}});
  }

  void performance_false_negative_diagnostics::test_significance_gate()
  {
    using namespace std::chrono_literals;

    check_relative_performance("An interval of 3 standard errors includes a speed-up of 1, and overlaps (2.6, 3.17)",
                               make_cycling_spinner({1ms, 1ms, 1ms, 2ms, 2ms}),
                               make_cycling_spinner({4ms, 4ms, 4ms, 3ms, 1ms}),
                               {.min_speedup{2.6}, .max_speedup{3.17}, .trials{5}});
  }

  [[nodiscard]]
  std::filesystem::path performance_false_positive_diagnostics::source_file()
  {
    return std::source_location::current().file_name();
  }

  void performance_false_positive_diagnostics::run_tests()
  {
    test_relative_performance();
    test_confidence_multiplier();
  }

  void performance_false_positive_diagnostics::test_relative_performance()
  {
    using namespace std::chrono_literals;

    check_relative_performance("Speed-up of 2, predicted (1.8, 2.1)",
                               []() { spin_for(1ms); },
                               []() { spin_for(2ms); },
                               {.min_speedup{1.8}, .max_speedup{2.1}, .trials{5}});

    check_relative_performance("Speed-up of 4, predicted (3.4, 4.1)",
                               []() { spin_for(1ms); },
                               []() { spin_for(4ms); },
                               {.min_speedup{3.4}, .max_speedup{4.1}, .trials{5}});

    check_relative_performance("Durations drifting together, a speed-up of 2 in every trial, predicted (1.8, 2.2)",
                               make_cycling_spinner({1ms, 2ms, 3ms,  4ms,  5ms}),
                               make_cycling_spinner({2ms, 4ms, 6ms,  8ms, 10ms}),
                               {.min_speedup{1.8}, .max_speedup{2.2}, .trials{5}});
  }

  void performance_false_positive_diagnostics::test_confidence_multiplier()
  {
    using namespace std::chrono_literals;

    check_relative_performance("Final attempt: an interval of 3 standard errors ends inside (6.57, 8.54)",
                               []() { spin_for(1ms); },
                               make_cycling_spinner({1ms, 2ms, 4ms, 6ms, 6ms}),
                               {.min_speedup{6.57}, .max_speedup{8.54}, .trials{5}});

    check_relative_performance("Final attempt: an interval of 3 standard errors starts inside (1.05, 1.37)",
                               []() { spin_for(1ms); },
                               make_cycling_spinner({1ms, 2ms, 2ms, 6ms, 8ms}),
                               {.min_speedup{1.05}, .max_speedup{1.37}, .trials{5}});
  }

  [[nodiscard]]
  std::filesystem::path performance_utilities_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void performance_utilities_test::run_tests()
  {
    test_postprocessing();
    test_coarse_sleep();
    test_invalid_arguments();
    test_throwing_task();
    test_task_constraints();
  }

  void performance_utilities_test::test_postprocessing()
  {
    {
      std::string_view latest   {"Speed-up: 1.34 in [1.22, 1.47]; predicted (2, 3)\n"};
      std::string_view reference{"Speed-up: 2.01 in [1.98, 2.04]; predicted (2, 3)\n"};

      check(equality, "", postprocess(latest, reference), reference);
    }

    {
      std::string_view latest   {"Speed-up: 1.34 in [1.22, 1.47]; predicted (2, 3)\n"};
      std::string_view reference{"Speed-up: 1.34 in [1.22, 1.47]; predicted (2.1, 3)\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest   {"Speed-up: 1.34 in [1.22, 1.47]; predicted (2, 3)\n"};
      std::string_view reference{"Speed-up: 1.34 in [1.22, 1.47]; predicted (2, 3.1)\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest   {"Speed-up: 1.34 in [1.22, 1.47]; predicted (2, 3)\n"};
      std::string_view reference{"Speed-up: 1.34 in (1.22, 1.47); predicted (2, 3)\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest   {"Speed-up: 1.34 in [1.22, 1.47]; predicted (2, 3)\n"};
      std::string_view reference{"Speed-up: 1.340 in [1.22, 1.47]; predicted (2, 3)\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest   {"Speed-up: 1.34 in [1.22, 1.47]; predicted (2, 3)\n"};
      std::string_view reference{"Speed-up: 1.34 in [1.22, 1.47]; predicted (2, 3) 4 5 6 7\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest   {"Speed-up: 1.34 in [1.22, 1.47]; predicted (2, 3)\n"};
      std::string_view reference{"Speed-up: 1.34 in [1.22]; predicted (2, 3)\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest   {"Trials: 15, attempt 3 of 3\n"};
      std::string_view reference{"Trials: 5, attempt 1 of 3\n"};

      check(equality, "", postprocess(latest, reference), reference);
    }

    {
      std::string_view latest   {"Trials: 5, attempt 1 of 3\n"};
      std::string_view reference{"Trials: 5, attempt 1 of 4\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest   {"Trials: 5, attempt 1 of 3\n"};
      std::string_view reference{"Trials: 05, attempt 1 of 3\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest   {"Trials: 5, attempt 1 of 3\n"};
      std::string_view reference{"Trials: 5.5, attempt 1 of 3\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest   {"Trials: 5, attempt 1 of 3\n"};
      std::string_view reference{"Trials: -5, attempt 1 of 3\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest   {"Task durations: fast 0.005s, slow 0.0067s\n"};
      std::string_view reference{"Task durations: fast 0.00101s, slow 0.00402s\n"};

      check(equality, "", postprocess(latest, reference), reference);
    }

    {
      std::string_view latest   {"Task durations: fast 0.005s, slow 0.0067s\n"};
      std::string_view reference{"Task durations: fast 0.005ms, slow 0.0067s\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest   {"Task durations: fast 0.005s, slow 0.0067s\n"};
      std::string_view reference{"Task durations: slow 0.005s, fast 0.0067s\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest   {"Task durations: fast 0.005s, slow 0.0067s\n"};
      std::string_view reference{"Task durations: fast 0.005s, slow 0.0067s FAILED\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest   {"The fast task is slower than the slow one\n"};
      std::string_view reference{"The fast task is not distinguishably faster than the slow one\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest   {"Line 40\n"
                                 "The fast task is slower than the slow one\n"
                                 "Speed-up: 0.5 in [0.499, 0.501]; predicted (2, 3)\n"
                                 "Trials: 5, attempt 1 of 3\n"
                                 "Task durations: fast 0.002s, slow 0.001s\n"};
      std::string_view reference{"Line 40\n"
                                 "The fast task is slower than the slow one\n"
                                 "Speed-up: 0.497 in [0.495, 0.499]; predicted (2, 3)\n"
                                 "Trials: 10, attempt 2 of 3\n"
                                 "Task durations: fast 0.00201s, slow 0.001s\n"};

      check(equality, "", postprocess(latest, reference), reference);
    }

    {
      std::string_view latest   {"Line 40\n"
                                 "Speed-up: 0.5 in [0.499, 0.501]; predicted (2, 3)\n"};
      std::string_view reference{"Line 41\n"
                                 "Speed-up: 0.497 in [0.495, 0.499]; predicted (2, 3)\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest   {"Speed-up: 1.34 in [1.22, 1.47]; predicted (2, 3)\n"
                                 "Trials: 15, attempt 3 of 3\n"};
      std::string_view reference{"Trials: 15, attempt 3 of 3\n"
                                 "Speed-up: 1.34 in [1.22, 1.47]; predicted (2, 3)\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest   {"Speed-up: 1.34 in [1.22, 1.47]; predicted (2, 3)\n"};
      std::string_view reference{"Speed-up: 1.34 in [1.22, 1.47]; predicted (2, 3)\n"
                                 "Trials: 15, attempt 3 of 3\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest   {"Speed-up: 1.34 in [1.22, 1.47]; predicted (2, 3)\n"};
      std::string_view reference{""};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest   {""};
      std::string_view reference{"Speed-up: 1.34 in [1.22, 1.47]; predicted (2, 3)\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      constexpr auto infinity{std::numeric_limits<double>::infinity()};
      constexpr auto nan{std::numeric_limits<double>::quiet_NaN()};

      const std::string latest   {append_lines(speedup_summary(infinity, 1e+06, -nan, 2, 3), task_durations_summary(9.9e-05, 0))};
      const std::string reference{append_lines(speedup_summary(1.04,     1e-07,  2.5, 2, 3), task_durations_summary(0.001,   nan))};

      check(equality, "", postprocess(latest, reference), std::string_view{reference});
    }

    {
      const std::string latest   {append_lines(trials_summary(15, 3, 3), speedup_summary(1.04, 1.01, 1.07, 2, 3))};
      const std::string reference{append_lines(trials_summary(5,  1, 3), speedup_summary(1.02, 1.00, 1.04, 2, 3))};

      check(equality, "", postprocess(latest, reference), std::string_view{reference});
    }

    {
      const std::string latest   {trials_summary(15, 3, 3)};
      const std::string reference{trials_summary(15, 3, 4)};

      check(equality, "", postprocess(latest, reference), std::string_view{latest});
    }

    {
      const std::string latest   {speedup_summary(1.04, 1.01, 1.07, 2, 3)};
      const std::string reference{speedup_summary(1.04, 1.01, 1.07, 2, 4)};

      check(equality, "", postprocess(latest, reference), std::string_view{latest});
    }
  }

  void performance_utilities_test::test_coarse_sleep()
  {
    using fractional_milliseconds = std::chrono::duration<double, std::milli>;

    constexpr fractional_milliseconds target{5.0};

    check("Rounded up to Windows' default tick", is_coarse_sleep(fractional_milliseconds{15.2}, target));
    check("Exactly twice the target", is_coarse_sleep(fractional_milliseconds{10.0}, target));
    check("Just under twice the target", !is_coarse_sleep(fractional_milliseconds{9.9}, target));
    check("A 1 ms timer resolution in effect", !is_coarse_sleep(fractional_milliseconds{5.4}, target));

    write_to_file(working_materials() /= "CoarseSleepMessage.txt",
                  coarse_sleep_message(fractional_milliseconds{15.2}, target),
                  std::ios_base::out);

    check(equivalence,
          "Message",
          working_materials() /= "CoarseSleepMessage.txt",
          predictive_materials() /= "CoarseSleepMessage.txt");
  }

  void performance_utilities_test::test_invalid_arguments()
  {
    constexpr double nan{std::numeric_limits<double>::quiet_NaN()};
    constexpr std::string_view description{"Relative performance with invalid arguments"};
    test_logger<test_mode::standard> logger{};

    auto relativePerformanceCheck{
      [&logger, description](const relative_performance_parameters& parameters) {
        return [&logger, description, parameters]() {
          return check_relative_performance(description, logger, []() {}, []() {}, parameters);
        };
      }
    };

    check_exception_thrown<std::invalid_argument>("Minimum speed-up of 1",
                                                  relativePerformanceCheck({.min_speedup{1.0},
                                                                            .max_speedup{2.0},
                                                                            .trials{5}}));
    check_exception_thrown<std::invalid_argument>("Maximum speed-up of 1",
                                                  relativePerformanceCheck({.min_speedup{1.5},
                                                                            .max_speedup{1.0},
                                                                            .trials{5}}));
    check_exception_thrown<std::invalid_argument>("Minimum speed-up of NaN",
                                                  relativePerformanceCheck({.min_speedup{nan},
                                                                            .max_speedup{2.0},
                                                                            .trials{5}}));
    check_exception_thrown<std::invalid_argument>("Minimum speed-up exceeding the maximum",
                                                  relativePerformanceCheck({.min_speedup{2.5},
                                                                            .max_speedup{2.0},
                                                                            .trials{5}}));
    check_exception_thrown<std::invalid_argument>("Four trials",
                                                  relativePerformanceCheck({.min_speedup{2.0},
                                                                            .max_speedup{3.0},
                                                                            .trials{4}}));

    const auto& exitInfo{logger.last_check_exit_info()};
    if(check("An invalid argument is attributed to the check refusing it", exitInfo.has_value()))
    {
      check("Exit via an exception", exitInfo->via_exception);
      check(equality, "Message of the check refusing the argument", exitInfo->message, std::string{description});
    }
  }

  void performance_utilities_test::test_throwing_task()
  {
    constexpr std::string_view description{"Relative performance with a throwing task"};
    test_logger<test_mode::standard> logger{};

    auto checkWithThrowingFastTask{
      [&logger, description]() {
        return check_relative_performance(description, logger,
                                          []() { throw std::runtime_error{"Fast task failure"}; },
                                          []() {},
                                          {.min_speedup{2.0},
                                           .max_speedup{3.0},
                                           .trials{5}});
      }
    };

    check_exception_thrown<std::runtime_error>("Fast task throws", checkWithThrowingFastTask);

    const auto& exitInfo{logger.last_check_exit_info()};
    if(check("A throwing task is attributed to its check", exitInfo.has_value()))
    {
      check("Exit via an exception", exitInfo->via_exception);
      check(equality, "Message of the check whose task threw", exitInfo->message, std::string{description});
    }
  }

  void performance_utilities_test::test_task_constraints()
  {
    STATIC_CHECK( profilable<   copyable_task>);
    STATIC_CHECK(!profilable<rvalue_only_task>);

    STATIC_CHECK( checkable_tasks_v<   copyable_task,    copyable_task>);
    STATIC_CHECK(!checkable_tasks_v<  move_only_task,    copyable_task>);
    STATIC_CHECK(!checkable_tasks_v<   copyable_task,   move_only_task>);
    STATIC_CHECK(!checkable_tasks_v<rvalue_only_task,    copyable_task>);
    STATIC_CHECK(!checkable_tasks_v<   copyable_task, rvalue_only_task>);

    STATIC_CHECK( checkable_tasks_by_extender_v<   copyable_task,    copyable_task>);
    STATIC_CHECK(!checkable_tasks_by_extender_v<  move_only_task,    copyable_task>);
    STATIC_CHECK(!checkable_tasks_by_extender_v<   copyable_task,   move_only_task>);
    STATIC_CHECK(!checkable_tasks_by_extender_v<rvalue_only_task,    copyable_task>);
    STATIC_CHECK(!checkable_tasks_by_extender_v<   copyable_task, rvalue_only_task>);
  }
}
