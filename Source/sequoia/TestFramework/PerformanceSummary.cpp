////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/TestFramework/PerformanceSummary.hpp"

#include <array>
#include <charconv>
#include <format>
#include <optional>
#include <ranges>
#include <stdexcept>

namespace sequoia::testing
{
  namespace
  {
    /** \brief Extracts the numbers in `text`, if there are exactly `N`.

        Anything identifiable as a number of type `T` is extracted. That is
        whatever `std::from_chars` reads. Reading tries each character from
        left to right, and resumes after each number it reads. So for a
        floating-point `T`, integers count, and so do `inf` and `nan`, even
        inside a word. A `-` directly before a number is its sign, if `T` has one.
     */
    template<class T, std::size_t N>
    [[nodiscard]]
    std::optional<std::array<T, N>> extract_numbers_from(std::string_view text)
    {
      std::array<T, N> numbers{};
      std::size_t count{};
      const auto last{text.data() + text.size()};
      auto first{text.data()};
      while(first != last)
      {
        T value{};
        if(const auto [next, error]{std::from_chars(first, last, value)}; error == std::errc{})
        {
          if(count == N)
            return std::nullopt;

          numbers[count++] = value;
          first = next;
        }
        else
        {
          ++first;
        }
      }

      return count == N ? std::optional{numbers} : std::nullopt;
    }

    /** \brief Returns `line` with its measured values set to zero.

        `text_with_zeroed_measurements` states which lines hold measured
        values, and which values they are.
     */
    [[nodiscard]]
    std::string line_with_zeroed_measurements(std::string_view line)
    {
      if(const auto numbers{extract_numbers_from<double, 5>(line)})
      {
        const auto [speedup, intervalMin, intervalMax, minSpeedup, maxSpeedup]{*numbers};
        if(speedup_summary(speedup, intervalMin, intervalMax, minSpeedup, maxSpeedup) == line)
          return speedup_summary(0, 0, 0, minSpeedup, maxSpeedup);
      }

      if(const auto numbers{extract_numbers_from<std::size_t, 3>(line)})
      {
        const auto [trials, attempt, maxAttempts]{*numbers};
        if(trials_summary(trials, attempt, maxAttempts) == line)
          return trials_summary(0, 0, maxAttempts);
      }

      if(const auto numbers{extract_numbers_from<double, 2>(line)})
      {
        const auto [fastDuration, slowDuration]{*numbers};
        if(task_durations_summary(fastDuration, slowDuration) == line)
          return task_durations_summary(0, 0);
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

    throw std::logic_error{"Unknown relative_performance_failure"};
  }

  [[nodiscard]]
  std::string speedup_summary(double speedup,
                              double intervalMin,
                              double intervalMax,
                              double minSpeedup,
                              double maxSpeedup)
  {
    return std::format("Speed-up: {:.3g} in [{:.3g}, {:.3g}]; predicted ({:g}, {:g})",
                       speedup,
                       intervalMin,
                       intervalMax,
                       minSpeedup,
                       maxSpeedup);
  }

  [[nodiscard]]
  std::string trials_summary(std::size_t trials, std::size_t attempt, std::size_t maxAttempts)
  {
    return std::format("Trials: {}, attempt {} of {}", trials, attempt, maxAttempts);
  }

  [[nodiscard]]
  std::string task_durations_summary(double fastDuration, double slowDuration)
  {
    return std::format("Task durations: fast {:.3g}s, slow {:.3g}s", fastDuration, slowDuration);
  }

  [[nodiscard]]
  std::string text_with_zeroed_measurements(std::string_view text)
  {
    auto lineWithZeroedMeasurements{
      [](auto line) { return line_with_zeroed_measurements(std::string_view{line}); }
    };

    return
        text
      | std::views::split('\n')
      | std::views::transform(lineWithZeroedMeasurements)
      | std::views::join_with('\n')
      | std::ranges::to<std::string>();
  }
}
