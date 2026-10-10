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
#include "sequoia/TestFramework/RegularTestCore.hpp"
#include "sequoia/TestFramework/FileEditors.hpp"

#include <algorithm>
#include <chrono>
#include <concepts>
#include <iterator>
#include <optional>
#include <random>
#include <ranges>
#include <span>
#include <type_traits>

#if defined(_MSC_VER) && !defined(__clang__)
  #include <atomic>
  #include <memory>
#endif

namespace sequoia::testing
{
  /** \brief Prevents the optimizer from removing the computation of `value`.

      The compiler must treat `value` as read at the call, and as possibly
      changed by it. So an optimized build computes `value` before the call,
      even if nothing else reads `value`. And a computation after the call
      which depends on the state of `value` is made after the call.

      The compiler may still compute `value` earlier, at any point after the
      inputs to the computation are known. That may be during compilation.
   */
  template<class T>
  void do_not_optimize_away(const T& value) noexcept
  {
#if defined(_MSC_VER) && !defined(__clang__)
    // The store lets code outside this function reach `value`. MSVC compiles
    // the fence as a compiler barrier: every write to memory such code can
    // reach completes before the fence, and every read of it after the fence
    // is made afresh. Without either, the computation of `value` may be lost.
    static thread_local const T* volatile observedAddress{};
    observedAddress = std::addressof(value);
    std::atomic_signal_fence(std::memory_order_seq_cst);
#else
    // The memory clobber makes the compiler treat all memory as possibly read
    // and changed: both `value` and memory it points to, such as a vector's
    // elements. Without it, only the bytes of `value` would be read.
    asm volatile("" : : "m"(value) : "memory");
#endif
  }

  /** \brief Returns the duration of a call of `task`.

      The duration includes the computation of any value the call returns,
      even if nothing else reads the value.
   */
  template<class Task>
    requires std::invocable<Task&>
  [[nodiscard]]
  std::chrono::duration<double> profile(Task task)
  {
    const timer t{};

    // Without this call, the compiler may make the task's computation once,
    // before any timing starts, if it can see that every call of the task
    // gives the same result.
    do_not_optimize_away(task);

    if constexpr(std::is_void_v<std::invoke_result_t<Task&>>)
      task();
    else
      do_not_optimize_away(task());

    return t.time_elapsed();
  }

  /** \brief A type that `check_relative_performance` accepts as a task.

      Each trial copies the task, and calls the copy as an lvalue, with no
      arguments.
   */
  template<class T>
  concept performance_task = std::copy_constructible<T> && std::invocable<T&>;

  /** \brief The maximum number of attempts `check_relative_performance`
             makes, counting the first.
   */
  inline constexpr std::size_t relative_performance_max_attempts{3};

  /** \brief The interval of speed-ups predicted for a fast task over a slow
             one, and the fewest trials in any attempt.
   */
  struct relative_performance_parameters
  {
    relative_performance_interval prediction;
    std::size_t                   minimum_trials;
  };

  namespace impl
  {
    /** \brief The outcome of one attempt of `check_relative_performance`:
               why it failed, if it did, and its measurements.
     */
    struct relative_performance_outcome
    {
      std::optional<relative_performance_failure> failure{};
      relative_performance_estimate               estimate{};
      relative_performance_durations              durations{};
    };

    /** \brief Judges the speed-up of a fast task over a slow one from their
               durations in each trial of one attempt.

        \pre `trialDurations` has at least 5 elements.

        \throws std::runtime_error if any duration is not greater than zero.
     */
    [[nodiscard]]
    relative_performance_outcome judge_attempt(std::span<const relative_performance_durations> trialDurations,
                                               double overlapMultiplier,
                                               relative_performance_interval prediction);

    /** \brief Summarizes an attempt in lines of text.

        The lines give the reason the attempt failed, if it did. Then they give
        the speed-up, the trials and the task durations.
     */
    [[nodiscard]]
    std::string summarize_attempt(const relative_performance_outcome& outcome,
                                  std::size_t trials,
                                  std::size_t attempt,
                                  relative_performance_interval prediction);

    /** \brief Returns the half-width, in standard errors, of the interval
               which attempt `attempt` compares with the predicted interval.
     */
    [[nodiscard]]
    double overlap_multiplier(test_mode mode, std::size_t attempt);

