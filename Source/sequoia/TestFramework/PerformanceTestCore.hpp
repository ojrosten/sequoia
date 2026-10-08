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

#include "sequoia/TestFramework/RegularTestCore.hpp"
#include "sequoia/Maths/Statistics/StatisticalAlgorithms.hpp"
#include "sequoia/TestFramework/FileEditors.hpp"

#include <algorithm>
#include <chrono>
#include <random>
#include <ranges>

namespace sequoia::testing
{
  template<std::invocable Task>
  [[nodiscard]]
  std::chrono::duration<double> profile(Task task)
  {
    const timer t{};
    task();

    return t.time_elapsed();
  }

  /** \brief Returns a line reporting a task's mean duration and its standard
             deviation, in seconds.

      The line also gives `numSds`, the number of standard deviations used to
      define a significant result.
   */
  [[nodiscard]]
  std::string duration_summary(std::string_view prefix, double mean, double numSds, double sd);

  /** \brief Returns a suffix reporting the measured speed-up and the range
             predicted for it.
   */
  [[nodiscard]]
  std::string speedup_summary(double speedup, double minSpeedup, double maxSpeedup);

  /** \brief The range of speed-ups predicted for a fast task over a slow one,
             and the statistics which test the prediction.

      - `trials`: the number of trials in the first attempt. Each further
        attempt runs `trials` more than the one before.
      - `num_sds`: the number of standard deviations used to define a
        significant result.
      - `max_attempts`: the number of attempts allowed, counting the first.
        An attempt without the expected outcome is retried until none remain.
   */
  struct relative_performance_parameters
  {
    double      min_speedup;
    double      max_speedup;
    std::size_t trials;
    double      num_sds;
    std::size_t max_attempts;
  };

  /** \brief Function for comparing the performance of a fast task to a slow task.

       \param description the description reported with the check
       \param logger      the logger to which the result is reported
       \param fast        the task predicted to be the faster of the two
       \param slow        the task against which fast is compared
       \param parameters  the predicted range of speed-ups, and the statistics which test it

       For each trial, both the supposedly fast and slow tasks are run. Their order is random.
       Each trial runs its own copy of each task, so state that a task changes in itself does
       not carry over to the next trial.
       When all trials have been completed, the mean and standard deviations are computed for
       both fast and slow tasks. Denote these by fastMean, fastSd and slowMean, slowSd.

       The test fails unless

          fastMean + fastSd < slowMean - slowSd

       that is, unless the fast task is faster by more than the sum of the standard deviations.
       If it is, the analysis branches depending on which standard deviation is bigger.

       if (fastSd >= slowSd)

       then we multiply fastMean by both the min/max predicted speed-up and compare to the range of
       values around slowMean defined by the number of standard deviations. In particular, the test
       is taken to pass if

          (min_speedup * fastMean <= (slowMean + num_sds * slowSd))
       && (max_speedup * fastMean >= (slowMean - num_sds * slowSd))

       which is essentially saying that the range of predicted speed-ups must fall within
       the specified number of standard deviations of slowMean.

       On the other hand

       if (slowSd > fastSd)

       then we divide slowMean by both the min/max predicted speed-up and compare to the range of
       values around fastMean defined by the number of standard deviations. In particular, the test
       is taken to pass if

          (slowMean / max_speedup <= (fastMean + num_sds * fastSd))
       && (slowMean / min_speedup >= (fastMean - num_sds * fastSd))

       \throws std::invalid_argument if a speed-up factor is not greater than 1,
       if min_speedup exceeds max_speedup, if num_sds is not greater than 1, if
       max_attempts is 0 or if trials is less than 5.
   */
  template<test_mode Mode, std::invocable F, std::invocable S>
    requires std::copy_constructible<F> && std::copy_constructible<S>
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

    if(!(parameters.num_sds > 1))
      throw std::invalid_argument{"Number of standard deviations is required to be > 1"};

    if(!parameters.max_attempts)
      throw std::invalid_argument{"Number of attempts is required to be > 0"};

    if(parameters.trials < 5)
      throw std::invalid_argument{"Number of trials is required to be > 4"};

    using namespace std::chrono;
    using namespace maths;

    std::string summary{};
    std::size_t remainingAttempts{parameters.max_attempts};
    bool passed{};

    auto timer{
       [](auto task, std::vector<double>& timings){
         timings.push_back(profile(task).count());
       }
    };

    while(remainingAttempts > 0)
    {
      const auto adjustedTrials{parameters.trials*(parameters.max_attempts - remainingAttempts + 1)};

      std::vector<double> fastData{}, slowData{};
      fastData.reserve(adjustedTrials);
      slowData.reserve(adjustedTrials);

      std::random_device generator{};
      for([[maybe_unused]] auto _ : std::views::iota(0uz, adjustedTrials))
      {
        std::uniform_real_distribution<double> distribution{0.0, 1.0};
        const bool fastFirst{(distribution(generator) < 0.5)};

        if(fastFirst)
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

      auto computeStats{
        [](auto first, auto last) {
          const auto data{sample_standard_deviation(first, last)};
          return std::make_pair(data.first.value(), data.second.value());
        }
      };

      std::ranges::sort(fastData);
      std::ranges::sort(slowData);

      const auto [fastSd, fastMean]{computeStats(fastData.cbegin()+1, fastData.cend()-1)};
      const auto [slowSd, slowMean]{computeStats(slowData.cbegin()+1, slowData.cend()-1)};

      if(fastMean + fastSd < slowMean - slowSd)
      {
        if(fastSd >= slowSd)
        {
          passed =    (parameters.min_speedup * fastMean <= (slowMean + parameters.num_sds * slowSd))
                   && (parameters.max_speedup * fastMean >= (slowMean - parameters.num_sds * slowSd));
        }
        else
        {
          passed =    (slowMean / parameters.max_speedup <= (fastMean + parameters.num_sds * fastSd))
                   && (slowMean / parameters.min_speedup >= (fastMean - parameters.num_sds * fastSd));
        }
      }
      else
      {
        passed = false;
      }

      summary = append_lines(duration_summary("Fast", fastMean, parameters.num_sds, fastSd),
                             duration_summary("Slow", slowMean, parameters.num_sds, slowSd)
                           + speedup_summary(slowMean / fastMean, parameters.min_speedup, parameters.max_speedup));

      if((test_logger<Mode>::mode == test_mode::false_negative) ? !passed : passed)
      {
        break;
      }

      --remainingAttempts;
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

    template<class Self, std::invocable F, std::invocable S>
      requires std::copy_constructible<F> && std::copy_constructible<S>
    bool check_relative_performance(this Self& self,
                                    const reporter& description,
                                    F fast,
                                    S slow,
                                    const relative_performance_parameters& parameters)
    {
      return testing::check_relative_performance(self.report(description), self.m_Logger, fast, slow, parameters);
    }
  protected:
    ~performance_extender() = default;

    performance_extender(performance_extender&&)            noexcept = default;
    performance_extender& operator=(performance_extender&&) noexcept = default;
  };

  /** \brief Chooses between a run's diagnostics output and the reference.

      \returns
      -# `referenceOutput`, if it differs from `testOutput` only in measured
         values;
      -# `testOutput`, otherwise.

      The measured values are the mean, the standard deviation and the
      speed-up, in each line the formatter could have printed. A line counts
      if `duration_summary` reprints it exactly, with or without
      `speedup_summary` after it.
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
