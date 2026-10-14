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
#include <cmath>
#include <format>
#include <initializer_list>
#include <limits>
#include <memory>
#include <string_view>
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

    using copyable_task_with_baseline = task_with_baseline<copyable_task, copyable_task>;

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

    template<class Fast, class FastBaseline, class Slow, class SlowBaseline>
    inline constexpr bool checkable_with_baselines_v{
      requires(performance_extender<test_mode::standard>& extender,
               Fast fast,
               FastBaseline fastBaseline,
               Slow slow,
               SlowBaseline slowBaseline,
               const relative_performance_parameters& parameters) {
        extender.check_relative_performance(
          "",
          task_with_baseline{.task = std::move(fast), .baseline = std::move(fastBaseline)},
          task_with_baseline{.task = std::move(slow), .baseline = std::move(slowBaseline)},
          parameters
        );
      }
    };
  }

  [[nodiscard]]
  std::filesystem::path performance_false_negative_diagnostics::source_file()
  {
    return std::source_location::current().file_name();
  }

  [[nodiscard]]
  log_summary performance_false_negative_diagnostics::summarize(duration delta) const
  {
    return performance_false_negative_test::summarize(delta, attempt_count_stability::typically_stable);
  }

  void performance_false_negative_diagnostics::run_tests()
  {
    test_relative_performance();
    test_confidence_multiplier();
    test_significance_gate();
    test_fail_at_second_attempt();
    test_confidence_multiplier_from_first_attempt();
    test_relative_performance_with_baselines();
    test_tasks_no_longer_than_baselines();
    test_fail_at_second_attempt_with_baselines();
  }

  void performance_false_negative_diagnostics::test_relative_performance()
  {
    using namespace std::chrono_literals;

    check_relative_performance("Fast task 4 ms, slow task cycling through (2, 3, 4, 5, 6) ms, predicted [1.2, 1.5]",
                               []() { spin_for(4ms); },
                               make_cycling_spinner({2ms, 3ms, 4ms, 5ms, 6ms}),
                               {.prediction{.lower{1.2}, .upper{1.5}}});

    check_relative_performance("Fast task 2 ms, slow task 1 ms, predicted [1.5, 2.0]",
                               []() { spin_for(2ms); },
                               []() { spin_for(1ms); },
                               {.prediction{.lower{1.5}, .upper{2.0}}});

    check_relative_performance("Speed-up of 2, predicted [3.0, 3.0]",
                               []() { spin_for(1ms); },
                               []() { spin_for(2ms); },
                               {.prediction{.lower{3.0}, .upper{3.0}}});

    check_relative_performance("Speed-up of 4, predicted [2.0, 2.5]",
                               []() { spin_for(1ms); },
                               []() { spin_for(4ms); },
                               {.prediction{.lower{2.0}, .upper{2.5}}});
  }

  void performance_false_negative_diagnostics::test_confidence_multiplier()
  {
    using namespace std::chrono_literals;

    check_relative_performance("Attempt 1's interval lies below the prediction, "
                               "and would overlap the prediction only if substantially wider; "
                               "later speed-ups lie inside the prediction",
                               counted_spinner{5ms, trials_for_minimum_10[0], 1ms},
                               make_cycling_spinner({5ms, 30ms, 40ms, 40ms, 40ms}),
                               {.prediction{.lower{20.4}, .upper{41.1}}});

    check_relative_performance("Attempt 1's interval lies above the prediction, "
                               "and would overlap the prediction only if substantially wider; "
                               "later speed-ups lie inside the prediction",
                               counted_spinner{8ms, trials_for_minimum_10[0], 20ms},
                               make_cycling_spinner({30ms, 30ms, 30ms, 60ms, 80ms}),
                               {.prediction{.lower{1.7}, .upper{2.62}}});

    check_relative_performance("Attempt 1's interval overlaps the prediction, "
                               "but would lie above the prediction if substantially narrower; "
                               "attempt 2 is slower",
                               make_cycling_spinner({3ms, 4ms, 12ms, 12ms, 12ms}),
                               counted_spinner{50ms, trials_for_minimum_10[0], 1ms},
                               {.prediction{.lower{2.34}, .upper{3.66}}});
  }

  void performance_false_negative_diagnostics::test_significance_gate()
  {
    using namespace std::chrono_literals;

    check_relative_performance("Attempt 1's interval includes a speed-up of 1, and overlaps the prediction",
                               []() { spin_for(2ms); },
                               make_cycling_spinner({2ms, 2ms, 3ms, 4ms, 9ms}),
                               {.prediction{.lower{1.4}, .upper{1.8}}});
  }

  void performance_false_negative_diagnostics::test_fail_at_second_attempt()
  {
    using namespace std::chrono_literals;

    constexpr relative_performance_parameters parameters{.prediction{.lower{1.8}, .upper{2.2}}};

    check_relative_performance("A speed-up of 2, then of 0.5, then of 2",
                               counted_spinner{1ms, trials_for_minimum_10[0],                        4ms},
                               counted_spinner{2ms, trials_before_attempt(trials_for_minimum_10, 3), 8ms},
                               parameters);
  }

  void performance_false_negative_diagnostics::test_confidence_multiplier_from_first_attempt()
  {
    using namespace std::chrono_literals;

    constexpr relative_performance_parameters parameters{.prediction{.lower{2.92}, .upper{4.38}}};

    check_relative_performance("Attempt 1's interval overlaps the prediction, "
                               "but would lie below the prediction if substantially narrower; "
                               "attempt 2 is slower",
                               counted_spinner{1ms, trials_for_minimum_10[0], 4ms},
                               make_cycling_spinner({2ms, 2ms, 3ms, 3ms, 3ms}),
                               parameters);
  }

  void performance_false_negative_diagnostics::test_relative_performance_with_baselines()
  {
    using namespace std::chrono_literals;

    check_relative_performance("Tasks of 4 ms and 16 ms, with baselines of 1 ms and 10 ms, predicted [3.6, 4.4]",
                               task_with_baseline{.task     = []() { spin_for(4ms); },
                                                  .baseline = []() { spin_for(1ms); }},
                               task_with_baseline{.task     = []() { spin_for(16ms); },
                                                  .baseline = []() { spin_for(10ms); }},
                               {.prediction{.lower{3.6}, .upper{4.4}}});

    check_relative_performance("Tasks of 7 ms and 13 ms, without baselines, predicted [3.6, 4.4]",
                               []() { spin_for(7ms); },
                               []() { spin_for(13ms); },
                               {.prediction{.lower{3.6}, .upper{4.4}}});
  }

  void performance_false_negative_diagnostics::test_tasks_no_longer_than_baselines()
  {
    using namespace std::chrono_literals;

    // Net of the baselines, the tasks take 3 ms and 9 ms, except in the
    // trials in which a baseline spins for longer than its task. The first
    // attempt has ten trials, so the trim drops one trial from each end.
    constexpr relative_performance_parameters parameters{.prediction{.lower{2.5}, .upper{3.5}}};

    check_relative_performance(
      "Two trials in ten in which the fast task takes no longer than its baseline",
      task_with_baseline{.task     = []() { spin_for(4ms); },
                         .baseline = make_cycling_spinner({1ms, 1ms, 7ms, 1ms, 1ms, 1ms, 7ms, 1ms, 1ms, 1ms})},
      task_with_baseline{.task = []() { spin_for(10ms); }, .baseline = []() { spin_for(1ms); }},
      parameters
    );

    check_relative_performance(
      "Two trials in ten in which the slow task takes no longer than its baseline",
      task_with_baseline{.task = []() { spin_for(4ms); }, .baseline = []() { spin_for(1ms); }},
      task_with_baseline{.task     = []() { spin_for(10ms); },
                         .baseline = make_cycling_spinner({1ms, 1ms, 13ms, 1ms, 1ms, 1ms, 13ms, 1ms, 1ms, 1ms})},
      parameters
    );

    check_relative_performance(
      "One trial in ten in which both tasks take no longer than their baselines, "
      "and another in which the fast task does",
      task_with_baseline{.task     = []() { spin_for(4ms); },
                         .baseline = make_cycling_spinner({1ms, 1ms, 7ms, 1ms, 1ms, 1ms, 7ms, 1ms, 1ms, 1ms})},
      task_with_baseline{.task     = []() { spin_for(10ms); },
                         .baseline = make_cycling_spinner({1ms, 1ms, 13ms, 1ms, 1ms, 1ms, 1ms, 1ms, 1ms, 1ms})},
      parameters
    );

    check_relative_performance(
      "One trial in ten in which both tasks take no longer than their baselines, "
      "and another in which the slow task does",
      task_with_baseline{.task     = []() { spin_for(4ms); },
                         .baseline = make_cycling_spinner({1ms, 1ms, 7ms, 1ms, 1ms, 1ms, 1ms, 1ms, 1ms, 1ms})},
      task_with_baseline{.task     = []() { spin_for(10ms); },
                         .baseline = make_cycling_spinner({1ms, 1ms, 13ms, 1ms, 1ms, 1ms, 13ms, 1ms, 1ms, 1ms})},
      parameters
    );
  }

  void performance_false_negative_diagnostics::test_fail_at_second_attempt_with_baselines()
  {
    using namespace std::chrono_literals;

    constexpr relative_performance_parameters parameters{.prediction{.lower{1.8}, .upper{2.2}}};

    auto baseline{[]() { spin_for(1ms); }};

    check_relative_performance(
      "Baselines of 1 ms; net of them, a speed-up of 2, then of 0.5, then of 2",
      task_with_baseline{.task = counted_spinner{4ms, trials_for_minimum_10[0], 13ms}, .baseline = baseline},
      task_with_baseline{.task = counted_spinner{7ms, trials_before_attempt(trials_for_minimum_10, 3), 25ms},
                         .baseline = baseline},
      parameters
    );
  }

  [[nodiscard]]
  std::filesystem::path performance_false_positive_diagnostics::source_file()
  {
    return std::source_location::current().file_name();
  }

  [[nodiscard]]
  log_summary performance_false_positive_diagnostics::summarize(duration delta) const
  {
    return performance_false_positive_test::summarize(delta, attempt_count_stability::typically_stable);
  }

  void performance_false_positive_diagnostics::run_tests()
  {
    test_relative_performance();
    test_confidence_multiplier();
    test_computations_not_optimized_away();
    test_relative_performance_with_baselines();
    test_tasks_no_longer_than_baselines();
    test_baselines_doing_nothing();
  }

  void performance_false_positive_diagnostics::test_relative_performance()
  {
    using namespace std::chrono_literals;

    check_relative_performance("Speed-up of 2, predicted [1.8, 2.1]",
                               []() { spin_for(1ms); },
                               []() { spin_for(2ms); },
                               {.prediction{.lower{1.8}, .upper{2.1}}});

    check_relative_performance("Speed-up of 4, predicted [3.4, 4.1]",
                               []() { spin_for(1ms); },
                               []() { spin_for(4ms); },
                               {.prediction{.lower{3.4}, .upper{4.1}}});

    check_relative_performance("Durations varying together, a speed-up of 2 in every trial, predicted [1.8, 2.2]",
                               make_cycling_spinner({1ms,  5ms, 1ms,  5ms, 1ms}),
                               make_cycling_spinner({2ms, 10ms, 2ms, 10ms, 2ms}),
                               {.prediction{.lower{1.8}, .upper{2.2}}});
  }

  void performance_false_positive_diagnostics::test_confidence_multiplier()
  {
    using namespace std::chrono_literals;

    check_relative_performance("Attempts 1 and 2 fail at the gate; "
                               "attempt 3's interval overlaps the prediction, "
                               "but would not if substantially narrower",
                               counted_spinner{8ms, trials_before_attempt(trials_for_minimum_10, 3), 5ms},
                               make_cycling_spinner({4ms, 6ms, 20ms, 30ms, 30ms}),
                               {.prediction{.lower{4.86}, .upper{7.43}}});

    check_relative_performance("Attempt 2's interval lies above the prediction, "
                               "and would overlap the prediction only if substantially wider; "
                               "attempt 3's interval overlaps the prediction, "
                               "but would not if substantially narrower",
                               counted_spinner{1ms, trials_before_attempt(trials_for_minimum_10, 3), 2ms},
                               make_cycling_spinner({5ms, 5ms, 10ms, 40ms, 40ms}),
                               {.prediction{.lower{2.21}, .upper{3.54}}});
  }

  void performance_false_positive_diagnostics::test_computations_not_optimized_away()
  {
    // Nothing but do_not_optimize_away uses the result of either task's
    // computation. Without do_not_optimize_away, an optimized build removes
    // the computation. A trial then times the tasks at or near zero, and the
    // check fails or throws. An unoptimized build removes nothing, so in such
    // a build these checks pass with or without do_not_optimize_away. The
    // seed is drawn at run time, so that no build can compute the result at
    // compile time.
    const std::uint64_t seed{std::random_device{}()};
    constexpr std::size_t steps{500'000};

    check_relative_performance("A speed-up of 2 in computing a value a task passes to do_not_optimize_away, "
                               "predicted [1.5, 2.5]",
                               [seed]() { do_not_optimize_away(xorshift_state(seed,     steps)); },
                               [seed]() { do_not_optimize_away(xorshift_state(seed, 2 * steps)); },
                               {.prediction{.lower{1.5}, .upper{2.5}}});

    check_relative_performance("A speed-up of 2 in computing the value a task returns, predicted [1.5, 2.5]",
                               [seed]() { return xorshift_state(seed,     steps); },
                               [seed]() { return xorshift_state(seed, 2 * steps); },
                               {.prediction{.lower{1.5}, .upper{2.5}}});
  }

  void performance_false_positive_diagnostics::test_relative_performance_with_baselines()
  {
    using namespace std::chrono_literals;

    check_relative_performance("Tasks of 7 ms and 13 ms, with baselines of 4 ms and 1 ms, predicted [3.6, 4.4]",
                               task_with_baseline{.task     = []() { spin_for(7ms); },
                                                  .baseline = []() { spin_for(4ms); }},
                               task_with_baseline{.task     = []() { spin_for(13ms); },
                                                  .baseline = []() { spin_for(1ms); }},
                               {.prediction{.lower{3.6}, .upper{4.4}}});
  }

  void performance_false_positive_diagnostics::test_tasks_no_longer_than_baselines()
  {
    using namespace std::chrono_literals;

    // Net of the baselines, the tasks take 3 ms and 9 ms, except in the
    // trials in which a baseline spins for longer than its task. The first
    // attempt has ten trials, so the trim drops one trial from each end.
    constexpr relative_performance_parameters parameters{.prediction{.lower{2.5}, .upper{3.5}}};

    check_relative_performance(
      "One trial in ten in which the fast task takes no longer than its baseline",
      task_with_baseline{.task     = []() { spin_for(4ms); },
                         .baseline = make_cycling_spinner({1ms, 1ms, 7ms, 1ms, 1ms, 1ms, 1ms, 1ms, 1ms, 1ms})},
      task_with_baseline{.task = []() { spin_for(10ms); }, .baseline = []() { spin_for(1ms); }},
      parameters
    );

    check_relative_performance(
      "One trial in ten in which the slow task takes no longer than its baseline",
      task_with_baseline{.task = []() { spin_for(4ms); }, .baseline = []() { spin_for(1ms); }},
      task_with_baseline{.task     = []() { spin_for(10ms); },
                         .baseline = make_cycling_spinner({1ms, 1ms, 13ms, 1ms, 1ms, 1ms, 1ms, 1ms, 1ms, 1ms})},
      parameters
    );

    check_relative_performance(
      "One trial in ten in which the fast task takes no longer than its baseline, "
      "and another in which the slow task does",
      task_with_baseline{.task     = []() { spin_for(4ms); },
                         .baseline = make_cycling_spinner({1ms, 1ms, 7ms, 1ms, 1ms, 1ms, 1ms, 1ms, 1ms, 1ms})},
      task_with_baseline{.task     = []() { spin_for(10ms); },
                         .baseline = make_cycling_spinner({1ms, 1ms, 1ms, 1ms, 1ms, 1ms, 13ms, 1ms, 1ms, 1ms})},
      parameters
    );

    check_relative_performance(
      "One trial in ten in which both tasks take no longer than their baselines",
      task_with_baseline{.task     = []() { spin_for(4ms); },
                         .baseline = make_cycling_spinner({1ms, 1ms, 7ms, 1ms, 1ms, 1ms, 1ms, 1ms, 1ms, 1ms})},
      task_with_baseline{.task     = []() { spin_for(10ms); },
                         .baseline = make_cycling_spinner({1ms, 1ms, 13ms, 1ms, 1ms, 1ms, 1ms, 1ms, 1ms, 1ms})},
      parameters
    );
  }

  void performance_false_positive_diagnostics::test_baselines_doing_nothing()
  {
    using namespace std::chrono_literals;

    check_relative_performance("Tasks of 4 ms and 12 ms, with baselines which do nothing, predicted [2.5, 3.5]",
                               task_with_baseline{.task = []() { spin_for(4ms); },  .baseline = []() {}},
                               task_with_baseline{.task = []() { spin_for(12ms); }, .baseline = []() {}},
                               {.prediction{.lower{2.5}, .upper{3.5}}});
  }

  [[nodiscard]]
  std::filesystem::path performance_utilities_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void performance_utilities_test::run_tests()
  {
    test_task_constraints();
    test_exception_specifications();
    test_default_minimum_trials();
    test_judgement_with_baselines();
    test_shuffled_task_orders_with_baselines();
    test_postprocessing();
    test_zeroed_measurements();
    test_coarse_sleep();
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

    STATIC_CHECK( checkable_with_baselines_v<   copyable_task,    copyable_task,    copyable_task,    copyable_task>);
    STATIC_CHECK(!checkable_with_baselines_v<  move_only_task,    copyable_task,    copyable_task,    copyable_task>);
    STATIC_CHECK(!checkable_with_baselines_v<   copyable_task,   move_only_task,    copyable_task,    copyable_task>);
    STATIC_CHECK(!checkable_with_baselines_v<   copyable_task,    copyable_task,   move_only_task,    copyable_task>);
    STATIC_CHECK(!checkable_with_baselines_v<   copyable_task,    copyable_task,    copyable_task,   move_only_task>);
    STATIC_CHECK(!checkable_with_baselines_v<rvalue_only_task,    copyable_task,    copyable_task,    copyable_task>);
    STATIC_CHECK(!checkable_with_baselines_v<   copyable_task, rvalue_only_task,    copyable_task,    copyable_task>);
    STATIC_CHECK(!checkable_with_baselines_v<   copyable_task,    copyable_task, rvalue_only_task,    copyable_task>);
    STATIC_CHECK(!checkable_with_baselines_v<   copyable_task,    copyable_task,    copyable_task, rvalue_only_task>);

    STATIC_CHECK(!checkable_tasks_v<              copyable_task, copyable_task_with_baseline>);
    STATIC_CHECK(!checkable_tasks_v<copyable_task_with_baseline,               copyable_task>);
  }

  void performance_utilities_test::test_exception_specifications()
  {
    STATIC_CHECK(noexcept(do_not_optimize_away(0)));
  }

  void performance_utilities_test::test_default_minimum_trials()
  {
    constexpr relative_performance_parameters parameters{.prediction{.lower{2.0}, .upper{3.0}}};

    STATIC_CHECK(parameters.minimum_trials == relative_performance_minimum_trials_floor);
  }

  void performance_utilities_test::test_judgement_with_baselines()
  {
    using namespace std::chrono_literals;
    using durations_with_baselines = impl::relative_performance_durations_with_baselines;
    using enum relative_performance_failure;

    constexpr relative_performance_interval prediction{.lower{7.5}, .upper{8.5}};
    constexpr double tolerance{1e-12};

    // The trim drops one trial from each end of the floor's number of
    // trials, and two from each end of fifteen.
    constexpr std::size_t trialsAtFloor                  {relative_performance_minimum_trials_floor},
                          trialsWithTwoTrimmedFromEachEnd{15};

    // Net of the baselines, the speed-up is 8. Without them it is 3. With the
    // baselines exchanged it is 3.5; with the fast baseline taken from both
    // tasks it is 7, and with the slow baseline taken from both it is 4.
    constexpr durations_with_baselines typical{.fast{.task{3s}, .baseline{2s}}, .slow{.task{9s}, .baseline{1s}}};

    // A task shorter than its baseline has a negative duration net of the
    // baseline. If both tasks are shorter than their baselines, the ratio of
    // the slow task's net duration to the fast task's is positive, and the
    // trial still counts at each end.
    constexpr durations_with_baselines
      fastAsLongAsBaseline    {.fast{.task{2s}, .baseline{2s}}, .slow{typical.slow}},
      slowAsLongAsBaseline    {.fast{typical.fast},             .slow{.task{2s},  .baseline{2s}}},
      bothAsLongAsBaselines   {.fast{.task{2s}, .baseline{2s}}, .slow{.task{2s},  .baseline{2s}}},
      fastShorterThanBaseline {.fast{.task{1s}, .baseline{2s}}, .slow{typical.slow}},
      slowShorterThanBaseline {.fast{typical.fast},             .slow{.task{1s},  .baseline{3s}}},
      bothShorterThanBaselines{.fast{.task{1s}, .baseline{2s}}, .slow{.task{1s},  .baseline{3s}}},
      speedupOf4              {.fast{typical.fast},             .slow{.task{5s},  .baseline{1s}}},
      speedupOf16             {.fast{typical.fast},             .slow{.task{17s}, .baseline{1s}}};

    auto judge{
      [prediction](std::span<const durations_with_baselines> trialDurations) {
        return impl::judge_attempt(trialDurations, 1.0, prediction);
      }
    };

    // Judges as many trials as the floor: those of `atypical`, followed by
    // typical ones.
    auto judgeAtFloor{
      [judge, typical](std::initializer_list<durations_with_baselines> atypical) {
        return judge(trials_with_atypical(trialsAtFloor, typical, atypical));
      }
    };

    auto checkSpeedup{
      [this, judgeAtFloor, tolerance](const reporter& description,
                                      std::initializer_list<durations_with_baselines> atypical,
                                      double speedup) {
        const auto outcome{judgeAtFloor(atypical)};
        if(check(description, outcome.estimate.has_value()))
        {
          check(within_tolerance{tolerance}, description, outcome.estimate->speedup, speedup);
        }
      }
    };

    {
      const auto outcome{judgeAtFloor({})};

      check("No failure net of the baselines", !outcome.failure.has_value());

      if(check("An estimate net of the baselines", outcome.estimate.has_value()))
      {
        check(within_tolerance{tolerance}, "The speed-up net of the baselines", outcome.estimate->speedup, 8.0);
      }

      check(within_tolerance{tolerance}, "The fast task's own duration", outcome.task_durations.fast.count(), 3.0);
      check(within_tolerance{tolerance}, "The slow task's own duration", outcome.task_durations.slow.count(), 9.0);

      if(check("The baseline durations are given", outcome.baseline_durations.has_value()))
      {
        check(within_tolerance{tolerance},
              "The fast baseline's duration",
              outcome.baseline_durations->fast.count(),
              2.0);
        check(within_tolerance{tolerance},
              "The slow baseline's duration",
              outcome.baseline_durations->slow.count(),
              1.0);
      }
    }

    // `relation` is "as long as" or "shorter than". Every finite speed-up is
    // 4, 8 or 16, which is 2 to the power 2, 3 or 4. So each expected
    // speed-up is 2 to the power of the mean of the exponents the trim keeps.
    auto checkJudgementsWithTasksNoLongerThanBaselines{
      [&](std::string_view relation,
          durations_with_baselines fastNoLongerThanBaseline,
          durations_with_baselines slowNoLongerThanBaseline,
          durations_with_baselines bothNoLongerThanBaselines) {
        checkSpeedup(std::format("The trim drops a trial with a fast task {} its baseline, "
                                 "and the lowest speed-up",
                                 relation),
                     {fastNoLongerThanBaseline, speedupOf4},
                     8.0);

        checkSpeedup(std::format("The trim drops a trial with a fast task {} its baseline, "
                                 "and keeps the highest speed-up",
                                 relation),
                     {fastNoLongerThanBaseline, speedupOf16},
                     std::exp2(25.0 / 8.0));

        checkSpeedup(std::format("The trim drops a trial with a slow task {} its baseline, "
                                 "and the highest speed-up",
                                 relation),
                     {slowNoLongerThanBaseline, speedupOf16},
                     8.0);

        checkSpeedup(std::format("The trim drops a trial with a slow task {} its baseline, "
                                 "and keeps the lowest speed-up",
                                 relation),
                     {slowNoLongerThanBaseline, speedupOf4},
                     std::exp2(23.0 / 8.0));

        checkSpeedup(std::format("The trim drops one trial with each task {} its baseline", relation),
                     {fastNoLongerThanBaseline, slowNoLongerThanBaseline},
                     8.0);

        checkSpeedup(std::format("A trial with both tasks {} their baselines fills one trimmed place at "
                                 "each end, so the trim keeps the lowest and highest speed-ups",
                                 relation),
                     {bothNoLongerThanBaselines, speedupOf4, speedupOf16},
                     8.0);

        checkSpeedup(std::format("A trial with both tasks {} their baselines fills one trimmed place at "
                                 "each end, so the trim keeps the highest speed-up",
                                 relation),
                     {bothNoLongerThanBaselines, speedupOf16},
                     std::exp2(28.0 / 9.0));

        {
          const auto outcome{judgeAtFloor({fastNoLongerThanBaseline, fastNoLongerThanBaseline})};

          check(std::format("A fast task {} its baseline in more trials than the trim drops", relation),
                outcome.failure == fast_task_too_often_no_longer_than_baseline);
          check(std::format("No estimate with a fast task too often {} its baseline", relation),
                !outcome.estimate.has_value());
          check(std::format("The baseline durations are given with a fast task too often {} its baseline", relation),
                outcome.baseline_durations.has_value());
        }

        {
          const auto outcome{judgeAtFloor({slowNoLongerThanBaseline, slowNoLongerThanBaseline})};

          check(std::format("A slow task {} its baseline in more trials than the trim drops", relation),
                outcome.failure == slow_task_too_often_no_longer_than_baseline);
          check(std::format("No estimate with a slow task too often {} its baseline", relation),
                !outcome.estimate.has_value());
        }

        check(std::format("A trial with both tasks {} their baselines counts against the fast task", relation),
              judgeAtFloor({bothNoLongerThanBaselines, fastNoLongerThanBaseline}).failure
                == fast_task_too_often_no_longer_than_baseline);

        check(std::format("A trial with both tasks {} their baselines counts against the slow task", relation),
              judgeAtFloor({bothNoLongerThanBaselines, slowNoLongerThanBaseline}).failure
                == slow_task_too_often_no_longer_than_baseline);

        check(std::format("The fast task's reason takes precedence, with tasks {} their baselines", relation),
              judgeAtFloor({fastNoLongerThanBaseline,
                            fastNoLongerThanBaseline,
                            slowNoLongerThanBaseline,
                            slowNoLongerThanBaseline}).failure
                == fast_task_too_often_no_longer_than_baseline);
      }
    };

    checkJudgementsWithTasksNoLongerThanBaselines("as long as",
                                                  fastAsLongAsBaseline,
                                                  slowAsLongAsBaseline,
                                                  bothAsLongAsBaselines);

    checkJudgementsWithTasksNoLongerThanBaselines("shorter than",
                                                  fastShorterThanBaseline,
                                                  slowShorterThanBaseline,
                                                  bothShorterThanBaselines);

    check("Two trials in fifteen with a fast task as long as its baseline lie within the trim",
          !judge(trials_with_atypical(trialsWithTwoTrimmedFromEachEnd,
                                      typical,
                                      {fastAsLongAsBaseline, fastAsLongAsBaseline})).failure.has_value());

    check("Three trials in fifteen with a fast task as long as its baseline exceed the trim",
          judge(trials_with_atypical(trialsWithTwoTrimmedFromEachEnd,
                                     typical,
                                     {fastAsLongAsBaseline, fastAsLongAsBaseline, fastAsLongAsBaseline})).failure
            == fast_task_too_often_no_longer_than_baseline);

    check("Two trials in fifteen with a slow task as long as its baseline lie within the trim",
          !judge(trials_with_atypical(trialsWithTwoTrimmedFromEachEnd,
                                      typical,
                                      {slowAsLongAsBaseline, slowAsLongAsBaseline})).failure.has_value());

    check("Three trials in fifteen with a slow task as long as its baseline exceed the trim",
          judge(trials_with_atypical(trialsWithTwoTrimmedFromEachEnd,
                                     typical,
                                     {slowAsLongAsBaseline, slowAsLongAsBaseline, slowAsLongAsBaseline})).failure
            == slow_task_too_often_no_longer_than_baseline);

    {
      constexpr durations_with_baselines
        noFastBaseline{.fast{.task{3s}, .baseline{0s}}, .slow{typical.slow}},
        noSlowBaseline{.fast{typical.fast},             .slow{.task{9s}, .baseline{0s}}};

      const auto withoutFastBaseline{judge(trials_with_atypical(trialsAtFloor, noFastBaseline, {}))},
                 withoutSlowBaseline{judge(trials_with_atypical(trialsAtFloor, noSlowBaseline, {}))};

      if(check("An estimate with a fast baseline of zero", withoutFastBaseline.estimate.has_value()))
      {
        check(within_tolerance{tolerance},
              "The speed-up with a fast baseline of zero",
              withoutFastBaseline.estimate->speedup,
              8.0 / 3.0);
      }

      if(check("An estimate with a slow baseline of zero", withoutSlowBaseline.estimate.has_value()))
      {
        check(within_tolerance{tolerance},
              "The speed-up with a slow baseline of zero",
              withoutSlowBaseline.estimate->speedup,
              9.0);
      }

      if(check("The baseline durations are given with a fast baseline of zero",
               withoutFastBaseline.baseline_durations.has_value()))
      {
        check(equality,
              "The fast baseline's duration of zero",
              withoutFastBaseline.baseline_durations->fast.count(),
              0.0);
      }

      if(check("The baseline durations are given with a slow baseline of zero",
               withoutSlowBaseline.baseline_durations.has_value()))
      {
        check(equality,
              "The slow baseline's duration of zero",
              withoutSlowBaseline.baseline_durations->slow.count(),
              0.0);
      }
    }

    {
      // Five trials have a fast baseline of 1 s, four of 3 s and one of 11 s.
      // The trim drops a trial of 1 s and the trial of 11 s, so the trimmed
      // mean is 2 s. The untrimmed mean is 2.8 s, and the trimmed geometric
      // mean is sqrt(3) s.
      constexpr durations_with_baselines
        fastBaselineOf1s {.fast{.task{2s},  .baseline{1s}},  .slow{typical.slow}},
        fastBaselineOf3s {.fast{.task{4s},  .baseline{3s}},  .slow{typical.slow}},
        fastBaselineOf11s{.fast{.task{12s}, .baseline{11s}}, .slow{typical.slow}};

      const auto outcome{
        judge(trials_with_atypical(trialsAtFloor,
                                   fastBaselineOf1s,
                                   {fastBaselineOf3s,
                                    fastBaselineOf3s,
                                    fastBaselineOf3s,
                                    fastBaselineOf3s,
                                    fastBaselineOf11s}))
      };

      if(check("The baseline durations are given for baselines of three durations",
               outcome.baseline_durations.has_value()))
      {
        check(within_tolerance{tolerance},
              "A baseline's typical duration is its trimmed mean",
              outcome.baseline_durations->fast.count(),
              2.0);
      }
    }
  }

  void performance_utilities_test::test_shuffled_task_orders_with_baselines()
  {
    using impl::task_order, impl::task_and_baseline_order, impl::task_order_with_baselines;

    // Enough trials that independent shuffles agree in every trial only with
    // a negligible probability. The generator's seed is fixed, so a given
    // standard library gives the same orders on every run.
    constexpr std::size_t trials{101};
    std::mt19937 generator{std::mt19937::default_seed};

    const auto orders{impl::shuffled_task_orders_with_baselines(trials, generator)};

    auto trialsWhere{
      [&orders](auto predicate) { return static_cast<std::size_t>(std::ranges::count_if(orders, predicate)); }
    };

    auto fastPairRunsFirst{[](task_order_with_baselines order) { return order.pairs == task_order::fast_then_slow; }};
    auto fastTaskRunsFirst{
      [](task_order_with_baselines order) { return order.fast == task_and_baseline_order::task_then_baseline; }
    };
    auto slowTaskRunsFirst{
      [](task_order_with_baselines order) { return order.slow == task_and_baseline_order::task_then_baseline; }
    };

    // Two orders shuffled independently agree in some trials, and not in all.
    auto agreeInSomeTrialsNotAll{
      [&trialsWhere](auto firstOrder, auto secondOrder) {
        auto bothAgree{
          [firstOrder, secondOrder](task_order_with_baselines order) { return firstOrder(order) == secondOrder(order); }
        };

        const auto agreements{trialsWhere(bothAgree)};
        return (agreements > 0) && (agreements < trials);
      }
    };

    check(equality,
          "Trials in which the fast task and its baseline run first",
          trialsWhere(fastPairRunsFirst),
          trials / 2);
    check(equality,
          "Trials in which the fast task runs before its baseline",
          trialsWhere(fastTaskRunsFirst),
          trials / 2);
    check(equality,
          "Trials in which the slow task runs before its baseline",
          trialsWhere(slowTaskRunsFirst),
          trials / 2);
    check("The pair order and the fast task's order beside its baseline are shuffled independently",
          agreeInSomeTrialsNotAll(fastPairRunsFirst, fastTaskRunsFirst));
    check("The pair order and the slow task's order beside its baseline are shuffled independently",
          agreeInSomeTrialsNotAll(fastPairRunsFirst, slowTaskRunsFirst));
    check("The fast and slow tasks' orders beside their baselines are shuffled independently",
          agreeInSomeTrialsNotAll(fastTaskRunsFirst, slowTaskRunsFirst));
  }

  void performance_utilities_test::test_postprocessing()
  {
    using enum attempt_count_stability;

    {
      std::string_view latest   {"Speed-up: 1.34 in [1.22, 1.47]; predicted [2, 3]\n"};
      std::string_view reference{"Speed-up: 2.01 in [1.98, 2.04]; predicted [2, 3]\n"};

      check(equality, "", postprocess(latest, reference, fluctuating), reference);
    }

    {
      std::string_view latest   {"Speed-up: 1.34 in [1.22, 1.47]; predicted [2, 3]\n"};
      std::string_view reference{"Speed-up: 1.34 in [1.22, 1.47]; predicted [2.1, 3]\n"};

      check(equality, "", postprocess(latest, reference, fluctuating), latest);
    }

    {
      std::string_view latest   {"Speed-up: 1.34 in [1.22, 1.47]; predicted [2, 3]\n"};
      std::string_view reference{"Speed-up: 1.34 in [1.22, 1.47]; predicted [2, 3.1]\n"};

      check(equality, "", postprocess(latest, reference, fluctuating), latest);
    }

    {
      std::string_view latest   {"Speed-up: 1.34 in [1.22, 1.47]; predicted [2, 3]\n"};
      std::string_view reference{"Speed-up: 1.34 in (1.22, 1.47); predicted [2, 3]\n"};

      check(equality, "", postprocess(latest, reference, fluctuating), latest);
    }

    {
      std::string_view latest   {"Speed-up: 1.34 in [1.22, 1.47]; predicted [2, 3]\n"};
      std::string_view reference{"Speed-up: 1.340 in [1.22, 1.47]; predicted [2, 3]\n"};

      check(equality, "", postprocess(latest, reference, fluctuating), latest);
    }

    {
      std::string_view latest   {"Speed-up: 1.34 in [1.22, 1.47]; predicted [2, 3]\n"};
      std::string_view reference{"Speed-up: 1.34 in [1.22, 1.47]; predicted [2, 3] 4 5 6 7\n"};

      check(equality, "", postprocess(latest, reference, fluctuating), latest);
    }

    {
      std::string_view latest   {"Speed-up: 1.34 in [1.22, 1.47]; predicted [2, 3]\n"};
      std::string_view reference{"Speed-up: 1.34 in [1.22]; predicted [2, 3]\n"};

      check(equality, "", postprocess(latest, reference, fluctuating), latest);
    }

    {
      std::string_view latest   {"Trials: 15, attempt 3 of 3\n"};
      std::string_view reference{"Trials: 5, attempt 1 of 3\n"};

      check(equality, "", postprocess(latest, reference, fluctuating), reference);
    }

    {
      std::string_view latest   {"Trials: 5, attempt 1 of 3\n"};
      std::string_view reference{"Trials: 5, attempt 1 of 4\n"};

      check(equality, "", postprocess(latest, reference, fluctuating), latest);
    }

    {
      std::string_view latest   {"Trials: 5, attempt 1 of 3\n"};
      std::string_view reference{"Trials: 05, attempt 1 of 3\n"};

      check(equality, "", postprocess(latest, reference, fluctuating), latest);
    }

    {
      std::string_view latest   {"Trials: 5, attempt 1 of 3\n"};
      std::string_view reference{"Trials: 5.5, attempt 1 of 3\n"};

      check(equality, "", postprocess(latest, reference, fluctuating), latest);
    }

    {
      std::string_view latest   {"Trials: 5, attempt 1 of 3\n"};
      std::string_view reference{"Trials: -5, attempt 1 of 3\n"};

      check(equality, "", postprocess(latest, reference, fluctuating), latest);
    }

    {
      std::string_view latest   {"Task durations: fast 0.005s, slow 0.0067s\n"};
      std::string_view reference{"Task durations: fast 0.00101s, slow 0.00402s\n"};

      check(equality, "", postprocess(latest, reference, fluctuating), reference);
    }

    {
      std::string_view latest   {"Task durations: fast 0.005s, slow 0.0067s\n"};
      std::string_view reference{"Task durations: fast 0.005ms, slow 0.0067s\n"};

      check(equality, "", postprocess(latest, reference, fluctuating), latest);
    }

    {
      std::string_view latest   {"Task durations: fast 0.005s, slow 0.0067s\n"};
      std::string_view reference{"Task durations: slow 0.005s, fast 0.0067s\n"};

      check(equality, "", postprocess(latest, reference, fluctuating), latest);
    }

    {
      std::string_view latest   {"Task durations: fast 0.005s, slow 0.0067s\n"};
      std::string_view reference{"Task durations: fast 0.005s, slow 0.0067s FAILED\n"};

      check(equality, "", postprocess(latest, reference, fluctuating), latest);
    }

    {
      std::string_view latest   {"Baseline durations: fast 0.005s, slow 0.0067s\n"};
      std::string_view reference{"Baseline durations: fast 0.00101s, slow 0.00402s\n"};

      check(equality, "", postprocess(latest, reference, fluctuating), reference);
    }

    {
      std::string_view latest   {"Baseline durations: fast 0.005s, slow 0.0067s\n"};
      std::string_view reference{"Task durations: fast 0.005s, slow 0.0067s\n"};

      check(equality, "", postprocess(latest, reference, fluctuating), latest);
    }

    {
      std::string_view latest   {"The fast task is slower than the slow one\n"};
      std::string_view reference{"The fast task is not distinguishably faster than the slow one\n"};

      check(equality, "", postprocess(latest, reference, fluctuating), latest);
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

      check(equality, "", postprocess(latest, reference, fluctuating), reference);
    }

    {
      std::string_view latest   {"Line 40\n"
                                 "Speed-up: 0.5 in [0.499, 0.501]; predicted [2, 3]\n"};
      std::string_view reference{"Line 41\n"
                                 "Speed-up: 0.497 in [0.495, 0.499]; predicted [2, 3]\n"};

      check(equality, "", postprocess(latest, reference, fluctuating), latest);
    }

    {
      std::string_view latest   {"Speed-up: 1.34 in [1.22, 1.47]; predicted [2, 3]\n"
                                 "Trials: 15, attempt 3 of 3\n"};
      std::string_view reference{"Trials: 15, attempt 3 of 3\n"
                                 "Speed-up: 1.34 in [1.22, 1.47]; predicted [2, 3]\n"};

      check(equality, "", postprocess(latest, reference, fluctuating), latest);
    }

    {
      std::string_view latest   {"Speed-up: 1.34 in [1.22, 1.47]; predicted [2, 3]\n"};
      std::string_view reference{"Speed-up: 1.34 in [1.22, 1.47]; predicted [2, 3]\n"
                                 "Trials: 15, attempt 3 of 3\n"};

      check(equality, "", postprocess(latest, reference, fluctuating), latest);
    }

    {
      std::string_view latest   {"Speed-up: 1.34 in [1.22, 1.47]; predicted [2, 3]\n"};
      std::string_view reference{""};

      check(equality, "", postprocess(latest, reference, fluctuating), latest);
    }

    {
      std::string_view latest   {""};
      std::string_view reference{"Speed-up: 1.34 in [1.22, 1.47]; predicted [2, 3]\n"};

      check(equality, "", postprocess(latest, reference, fluctuating), latest);
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

      check(equality, "", postprocess(latest, reference, fluctuating), std::string_view{reference});
    }

    {
      const std::string latest{
        append_lines(trials_summary(15, {3, 3}), speedup_summary({1.04, {1.01, 1.07}}, {2, 3}))
      };

      const std::string reference{
        append_lines(trials_summary(5,  {1, 3}), speedup_summary({1.02, {1.00, 1.04}}, {2, 3}))
      };

      check(equality, "", postprocess(latest, reference, fluctuating), std::string_view{reference});
    }

    {
      const std::string latest   {trials_summary(15, {3, 3})};
      const std::string reference{trials_summary(15, {3, 4})};

      check(equality, "", postprocess(latest, reference, fluctuating), std::string_view{latest});
    }

    {
      using fractional_seconds = std::chrono::duration<double>;

      const std::string latest{
        append_lines(speedup_summary({1.04, {1.01, 1.07}}, {2, 3}),
                     trials_summary(15, {3, 3}),
                     task_durations_summary({fractional_seconds{0.001},  fractional_seconds{0.00104}}))
      };

      const std::string reference{
        append_lines(speedup_summary({1.02, {1.00, 1.04}}, {2, 3}),
                     trials_summary(15, {3, 3}),
                     task_durations_summary({fractional_seconds{0.0011}, fractional_seconds{0.00112}}))
      };

      check(equality, "", postprocess(latest, reference, typically_stable), std::string_view{reference});
    }

    {
      const std::string latest   {trials_summary(15, {3, 3})};
      const std::string reference{trials_summary(20, {3, 3})};

      check(equality, "", postprocess(latest, reference, typically_stable), std::string_view{latest});
    }

    {
      const std::string latest   {trials_summary(15, {3, 3})};
      const std::string reference{trials_summary(15, {2, 3})};

      check(equality, "", postprocess(latest, reference, typically_stable), std::string_view{latest});
    }

    {
      const std::string latest   {speedup_summary({1.04, {1.01, 1.07}}, {2, 3})};
      const std::string reference{speedup_summary({1.04, {1.01, 1.07}}, {2, 4})};

      check(equality, "", postprocess(latest, reference, fluctuating), std::string_view{latest});
    }

    {
      const std::string latest   {std::format("Line 40\n\n{}", speedup_summary({1.04, {1.01, 1.07}}, {2, 3}))};
      const std::string reference{std::format("Line 40\n{}",   speedup_summary({1.02, {1.00, 1.04}}, {2, 3}))};

      check(equality, "", postprocess(latest, reference, fluctuating), std::string_view{latest});
    }

    {
      const std::string latest   {std::format("{}\n", speedup_summary({1.04, {1.01, 1.07}}, {2, 3}))};
      const std::string reference{std::format("{}",   speedup_summary({1.02, {1.00, 1.04}}, {2, 3}))};

      check(equality, "", postprocess(latest, reference, fluctuating), std::string_view{latest});
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

      check(equality, "", postprocess(latest, reference, fluctuating), std::string_view{reference});
    }

    {
      const std::string latest   {indent(speedup_summary({1.04, {1.01, 1.07}}, {2, 3}), indentation{"  "})};
      const std::string reference{indent(speedup_summary({1.04, {1.01, 1.07}}, {2, 3}), indentation{"    "})};

      check(equality, "", postprocess(latest, reference, fluctuating), std::string_view{latest});
    }
  }

  void performance_utilities_test::test_zeroed_measurements()
  {
    using fractional_seconds = std::chrono::duration<double>;
    using enum attempt_count_stability;

    check(equality,
          "The speed-up and the interval around it are zeroed, and the predicted interval kept",
          text_with_zeroed_measurements(speedup_summary({1.04, {1.01, 1.07}}, {2, 3}), fluctuating),
          speedup_summary({}, {2, 3}));

    check(equality,
          "The number of trials and the attempt are zeroed, and the number of attempts allowed kept",
          text_with_zeroed_measurements(trials_summary(15, {3, 3}), fluctuating),
          trials_summary(0, {0, 3}));

    check(equality,
          "The task durations are zeroed",
          text_with_zeroed_measurements(task_durations_summary({fractional_seconds{0.001}, fractional_seconds{0.002}}),
                                        fluctuating),
          task_durations_summary({}));

    check(equality,
          "The baseline durations are zeroed",
          text_with_zeroed_measurements(
            baseline_durations_summary({fractional_seconds{0.001}, fractional_seconds{0.002}}),
            fluctuating),
          baseline_durations_summary({}));

    check(equality,
          "With the attempt count typically stable, the number of trials and the attempt are kept",
          text_with_zeroed_measurements(trials_summary(15, {3, 3}), typically_stable),
          trials_summary(15, {3, 3}));

    check(equality,
          "With the attempt count typically stable, the speed-up, its interval and the task durations are zeroed",
          text_with_zeroed_measurements(
            append_lines(speedup_summary({1.04, {1.01, 1.07}}, {2, 3}),
                         task_durations_summary({fractional_seconds{0.001}, fractional_seconds{0.002}})),
            typically_stable),
          append_lines(speedup_summary({}, {2, 3}), task_durations_summary({})));
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
}