    /** \brief Returns the number of trials which attempt `attempt` runs. */
    [[nodiscard]]
    constexpr std::size_t trials_of_attempt(std::size_t minimumTrials, std::size_t attempt) noexcept
    {
      return minimumTrials * (attempt + 1) / 2;
    }

    enum class task_order { fast_then_slow, slow_then_fast };

    /** \brief Returns the order in which the two tasks run in each of
               `trials` trials.

        The fast task runs first in half of the trials, rounded down.
        `generator` chooses which trials those are.
     */
    [[nodiscard]]
    std::vector<task_order> shuffled_task_orders(std::size_t trials, std::mt19937& generator);

    /** \brief Returns the duration of a call of each task, with the tasks
               called in `order`.
     */
    template<performance_task F, performance_task S>
    [[nodiscard]]
    relative_performance_durations time_trial(const F& fast, const S& slow, task_order order)
    {
      if(order == task_order::fast_then_slow)
        return {.fast{profile(fast)}, .slow{profile(slow)}};

      const auto slowDuration{profile(slow)};
      return {.fast{profile(fast)}, .slow{slowDuration}};
    }

    /** \brief The attempt which decided `check_relative_performance`, and its
               outcome.
     */
    struct relative_performance_decision
    {
      std::size_t                  attempt{};
      relative_performance_outcome outcome{};
    };

    /** \brief Executes attempts until one decides the check, and returns the
               last attempt executed.

        An attempt which passes decides the check. In false-negative mode, an
        attempt which fails decides the check instead. The last attempt
        allowed decides the check whatever its outcome.
     */
    template<test_mode Mode, performance_task F, performance_task S>
    [[nodiscard]]
    relative_performance_decision execute_attempts(const F& fast,
                                                   const S& slow,
                                                   const relative_performance_parameters& parameters)
    {
      static_assert(relative_performance_max_attempts > 0);

      std::mt19937 generator{std::random_device{}()};

      auto timeTrial{[&fast, &slow](task_order order) { return time_trial(fast, slow, order); }};

      auto executeAttempt{
        [&generator, &timeTrial, &parameters](std::size_t attempt) {
          const auto trials{trials_of_attempt(parameters.minimum_trials, attempt)};

          // Not std::views::transform: timeTrial is not equality-preserving,
          // so it does not model the regular_invocable the view requires.
          std::vector<relative_performance_durations> trialDurations{};
          trialDurations.reserve(trials);
          std::ranges::transform(shuffled_task_orders(trials, generator),
                                 std::back_inserter(trialDurations),
                                 timeTrial);

          return judge_attempt(trialDurations, overlap_multiplier(Mode, attempt), parameters.prediction);
        }
      };

      auto isDecisive{
        [](const relative_performance_outcome& outcome) {
          return (Mode == test_mode::false_negative) ? outcome.failure.has_value() : !outcome.failure.has_value();
        }
      };

      for(const auto attempt : std::views::iota(1uz, relative_performance_max_attempts))
      {
        if(const auto outcome{executeAttempt(attempt)}; isDecisive(outcome))
          return {.attempt{attempt}, .outcome{outcome}};
      }

      return {.attempt{relative_performance_max_attempts}, .outcome{executeAttempt(relative_performance_max_attempts)}};
    }
  }

