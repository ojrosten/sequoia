////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "PerformanceTestDiagnostics.hpp"
#include "PerformanceTestingUtilities.hpp"

#include <chrono>

namespace sequoia::testing
{
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
                               counted_spinner{5ms, trials_per_attempt_with_minimum_10[0], 1ms},
                               make_cycling_spinner({5ms, 30ms, 40ms, 40ms, 40ms}),
                               {.prediction{.lower{20.4}, .upper{41.1}}});

    check_relative_performance("Attempt 1's interval lies above the prediction, "
                               "and would overlap the prediction only if substantially wider; "
                               "later speed-ups lie inside the prediction",
                               counted_spinner{8ms, trials_per_attempt_with_minimum_10[0], 20ms},
                               make_cycling_spinner({30ms, 30ms, 30ms, 60ms, 80ms}),
                               {.prediction{.lower{1.7}, .upper{2.62}}});

    check_relative_performance("Attempt 1's interval overlaps the prediction, "
                               "but would lie above the prediction if substantially narrower; "
                               "attempt 2 is slower",
                               make_cycling_spinner({3ms, 4ms, 12ms, 12ms, 12ms}),
                               counted_spinner{50ms, trials_per_attempt_with_minimum_10[0], 1ms},
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
                               counted_spinner{1ms, trials_per_attempt_with_minimum_10[0],                            4ms},
                               counted_spinner{2ms, trials_before_nth_attempt(trials_per_attempt_with_minimum_10, 3), 8ms},
                               parameters);
  }

  void performance_false_negative_diagnostics::test_confidence_multiplier_from_first_attempt()
  {
    using namespace std::chrono_literals;

    constexpr relative_performance_parameters parameters{.prediction{.lower{2.92}, .upper{4.38}}};

    check_relative_performance("Attempt 1's interval overlaps the prediction, "
                               "but would lie below the prediction if substantially narrower; "
                               "attempt 2 is slower",
                               counted_spinner{1ms, trials_per_attempt_with_minimum_10[0], 4ms},
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

    // The tasks' net durations are 3 ms and 9 ms, except in the trials in
    // which a baseline spins for longer than its task. The first attempt has
    // ten trials, so the trim drops one trial from each end.
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
      "Baselines of 1 ms; from the net durations, a speed-up of 2, then of 0.5, then of 2",
      task_with_baseline{.task = counted_spinner{4ms, trials_per_attempt_with_minimum_10[0], 13ms}, .baseline = baseline},
      task_with_baseline{.task = counted_spinner{7ms, trials_before_nth_attempt(trials_per_attempt_with_minimum_10, 3), 25ms},
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
                               counted_spinner{8ms, trials_before_nth_attempt(trials_per_attempt_with_minimum_10, 3), 5ms},
                               make_cycling_spinner({4ms, 6ms, 20ms, 30ms, 30ms}),
                               {.prediction{.lower{4.86}, .upper{7.43}}});

    check_relative_performance("Attempt 2's interval lies above the prediction, "
                               "and would overlap the prediction only if substantially wider; "
                               "attempt 3's interval overlaps the prediction, "
                               "but would not if substantially narrower",
                               counted_spinner{1ms, trials_before_nth_attempt(trials_per_attempt_with_minimum_10, 3), 2ms},
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

    // The tasks' net durations are 3 ms and 9 ms, except in the trials in
    // which a baseline spins for longer than its task. The first attempt has
    // ten trials, so the trim drops one trial from each end.
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
}
