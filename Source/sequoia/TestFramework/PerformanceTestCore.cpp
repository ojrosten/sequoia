////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/TestFramework/PerformanceTestCore.hpp"
#include "sequoia/Parsing/CommandLineArguments.hpp"
#include "sequoia/Streaming/Streaming.hpp"
#include "sequoia/TestFramework/PathCheckers.hpp"

#include "sequoia/Maths/Statistics/StatisticalAlgorithms.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <functional>
#include <ranges>

namespace sequoia::testing
{
  namespace
  {
    /** \brief The half-width, in standard errors, of the interval with which
               every attempt judges whether the fast task is distinguishably
               faster.
     */
    constexpr double confidence_multiplier{3.0};

    /** \brief Returns the number trimmed from each end of `n` sorted data: a
               tenth of `n`, rounded up.
     */
    [[nodiscard]]
    constexpr std::size_t num_trimmed_from_each_end(std::size_t n) noexcept
    {
      return (n + 9) / 10;
    }

    /** \brief The mean of data once the extremes are trimmed, and its standard
               error.
     */
    struct trimmed_mean_estimate
    {
      double mean{}, standard_error{};
    };

    /** \brief Estimates the mean of `data` with `numTrimmed` data trimmed
               from each end, and the mean's standard error.

        The standard error is Tukey and McLaughlin's:

          SE = s_w / ((1 - 2g/N) * sqrt(N))

        Here N is the number of data, g the number trimmed from each end, and
        s_w the winsorized sample standard deviation.
     */
    [[nodiscard]]
    trimmed_mean_estimate estimate_trimmed_mean(std::vector<double> data, std::size_t numTrimmed)
    {
      std::ranges::sort(data);

      const auto n{data.size()};

      const auto winsorizedVariance{
        maths::winsorized_sample_variance(data, numTrimmed).variance
      };

      const std::ranges::subrange kept{data.cbegin() + numTrimmed, data.cend() - numTrimmed};
      const double fractionKept{1.0 - 2.0 * numTrimmed / n};

      return {
        .mean{maths::mean(kept).value()},
        .standard_error{std::sqrt(winsorizedVariance.value()) / (fractionKept * std::sqrt(n))}
      };
    }

    /** \brief Estimates the trimmed mean of `data`, and its standard error.

        The estimate trims `num_trimmed_from_each_end(data.size())` data from
        each end.
     */
    [[nodiscard]]
    trimmed_mean_estimate estimate_trimmed_mean(std::vector<double> data)
    {
      const auto numTrimmed{num_trimmed_from_each_end(data.size())};
      return estimate_trimmed_mean(std::move(data), numTrimmed);
    }

    template<class Durations>
    concept range_of_durations =    std::ranges::input_range<Durations>
                             && std::ranges::viewable_range<Durations>
                             && std::same_as<std::ranges::range_value_t<Durations>, std::chrono::duration<double>>;

    /** \brief Returns the trimmed mean of `durations`. */
    template<range_of_durations Durations>
    [[nodiscard]]
    std::chrono::duration<double> trimmed_mean(Durations&& durations)
    {
      auto countOf{[](std::chrono::duration<double> duration) { return duration.count(); }};

      auto counts{
          std::forward<Durations>(durations)
        | std::views::transform(countOf)
        | std::ranges::to<std::vector>()
      };

      return std::chrono::duration<double>{estimate_trimmed_mean(std::move(counts)).mean};
    }

    /** \brief Returns the exponential of the trimmed mean of the logarithms of
               `durations`.
     */
    template<range_of_durations Durations>
    [[nodiscard]]
    std::chrono::duration<double> trimmed_geometric_mean(Durations&& durations)
    {
      auto logOf{[](std::chrono::duration<double> duration) { return std::log(duration.count()); }};

      auto logDurations{
          std::forward<Durations>(durations)
        | std::views::transform(logOf)
        | std::ranges::to<std::vector>()
      };

      return std::chrono::duration<double>{std::exp(estimate_trimmed_mean(std::move(logDurations)).mean)};
    }

    /** \brief Returns `average` of the fast durations, and of the slow. */
    template<class Averager>
    [[nodiscard]]
    relative_performance_durations typical_durations(std::span<const relative_performance_durations> trialDurations,
                                                     Averager average)
    {
      auto fastOf{[](relative_performance_durations durations) { return durations.fast; }};
      auto slowOf{[](relative_performance_durations durations) { return durations.slow; }};

      return {
        .fast{average(trialDurations | std::views::transform(fastOf))},
        .slow{average(trialDurations | std::views::transform(slowOf))}
      };
    }