  /** \brief Checks that the speed-up of `fast` over `slow` is consistent with
             the predicted interval.

       \param description The description reported with the check
       \param logger      The logger to which the result is reported
       \param fast        The task predicted to be the faster of the two
       \param slow        The task against which fast is compared
       \param parameters  The predicted interval of speed-ups, and the fewest
                          trials in any attempt

       The check makes up to A attempts, where A is
       `relative_performance_max_attempts`. Each attempt runs at least
       `parameters.minimum_trials` trials. Each trial runs both tasks, and
       times each. The fast task runs first in half of an attempt's trials,
       rounded down. Those trials are chosen at random.

       Each trial runs its own copy of each task, made before the timing
       starts. So state a task holds by value starts afresh in every trial,
       while any other state it uses, such as state reached through a
       reference or a pointer, is shared by every trial.

       If a task returns a value, each trial passes the value to
       `do_not_optimize_away` before the timing stops. So the timing includes
       the computation of the value. An optimized build may remove any other
       computation whose result nothing uses. To time such a computation,
       pass its result to `do_not_optimize_away` within the task.

       Each attempt estimates the speed-up from the ratio of the tasks'
       durations in each trial, and puts an interval around the estimate. The
       attempt passes if both the fast task is distinguishably faster and the
       interval overlaps the predicted one.

       The check passes if any attempt passes, and stops at the first that
       does. In false-negative mode the polarity is reversed: the check passes
       only if every attempt passes, and stops at the first that fails. In
       every mode each task runs at most
       `parameters.minimum_trials * A * (A + 3) / 4` times.

       The summary reports the last attempt. It gives the speed-up, the
       interval around it, the number of trials, the attempt, and each task's
       typical duration. If the last attempt failed, the summary also gives
       the reason, a `relative_performance_failure`.

       Any exception thrown by `fast` or `slow` propagates.

       \throws std::invalid_argument if either end of `parameters.prediction`
       is not greater than 1, if its lower end exceeds its upper end, or if
       `parameters.minimum_trials` is less than 10.

       \throws std::runtime_error if a trial times either task as zero.
   */
  template<test_mode Mode, performance_task F, performance_task S>
  bool check_relative_performance(std::string_view description,
                                  test_logger<Mode>& logger,
                                  F fast,
                                  S slow,
                                  const relative_performance_parameters& parameters)
  {
    sentinel<Mode> sentry{logger, std::string{description}};
    sentry.log_performance_check();

    if(!(parameters.prediction.lower > 1) || !(parameters.prediction.upper > 1))
      throw std::invalid_argument{"Relative performance test requires speed-up factors > 1"};

    if(parameters.prediction.lower > parameters.prediction.upper)
      throw std::invalid_argument{"prediction.upper must be >= prediction.lower"};

    if(parameters.minimum_trials < 10)
      throw std::invalid_argument{"Relative performance test requires minimum_trials >= 10"};

    const auto [attempt, outcome]{impl::execute_attempts<Mode>(fast, slow, parameters)};

    sentry.append_to_message(
      impl::summarize_attempt(outcome,
                              impl::trials_of_attempt(parameters.minimum_trials, attempt),
                              attempt,
                              parameters.prediction)
    );

    if(outcome.failure)
    {
      sentry.log_performance_failure("");
    }

    return !outcome.failure;
  }

  /** \brief Whether `slept`, compared to `target`, indicates sleeps rounded up to a coarse timer tick. */
  [[nodiscard]]
  bool is_coarse_sleep(std::chrono::duration<double, std::milli> slept,
                       std::chrono::duration<double, std::milli> target);

  /** \brief A warning that sleeps of `target` lasted `slept` or more, so timings built on sleeps are unreliable. */
  [[nodiscard]]
  std::string coarse_sleep_message(std::chrono::duration<double, std::milli> slept,
                                   std::chrono::duration<double, std::milli> target);

  /** \brief class template for plugging into the checker class template
      \anchor performance_extender_primary
   */
  template<test_mode Mode>
  class performance_extender
  {
  public:
    constexpr static test_mode mode{Mode};

    performance_extender() = default;

    template<class Self, performance_task F, performance_task S>
    bool check_relative_performance(this Self& self,
                                    const reporter& description,
                                    F fast,
                                    S slow,
                                    const relative_performance_parameters& parameters)
    {
      return testing::check_relative_performance(self.report(description),
                                                 self.m_Logger,
                                                 std::move(fast),
                                                 std::move(slow),
                                                 parameters);
    }
  protected:
    ~performance_extender() = default;

    performance_extender(performance_extender&&)            noexcept = default;
    performance_extender& operator=(performance_extender&&) noexcept = default;
  };

  /** \brief Chooses between a run's diagnostics output and the reference.

      \returns
      -# `referenceOutput`, if `text_with_zeroed_measurements` gives the same
         result for it as for `testOutput`;
      -# `testOutput`, otherwise.
   */
  [[nodiscard]]
  std::string_view postprocess(std::string_view testOutput, std::string_view referenceOutput);

  /**\brief class template from which all concrete tests should derive */

  template<test_mode Mode>
  class basic_performance_test : public basic_test<Mode, performance_extender<Mode>>
  {
  public:
    using base_type = basic_test<Mode, performance_extender<Mode>>;
    using duration  = base_type::duration;

    using base_type::base_type;

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
  inline constexpr bool is_performance_test_v{std::derived_from<T, basic_performance_test<T::mode>>};

  template<concrete_test T>
    requires is_performance_test_v<T>
  struct is_parallelizable<T> : std::false_type {};
}
