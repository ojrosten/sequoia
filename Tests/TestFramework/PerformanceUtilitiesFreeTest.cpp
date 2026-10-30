////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "PerformanceUtilitiesFreeTest.hpp"
#include "PerformanceTestingUtilities.hpp"

#include "sequoia/PlatformSpecific/Macros.hpp"
#include "sequoia/TestFramework/Macros.hpp"

import std;
import sequoia.test_framework;

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

    struct non_const_now_clock
    {
      using time_point = std::chrono::steady_clock::time_point;

      time_point now() { return {}; }
    };

    struct int_now_clock
    {
      using time_point = int;

      time_point now() const { return 0; }
    };

    struct mismatched_now_clock
    {
      using time_point = std::chrono::steady_clock::time_point;

      std::chrono::time_point<std::chrono::steady_clock, std::chrono::milliseconds> now() const { return {}; }
    };

    struct rvalue_only_subtrahend_clock
    {
      struct time_point
      {
        friend std::chrono::nanoseconds operator-(const time_point&, time_point&&) { return {}; }
      };

      time_point now() const { return {}; }
    };

    template<class Task, class Clock>
    concept profilable_with = requires(Task task, const Clock& clock) { profile(std::move(task), clock); };

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

    template<class Clock>
    inline constexpr bool checkable_with_clock_v{
      requires(performance_extender<test_mode::standard>& extender,
               const relative_performance_parameters& parameters,
               const Clock& clock) {
        extender.check_relative_performance("", copyable_task{}, copyable_task{}, parameters, clock);
      }
    };

    template<class Clock>
    inline constexpr bool checkable_with_baselines_and_clock_v{
      requires(performance_extender<test_mode::standard>& extender,
               const relative_performance_parameters& parameters,
               const Clock& clock) {
        extender.check_relative_performance("",
                                            copyable_task_with_baseline{},
                                            copyable_task_with_baseline{},
                                            parameters,
                                            clock);
      }
    };

    constexpr scripted_clock::duration fast_task_duration        {std::chrono::milliseconds{4}},
                                       shorter_slow_task_duration{std::chrono::milliseconds{8}},
                                       longer_slow_task_duration {std::chrono::milliseconds{16}};

    /** \brief The conclusion of executed attempts, held with their mode and
               prediction.
     */
    struct execution_of_attempts
    {
      test_mode                             mode{};
      relative_performance_interval         prediction{};
      impl::relative_performance_conclusion conclusion{};
    };

    /** \brief Executes attempts in `Mode` against `prediction`, timing a
               fast task which takes `fast_task_duration` against a slow task
               which takes `shorter_slow_task_duration` and
               `longer_slow_task_duration` in turn.
     */
    template<test_mode Mode>
    [[nodiscard]]
    execution_of_attempts execute_scripted_attempts(relative_performance_interval prediction)
    {
      scripted_clock clock{};

      return {
        .mode{Mode},
        .prediction{prediction},
        .conclusion{
          impl::execute_attempts<Mode>(
            counted_scripted_task{clock, fast_task_duration},
            make_cycling_scripted_task(clock, {shorter_slow_task_duration, longer_slow_task_duration}),
            {.prediction{prediction}, .minimum_trials{relative_performance_minimum_trials_floor}},
            clock)
        }
      };
    }

    /** \brief Returns the durations of the trials of attempt `attempt` of
               `execute_scripted_attempts`.
     */
    [[nodiscard]]
    std::vector<relative_performance_durations> trial_durations_of_attempt(std::size_t attempt)
    {
      auto durationsOfTrial{
        [](std::size_t trial) -> relative_performance_durations {
          // Every attempt follows an even number of trials, so the slow
          // task's cycle starts afresh with each attempt.
          return {
            .fast{fast_task_duration},
            .slow{(trial % 2 == 0) ? shorter_slow_task_duration : longer_slow_task_duration}
          };
        }
      };

      return
          std::views::iota(0uz, trials_per_attempt_for_minimum_10.at(attempt - 1))
        | std::views::transform(durationsOfTrial)
        | std::ranges::to<std::vector>();
    }
  }

  [[nodiscard]]
  std::filesystem::path performance_utilities_free_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void performance_utilities_free_test::run_tests()
  {
    test_task_constraints();
    test_clock_constraints();
    test_timing_constraints();
    test_exception_specifications();
    test_default_minimum_trials();
    test_overlap_multipliers();
    test_multiplier_given_to_each_attempt();
    test_judgement_with_baselines();
    test_shuffled_baselined_task_orders();
    test_call_in_order();
    test_postprocessing();
    test_zeroed_measurements();
  }

  void performance_utilities_free_test::test_task_constraints()
  {
    STATIC_CHECK( profilable_with<   copyable_task, std::chrono::steady_clock>);
    STATIC_CHECK(!profilable_with<rvalue_only_task, std::chrono::steady_clock>);

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

  void performance_utilities_free_test::test_clock_constraints()
  {
    STATIC_CHECK( performance_clock<std::chrono::steady_clock>);
    STATIC_CHECK( performance_clock<scripted_clock>);
    STATIC_CHECK(!performance_clock<int>);
    STATIC_CHECK(!performance_clock<non_const_now_clock>);
    STATIC_CHECK(!performance_clock<int_now_clock>);
    STATIC_CHECK(!performance_clock<mismatched_now_clock>);
    STATIC_CHECK(!performance_clock<rvalue_only_subtrahend_clock>);

    STATIC_CHECK( profilable_with<copyable_task, scripted_clock>);
    STATIC_CHECK(!profilable_with<copyable_task, non_const_now_clock>);

    STATIC_CHECK( checkable_with_clock_v<scripted_clock>);
    STATIC_CHECK(!checkable_with_clock_v<non_const_now_clock>);

    STATIC_CHECK( checkable_with_baselines_and_clock_v<scripted_clock>);
    STATIC_CHECK(!checkable_with_baselines_and_clock_v<non_const_now_clock>);
  }

  void performance_utilities_free_test::test_timing_constraints()
  {
    using impl::call_order, impl::baselined_task_order;
    using impl::constructible_from_calls_v, impl::one_trial_timeable_v, impl::timeable_against;

    using int_call         = int(*)();
    using string_view_call = std::string_view(*)();
    using unary_call       = int(*)(int);

    using int_pair             = std::pair<int, int>;
    using int_string_view_pair = std::pair<int, std::string_view>;

    STATIC_CHECK( constructible_from_calls_v<int_pair,             int_call,         int_call>);
    STATIC_CHECK( constructible_from_calls_v<int_string_view_pair, int_call,         string_view_call>);
    STATIC_CHECK(!constructible_from_calls_v<int_string_view_pair, string_view_call, int_call>);
    STATIC_CHECK(!constructible_from_calls_v<int_pair,             unary_call,       int_call>);
    STATIC_CHECK(!constructible_from_calls_v<int_pair,             int_call,         unary_call>);

    using std::chrono::steady_clock;
    using baselined_task = copyable_task_with_baseline;

    STATIC_CHECK( one_trial_timeable_v<copyable_task,    copyable_task,    call_order,           steady_clock>);
    STATIC_CHECK(!one_trial_timeable_v<copyable_task,    copyable_task,    baselined_task_order, steady_clock>);
    STATIC_CHECK(!one_trial_timeable_v<rvalue_only_task, copyable_task,    call_order,           steady_clock>);
    STATIC_CHECK(!one_trial_timeable_v<copyable_task,    rvalue_only_task, call_order,           steady_clock>);
    STATIC_CHECK(!one_trial_timeable_v<copyable_task,    copyable_task,    call_order,           non_const_now_clock>);
    STATIC_CHECK( one_trial_timeable_v<baselined_task,   baselined_task,   baselined_task_order, steady_clock>);
    STATIC_CHECK(!one_trial_timeable_v<baselined_task,   baselined_task,   call_order,           steady_clock>);
    STATIC_CHECK(!one_trial_timeable_v<baselined_task,   baselined_task,   baselined_task_order, non_const_now_clock>);

    STATIC_CHECK( timeable_against<copyable_task, copyable_task, steady_clock>);
    STATIC_CHECK(!timeable_against<copyable_task, copyable_task, non_const_now_clock>);
  }

  void performance_utilities_free_test::test_exception_specifications()
  {
    STATIC_CHECK(noexcept(do_not_optimize_away(0)));
  }

  void performance_utilities_free_test::test_default_minimum_trials()
  {
    constexpr relative_performance_parameters parameters{.prediction{.lower{2.0}, .upper{3.0}}};

    STATIC_CHECK(parameters.minimum_trials == relative_performance_minimum_trials_floor);
  }

  void performance_utilities_free_test::test_overlap_multipliers()
  {
    using enum test_mode;

    // The multipliers are not in the contract. These checks pin the
    // implementation's choice, and change with it.
    check(equality, "Multiplier in standard mode, attempt 1",       impl::overlap_multiplier(standard,       1), 1.0);
    check(equality, "Multiplier in standard mode, attempt 2",       impl::overlap_multiplier(standard,       2), 2.0);
    check(equality, "Multiplier in standard mode, attempt 3",       impl::overlap_multiplier(standard,       3), 3.0);
    check(equality, "Multiplier in false-positive mode, attempt 1", impl::overlap_multiplier(false_positive, 1), 1.0);
    check(equality, "Multiplier in false-positive mode, attempt 2", impl::overlap_multiplier(false_positive, 2), 2.0);
    check(equality, "Multiplier in false-positive mode, attempt 3", impl::overlap_multiplier(false_positive, 3), 3.0);
    check(equality, "Multiplier in false-negative mode, attempt 1", impl::overlap_multiplier(false_negative, 1), 3.0);
    check(equality, "Multiplier in false-negative mode, attempt 2", impl::overlap_multiplier(false_negative, 2), 3.0);
    check(equality, "Multiplier in false-negative mode, attempt 3", impl::overlap_multiplier(false_negative, 3), 3.0);
  }

  void performance_utilities_free_test::test_multiplier_given_to_each_attempt()
  {
    using enum test_mode;

    auto lowerEndOfEstimate{[](const relative_performance_estimate& estimate) { return estimate.interval.lower; }};
    auto upperEndOfEstimate{[](const relative_performance_estimate& estimate) { return estimate.interval.upper; }};

    auto lowerEndAt{
      [lowerEndOfEstimate](std::size_t attempt, double multiplier) {
        // Any prediction gives the same interval.
        constexpr relative_performance_interval prediction{.lower{2}, .upper{3}};

        const auto estimate{impl::judge_trials(trial_durations_of_attempt(attempt), multiplier, prediction).estimate};
        return estimate.transform(lowerEndOfEstimate);
      }
    };

    constexpr std::array<std::pair<std::size_t, std::size_t>, 3> pairsOfAttempts{{{1, 2}, {1, 3}, {2, 3}}};

    for(const auto attempt : std::views::iota(1uz, relative_performance_max_attempts + 1))
    {
      for(const auto& [first, second] : pairsOfAttempts)
      {
        check(std::format("Attempt {}'s trials give intervals with different lower ends "
                          "at the standard multipliers of attempts {} and {}",
                          attempt,
                          first,
                          second),
              lowerEndAt(attempt, impl::overlap_multiplier(standard, first))
                != lowerEndAt(attempt, impl::overlap_multiplier(standard, second)));
      }
    }

    auto checkConclusion{
      [this, lowerEndOfEstimate, upperEndOfEstimate](std::string_view scenario,
                                                     const execution_of_attempts& execution,
                                                     std::size_t attempts) {
        const auto& conclusion{execution.conclusion};
        const auto expected{
          impl::judge_trials(trial_durations_of_attempt(attempts),
                             impl::overlap_multiplier(execution.mode, attempts),
                             execution.prediction)
        };

        check(equality, std::format("{}: the number of attempts", scenario), conclusion.attempts, attempts);
        check(std::format("{}: the failure of attempt {}", scenario, attempts),
              conclusion.judgement.failure == expected.failure);
        check(equality,
              std::format("{}: the lower end of attempt {}'s interval", scenario, attempts),
              conclusion.judgement.estimate.transform(lowerEndOfEstimate),
              expected.estimate.transform(lowerEndOfEstimate));
        check(equality,
              std::format("{}: the upper end of attempt {}'s interval", scenario, attempts),
              conclusion.judgement.estimate.transform(upperEndOfEstimate),
              expected.estimate.transform(upperEndOfEstimate));
      }
    };

    // The predictions below suit the estimator, the multipliers and the
    // number of trials in each attempt, none of which is in the contract.
    // These checks pin the implementation's choices, and change with them.
    checkConclusion("Standard mode, with attempt 1's interval overlapping the prediction",
                    execute_scripted_attempts<standard>({.lower{2.5}, .upper{3}}),
                    1);

    checkConclusion("Standard mode, with attempt 1's interval below the prediction, and attempt 2's overlapping it",
                    execute_scripted_attempts<standard>({.lower{3.4}, .upper{5}}),
                    2);

    checkConclusion("Standard mode, with the intervals of attempts 1 and 2 below the prediction, "
                    "and attempt 3's overlapping it",
                    execute_scripted_attempts<standard>({.lower{3.7}, .upper{5}}),
                    3);

    checkConclusion("False-positive mode, with attempt 1's interval below the prediction, "
                    "and attempt 2's overlapping it",
                    execute_scripted_attempts<false_positive>({.lower{3.4}, .upper{5}}),
                    2);

    checkConclusion("False-negative mode, with attempt 1's interval below the prediction",
                    execute_scripted_attempts<false_negative>({.lower{5}, .upper{6}}),
                    1);

    checkConclusion("False-negative mode, with attempt 1's interval overlapping the prediction, "
                    "and attempt 2's below it",
                    execute_scripted_attempts<false_negative>({.lower{4.2}, .upper{6}}),
                    2);

    checkConclusion("False-negative mode, with every attempt's interval overlapping the prediction",
                    execute_scripted_attempts<false_negative>({.lower{3.4}, .upper{5}}),
                    3);
  }

  void performance_utilities_free_test::test_judgement_with_baselines()
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

    // From the net durations, the speed-up is 8; from the tasks' own, 3. With the
    // baselines exchanged it is 3.5; with the fast baseline taken from both
    // tasks it is 7, and with the slow baseline taken from both it is 4.
    constexpr durations_with_baselines typical{.fast{.task{3s}, .baseline{2s}}, .slow{.task{9s}, .baseline{1s}}};

    // A task shorter than its baseline has a negative net duration. If both
    // tasks are shorter than their baselines, the ratio of the slow task's net
    // duration to the fast task's is positive, and the trial still counts at
    // each end.
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
        return impl::judge_trials(trialDurations, 1.0, prediction);
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
        const auto judgement{judgeAtFloor(atypical)};
        if(check(description, judgement.estimate.has_value()))
        {
          check(within_tolerance{tolerance}, description, judgement.estimate->speedup, speedup);
        }
      }
    };

    {
      const auto judgement{judgeAtFloor({})};

      check("No failure from the net durations", !judgement.failure.has_value());

      if(check("An estimate from the net durations", judgement.estimate.has_value()))
      {
        check(within_tolerance{tolerance}, "The speed-up from the net durations", judgement.estimate->speedup, 8.0);
      }

      check(within_tolerance{tolerance}, "The fast task's own duration", judgement.task_durations.fast.count(), 3.0);
      check(within_tolerance{tolerance}, "The slow task's own duration", judgement.task_durations.slow.count(), 9.0);

      if(check("The baseline durations are given", judgement.baseline_durations.has_value()))
      {
        check(within_tolerance{tolerance},
              "The fast baseline's duration",
              judgement.baseline_durations->fast.count(),
              2.0);
        check(within_tolerance{tolerance},
              "The slow baseline's duration",
              judgement.baseline_durations->slow.count(),
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
          const auto judgement{judgeAtFloor({fastNoLongerThanBaseline, fastNoLongerThanBaseline})};

          check(std::format("A fast task {} its baseline in more trials than the trim drops", relation),
                judgement.failure == fast_task_too_often_no_longer_than_baseline);
          check(std::format("No estimate with a fast task too often {} its baseline", relation),
                !judgement.estimate.has_value());
          check(std::format("The baseline durations are given with a fast task too often {} its baseline", relation),
                judgement.baseline_durations.has_value());
        }

        {
          const auto judgement{judgeAtFloor({slowNoLongerThanBaseline, slowNoLongerThanBaseline})};

          check(std::format("A slow task {} its baseline in more trials than the trim drops", relation),
                judgement.failure == slow_task_too_often_no_longer_than_baseline);
          check(std::format("No estimate with a slow task too often {} its baseline", relation),
                !judgement.estimate.has_value());
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
      // Of the ten trials, six have a fast baseline of 1 s; the other four
      // have baselines of 2 s, 3 s, 4 s and 11 s. Trimming one trial from each
      // end drops a trial of 1 s and the trial of 11 s, so the trimmed mean is
      // 1.75 s. Every rival gives another value: the median 1 s, the
      // winsorized mean 1.9 s, the untrimmed mean 2.6 s, a trim of two from
      // each end 1.5 s, and the trimmed geometric mean 24^(1/8) s.
      constexpr durations_with_baselines
        fastBaselineOf1s {.fast{.task{2s},  .baseline{1s}},  .slow{typical.slow}},
        fastBaselineOf2s {.fast{.task{3s},  .baseline{2s}},  .slow{typical.slow}},
        fastBaselineOf3s {.fast{.task{4s},  .baseline{3s}},  .slow{typical.slow}},
        fastBaselineOf4s {.fast{.task{5s},  .baseline{4s}},  .slow{typical.slow}},
        fastBaselineOf11s{.fast{.task{12s}, .baseline{11s}}, .slow{typical.slow}};

      const auto judgement{
        judge(trials_with_atypical(trialsAtFloor,
                                   fastBaselineOf1s,
                                   {fastBaselineOf2s,
                                    fastBaselineOf3s,
                                    fastBaselineOf4s,
                                    fastBaselineOf11s}))
      };

      if(check("The baseline durations are given for baselines of five durations",
               judgement.baseline_durations.has_value()))
      {
        check(within_tolerance{tolerance},
              "A baseline's typical duration is its trimmed mean",
              judgement.baseline_durations->fast.count(),
              1.75);
      }
    }
  }

  void performance_utilities_free_test::test_shuffled_baselined_task_orders()
  {
    using impl::call_order, impl::baselined_task_order;

    // Enough trials that independent shuffles agree in every trial only with
    // a negligible probability. The generator's seed is fixed, so a given
    // standard library gives the same orders on every run.
    constexpr std::size_t trials{100};
    std::mt19937 generator{std::mt19937::default_seed};

    const auto orders{impl::shuffled_baselined_task_orders(trials, generator)};

    auto trialsWhere{
      [&orders](auto predicate) { return static_cast<std::size_t>(std::ranges::count_if(orders, predicate)); }
    };

    auto fastPairRunsFirst{
      [](baselined_task_order order) { return order.fast_vs_slow == call_order::first_then_second; }
    };
    auto fastTaskRunsFirst{
      [](baselined_task_order order) { return order.fast_vs_baseline == call_order::first_then_second; }
    };
    auto slowTaskRunsFirst{
      [](baselined_task_order order) { return order.slow_vs_baseline == call_order::first_then_second; }
    };

    // Two orders shuffled independently agree in some trials, and not in all.
    auto agreeInSomeTrialsNotAll{
      [&trialsWhere](auto firstOrder, auto secondOrder) {
        auto bothAgree{
          [firstOrder, secondOrder](baselined_task_order order) { return firstOrder(order) == secondOrder(order); }
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

  void performance_utilities_free_test::test_call_in_order()
  {
    using impl::call_order;
    using results_t = std::pair<int, int>;

    // Each call records itself, so the record gives the order of the calls.
    auto callInOrder{
      [](call_order order) {
        std::vector<std::string_view> calls{};
        const auto results{
          impl::call_in_order<results_t>([&calls]() { calls.push_back("first");  return 1; },
                                         [&calls]() { calls.push_back("second"); return 2; },
                                         order)
        };

        return std::pair{calls, results};
      }
    };

    {
      const auto [calls, results]{callInOrder(call_order::first_then_second)};
      check(equality,
            "first_then_second calls the first first",
            calls,
            std::vector<std::string_view>{"first", "second"});
      check(equality,
            "first_then_second gives the results in the order of the calls' parameters",
            results,
            results_t{1, 2});
    }

    {
      const auto [calls, results]{callInOrder(call_order::second_then_first)};
      check(equality,
            "second_then_first calls the second first",
            calls,
            std::vector<std::string_view>{"second", "first"});
      check(equality,
            "second_then_first gives the results in the order of the calls' parameters",
            results,
            results_t{1, 2});
    }
  }

  void performance_utilities_free_test::test_postprocessing()
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
      std::string_view latest   {"Baseline durations: fast 0.005s, slow 0.0067s\n"};
      std::string_view reference{"Baseline durations: fast 0.00101s, slow 0.00402s\n"};

      check(equality, "", postprocess(latest, reference), reference);
    }

    {
      std::string_view latest   {"Baseline durations: fast 0.005s, slow 0.0067s\n"};
      std::string_view reference{"Task durations: fast 0.005s, slow 0.0067s\n"};

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

      const std::string latest{
        append_lines(speedup_summary({infinity, {1e+06, -nan}}, {2, 3}),
                     task_durations_summary({double_precision_seconds{9.9e-05}, double_precision_seconds{0}}))
      };

      const std::string reference{
        append_lines(speedup_summary({1.04,     {1e-07,  2.5}}, {2, 3}),
                     task_durations_summary({double_precision_seconds{0.001},   double_precision_seconds{nan}}))
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
      const indentation nested{"  "};

      const std::string latest{
        indent(append_lines(speedup_summary({1.04, {1.01, 1.07}}, {2, 3}),
                            trials_summary(15, {3, 3}),
                            task_durations_summary(
                              {double_precision_seconds{0.001},  double_precision_seconds{0.00104}})),
               nested)
      };

      const std::string reference{
        indent(append_lines(speedup_summary({1.02, {1.00, 1.04}}, {2, 3}),
                            trials_summary(5,  {1, 3}),
                            task_durations_summary(
                              {double_precision_seconds{0.0011}, double_precision_seconds{0.00112}})),
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

  void performance_utilities_free_test::test_zeroed_measurements()
  {
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
          text_with_zeroed_measurements(
            task_durations_summary({double_precision_seconds{0.001}, double_precision_seconds{0.002}})),
          task_durations_summary({}));

    check(equality,
          "The baseline durations are zeroed",
          text_with_zeroed_measurements(
            baseline_durations_summary({double_precision_seconds{0.001}, double_precision_seconds{0.002}})),
          baseline_durations_summary({}));
  }
}
