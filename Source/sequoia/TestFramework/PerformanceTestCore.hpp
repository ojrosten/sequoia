////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Extension of the testing framework for performance testing.
*/

#include "sequoia/TestFramework/PerformanceSummary.hpp"
#include "sequoia/TestFramework/PerformanceTestTraits.hpp"
#include "sequoia/TestFramework/FreeTestCore.hpp"
#include "sequoia/TestFramework/TimingUtilities.hpp"

#include <algorithm>
#include <chrono>
#include <concepts>
#include <format>
#include <iterator>
#include <optional>
#include <random>
#include <ranges>
#include <span>
#include <type_traits>

namespace sequoia::testing
{
  /** \brief A type that `check_relative_performance` accepts as a task. */
  template<class T>
  concept performance_task = std::copy_constructible<T> && std::invocable<T&>;

  /** \brief A task, paired with a baseline task.

      The purpose of a baseline is to do the part of its task's work which is
      not under test, such as setup or copying. In each trial,
      `check_relative_performance` subtracts the baseline's duration from its
      task's, and compares the fast and slow tasks using what remains.

      A baseline may do nothing at all.
   */
  template<class Task, class Baseline>
  struct task_with_baseline
  {
    Task     task;
    Baseline baseline;
  };

  /** \brief The maximum number of attempts `check_relative_performance`
             makes, counting the first.
   */
  inline constexpr std::size_t relative_performance_max_attempts{3};

  /** \brief The least `minimum_trials` which `check_relative_performance`
             accepts.
   */
  inline constexpr std::size_t relative_performance_minimum_trials_floor{10};

  /** \brief The interval of speed-ups predicted for a fast task over a slow
             one, and the fewest trials allowed in any attempt.
   */
  struct relative_performance_parameters
  {
    relative_performance_interval prediction;
    std::size_t                   minimum_trials{relative_performance_minimum_trials_floor};
  };

  namespace impl
  {
    /** \brief A judgement of the speed-up of a fast task over a slow one,
               and the measurements behind it.

        `failure` is `std::nullopt` if the judgement did not fail.
     */
    struct relative_performance_judgement
    {
      std::optional<relative_performance_failure>   failure{};
      std::optional<relative_performance_estimate>  estimate{};

      /** \brief The trimmed geometric mean of each task's durations. */
      relative_performance_durations                task_durations{};

      /** \brief The trimmed arithmetic mean of each baseline's durations, if
                 the tasks had baselines.
       */
      std::optional<relative_performance_durations> baseline_durations{};
    };

    /** \brief The durations of a task and of its baseline. */
    struct task_and_baseline_durations
    {
      std::chrono::duration<double> task{},
                                    baseline{};
    };

    /** \brief The durations of the fast and slow tasks and their baselines. */
    struct relative_performance_durations_with_baselines
    {
      task_and_baseline_durations fast{},
                                  slow{};
    };

    /** \brief Judges the speed-up of a fast task over a slow one from
               `trialDurations`.

        \throws std::invalid_argument if `trialDurations` has fewer elements
        than `relative_performance_minimum_trials_floor`.

        \throws std::runtime_error if any duration is not greater than zero.
     */
    [[nodiscard]]
    relative_performance_judgement judge_trials(std::span<const relative_performance_durations> trialDurations,
                                                double overlapMultiplier,
                                                relative_performance_interval prediction);

    /** \brief Judges the speed-up of a fast task over a slow one, each with
               its baseline's duration subtracted, from `trialDurations`.

        Each trial gives each task a net duration: its own, less its
        baseline's. A trial in which both net durations are positive gives a
        speed-up. Otherwise, each net duration which is not positive is
        recorded as an extreme.

        The judgement is a failure with no estimate if either task's net
        duration is extreme in more than a tenth of the trials, rounded up.
        Otherwise the speed-ups are judged as the other overload judges them,
        trimming a tenth of the trials, rounded up, from each end. The
        extremes are included before trimming, but an estimate, when there is
        one, uses no extreme's value.

        \throws std::invalid_argument if `trialDurations` has fewer elements
        than `relative_performance_minimum_trials_floor`.

        \throws std::runtime_error if any task's duration is not greater than
        zero, or if any baseline's duration is negative.
     */
    [[nodiscard]]
    relative_performance_judgement
      judge_trials(std::span<const relative_performance_durations_with_baselines> trialDurations,
                   double overlapMultiplier,
                   relative_performance_interval prediction);