    /** \brief Returns the trimmed geometric mean of each task's durations. */
    [[nodiscard]]
    relative_performance_durations typical_task_durations(std::span<const relative_performance_durations> taskDurations)
    {
      return typical_durations(taskDurations, [](auto durations) { return trimmed_geometric_mean(durations); });
    }

    /** \brief Returns the trimmed mean of each baseline's durations.

        The mean is arithmetic: a baseline's duration is subtracted from its
        task's, and may be zero, which has no logarithm.
     */
    [[nodiscard]]
    relative_performance_durations
      typical_baseline_durations(std::span<const relative_performance_durations> baselineDurations)
    {
      return typical_durations(baselineDurations, [](auto durations) { return trimmed_mean(durations); });
    }

    [[nodiscard]]
    constexpr bool is_positive(std::chrono::duration<double> duration) noexcept
    {
      return duration > std::chrono::duration<double>::zero();
    }

    [[nodiscard]]
    constexpr bool both_positive(relative_performance_durations durations) noexcept
    {
      return is_positive(durations.fast) && is_positive(durations.slow);
    }

    /** \brief Returns the logarithm of the slow duration over the fast. */
    [[nodiscard]]
    double speedup_logarithm(relative_performance_durations durations)
    {
      return std::log(durations.slow / durations.fast);
    }

    /** \brief The reason a speed-up departs from the prediction, if it does,
               and the estimate of the speed-up.
     */
    struct speedup_judgement
    {
      std::optional<relative_performance_failure> failure{};
      relative_performance_estimate               estimate{};
    };

    /** \brief Judges the speed-up of a fast task over a slow one from the
               trimmed mean of the logarithms of their trials' speed-ups.
     */
    [[nodiscard]]
    speedup_judgement judge_speedup(trimmed_mean_estimate speedupLogarithmEstimate,
                                    double overlapMultiplier,
                                    relative_performance_interval prediction)
    {
      const auto [mean, standardError]{speedupLogarithmEstimate};

      // The gate takes confidence_multiplier on every attempt, not
      // overlapMultiplier. An early attempt's overlapMultiplier is smaller,
      // and with it tasks of equal speed would too often pass the gate.
      const double gateLower    {mean - confidence_multiplier * standardError},
                   gateUpper    {mean + confidence_multiplier * standardError},
                   intervalLower{mean - overlapMultiplier * standardError},
                   intervalUpper{mean + overlapMultiplier * standardError};

      const auto failure{
        [gateLower, gateUpper, intervalLower, intervalUpper, prediction]()
          -> std::optional<relative_performance_failure> {
          if(!(gateLower > 0))
            return gateUpper < 0 ? relative_performance_failure::slower
                                 : relative_performance_failure::not_distinguishably_faster;

          if(intervalUpper < std::log(prediction.lower))
            return relative_performance_failure::faster_but_less_than_predicted;

          if(intervalLower > std::log(prediction.upper))
            return relative_performance_failure::suspiciously_fast;

          return std::nullopt;
        }()
      };

      return {
        .failure{failure},
        .estimate{
          .speedup{std::exp(mean)},
          .interval{.lower{std::exp(intervalLower)}, .upper{std::exp(intervalUpper)}}
        }
      };
    }

    void throw_if_too_few_trials(std::size_t trials)
    {
      if(trials < relative_performance_minimum_trials_floor)
        throw std::invalid_argument{
          std::format("Relative performance attempt requires at least {} trials; it has {}",
                      relative_performance_minimum_trials_floor,
                      trials)
        };
    }

    void throw_if_any_task_duration_not_positive(std::span<const relative_performance_durations> trialDurations)
    {
      auto bothPositive{[](relative_performance_durations durations) { return both_positive(durations); }};

      if(!std::ranges::all_of(trialDurations, bothPositive))
        throw std::runtime_error{"Relative performance test requires task durations > 0; "
                                 "a task too quick for the clock times as zero"};
    }

    void throw_if_any_baseline_duration_negative(std::span<const relative_performance_durations> trialDurations)
    {
      auto eitherNegative{
        [](relative_performance_durations durations) {
          constexpr auto zero{std::chrono::duration<double>::zero()};
          return (durations.fast < zero) || (durations.slow < zero);
        }
      };

      if(std::ranges::any_of(trialDurations, eitherNegative))
        throw std::runtime_error{"Relative performance test requires baseline durations >= 0"};
    }

