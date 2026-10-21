////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/TestFramework/PerformanceTestCore.hpp"
#include "sequoia/Streaming/Streaming.hpp"
#include "sequoia/TestFramework/PathCheckers.hpp"

#include "sequoia/Maths/Statistics/StatisticalAlgorithms.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <functional>
#include <ranges>
#include <stdexcept>

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

    // A judgement which is not a failure has extremes in at most
    // 2 * num_trimmed_from_each_end(n) of its n trials. Some trial then has a
    // finite speed-up, as estimate_trimmed_mean requires, provided
    // 2 * num_trimmed_from_each_end(n) < n. The inequality holds for every n
    // from three up, so it holds for every number of trials judged if it
    // holds for the floor.
    static_assert(relative_performance_minimum_trials_floor
                    > 2 * num_trimmed_from_each_end(relative_performance_minimum_trials_floor));

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

    /** \brief Returns the trimmed arithmetic mean of `durations`. */
    template<range_of_durations Durations>
    [[nodiscard]]
    std::chrono::duration<double> trimmed_arithmetic_mean(Durations&& durations)
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

    /** \brief Returns `average` of the fast task's durations, and of the
               slow task's.
     */
    template<class Averager>
    [[nodiscard]]
    relative_performance_durations averages_by_task(std::span<const relative_performance_durations> trialDurations,
                                                    Averager average)
    {
      auto fastOf{[](relative_performance_durations durations) { return durations.fast; }};
      auto slowOf{[](relative_performance_durations durations) { return durations.slow; }};

      return {
        .fast{average(trialDurations | std::views::transform(fastOf))},
        .slow{average(trialDurations | std::views::transform(slowOf))}
      };
    }

    /** \brief Returns the trimmed geometric mean of the fast task's
               durations, and of the slow task's.
     */
    [[nodiscard]]
    relative_performance_durations
      trimmed_geometric_means(std::span<const relative_performance_durations> trialDurations)
    {
      return averages_by_task(trialDurations, [](auto durations) { return trimmed_geometric_mean(durations); });
    }

    /** \brief Returns the trimmed arithmetic mean of the fast task's
               durations, and of the slow task's.
     */
    [[nodiscard]]
    relative_performance_durations
      trimmed_arithmetic_means(std::span<const relative_performance_durations> trialDurations)
    {
      return averages_by_task(trialDurations, [](auto durations) { return trimmed_arithmetic_mean(durations); });
    }

    constexpr auto is_positive{
      [](std::chrono::duration<double> duration) noexcept { return duration > std::chrono::duration<double>::zero(); }
    };

    constexpr auto both_positive{
      [](relative_performance_durations durations) noexcept {
        return is_positive(durations.fast) && is_positive(durations.slow);
      }
    };

    /** \brief Returns the logarithm of the slow duration over the fast.

        \throws std::logic_error if either duration is not positive.
     */
    constexpr auto speedup_logarithm{
      [](relative_performance_durations durations) {
        if(!both_positive(durations))
          throw std::logic_error{"A speed-up's logarithm requires positive durations"};

        return std::log(durations.slow / durations.fast);
      }
    };

    /** \brief The estimate of a speed-up, and the reason the speed-up departs
               from the prediction.

        `failure` is `std::nullopt` if there is no departure.
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
          {
            return gateUpper < 0 ? relative_performance_failure::slower
                                 : relative_performance_failure::not_distinguishably_faster;
          }

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

    void throw_if_too_few_trials(std::size_t numTrials)
    {
      if(numTrials < relative_performance_minimum_trials_floor)
      {
        throw std::invalid_argument{
          std::format("Judging relative performance requires at least {} trials; there are {}",
                      relative_performance_minimum_trials_floor,
                      numTrials)
        };
      }
    }

    void throw_if_any_task_duration_not_positive(std::span<const relative_performance_durations> trialDurations)
    {
      if(!std::ranges::all_of(trialDurations, both_positive))
      {
        throw std::runtime_error{"Relative performance test requires task durations > 0; "
                                 "a task too quick for the clock times as zero"};
      }
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

    /** \brief The logarithms of the speed-ups given by net durations, and
               the number of extremes at each end.

        A net duration which is not positive is an extreme: the task could not
        be distinguished from its baseline, so the trial gives no speed-up.
        Where only the fast task's net duration is extreme, the speed-up is
        likely large, so the extreme is counted above every speed-up; where
        only the slow task's is, below. A trial in which both are extreme
        determines nothing, and counts once at each end, favouring neither.
        Extremes are not furnished with floating-point values; their
        occurrences are merely counted.
     */
    struct speedup_logarithms
    {
      std::vector<double> finite{};
      std::size_t         num_extremes_above{},
                          num_extremes_below{};
    };

    /** \brief Returns the logarithms of the speed-ups given by
               `netDurations`, with each extreme counted at its end.
     */
    [[nodiscard]]
    speedup_logarithms speedup_logarithms_of(std::span<const relative_performance_durations> netDurations)
    {
      auto numNotPositive{
        [netDurations](auto projection) {
          return static_cast<std::size_t>(std::ranges::count_if(netDurations, std::not_fn(is_positive), projection));
        }
      };

      return {
        .finite{
            netDurations
          | std::views::filter(both_positive)
          | std::views::transform(speedup_logarithm)
          | std::ranges::to<std::vector>()
        },
        .num_extremes_above{numNotPositive(&relative_performance_durations::fast)},
        .num_extremes_below{numNotPositive(&relative_performance_durations::slow)}
      };
    }

    /** \brief Returns the reason for failure, if either task's net duration
               is extreme in more than `numTrimmed` trials.

        An estimate trims `numTrimmed` logarithms from each end, so each end
        may hold that many extremes, and no more. The fast task
        takes precedence over the slow task.
     */
    [[nodiscard]]
    std::optional<relative_performance_failure>
      failure_to_exceed_baselines(const speedup_logarithms& speedupLogarithms, std::size_t numTrimmed)
    {
      if(speedupLogarithms.num_extremes_above > numTrimmed)
        return relative_performance_failure::fast_task_too_often_no_longer_than_baseline;

      if(speedupLogarithms.num_extremes_below > numTrimmed)
        return relative_performance_failure::slow_task_too_often_no_longer_than_baseline;

      return std::nullopt;
    }

    /** \brief Estimates the mean of `speedupLogarithms` with `numTrimmed`
               trimmed from each end, and the mean's standard error.

        Requires that neither count of extremes exceeds `numTrimmed`, so that
        every extreme is among the logarithms trimmed from the ends. The mean
        ignores those logarithms, and the winsorized standard error
        replaces each with the nearest logarithm kept. So an extreme below can
        be replaced by any value no greater than the least finite logarithm,
        and one above by any value no less than the greatest. The least and
        greatest finite logarithms themselves are used.

        \throws std::logic_error if `speedupLogarithms.finite` is empty.
     */
    [[nodiscard]]
    trimmed_mean_estimate estimate_trimmed_mean(speedup_logarithms speedupLogarithms, std::size_t numTrimmed)
    {
      if(speedupLogarithms.finite.empty())
        throw std::logic_error{"A trimmed mean of speed-ups requires at least one finite logarithm"};

      const auto [least, greatest]{std::ranges::minmax(speedupLogarithms.finite)};

      auto data{std::move(speedupLogarithms.finite)};
      data.insert(data.end(), speedupLogarithms.num_extremes_below, least);
      data.insert(data.end(), speedupLogarithms.num_extremes_above, greatest);

      return estimate_trimmed_mean(std::move(data), numTrimmed);
    }
  }

  namespace impl
  {
    [[nodiscard]]
    relative_performance_judgement judge_trials(std::span<const relative_performance_durations> trialDurations,
                                                double overlapMultiplier,
                                                relative_performance_interval prediction)
    {
      throw_if_too_few_trials(trialDurations.size());
      throw_if_any_task_duration_not_positive(trialDurations);

      auto speedupLogarithmsOfTrials{
          trialDurations
        | std::views::transform(speedup_logarithm)
        | std::ranges::to<std::vector>()
      };

      const auto [failure, estimate]{
        judge_speedup(estimate_trimmed_mean(std::move(speedupLogarithmsOfTrials)), overlapMultiplier, prediction)
      };

      return {.failure{failure}, .estimate{estimate}, .task_durations{trimmed_geometric_means(trialDurations)}};
    }

    [[nodiscard]]
    relative_performance_judgement
      judge_trials(std::span<const relative_performance_durations_with_baselines> trialDurations,
                   double overlapMultiplier,
                   relative_performance_interval prediction)
    {
      throw_if_too_few_trials(trialDurations.size());

      auto taskDurationsOf{
        [](relative_performance_durations_with_baselines durations) -> relative_performance_durations {
          return {.fast{durations.fast.task}, .slow{durations.slow.task}};
        }
      };

      auto baselineDurationsOf{
        [](relative_performance_durations_with_baselines durations) -> relative_performance_durations {
          return {.fast{durations.fast.baseline}, .slow{durations.slow.baseline}};
        }
      };

      auto netDurationsOf{
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

      const auto taskDurations    {transformedTrials(taskDurationsOf)},
                 baselineDurations{transformedTrials(baselineDurationsOf)};

      throw_if_any_task_duration_not_positive(taskDurations);
      throw_if_any_baseline_duration_negative(baselineDurations);

      // Task durations enter the speed-up through ratios, so their means are
      // geometric. Baseline durations enter through subtraction, so theirs
      // are arithmetic: the mean of the differences is then the difference of
      // the means, and a baseline of zero duration is an ordinary value.
      const auto taskMeans    {trimmed_geometric_means(taskDurations)},
                 baselineMeans{trimmed_arithmetic_means(baselineDurations)};

      // An extreme is kept, not dropped. The trim removes it as it removes
      // any outlier, so it adds no bias beyond the trim's own.
      auto speedupLogarithms{speedup_logarithms_of(transformedTrials(netDurationsOf))};
      const auto numTrimmed{num_trimmed_from_each_end(trialDurations.size())};

      if(const auto failure{failure_to_exceed_baselines(speedupLogarithms, numTrimmed)})
      {
        return {
          .failure{failure},
          .task_durations{taskMeans},
          .baseline_durations{baselineMeans}
        };
      }

      const auto [failure, estimate]{
        judge_speedup(estimate_trimmed_mean(std::move(speedupLogarithms), numTrimmed), overlapMultiplier, prediction)
      };

      return {
        .failure{failure},
        .estimate{estimate},
        .task_durations{taskMeans},
        .baseline_durations{baselineMeans}
      };
    }

    [[nodiscard]]
    std::string summarize_attempt(const relative_performance_judgement& judgement,
                                  std::size_t numTrials,
                                  std::size_t attemptsMade,
                                  relative_performance_interval prediction)
    {
      return append_lines(judgement.failure ? verdict_summary(*judgement.failure) : "",
                          judgement.estimate ? speedup_summary(*judgement.estimate, prediction) : "",
                          trials_summary(numTrials, {.made{attemptsMade}, .allowed{relative_performance_max_attempts}}),
                          task_durations_summary(judgement.task_durations),
                          judgement.baseline_durations ? baseline_durations_summary(*judgement.baseline_durations) : "");
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
    std::vector<call_order> shuffled_call_orders(std::size_t numTrials, std::mt19937& generator)
    {
      if(numTrials % 2)
        throw std::invalid_argument{std::format("Call orders require an even number of trials, not {}", numTrials)};

      auto orderOfTrial{
        [numFirst{numTrials / 2}](std::size_t trial) {
          return trial < numFirst ? call_order::first_then_second : call_order::second_then_first;
        }
      };

      auto orders{
          std::views::iota(0uz, numTrials)
        | std::views::transform(orderOfTrial)
        | std::ranges::to<std::vector>()
      };

      std::ranges::shuffle(orders, generator);
      return orders;
    }

    [[nodiscard]]
    std::vector<baselined_task_order> shuffled_baselined_task_orders(std::size_t numTrials, std::mt19937& generator)
    {
      const auto fastVsSlowOrders    {shuffled_call_orders(numTrials, generator)},
                 fastVsBaselineOrders{shuffled_call_orders(numTrials, generator)},
                 slowVsBaselineOrders{shuffled_call_orders(numTrials, generator)};

      auto trialOrder{
        [](call_order fastVsSlow, call_order fastVsBaseline, call_order slowVsBaseline) -> baselined_task_order {
          return {.fast_vs_slow{fastVsSlow}, .fast_vs_baseline{fastVsBaseline}, .slow_vs_baseline{slowVsBaseline}};
        }
      };

      return std::views::zip_transform(trialOrder, fastVsSlowOrders, fastVsBaselineOrders, slowVsBaselineOrders)
           | std::ranges::to<std::vector>();
    }
  }

  [[nodiscard]]
  std::string_view postprocess(std::string_view testOutput, std::string_view referenceOutput)
  {
    const bool differOnlyInMeasurements{
      text_with_zeroed_measurements(testOutput) == text_with_zeroed_measurements(referenceOutput)
    };

    return differOnlyInMeasurements ? referenceOutput : testOutput;
  }

  template<test_mode Mode>
  [[nodiscard]]
  log_summary basic_performance_test<Mode>::summarize(duration delta) const
  {
    if constexpr(Mode == test_mode::standard)
    {
      return base_type::summarize(delta);
    }
    else
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

      std::string outputToUse{postprocess(summary.diagnostics_output(), referenceOutput)};
      summary.diagnostics_output(std::move(outputToUse));

      return summary;
    }
  }

  template class basic_performance_test<test_mode::standard>;
  template class basic_performance_test<test_mode::false_negative>;
  template class basic_performance_test<test_mode::false_positive>;
}