    /** \brief Summarizes an attempt in lines of text.

        The lines give:
        -# The reason the attempt failed, if it did;
        -# The speed-up, if the attempt estimated one;
        -# The number of trials, and the attempt;
        -# The task durations;
        -# The baseline durations, if `judgement` has them.
     */
    [[nodiscard]]
    std::string summarize_attempt(const relative_performance_judgement& judgement,
                                  std::size_t numTrials,
                                  std::size_t attemptsMade,
                                  relative_performance_interval prediction);

    /** \brief Returns the half-width, in standard errors, of the interval
               which attempt `attempt` compares with the predicted interval.
     */
    [[nodiscard]]
    double overlap_multiplier(test_mode mode, std::size_t attempt);

    /** \brief Returns the number of trials which attempt `attempt` runs: an
               even number, so that each order of the tasks runs in half of
               them.
     */
    [[nodiscard]]
    constexpr std::size_t num_trials_for_attempt(std::size_t minimumTrials, std::size_t attempt) noexcept
    {
      const auto numTrials{minimumTrials * (attempt + 1) / 2};
      return numTrials + numTrials % 2;
    }

    /** \brief The order in which two calls are made. */
    enum class call_order : bool { first_then_second, second_then_first };

    /** \brief The order in which a trial runs the fast and slow tasks,
               and the order of each with respect to its baseline.
     */
    struct baselined_task_order
    {
      call_order fast_vs_slow{},
                 fast_vs_baseline{},
                 slow_vs_baseline{};
    };

    /** \brief Returns an order of two calls for each of `numTrials` trials.

        The first call is made first in half of the trials. `generator`
        chooses which trials those are.

        \throws std::invalid_argument if `numTrials` is odd.
     */
    [[nodiscard]]
    std::vector<call_order> shuffled_call_orders(std::size_t numTrials, std::mt19937& generator);

    /** \brief Returns the order in which the two tasks and their baselines
               run in each of `numTrials` trials.

        Each task runs beside its baseline. The fast task and its baseline
        run first in half of the trials. Each task runs before its baseline
        in half of the trials. `generator` chooses those trials,
        independently for the order of the pairs and for each task's order
        beside its baseline.

        \throws std::invalid_argument if `numTrials` is odd.
     */
    [[nodiscard]]
    std::vector<baselined_task_order> shuffled_baselined_task_orders(std::size_t numTrials, std::mt19937& generator);

    /** \brief Whether a `Result` can be made from the results of calling a
               `First` and a `Second`, in that order.
     */
    template<class Result, class First, class Second>
    inline constexpr bool constructible_from_calls_v{
      requires(First& first, Second& second) { Result{first(), second()}; }
    };

    /** \brief Calls `first` and `second` in `order`, and returns a `Result`
               made from their results, in the order of the parameters.
     */
    template<class Result, class First, class Second>
      requires constructible_from_calls_v<Result, First, Second>
    [[nodiscard]]
    Result call_in_order(First first, Second second, call_order order)
    {
      if(order == call_order::first_then_second)
        return {first(), second()};

      auto secondResult{second()};
      return {first(), std::move(secondResult)};
    }

    /** \brief Returns the duration of a call of each task, as measured by
               `clock`, with the tasks called in `order`.
     */
    template<performance_task F, performance_task S, performance_clock Clock>
    [[nodiscard]]
    relative_performance_durations time_one_trial(const F& fast,
                                                  const S& slow,
                                                  call_order order,
                                                  const Clock& clock)
    {
      return call_in_order<relative_performance_durations>(
               [&fast, &clock]() { return testing::profile(fast, clock); },
               [&slow, &clock]() { return testing::profile(slow, clock); },
               order);
    }

