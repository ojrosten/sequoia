////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/TestFramework/PerformanceSummary.hpp"
#include "sequoia/TextProcessing/Numbers.hpp"

#include <algorithm>
#include <format>
#include <ranges>
#include <stdexcept>

namespace sequoia::testing
{
  namespace
  {
    /** \brief Returns `line` with its measurements set to zero.

        `text_with_zeroed_measurements` states which lines hold measurements,
        and which values they are.
     */
    [[nodiscard]]
    std::string line_with_zeroed_measurements(std::string_view line, deciding_attempt decidingAttempt)
    {
      const auto indentationSize{std::min(line.find_first_not_of(" \t"), line.size())};
      const std::string_view indentation{line.substr(0, indentationSize)}, unindented{line.substr(indentationSize)};

      auto indented{[indentation](std::string_view zeroed) { return std::format("{}{}", indentation, zeroed); }};

      if(const auto numbers{extract_numbers_from<double, 5>(unindented)})
      {
        const auto [speedup, obtainedLower, obtainedUpper, predictionLower, predictionUpper]{*numbers};
        const relative_performance_estimate obtained{
          .speedup{speedup},
          .interval{.lower{obtainedLower}, .upper{obtainedUpper}}
        };

        const relative_performance_interval prediction{.lower{predictionLower}, .upper{predictionUpper}};

        if(speedup_summary(obtained, prediction) == unindented)
          return indented(speedup_summary({}, prediction));
      }

      if(const auto numbers{extract_numbers_from<std::size_t, 3>(unindented)};
         numbers && (decidingAttempt == deciding_attempt::varies))
      {
        const auto [trials, currentAttempt, maximumAttempts]{*numbers};
        const relative_performance_attempts attempts{.current{currentAttempt}, .maximum{maximumAttempts}};

        if(trials_summary(trials, attempts) == unindented)
          return indented(trials_summary(0, {.maximum{attempts.maximum}}));
      }

      if(const auto numbers{extract_numbers_from<double, 2>(unindented)})
      {
        const auto [fastSeconds, slowSeconds]{*numbers};
        const relative_performance_durations durations{
          .fast{std::chrono::duration<double>{fastSeconds}},
          .slow{std::chrono::duration<double>{slowSeconds}}
        };

        if(task_durations_summary(durations) == unindented)
          return indented(task_durations_summary({}));
      }

      return std::string{line};
    }
  }

  [[nodiscard]]
  std::string_view verdict_summary(relative_performance_failure failure)
  {
    switch(failure)
    {
    case relative_performance_failure::not_distinguishably_faster:
      return "The fast task is not distinguishably faster than the slow one";
    case relative_performance_failure::slower:
      return "The fast task is slower than the slow one";
    case relative_performance_failure::faster_but_less_than_predicted:
      return "The fast task is faster than the slow one, but by less than predicted";
    case relative_performance_failure::suspiciously_fast:
      return "The fast task is suspiciously fast: faster than predicted";
    }

    throw std::logic_error{"Unrecognized case for relative_performance_failure"};
  }

  [[nodiscard]]
  std::string speedup_summary(const relative_performance_estimate& obtained, relative_performance_interval prediction)
  {
    return std::format("Speed-up: {:.3g} in [{:.3g}, {:.3g}]; predicted [{:g}, {:g}]",
                       obtained.speedup,
                       obtained.interval.lower,
                       obtained.interval.upper,
                       prediction.lower,
                       prediction.upper);
  }

  [[nodiscard]]
  std::string trials_summary(std::size_t trials, relative_performance_attempts attempts)
  {
    return std::format("Trials: {}, attempt {} of {}", trials, attempts.current, attempts.maximum);
  }

  [[nodiscard]]
  std::string task_durations_summary(relative_performance_durations durations)
  {
    return std::format("Task durations: fast {:.3g}s, slow {:.3g}s", durations.fast.count(), durations.slow.count());
  }

  [[nodiscard]]
  std::string text_with_zeroed_measurements(std::string_view text, deciding_attempt decidingAttempt)
  {
    auto lineWithZeroedMeasurements{
      [decidingAttempt](auto line) { return line_with_zeroed_measurements(std::string_view{line}, decidingAttempt); }
    };

    return
        text
      | std::views::split('\n')
      | std::views::transform(lineWithZeroedMeasurements)
      | std::views::join_with('\n')
      | std::ranges::to<std::string>();
  }
}
