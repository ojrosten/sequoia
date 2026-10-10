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
    test_fail_at_second_attempt();
    test_confidence_multiplier_from_first_attempt();
  }

  void performance_false_negative_diagnostics::test_relative_performance()
  {
    using namespace std::chrono_literals;

    check_relative_performance("Fast task 4 ms, slow task cycling through (2, 3, 4, 5, 6) ms, predicted [1.2, 1.5]",
                               []() { spin_for(4ms); },
                               make_cycling_spinner({2ms, 3ms, 4ms, 5ms, 6ms}),
                               {.prediction{.lower{1.2}, .upper{1.5}}, .minimum_trials{10}});

    check_relative_performance("Fast task 2 ms, slow task 1 ms, predicted [1.5, 2.0]",
                               []() { spin_for(2ms); },
                               []() { spin_for(1ms); },
                               {.prediction{.lower{1.5}, .upper{2.0}}, .minimum_trials{10}});

    check_relative_performance("Speed-up of 2, predicted [3.0, 3.0]",
                               []() { spin_for(1ms); },
                               []() { spin_for(2ms); },
                               {.prediction{.lower{3.0}, .upper{3.0}}, .minimum_trials{10}});

    check_relative_performance("Speed-up of 4, predicted [2.0, 2.5]",
                               []() { spin_for(1ms); },
                               []() { spin_for(4ms); },
                               {.prediction{.lower{2.0}, .upper{2.5}}, .minimum_trials{10}});
  }

  void performance_false_negative_diagnostics::test_confidence_multiplier()
  {
    using namespace std::chrono_literals;

    check_relative_performance("Attempt 1 takes 3 standard errors and needs 3.49 to overlap [3.15, 8.0]; "
                               "later speed-ups lie inside the prediction",
                               counted_spinner{3ms, trials_for_minimum_10[0], 1ms},
                               make_cycling_spinner({4ms, 7ms, 7ms, 7ms, 8ms}),
                               {.prediction{.lower{3.15}, .upper{8.0}}, .minimum_trials{10}});

    check_relative_performance("Attempt 1 takes 3 standard errors and needs 3.5 to overlap [1.05, 2.91]; "
                               "later speed-ups lie inside the prediction",
                               counted_spinner{1ms, trials_for_minimum_10[0], 2ms},
                               make_cycling_spinner({4ms, 4ms, 4ms, 4ms, 8ms}),
                               {.prediction{.lower{1.05}, .upper{2.91}}, .minimum_trials{10}});
  }

  void performance_false_negative_diagnostics::test_significance_gate()
  {
    using namespace std::chrono_literals;

    check_relative_performance("An interval of 3 standard errors includes a speed-up of 1, and overlaps [1.4, 1.8]",
                               []() { spin_for(2ms); },
                               make_cycling_spinner({2ms, 2ms, 3ms, 4ms, 9ms}),
                               {.prediction{.lower{1.4}, .upper{1.8}}, .minimum_trials{10}});
  }

  void performance_false_negative_diagnostics::test_fail_at_second_attempt()
  {
    using namespace std::chrono_literals;

    constexpr relative_performance_parameters parameters{
      .prediction{.lower{1.8}, .upper{2.2}}, .minimum_trials{10}
    };

    check_relative_performance("A speed-up of 2, then of 0.5, then of 2",
                               counted_spinner{1ms, trials_for_minimum_10[0],                        4ms},
                               counted_spinner{2ms, trials_before_attempt(trials_for_minimum_10, 3), 8ms},
                               parameters);
  }

  void performance_false_negative_diagnostics::test_confidence_multiplier_from_first_attempt()
  {
    using namespace std::chrono_literals;

    constexpr relative_performance_parameters parameters{
      .prediction{.lower{2.92}, .upper{4.38}}, .minimum_trials{10}
    };

    check_relative_performance("Attempt 1 overlaps [2.92, 4.38] at 3 standard errors, not at 1; attempt 2 is slower",
                               counted_spinner{1ms, trials_for_minimum_10[0], 4ms},
                               make_cycling_spinner({2ms, 2ms, 3ms, 3ms, 3ms}),
                               parameters);
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
    test_unused_results();
  }

  void performance_false_positive_diagnostics::test_relative_performance()
  {
    using namespace std::chrono_literals;

    check_relative_performance("Speed-up of 2, predicted [1.8, 2.1]",
                               []() { spin_for(1ms); },
                               []() { spin_for(2ms); },
                               {.prediction{.lower{1.8}, .upper{2.1}}, .minimum_trials{10}});

    check_relative_performance("Speed-up of 4, predicted [3.4, 4.1]",
                               []() { spin_for(1ms); },
                               []() { spin_for(4ms); },
                               {.prediction{.lower{3.4}, .upper{4.1}}, .minimum_trials{10}});

    check_relative_performance("Durations varying together, a speed-up of 2 in every trial, predicted [1.8, 2.2]",
                               make_cycling_spinner({1ms,  5ms, 1ms,  5ms, 1ms}),
                               make_cycling_spinner({2ms, 10ms, 2ms, 10ms, 2ms}),
                               {.prediction{.lower{1.8}, .upper{2.2}}, .minimum_trials{10}});
  }

  void performance_false_positive_diagnostics::test_confidence_multiplier()
  {
    using namespace std::chrono_literals;

    check_relative_performance("Attempts 1 and 2 fail at the gate; "
                               "attempt 3 takes 3 standard errors and needs 2.5 to overlap [4.57, 5.94]",
                               counted_spinner{3ms, trials_before_attempt(trials_for_minimum_10, 3), 2ms},
                               make_cycling_spinner({1ms, 6ms, 7ms, 7ms, 7ms}),
                               {.prediction{.lower{4.57}, .upper{5.94}}, .minimum_trials{10}});

    check_relative_performance("Attempt 3 takes 3 standard errors and needs 2.49 to overlap [1.86, 2.42]; "
                               "attempt 2 takes 2 and needs 7.62",
                               counted_spinner{1ms, trials_before_attempt(trials_for_minimum_10, 3), 2ms},
                               make_cycling_spinner({4ms, 4ms, 8ms, 8ms, 8ms}),
                               {.prediction{.lower{1.86}, .upper{2.42}}, .minimum_trials{10}});
  }

  void performance_false_positive_diagnostics::test_unused_results()
  {
    // Nothing but do_not_optimize_away uses the result of either task's
    // computation. Without it, an optimized build removes the computation. A
    // trial then times the tasks at or near zero, and the check fails or
    // throws. An unoptimized build removes nothing, so there these checks
    // pass either way. The seed is drawn at run time, so that no build can
    // compute the result at compile time.
    const std::uint64_t seed{std::random_device{}()};
    constexpr std::size_t steps{500'000};

    check_relative_performance("A speed-up of 2 in computing a value a task passes to do_not_optimize_away, "
                               "predicted [1.8, 2.2]",
                               [seed]() { do_not_optimize_away(xorshift(seed, steps)); },
                               [seed]() { do_not_optimize_away(xorshift(seed, 2 * steps)); },
                               {.prediction{.lower{1.8}, .upper{2.2}}, .minimum_trials{10}});

    check_relative_performance("A speed-up of 2 in computing the value a task returns, predicted [1.8, 2.2]",
                               [seed]() { return xorshift(seed, steps); },
                               [seed]() { return xorshift(seed, 2 * steps); },
                               {.prediction{.lower{1.8}, .upper{2.2}}, .minimum_trials{10}});
  }

  [[nodiscard]]
  std::filesystem::path performance_utilities_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void performance_utilities_test::run_tests()
  {
    test_postprocessing();
    test_zeroed_measurements();
    test_coarse_sleep();
    test_task_constraints();
    test_exception_specifications();
  }

  void performance_utilities_test::test_postprocessing()
  {
    {
      std::string_view latest   {"Speed-up: 1.34 in [1.22, 1.47]; predicted [2, 3]\n"};
      std::string_view reference{"Speed-up: 2.01 in [1.98, 2.04]; predicted [2, 3]\n"};

      check(equality, "", postprocess(latest, reference), reference);
    }

    {
      std::string_view latest   {"Speed-up: 1.34 in [1.22, 1.47]; predicted [2, 3]\n"};
      std::string_view reference{"Speed-up: 1.34 in [1.22, 1.47]; predicted [2.1, 3]\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest   {"Speed-up: 1.34 in [1.22, 1.47]; predicted [2, 3]\n"};
      std::string_view reference{"Speed-up: 1.34 in [1.22, 1.47]; predicted [2, 3.1]\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest   {"Speed-up: 1.34 in [1.22, 1.47]; predicted [2, 3]\n"};
      std::string_view reference{"Speed-up: 1.34 in (1.22, 1.47); predicted [2, 3]\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest   {"Speed-up: 1.34 in [1.22, 1.47]; predicted [2, 3]\n"};
      std::string_view reference{"Speed-up: 1.340 in [1.22, 1.47]; predicted [2, 3]\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest   {"Speed-up: 1.34 in [1.22, 1.47]; predicted [2, 3]\n"};
      std::string_view reference{"Speed-up: 1.34 in [1.22, 1.47]; predicted [2, 3] 4 5 6 7\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest   {"Speed-up: 1.34 in [1.22, 1.47]; predicted [2, 3]\n"};
      std::string_view reference{"Speed-up: 1.34 in [1.22]; predicted [2, 3]\n"};

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
                                 "Speed-up: 0.5 in [0.499, 0.501]; predicted [2, 3]\n"
                                 "Trials: 5, attempt 1 of 3\n"
                                 "Task durations: fast 0.002s, slow 0.001s\n"};
      std::string_view reference{"Line 40\n"
                                 "The fast task is slower than the slow one\n"
                                 "Speed-up: 0.497 in [0.495, 0.499]; predicted [2, 3]\n"
                                 "Trials: 10, attempt 2 of 3\n"
                                 "Task durations: fast 0.00201s, slow 0.001s\n"};

      check(equality, "", postprocess(latest, reference), reference);
    }

    {
      std::string_view latest   {"Line 40\n"
                                 "Speed-up: 0.5 in [0.499, 0.501]; predicted [2, 3]\n"};
      std::string_view reference{"Line 41\n"
                                 "Speed-up: 0.497 in [0.495, 0.499]; predicted [2, 3]\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest   {"Speed-up: 1.34 in [1.22, 1.47]; predicted [2, 3]\n"
                                 "Trials: 15, attempt 3 of 3\n"};
      std::string_view reference{"Trials: 15, attempt 3 of 3\n"
                                 "Speed-up: 1.34 in [1.22, 1.47]; predicted [2, 3]\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest   {"Speed-up: 1.34 in [1.22, 1.47]; predicted [2, 3]\n"};
      std::string_view reference{"Speed-up: 1.34 in [1.22, 1.47]; predicted [2, 3]\n"
                                 "Trials: 15, attempt 3 of 3\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest   {"Speed-up: 1.34 in [1.22, 1.47]; predicted [2, 3]\n"};
      std::string_view reference{""};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest   {""};
      std::string_view reference{"Speed-up: 1.34 in [1.22, 1.47]; predicted [2, 3]\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      constexpr auto infinity{std::numeric_limits<double>::infinity()};
      constexpr auto nan{std::numeric_limits<double>::quiet_NaN()};
      using fractional_seconds = std::chrono::duration<double>;

      const std::string latest{
        append_lines(speedup_summary({infinity, {1e+06, -nan}}, {2, 3}),
                     task_durations_summary({fractional_seconds{9.9e-05}, fractional_seconds{0}}))
      };

      const std::string reference{
        append_lines(speedup_summary({1.04,     {1e-07,  2.5}}, {2, 3}),
                     task_durations_summary({fractional_seconds{0.001},   fractional_seconds{nan}}))
      };

      check(equality, "", postprocess(latest, reference), std::string_view{reference});
    }

    {
      const std::string latest{
        append_lines(trials_summary(15, {3, 3}), speedup_summary({1.04, {1.01, 1.07}}, {2, 3}))
      };

      const std::string reference{
        append_lines(trials_summary(5,  {1, 3}), speedup_summary({1.02, {1.00, 1.04}}, {2, 3}))
      };

      check(equality, "", postprocess(latest, reference), std::string_view{reference});
    }

    {
      const std::string latest   {trials_summary(15, {3, 3})};
      const std::string reference{trials_summary(15, {3, 4})};

      check(equality, "", postprocess(latest, reference), std::string_view{latest});
    }

    {
      const std::string latest   {speedup_summary({1.04, {1.01, 1.07}}, {2, 3})};
      const std::string reference{speedup_summary({1.04, {1.01, 1.07}}, {2, 4})};

      check(equality, "", postprocess(latest, reference), std::string_view{latest});
    }

    {
      const std::string latest   {std::format("Line 40\n\n{}", speedup_summary({1.04, {1.01, 1.07}}, {2, 3}))};
      const std::string reference{std::format("Line 40\n{}",   speedup_summary({1.02, {1.00, 1.04}}, {2, 3}))};

      check(equality, "", postprocess(latest, reference), std::string_view{latest});
    }

    {
      const std::string latest   {std::format("{}\n", speedup_summary({1.04, {1.01, 1.07}}, {2, 3}))};
      const std::string reference{std::format("{}",   speedup_summary({1.02, {1.00, 1.04}}, {2, 3}))};

      check(equality, "", postprocess(latest, reference), std::string_view{latest});
    }

    {
      using fractional_seconds = std::chrono::duration<double>;
      const indentation nested{"  "};

      const std::string latest{
        indent(append_lines(speedup_summary({1.04, {1.01, 1.07}}, {2, 3}),
                            trials_summary(15, {3, 3}),
                            task_durations_summary({fractional_seconds{0.001},  fractional_seconds{0.00104}})),
               nested)
      };

      const std::string reference{
        indent(append_lines(speedup_summary({1.02, {1.00, 1.04}}, {2, 3}),
                            trials_summary(5,  {1, 3}),
                            task_durations_summary({fractional_seconds{0.0011}, fractional_seconds{0.00112}})),
               nested)
      };

      check(equality, "", postprocess(latest, reference), std::string_view{reference});
    }

    {
      const std::string latest   {indent(speedup_summary({1.04, {1.01, 1.07}}, {2, 3}), indentation{"  "})};
      const std::string reference{indent(speedup_summary({1.04, {1.01, 1.07}}, {2, 3}), indentation{"    "})};

      check(equality, "", postprocess(latest, reference), std::string_view{latest});
    }
  }

  void performance_utilities_test::test_zeroed_measurements()
  {
    using fractional_seconds = std::chrono::duration<double>;

    check(equality,
          "The speed-up and the interval around it are zeroed, and the predicted interval kept",
          text_with_zeroed_measurements(speedup_summary({1.04, {1.01, 1.07}}, {2, 3})),
          speedup_summary({}, {2, 3}));

    check(equality,
          "The number of trials and the attempt are zeroed, and the number of attempts allowed kept",
          text_with_zeroed_measurements(trials_summary(15, {3, 3})),
          trials_summary(0, {0, 3}));

    check(equality,
          "The task durations are zeroed",
          text_with_zeroed_measurements(task_durations_summary({fractional_seconds{0.001}, fractional_seconds{0.002}})),
          task_durations_summary({}));
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

  void performance_utilities_test::test_task_constraints()
  {
    STATIC_CHECK( profilable<   copyable_task>);
    STATIC_CHECK(!profilable<rvalue_only_task>);

    STATIC_CHECK( checkable_tasks_v<   copyable_task,    copyable_task>);
    STATIC_CHECK(!checkable_tasks_v<  move_only_task,    copyable_task>);
    STATIC_CHECK(!checkable_tasks_v<   copyable_task,   move_only_task>);
    STATIC_CHECK(!checkable_tasks_v<rvalue_only_task,    copyable_task>);
    STATIC_CHECK(!checkable_tasks_v<   copyable_task, rvalue_only_task>);
  }

  void performance_utilities_test::test_exception_specifications()
  {
    STATIC_CHECK(noexcept(do_not_optimize_away(0)));
  }
}