    /** \brief Returns the duration of a call of a task and of its baseline,
               as measured by `clock`, with the two called in `order`.
     */
    template<performance_task T, performance_task B, performance_clock Clock>
    [[nodiscard]]
    task_and_baseline_durations time_task_and_baseline(const task_with_baseline<T, B>& pair,
                                                       call_order order,
                                                       const Clock& clock)
    {
      return call_in_order<task_and_baseline_durations>(
               [&pair, &clock]() { return testing::profile(pair.task, clock); },
               [&pair, &clock]() { return testing::profile(pair.baseline, clock); },
               order);
    }

    /** \brief Returns the duration of a call of each task and of each
               baseline, as measured by `clock`, with the four called in
               `order`.
     */
    template<performance_task F, performance_task FB, performance_task S, performance_task SB, performance_clock Clock>
    [[nodiscard]]
    relative_performance_durations_with_baselines time_one_trial(const task_with_baseline<F, FB>& fast,
                                                                 const task_with_baseline<S, SB>& slow,
                                                                 baselined_task_order order,
                                                                 const Clock& clock)
    {
      return call_in_order<relative_performance_durations_with_baselines>(
               [&fast, order, &clock]() { return impl::time_task_and_baseline(fast, order.fast_vs_baseline, clock); },
               [&slow, order, &clock]() { return impl::time_task_and_baseline(slow, order.slow_vs_baseline, clock); },
               order.fast_vs_slow);
    }

    /** \brief Whether `time_one_trial` times a `Fast` against a `Slow` in an
               `Order`, reading a `Clock`.
     */
    template<class Fast, class Slow, class Order, class Clock>
    inline constexpr bool one_trial_timeable_v{
      requires(const Fast& fast, const Slow& slow, Order order, const Clock& clock) {
        impl::time_one_trial(fast, slow, order, clock);
      }
    };

    /** \brief Returns the durations of the tasks in one trial for each
               element of `orders`, as measured by `clock`.

        Each element of `orders` gives the order in which its trial calls
        the tasks.
     */
    template<class Fast, class Slow, class Order, class Clock>
      requires one_trial_timeable_v<Fast, Slow, Order, Clock>
    [[nodiscard]]
    auto time_each_trial(const Fast& fast, const Slow& slow, const std::vector<Order>& orders, const Clock& clock)
    {
      using durations_t = decltype(impl::time_one_trial(fast, slow, orders.front(), clock));

      // Not std::views::transform: time_one_trial is not equality-preserving,
      // so it does not model the regular_invocable the view requires.
      std::vector<durations_t> trialDurations{};
      trialDurations.reserve(orders.size());
      std::ranges::transform(
        orders,
        std::back_inserter(trialDurations),
        [&fast, &slow, &clock](Order order) { return impl::time_one_trial(fast, slow, order, clock); }
      );

      return trialDurations;
    }

    /** \brief Returns the durations of the tasks in each of `numTrials`
               trials, as measured by `clock`.

        `generator` chooses the order of the tasks in each trial.
     */
    template<performance_task F, performance_task S, performance_clock Clock>
    [[nodiscard]]
    std::vector<relative_performance_durations> time_attempt(const F& fast,
                                                             const S& slow,
                                                             std::size_t numTrials,
                                                             std::mt19937& generator,
                                                             const Clock& clock)
    {
      return impl::time_each_trial(fast, slow, shuffled_call_orders(numTrials, generator), clock);
    }

    /** \brief Returns the durations of the tasks and their baselines in each
               of `numTrials` trials, as measured by `clock`.

        `generator` chooses the order of the tasks and baselines in each
        trial.
     */
    template<performance_task F, performance_task FB, performance_task S, performance_task SB, performance_clock Clock>
    [[nodiscard]]
    std::vector<relative_performance_durations_with_baselines> time_attempt(const task_with_baseline<F, FB>& fast,
                                                                            const task_with_baseline<S, SB>& slow,
                                                                            std::size_t numTrials,
                                                                            std::mt19937& generator,
                                                                            const Clock& clock)
    {
      return impl::time_each_trial(fast, slow, shuffled_baselined_task_orders(numTrials, generator), clock);
    }

    /** \brief Whether `time_attempt` times `fast` against `slow`, reading
               `clock`.
     */
    template<class Fast, class Slow, class Clock>
    concept timeable_against
      = requires(const Fast& fast, const Slow& slow, std::mt19937& generator, const Clock& clock) {
          impl::time_attempt(fast, slow, std::size_t{}, generator, clock);
        };

