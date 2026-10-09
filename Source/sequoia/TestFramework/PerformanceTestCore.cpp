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

    /** \brief Estimates the trimmed mean of `data`, and its standard error.

        The standard error is Tukey and McLaughlin's:

          SE = s_w / ((1 - 2g/N) * sqrt(N))

        Here N is the number of data, g the number trimmed from each end, and
        s_w the winsorized sample standard deviation.
     */
    [[nodiscard]]
    trimmed_mean_estimate estimate_trimmed_mean(std::vector<double> data)
    {
      std::ranges::sort(data);

      const auto n{data.size()};
      const auto numTrimmed{num_trimmed_from_each_end(n)};

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

    /** \brief Returns the exponential of the trimmed mean of the logarithms of
               `durations`.
     */
    template<std::ranges::input_range Durations>
      requires std::same_as<std::ranges::range_value_t<Durations>, std::chrono::duration<double>>
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
  }

  namespace impl
  {
    [[nodiscard]]
    relative_performance_outcome judge_attempt(std::span<const relative_performance_durations> trialDurations,
                                               double overlapMultiplier,
                                               relative_performance_interval prediction)
    {
      auto eitherNotPositive{
        [](relative_performance_durations durations) {
          constexpr auto zero{std::chrono::duration<double>::zero()};
          return !(durations.fast > zero) || !(durations.slow > zero);
        }
      };

      if(std::ranges::any_of(trialDurations, eitherNotPositive))
        throw std::runtime_error{"Relative performance test requires task durations > 0; "
                                 "a task too quick for the clock times as zero"};

      auto logRatio{
        [](relative_performance_durations durations) { return std::log(durations.slow / durations.fast); }
      };

      auto logRatios{
          trialDurations
        | std::views::transform(logRatio)
        | std::ranges::to<std::vector>()
      };

      const auto [mean, standardError]{estimate_trimmed_mean(std::move(logRatios))};

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

      auto fastOf{[](relative_performance_durations durations) { return durations.fast; }};
      auto slowOf{[](relative_performance_durations durations) { return durations.slow; }};

      return {
        .failure{failure},
        .estimate{
          .speedup{std::exp(mean)},
          .interval{.lower{std::exp(intervalLower)}, .upper{std::exp(intervalUpper)}}
        },
        .durations{
          .fast{trimmed_geometric_mean(trialDurations | std::views::transform(fastOf))},
          .slow{trimmed_geometric_mean(trialDurations | std::views::transform(slowOf))}
        }
      };
    }

    [[nodiscard]]
    std::string attempt_summary(const relative_performance_outcome& outcome,
                                std::size_t trials,
                                std::size_t attempt,
                                relative_performance_interval prediction)
    {
      return append_lines(outcome.failure ? verdict_summary(*outcome.failure) : "",
                          speedup_summary(outcome.estimate, prediction),
                          trials_summary(trials, {.current{attempt}, .maximum{relative_performance_max_attempts}}),
                          task_durations_summary(outcome.durations));
    }

    [[nodiscard]]
    double overlap_multiplier(test_mode mode, std::size_t attempt)
    {
      // A check in standard mode fails only if its last attempt fails, and
      // the last attempt takes the full multiplier. False-negative mode stops
      // at the first failing attempt. So in that mode every attempt takes the
      // full multiplier, and each applies the rule which decides a failure.
      return (mode == test_mode::false_negative)
        ? confidence_multiplier
        : confidence_multiplier * static_cast<double>(attempt) / relative_performance_max_attempts;
    }

    [[nodiscard]]
    std::vector<task_order> shuffled_task_orders(std::size_t trials, std::mt19937& generator)
    {
      auto orderOfTrial{
        [numFastThenSlow{trials / 2}](std::size_t trial) {
          return trial < numFastThenSlow ? task_order::fast_then_slow : task_order::slow_then_fast;
        }
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

  [[nodiscard]]
  std::string_view postprocess(std::string_view testOutput, std::string_view referenceOutput)
  {
    return text_with_zeroed_measurements(testOutput) == text_with_zeroed_measurements(referenceOutput)
      ? referenceOutput
      : testOutput;
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
    auto summary{base_type::summarize(delta)};

    if constexpr(Mode != test_mode::standard)
    {
      const auto referenceOutput{
        [filename{this->diagnostics_file_paths().false_positive_or_negative_file_path()}]() -> std::string {
          if(std::filesystem::exists(filename))
          {
            if(auto contents{read_to_string(filename, std::ios_base::in | std::ios_base::binary)})
              return *std::move(contents);

            throw std::runtime_error{report_failed_read(filename)};
          }

          return "";
        }()
      };

      std::string outputToUse{postprocess(summary.diagnostics_output(), referenceOutput)};
      summary.diagnostics_output(std::move(outputToUse));
    }

    return summary;
  }

  template class basic_performance_test<test_mode::standard>;
  template class basic_performance_test<test_mode::false_negative>;
  template class basic_performance_test<test_mode::false_positive>;
}
