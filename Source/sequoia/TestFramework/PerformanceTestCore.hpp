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

  /** \brief The half-width, in standard errors, of the interval which
             decides the final attempt of `check_relative_performance`.
   */
  inline constexpr double relative_performance_confidence_multiplier{3.0};

  /** \brief The maximum number of attempts `check_relative_performance`
             makes, counting the first.
   */
  inline constexpr std::size_t relative_performance_max_attempts{3};

  /** \brief The range of speed-ups predicted for a fast task over a slow one,
             and the number of trials with which to test it.

      - `trials`: the number of trials in the first attempt. Attempt `k` runs
        `k * trials`.
   */
  struct relative_performance_parameters
  {
    double      min_speedup;
    double      max_speedup;
    std::size_t trials;
  };

  namespace impl
  {
    /** \brief The judgement of one attempt of `check_relative_performance`,
               and the measurements which support it.
     */
    struct relative_performance_judgement
    {
      std::optional<relative_performance_failure> failure{};
      double speedup{}, interval_min{}, interval_max{}, fast_duration{}, slow_duration{};
    };

    /** \brief Judges the speed-up of a fast task over a slow one from the
               durations of each in one attempt, paired by trial.
     */
    [[nodiscard]]
    relative_performance_judgement judge_attempt(std::span<const double> fastDurations,
                                                 std::span<const double> slowDurations,
                                                 double confidenceMultiplier,
                                                 const relative_performance_parameters& parameters);

    /** \brief Returns the lines reporting an attempt.

        The lines give the reason the attempt failed, if it did. Then they give
        the speed-up, the trials and the task durations.
     */
    [[nodiscard]]
    std::string attempt_summary(const relative_performance_judgement& judgement,
                                std::size_t trials,
                                std::size_t attempt,
                                const relative_performance_parameters& parameters);

    /** \brief Returns the half-width, in standard errors, of the interval
               which decides attempt `attempt`.
     */
    [[nodiscard]]
    double confidence_multiplier(test_mode mode, std::size_t attempt);

    enum class first_task { fast, slow };

    /** \brief Returns which task runs first in each of `trials` trials.

        The fast task runs first in half of the trials, rounded down. The
        order is shuffled by `generator`.
     */
    [[nodiscard]]
    std::vector<first_task> shuffled_task_orders(std::size_t trials, std::mt19937& generator);
  }

  /** \brief Checks that the speed-up of `fast` over `slow` is consistent with
             the predicted range.

       \param description The description reported with the check
       \param logger      The logger to which the result is reported
       \param fast        The task predicted to be the faster of the two
       \param slow        The task against which fast is compared
       \param parameters  The predicted range of speed-ups, and the number of
                          trials

       The check makes up to A attempts, where A is
       `relative_performance_max_attempts`. Attempt k runs k times
       `parameters.trials` trials. Each trial runs both tasks, and times each.
       The fast task runs first in half of an attempt's trials, rounded down.
       Those trials are chosen at random.

       Each trial runs its own copy of each task, made before the timing
       starts. So state a task holds by value starts afresh in every trial,
       while any other state it uses, such as state reached through a
       reference or a pointer, is shared by every trial.

       An attempt of N trials takes the log-ratio of each trial's durations,
       `d = ln(slow / fast)`. The check trims the smallest g and the largest g
       of the N log-ratios, where g is a tenth of N, rounded up. The estimate,
       m, is the mean of the rest. Its standard error is

          SE = s_w / ((1 - 2g/N) * sqrt(N))

       where s_w is the winsorized sample standard deviation of the
       log-ratios. Attempt k passes if and only if both

          m - c_k * SE > 0

       and the interval [m - c_k * SE, m + c_k * SE] overlaps
       [ln(min_speedup), ln(max_speedup)]. The multiplier rises with the
       attempts:

          c_k = c * k / A

       where c is `relative_performance_confidence_multiplier`.

       The check passes if any attempt passes, and stops at the first that
       does. In false-negative mode the polarity is reversed: the check passes
       only if every attempt passes, and stops at the first that fails. In
       that mode every attempt uses c, in place of c_k. In either mode each
       task runs at most `parameters.trials * A * (A + 1) / 2` times.

       The summary reports the last attempt. It gives the speed-up, exp(m),
       and the interval around it, [exp(m - c_k * SE), exp(m + c_k * SE)].
       It gives the number of trials, and the attempt. And it gives each
       task's typical duration: the exponential of the trimmed mean of the
       logarithms of its durations. If the last attempt failed, the summary
       also gives the reason, a `relative_performance_failure`:
       -# `slower`, if the interval lies wholly below 0;
       -# `not_distinguishably_faster`, if the interval otherwise includes 0;
       -# `faster_but_less_than_predicted`, if the interval lies wholly above
          0, and below ln(min_speedup);
       -# `suspiciously_fast`, if the interval lies wholly above
          ln(max_speedup).

       \throws std::invalid_argument if a speed-up factor is not greater
       than 1, if min_speedup exceeds max_speedup or if trials is less than 5.
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

    if(!(parameters.min_speedup > 1) || !(parameters.max_speedup > 1))
      throw std::invalid_argument{"Relative performance test requires speed-up factors > 1"};

    if(parameters.min_speedup > parameters.max_speedup)
      throw std::invalid_argument{"max_speedup must be >= min_speedup"};

    if(parameters.trials < 5)
      throw std::invalid_argument{"Number of trials is required to be > 4"};

    std::string summary{};
    bool passed{};

    auto timer{
       [](auto task, std::vector<double>& timings){
         timings.push_back(profile(std::move(task)).count());
       }
    };

    std::mt19937 generator{std::random_device{}()};

    for(const auto attempt : std::views::iota(1uz, relative_performance_max_attempts + 1))
    {
      const auto trialsOfAttempt{parameters.trials * attempt};

      std::vector<double> fastData{}, slowData{};
      fastData.reserve(trialsOfAttempt);
      slowData.reserve(trialsOfAttempt);

      for(const auto first : impl::shuffled_task_orders(trialsOfAttempt, generator))
      {
        if(first == impl::first_task::fast)
        {
          timer(fast, fastData);
          timer(slow, slowData);
        }
        else
        {
          timer(slow, slowData);
          timer(fast, fastData);
        }
      }

      const auto judgement{
        impl::judge_attempt(fastData, slowData, impl::confidence_multiplier(Mode, attempt), parameters)
      };

      passed  = !judgement.failure;
      summary = impl::attempt_summary(judgement, trialsOfAttempt, attempt, parameters);

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
      -# `referenceOutput`, if it differs from `testOutput` only in measured values, as
         `text_with_zeroed_measurements` defines them;
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