    /** \brief How many attempts `check_relative_performance` made, and the
               judgement of the last.
     */
    struct relative_performance_conclusion
    {
      std::size_t                    attempts{};
      relative_performance_judgement judgement{};
    };

    /** \brief Executes attempts in turn, and returns how many it made and the
               judgement of the last.

        In standard and false-positive modes, the attempts stop after the
        first which passes. In false-negative mode, they stop after the first
        which fails. In every mode, they stop after the last attempt allowed,
        whatever its outcome.
     */
    template<test_mode Mode, class Fast, class Slow, class Clock>
      requires timeable_against<Fast, Slow, Clock>
    [[nodiscard]]
    relative_performance_conclusion execute_attempts(const Fast& fast,
                                                     const Slow& slow,
                                                     const relative_performance_parameters& parameters,
                                                     const Clock& clock)
    {
      static_assert(relative_performance_max_attempts > 0);

      std::mt19937 generator{std::random_device{}()};

      auto executeAttempt{
        [&](std::size_t attempt) {
          const auto numTrials{num_trials_for_attempt(parameters.minimum_trials, attempt)};

          return judge_trials(impl::time_attempt(fast, slow, numTrials, generator, clock),
                              overlap_multiplier(Mode, attempt),
                              parameters.prediction);
        }
      };

      auto stopsFurtherAttempts{
        [](const relative_performance_judgement& judgement) {
          return (Mode == test_mode::false_negative) ? judgement.failure.has_value() : !judgement.failure.has_value();
        }
      };

      for(const auto attempt : std::views::iota(1uz, relative_performance_max_attempts))
      {
        if(const auto judgement{executeAttempt(attempt)}; stopsFurtherAttempts(judgement))
          return {.attempts{attempt}, .judgement{judgement}};
      }

      return {.attempts{relative_performance_max_attempts},
              .judgement{executeAttempt(relative_performance_max_attempts)}};
    }

    /** \brief Checks that the speed-up of `fast` over `slow` is consistent
               with the predicted interval, as each overload of
               `check_relative_performance` states.
     */
    template<test_mode Mode, class Fast, class Slow, class Clock>
      requires timeable_against<Fast, Slow, Clock>
    bool check_speedup(std::string_view description,
                       test_logger<Mode>& logger,
                       const Fast& fast,
                       const Slow& slow,
                       const relative_performance_parameters& parameters,
                       const Clock& clock)
    {
      sentinel<Mode> sentry{logger, std::string{description}};
      sentry.log_performance_check();

      if(!(parameters.prediction.lower > 1) || !(parameters.prediction.upper > 1))
        throw std::invalid_argument{"Relative performance test requires speed-up factors > 1"};

      if(parameters.prediction.lower > parameters.prediction.upper)
        throw std::invalid_argument{"prediction.upper must be >= prediction.lower"};

      if(parameters.minimum_trials < relative_performance_minimum_trials_floor)
      {
        throw std::invalid_argument{
          std::format("Relative performance test requires minimum_trials >= {}",
                      relative_performance_minimum_trials_floor)
        };
      }

      const auto [attempts, judgement]{execute_attempts<Mode>(fast, slow, parameters, clock)};
      sentry.log_performance_check_attempts(attempts);

      sentry.append_to_message(
        summarize_attempt(judgement,
                          num_trials_for_attempt(parameters.minimum_trials, attempts),
                          attempts,
                          parameters.prediction)
      );

      if(judgement.failure)
      {
        sentry.log_performance_failure("");
      }

      return !judgement.failure;
    }
  }