    /** \brief The logarithms of the speed-ups, net of the baselines, in the
               trials of an attempt.

        A trial in which the fast task took no longer than its baseline
        counts as a speed-up of infinity, and so as a logarithm unbounded
        above. A trial in which the slow task did counts as a speed-up of
        zero, and so as a logarithm unbounded below. A trial in which both
        did has no defined speed-up, and counts at each end. These trials are
        counted, not held, since a floating-point type need not represent
        infinity.
     */
    struct speedup_logarithms
    {
      std::vector<double> finite{};
      std::size_t         num_unbounded_above{},
                          num_unbounded_below{};
    };

    /** \brief Returns the logarithms of the speed-ups in each trial, given
               the tasks' durations net of their baselines'.
     */
    [[nodiscard]]
    speedup_logarithms speedup_logarithms_of(std::span<const relative_performance_durations> netDurations)
    {
      auto bothPositive{[](relative_performance_durations durations) { return both_positive(durations); }};
      auto isPositive  {[](std::chrono::duration<double> duration)    { return is_positive(duration); }};

      auto speedupLogarithmOf{[](relative_performance_durations durations) { return speedup_logarithm(durations); }};

      auto numNotPositive{
        [netDurations, isPositive](auto projection) {
          return static_cast<std::size_t>(std::ranges::count_if(netDurations, std::not_fn(isPositive), projection));
        }
      };

      return {
        .finite{
            netDurations
          | std::views::filter(bothPositive)
          | std::views::transform(speedupLogarithmOf)
          | std::ranges::to<std::vector>()
        },
        .num_unbounded_above{numNotPositive(&relative_performance_durations::fast)},
        .num_unbounded_below{numNotPositive(&relative_performance_durations::slow)}
      };
    }

    /** \brief Returns which task took no longer than its baseline in more
               than `numTrimmed` trials, if either did.

        An estimate trims `numTrimmed` logarithms from each end, so each end
        may hold that many unbounded logarithms, and no more. The fast task
        takes precedence over the slow task.
     */
    [[nodiscard]]
    std::optional<relative_performance_failure>
      failure_to_exceed_baselines(const speedup_logarithms& speedupLogarithms, std::size_t numTrimmed)
    {
      if(speedupLogarithms.num_unbounded_above > numTrimmed)
        return relative_performance_failure::fast_task_too_often_no_longer_than_baseline;

      if(speedupLogarithms.num_unbounded_below > numTrimmed)
        return relative_performance_failure::slow_task_too_often_no_longer_than_baseline;

      return std::nullopt;
    }

    /** \brief Estimates the mean of `speedupLogarithms` with `numTrimmed`
               trimmed from each end, and the mean's standard error.

        Requires that neither count of unbounded logarithms exceeds
        `numTrimmed`, and that the logarithms are of more than
        `2 * numTrimmed` trials. So some logarithm is finite.

        Each unbounded logarithm lies within the trim. The trim drops it,
        and winsorizing replaces it with the nearest logarithm kept, so only
        its place enters the estimate, not its value. The least finite
        logarithm therefore stands in for each one unbounded below, and the
        greatest for each one unbounded above.
     */
    [[nodiscard]]
    trimmed_mean_estimate estimate_trimmed_mean(speedup_logarithms speedupLogarithms, std::size_t numTrimmed)
    {
      const auto [least, greatest]{std::ranges::minmax(speedupLogarithms.finite)};

      auto data{std::move(speedupLogarithms.finite)};
      data.insert(data.end(), speedupLogarithms.num_unbounded_below, least);
      data.insert(data.end(), speedupLogarithms.num_unbounded_above, greatest);

      return estimate_trimmed_mean(std::move(data), numTrimmed);
    }

    /** \brief Returns an order for each of `trials` trials.

        `first` is the order of half of the trials, rounded down, and
        `second` the order of the rest. `generator` chooses which trials
        those are.
     */
    template<class Order>
    [[nodiscard]]
    std::vector<Order> shuffled_orders(std::size_t trials, Order first, Order second, std::mt19937& generator)
    {
      auto orderOfTrial{
        [first, second, numFirst{trials / 2}](std::size_t trial) { return trial < numFirst ? first : second; }
      };

      auto orders{
          std::views::iota(0uz, trials)
        | std::views::transform(orderOfTrial)
        | std::ranges::to<std::vector>()
      };

      std::ranges::shuffle(orders, generator);
      return orders;
    }
  }

