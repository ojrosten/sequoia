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
#include <compare>
#include <format>
#include <ranges>

namespace sequoia::testing
{
  namespace
  {
    /** \brief Orders doubles totally, so that sorting is defined even with
               NaNs among them.

        A task too quick for the clock times as zero, and the log-ratio of two
        such timings is NaN.
     */
    constexpr auto total_order{[](double lhs, double rhs) { return std::strong_order(lhs, rhs) < 0; }};

    /** \brief Returns the number trimmed from each end of `n` sorted data: a
               tenth of `n`, rounded up.
     */
    [[nodiscard]]
    std::size_t num_trimmed_from_each_end(std::size_t n) noexcept
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

        The standard error is Tukey and McLaughlin's: the winsorized sample
        standard deviation, divided by the fraction of data kept and by the
        square root of the number of data.
     */
    [[nodiscard]]
    trimmed_mean_estimate estimate_trimmed_mean(std::vector<double> data)
    {
      std::ranges::sort(data, total_order);

      const auto n{data.size()};
      const auto numTrimmed{num_trimmed_from_each_end(n)};
      const auto keptFirst{data.cbegin() + numTrimmed},
                 keptLast {data.cend()   - numTrimmed};

      const auto winsorizedVariance{
        maths::winsorized_sample_variance(data.cbegin(), data.cend(), static_cast<std::ptrdiff_t>(numTrimmed)).first
      };

      const double fractionKept{1.0 - 2.0 * numTrimmed / n};

      return {
        .mean{maths::mean(keptFirst, keptLast).value()},
        .standard_error{std::sqrt(winsorizedVariance.value()) / (fractionKept * std::sqrt(n))}
      };
    }

    /** \brief Returns the exponential of the trimmed mean of the logarithms of
               `durations`.
     */
    [[nodiscard]]
    double trimmed_geometric_mean(std::span<const double> durations)
    {
      auto logOf{[](double duration) { return std::log(duration); }};

      return std::exp(
        estimate_trimmed_mean(durations | std::views::transform(logOf) | std::ranges::to<std::vector>()).mean
      );
    }
  }

  namespace impl
  {
    [[nodiscard]]
    relative_performance_judgement judge_attempt(std::span<const double> fastDurations,
                                                 std::span<const double> slowDurations,
                                                 double confidenceMultiplier,
                                                 const relative_performance_parameters& parameters)
    {
      auto logRatio{[](double fast, double slow) { return std::log(slow / fast); }};

      const auto [mean, standardError]{
        estimate_trimmed_mean(
          std::views::zip_transform(logRatio, fastDurations, slowDurations) | std::ranges::to<std::vector>()
        )
      };

      const double intervalMin{mean - confidenceMultiplier * standardError},
                   intervalMax{mean + confidenceMultiplier * standardError};

      const auto failure{
        [intervalMin, intervalMax, &parameters]() -> std::optional<relative_performance_failure> {
          if(!(intervalMin > 0))
            return intervalMax < 0 ? relative_performance_failure::slower
                                   : relative_performance_failure::not_distinguishably_faster;

          if(intervalMax < std::log(parameters.min_speedup))
            return relative_performance_failure::faster_but_less_than_predicted;

          if(intervalMin > std::log(parameters.max_speedup))
            return relative_performance_failure::suspiciously_fast;

          return std::nullopt;
        }()
      };

      return {
        .failure{failure},
        .speedup{std::exp(mean)},
        .interval_min{std::exp(intervalMin)},
        .interval_max{std::exp(intervalMax)},
        .fast_duration{trimmed_geometric_mean(fastDurations)},
        .slow_duration{trimmed_geometric_mean(slowDurations)}
      };
    }

    [[nodiscard]]
    std::string attempt_summary(const relative_performance_judgement& judgement,
                                std::size_t trials,
                                std::size_t attempt,
                                const relative_performance_parameters& parameters)
    {
      return append_lines(judgement.failure ? verdict_summary(*judgement.failure) : "",
                          speedup_summary(judgement.speedup,
                                          judgement.interval_min,
                                          judgement.interval_max,
                                          parameters.min_speedup,
                                          parameters.max_speedup),
                          trials_summary(trials, attempt, relative_performance_max_attempts),
                          task_durations_summary(judgement.fast_duration, judgement.slow_duration));
    }

    [[nodiscard]]
    double confidence_multiplier(test_mode mode, std::size_t attempt)
    {
      return (mode == test_mode::false_negative)
        ? relative_performance_confidence_multiplier
        : relative_performance_confidence_multiplier * static_cast<double>(attempt) / relative_performance_max_attempts;
    }

    [[nodiscard]]
    std::vector<first_task> shuffled_task_orders(std::size_t trials, std::mt19937& generator)
    {
      auto firstTaskOfTrial{
        [numFastFirst{trials / 2}](std::size_t trial) {
          return trial < numFastFirst ? first_task::fast : first_task::slow;
        }
      };

      auto orders{
          std::views::iota(0uz, trials)
        | std::views::transform(firstTaskOfTrial)
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
              return contents.value();

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