  /** \brief Checks that the speed-up of `fast` over `slow` is consistent with
             the predicted interval.

       \param description The description reported with the check
       \param logger      The logger to which the result is reported
       \param fast        The task predicted to be the faster of the two
       \param slow        The task against which fast is compared
       \param parameters  The predicted interval of speed-ups, and the fewest
                          trials allowed in any attempt
       \param clock       The clock which measures each duration

       The check makes up to A attempts, where A is
       `relative_performance_max_attempts`. Each attempt runs an even number
       of trials, at least `parameters.minimum_trials`. Each trial runs both
       tasks, and times each by reading `clock` itself, never a copy. The
       fast task runs first in half of an attempt's trials, chosen at random.

       Each trial runs its own copy of each task, made before the timing
       starts. So state a task holds by value starts afresh in every trial,
       while any other state it uses, such as state reached through a
       reference or a pointer, is shared by every trial.

       If a task returns a value, each trial passes the value to
       `do_not_optimize_away` before the timing stops. So the timing includes
       the computation of the value. An optimized build may remove any other
       computation whose result nothing uses. To time such a computation,
       pass its result to `do_not_optimize_away` within the task.

       Each attempt estimates the speed-up: the slow task's duration divided
       by the fast task's, which is less than 1 if the fast task is in fact
       the slower. It does so from each trial's durations, and puts an
       interval around the estimate. The attempt passes if both the fast task
       is distinguishably faster and the interval overlaps the predicted one.

       The check passes if any attempt passes, and stops at the first that
       does. In false-negative mode the polarity is reversed: the check passes
       only if every attempt passes, and stops at the first that fails. In
       every mode each task runs at most
       `parameters.minimum_trials * A * (A + 3) / 4 + A` times.

       The logger's summary reports the last attempt. It gives the speed-up,
       the interval around it, the number of trials, the attempt, and each
       task's trimmed geometric mean duration. If the last attempt failed, the
       summary also gives the reason, a `relative_performance_failure`.

       Any exception thrown by `fast` or `slow` propagates.

       \throws std::invalid_argument if either end of `parameters.prediction`
       is not greater than 1, if its lower end exceeds its upper end, or if
       `parameters.minimum_trials` is less than
       `relative_performance_minimum_trials_floor`.

       \throws std::runtime_error if a trial times either task as zero.

       \throws std::logic_error if `fast` or `slow` performs a check, such as
       `check` or `check_relative_performance`, using the same instance of
       the logger.
   */
  template<test_mode Mode, performance_task F, performance_task S, performance_clock Clock>
  bool check_relative_performance(std::string_view description,
                                  test_logger<Mode>& logger,
                                  F fast,
                                  S slow,
                                  const relative_performance_parameters& parameters,
                                  const Clock& clock)
  {
    return impl::check_speedup(description, logger, fast, slow, parameters, clock);
  }

  /** \brief Checks that the speed-up of `fast.task` over `slow.task`, each
             with its baseline's duration subtracted, is consistent with the
             predicted interval.

       \param description The description reported with the check
       \param logger      The logger to which the result is reported
       \param fast        The task predicted to be the faster of the two, and
                          its baseline
       \param slow        The task against which fast is compared, and its
                          baseline
       \param parameters  The predicted interval of speed-ups, and the fewest
                          trials allowed in any attempt
       \param clock       The clock which measures each duration

       The check is the one which the overload of `check_relative_performance`
       taking two tasks makes, with these differences:
       -# Each trial runs and times the fast task, the slow task and each one's
          baseline. Each of the fast and slow tasks runs immediately before or
          after its baseline. The fast task and its baseline run first in half
          of an attempt's trials. Each task runs before its baseline in half
          of an attempt's trials. Each of those sets of trials is chosen at
          random.
       -# Each trial computes a net duration for each of the fast and slow
          tasks, by subtracting its baseline's duration from its own. The
          speed-up is `slow.task`'s net duration divided by `fast.task`'s.
       -# A net duration which is not positive is recorded as an extreme. An
          attempt fails if either task's net duration is extreme in more than
          a tenth of its trials, rounded up. Otherwise the estimate trims a
          tenth of the trials, rounded up, from each end, and uses no
          extreme's value.
       -# Each of the four tasks runs at most
          `parameters.minimum_trials * A * (A + 3) / 4 + A` times.
       -# The summary also gives each baseline's trimmed mean duration. The
          summary gives no speed-up if the last attempt failed for too many
          extremes.

       \throws std::invalid_argument if either end of `parameters.prediction`
       is not greater than 1, if its lower end exceeds its upper end, or if
       `parameters.minimum_trials` is less than
       `relative_performance_minimum_trials_floor`.

       \throws std::runtime_error if a trial times `fast.task` or `slow.task`
       as zero.

       \throws std::logic_error if any of the four tasks performs a check,
       such as `check` or `check_relative_performance`, using the same
       instance of the logger.
   */
  template<test_mode Mode,
           performance_task F,
           performance_task FB,
           performance_task S,
           performance_task SB,
           performance_clock Clock>
  bool check_relative_performance(std::string_view description,
                                  test_logger<Mode>& logger,
                                  task_with_baseline<F, FB> fast,
                                  task_with_baseline<S, SB> slow,
                                  const relative_performance_parameters& parameters,
                                  const Clock& clock)
  {
    return impl::check_speedup(description, logger, fast, slow, parameters, clock);
  }

