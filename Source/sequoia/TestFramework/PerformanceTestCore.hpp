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

#include <chrono>
#include <concepts>
#include <optional>
#include <random>
#include <ranges>
#include <span>

namespace sequoia::testing
{
  /** \brief Returns the duration of a call of `task`. */
  template<class Task>
    requires std::invocable<Task&>
  [[nodiscard]]
  std::chrono::duration<double> profile(Task task)
  {
    const timer t{};
    task();

    return t.time_elapsed();
  }

  /** \brief A task which can be copied, and called with no arguments through an lvalue. */
  template<class T>
  concept copy_constructible_task = std::invocable<T&> && std::copy_constructible<T>;

  /** \brief The maximum number of attempts `check_relative_performance`
             makes, counting the first.
   */
  inline constexpr std::size_t relative_performance_max_attempts{3};

  /** \brief The interval of speed-ups predicted for a fast task over a slow
             one, and the number of trials in the first attempt.

      Attempt `k` runs `k * trials` trials.
   */
  struct relative_performance_parameters
  {
    relative_performance_interval prediction{};
    std::size_t                   trials{};
  };

  namespace impl
  {
    /** \brief The judgement of one attempt of `check_relative_performance`,
               and the measurements which support it.
     */
    struct relative_performance_judgement
    {
      std::optional<relative_performance_failure> failure{};
      relative_performance_estimate               estimate{};
      relative_performance_durations              durations{};
    };

    /** \brief Judges the speed-up of a fast task over a slow one from the
               durations of each in one attempt, paired by trial.

        \pre `fastDurations` and `slowDurations` have the same size, which is
        at least 5.

        \throws std::runtime_error if any duration is not greater than zero.
     */
    [[nodiscard]]
    relative_performance_judgement judge_attempt(std::span<const std::chrono::duration<double>> fastDurations,
                                                 std::span<const std::chrono::duration<double>> slowDurations,
                                                 double overlapMultiplier,
                                                 relative_performance_interval prediction);

    /** \brief Returns the lines reporting an attempt.

        The lines give the reason the attempt failed, if it did. Then they give
        the speed-up, the trials and the task durations.
     */
    [[nodiscard]]
    std::string attempt_summary(const relative_performance_judgement& judgement,
                                std::size_t trials,
                                std::size_t attempt,
                                relative_performance_interval prediction);

    /** \brief Returns the half-width, in standard errors, of the interval
               which attempt `attempt` compares with the predicted interval.
     */
    [[nodiscard]]
    double overlap_multiplier(test_mode mode, std::size_t attempt);

    enum class first_task { fast, slow };

    /** \brief Returns which task runs first in each of `trials` trials.

        The fast task runs first in half of the trials, rounded down. The
        order is shuffled by `generator`.
     */
    [[nodiscard]]
    std::vector<first_task> shuffled_task_orders(std::size_t trials, std::mt19937& generator);
  }

  /** \brief Checks that the speed-up of `fast` over `slow` is consistent with
             the predicted interval.

       \param description The description reported with the check
       \param logger      The logger to which the result is reported
       \param fast        The task predicted to be the faster of the two
       \param slow        The task against which fast is compared
       \param parameters  The predicted interval of speed-ups, and the number
                          of trials

       The check makes up to A attempts, where A is
       `relative_performance_max_attempts`. Attempt k runs k times
       `parameters.trials` trials. Each trial runs both tasks, and times each.
       The fast task runs first in half of an attempt's trials, rounded down.
       Those trials are chosen at random.

       Each trial runs its own copy of each task, made before the timing
       starts. So state a task holds by value starts afresh in every trial,
       while any other state it uses, such as state reached through a
       reference or a pointer, is shared by every trial.

       Each attempt estimates the speed-up from the ratio of the tasks'
       durations in each trial, and puts an interval around the estimate. The
       attempt passes if both the fast task is distinguishably faster and the
       interval overlaps the predicted one.

       The check passes if any attempt passes, and stops at the first that
       does. In false-negative mode the polarity is reversed: the check passes
       only if every attempt passes, and stops at the first that fails. In
       every mode each task runs at most
       `parameters.trials * A * (A + 1) / 2` times.

       The summary reports the last attempt. It gives the speed-up, the
       interval around it, the number of trials, the attempt, and each task's
       typical duration. If the last attempt failed, the summary also gives
       the reason, a `relative_performance_failure`.

       Any exception thrown by `fast` or `slow` propagates.

       \throws std::invalid_argument if either end of `parameters.prediction`
       is not greater than 1, if its lower end exceeds its upper end, or if
       `parameters.trials` is less than 5.

       \throws std::runtime_error if a trial times either task as zero.
   */
  template<test_mode Mode, copy_constructible_task F, copy_constructible_task S>
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

    if(parameters.trials < 5)
      throw std::invalid_argument{"Number of trials is required to be > 4"};

    std::string summary{};
    bool passed{};

    auto recordDuration{
       [](auto task, std::vector<std::chrono::duration<double>>& durations){
         durations.push_back(profile(std::move(task)));
       }
    };

    std::mt19937 generator{std::random_device{}()};

    for(const auto attempt : std::views::iota(1uz, relative_performance_max_attempts + 1))
    {
      const auto trialsOfAttempt{parameters.trials * attempt};

      std::vector<std::chrono::duration<double>> fastDurations{}, slowDurations{};
      fastDurations.reserve(trialsOfAttempt);
      slowDurations.reserve(trialsOfAttempt);

      for(const auto first : impl::shuffled_task_orders(trialsOfAttempt, generator))
      {
        if(first == impl::first_task::fast)
        {
          recordDuration(fast, fastDurations);
          recordDuration(slow, slowDurations);
        }
        else
        {
          recordDuration(slow, slowDurations);
          recordDuration(fast, fastDurations);
        }
      }

      const auto judgement{
        impl::judge_attempt(fastDurations,
                            slowDurations,
                            impl::overlap_multiplier(Mode, attempt),
                            parameters.prediction)
      };

      passed  = !judgement.failure;
      summary = impl::attempt_summary(judgement, trialsOfAttempt, attempt, parameters.prediction);

      if((Mode == test_mode::false_negative) ? !passed : passed)
      {
        break;
      }
    }

    sentry.append_to_message(summary);

    if(!passed)
    {
      sentry.log_performance_failure("");
    }

    return passed;
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

    template<class Self, copy_constructible_task F, copy_constructible_task S>
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