  namespace impl
  {
    [[nodiscard]]
    relative_performance_attempt_outcome judge_attempt(std::span<const relative_performance_durations> trialDurations,
                                                       double overlapMultiplier,
                                                       relative_performance_interval prediction)
    {
      throw_if_too_few_trials(trialDurations.size());
      throw_if_any_task_duration_not_positive(trialDurations);

      auto speedupLogarithmOf{[](relative_performance_durations durations) { return speedup_logarithm(durations); }};

      auto trialSpeedupLogarithms{
          trialDurations
        | std::views::transform(speedupLogarithmOf)
        | std::ranges::to<std::vector>()
      };

      const auto [failure, estimate]{
        judge_speedup(estimate_trimmed_mean(std::move(trialSpeedupLogarithms)), overlapMultiplier, prediction)
      };

      return {.failure{failure}, .estimate{estimate}, .task_durations{typical_task_durations(trialDurations)}};
    }

    [[nodiscard]]
    relative_performance_attempt_outcome
      judge_attempt(std::span<const relative_performance_durations_with_baselines> trialDurations,
                    double overlapMultiplier,
                    relative_performance_interval prediction)
    {
      throw_if_too_few_trials(trialDurations.size());

      auto tasksOf{
        [](relative_performance_durations_with_baselines durations) -> relative_performance_durations {
          return {.fast{durations.fast.task}, .slow{durations.slow.task}};
        }
      };

      auto baselinesOf{
        [](relative_performance_durations_with_baselines durations) -> relative_performance_durations {
          return {.fast{durations.fast.baseline}, .slow{durations.slow.baseline}};
        }
      };

      auto netOf{
        [](relative_performance_durations_with_baselines durations) -> relative_performance_durations {
          return {
            .fast{durations.fast.task - durations.fast.baseline},
            .slow{durations.slow.task - durations.slow.baseline}
          };
        }
      };

      auto transformedTrials{
        [trialDurations](auto projection) {
          return
              trialDurations
            | std::views::transform(projection)
            | std::ranges::to<std::vector>();
        }
      };

      const auto taskDurations    {transformedTrials(tasksOf)},
                 baselineDurations{transformedTrials(baselinesOf)};

      throw_if_any_task_duration_not_positive(taskDurations);
      throw_if_any_baseline_duration_negative(baselineDurations);

      const auto typicalTaskDurations    {typical_task_durations(taskDurations)},
                 typicalBaselineDurations{typical_baseline_durations(baselineDurations)};

      // A trial in which a task took no longer than its baseline is kept, as
      // an extreme logarithm. The trim removes it as it removes any outlier,
      // so the trial adds no bias beyond the trim's own.
      auto speedupLogarithms{speedup_logarithms_of(transformedTrials(netOf))};
      const auto numTrimmed{num_trimmed_from_each_end(trialDurations.size())};

      if(const auto failure{failure_to_exceed_baselines(speedupLogarithms, numTrimmed)})
        return {
          .failure{failure},
          .task_durations{typicalTaskDurations},
          .baseline_durations{typicalBaselineDurations}
        };

      // At most 2 * numTrimmed trials have an unbounded logarithm. Twice a
      // tenth of the trials, rounded up, is fewer than the trials whenever
      // there are three or more, as there are if the floor passes this
      // assertion. So some logarithm is finite, as estimate_trimmed_mean
      // requires.
      static_assert(relative_performance_minimum_trials_floor
                      > 2 * num_trimmed_from_each_end(relative_performance_minimum_trials_floor));

      const auto [failure, estimate]{
        judge_speedup(estimate_trimmed_mean(std::move(speedupLogarithms), numTrimmed), overlapMultiplier, prediction)
      };

      return {
        .failure{failure},
        .estimate{estimate},
        .task_durations{typicalTaskDurations},
        .baseline_durations{typicalBaselineDurations}
      };
    }

    [[nodiscard]]
    std::string summarize_attempt(const relative_performance_attempt_outcome& outcome,
                                  std::size_t trials,
                                  std::size_t attemptsMade,
                                  relative_performance_interval prediction)
    {
      return append_lines(outcome.failure ? verdict_summary(*outcome.failure) : "",
                          outcome.estimate ? speedup_summary(*outcome.estimate, prediction) : "",
                          trials_summary(trials, {.made{attemptsMade}, .allowed{relative_performance_max_attempts}}),
                          task_durations_summary(outcome.task_durations),
                          outcome.baseline_durations ? baseline_durations_summary(*outcome.baseline_durations) : "");
    }