  /** \brief class template for plugging into the checker class template
      \anchor performance_extender_primary
   */
  template<test_mode Mode>
  class performance_extender
  {
  public:
    constexpr static test_mode mode{Mode};

    performance_extender() = default;

    template<class Self, performance_task F, performance_task S, performance_clock Clock = std::chrono::steady_clock>
    bool check_relative_performance(this Self& self,
                                    const reporter& description,
                                    F fast,
                                    S slow,
                                    const relative_performance_parameters& parameters,
                                    const Clock& clock = Clock{})
    {
      return testing::check_relative_performance(self.report(description),
                                                 self.m_Logger,
                                                 std::move(fast),
                                                 std::move(slow),
                                                 parameters,
                                                 clock);
    }

    template<class Self,
             performance_task F,
             performance_task FB,
             performance_task S,
             performance_task SB,
             performance_clock Clock = std::chrono::steady_clock>
    bool check_relative_performance(this Self& self,
                                    const reporter& description,
                                    task_with_baseline<F, FB> fast,
                                    task_with_baseline<S, SB> slow,
                                    const relative_performance_parameters& parameters,
                                    const Clock& clock = Clock{})
    {
      return testing::check_relative_performance(self.report(description),
                                                 self.m_Logger,
                                                 std::move(fast),
                                                 std::move(slow),
                                                 parameters,
                                                 clock);
    }
  protected:
    ~performance_extender() = default;

    performance_extender(performance_extender&&)            noexcept = default;
    performance_extender& operator=(performance_extender&&) noexcept = default;
  };

  /** \brief Chooses between a run's diagnostics output and the reference.

      \returns
      -# `referenceOutput`, if `text_with_zeroed_measurements` gives the same
         result for it as for `testOutput`, each given `attemptCountStability`;
      -# `testOutput`, otherwise.
   */
  [[nodiscard]]
  std::string_view postprocess(std::string_view testOutput,
                               std::string_view referenceOutput,
                               attempt_count_stability attemptCountStability);

  /**\brief class template from which all concrete tests should derive */

  template<test_mode Mode>
  class basic_performance_test : public basic_test<Mode, performance_extender<Mode>>
  {
  public:
    using base_type = basic_test<Mode, performance_extender<Mode>>;
    using duration  = base_type::duration;

    using base_type::base_type;

    /** \brief Summarizes the checks.

        In false-positive and false-negative modes, `postprocess` chooses
        between the run's diagnostics output and the contents of the existing
        diagnostics file, given `attempt_count_stability::fluctuating`. The
        summary carries the chosen text.
     */
    [[nodiscard]]
    log_summary summarize(duration delta) const;
  protected:
    ~basic_performance_test() = default;

    basic_performance_test(basic_performance_test&&)            noexcept = default;
    basic_performance_test& operator=(basic_performance_test&&) noexcept = default;
  };

  /** \anchor performance_test_alias */
  using performance_test                = basic_performance_test<test_mode::standard>;
  using performance_false_positive_test = basic_performance_test<test_mode::false_positive>;
  using performance_false_negative_test = basic_performance_test<test_mode::false_negative>;

  template<concrete_test T>
    requires std::derived_from<T, basic_performance_test<T::mode>>
  struct is_performance_test<T> : std::true_type {};

  template<concrete_test T>
    requires is_performance_test_v<T>
  struct is_parallelizable<T> : std::false_type {};
}