    [[nodiscard]]
    double overlap_multiplier(test_mode mode, std::size_t attempt)
    {
      // A check in standard mode fails only if its last attempt fails, and
      // the last attempt takes the full multiplier. False-negative mode stops
      // at the first failing attempt. So in that mode the failure of any
      // attempt is the check's outcome, and every attempt takes the full
      // multiplier.
      return (mode == test_mode::false_negative)
        ? confidence_multiplier
        : confidence_multiplier * static_cast<double>(attempt) / relative_performance_max_attempts;
    }

    [[nodiscard]]
    std::vector<task_order> shuffled_task_orders(std::size_t trials, std::mt19937& generator)
    {
      return shuffled_orders(trials, task_order::fast_then_slow, task_order::slow_then_fast, generator);
    }

    [[nodiscard]]
    std::vector<task_order_with_baselines> shuffled_task_orders_with_baselines(std::size_t trials,
                                                                               std::mt19937& generator)
    {
      auto shuffledBaselineOrders{
        [trials, &generator]() {
          return shuffled_orders(trials,
                                 task_and_baseline_order::task_then_baseline,
                                 task_and_baseline_order::baseline_then_task,
                                 generator);
        }
      };

      const auto pairOrders{shuffled_task_orders(trials, generator)};
      const auto fastOrders{shuffledBaselineOrders()},
                 slowOrders{shuffledBaselineOrders()};

      auto trialOrder{
        [](task_order pairs, task_and_baseline_order fast, task_and_baseline_order slow) -> task_order_with_baselines {
          return {.pairs{pairs}, .fast{fast}, .slow{slow}};
        }
      };

      return std::views::zip_transform(trialOrder, pairOrders, fastOrders, slowOrders) | std::ranges::to<std::vector>();
    }
  }

  [[nodiscard]]
  std::string_view postprocess(std::string_view testOutput,
                               std::string_view referenceOutput,
                               attempt_count_stability attemptCountStability)
  {
    const bool differOnlyInMeasurements{
         text_with_zeroed_measurements(testOutput,      attemptCountStability)
      == text_with_zeroed_measurements(referenceOutput, attemptCountStability)
    };

    return differOnlyInMeasurements ? referenceOutput : testOutput;
  }

  [[nodiscard]]
  bool is_coarse_sleep(std::chrono::duration<double, std::milli> slept,
                       std::chrono::duration<double, std::milli> target)
  {
    return slept >= 2 * target;
  }

  [[nodiscard]]
  std::string coarse_sleep_message(std::chrono::duration<double, std::milli> slept,
                                   std::chrono::duration<double, std::milli> target)
  {
    using parsing::commandline::warning;
    return warning({std::format("Sleeps of {:.1f} ms repeatedly lasted {:.1f} ms or more, "
                                "so timings built on sleeps are unreliable",
                                target.count(),
                                slept.count()),
                    "On Windows, the likely cause is that the finest timer resolution is not in effect"});
  }

  template<test_mode Mode>
  [[nodiscard]]
  log_summary basic_performance_test<Mode>::summarize(duration delta) const
  {
    if constexpr(Mode == test_mode::standard)
      return base_type::summarize(delta);
    else
      return summarize(delta, attempt_count_stability::fluctuating);
  }

  template<test_mode Mode>
  [[nodiscard]]
  log_summary basic_performance_test<Mode>::summarize(duration delta,
                                                      attempt_count_stability attemptCountStability) const
    requires (Mode != test_mode::standard)
  {
    auto summary{base_type::summarize(delta)};

    const auto referenceOutput{
      [filename{this->diagnostics_file_paths().false_positive_or_negative_file_path()}]() -> std::string {
        if(std::filesystem::exists(filename))
        {
          if(auto contents{read_to_string(filename, std::ios_base::in | std::ios_base::binary)})
            return std::move(*contents);

          throw std::runtime_error{report_failed_read(filename)};
        }

        return "";
      }()
    };

    std::string outputToUse{postprocess(summary.diagnostics_output(), referenceOutput, attemptCountStability)};
    summary.diagnostics_output(std::move(outputToUse));

    return summary;
  }

  template class basic_performance_test<test_mode::standard>;
  template class basic_performance_test<test_mode::false_negative>;
  template class basic_performance_test<test_mode::false_positive>;
}
