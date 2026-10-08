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

#include <array>
#include <charconv>
#include <format>
#include <optional>
#include <ranges>

namespace sequoia::testing
{
  namespace
  {
    /** \brief The numbers in `text`, if there are exactly `N`.

        Anything identifiable as a number is extracted as a `double`: whatever
        `std::from_chars` reads, trying each character from left to right and
        resuming after each number it reads. So an integer counts, as do `inf`
        and `nan`, even inside a word, and a `-` directly before a number is
        its sign.
     */
    template<std::size_t N>
    [[nodiscard]]
    std::optional<std::array<double, N>> extract_numbers_from(std::string_view text)
    {
      std::array<double, N> numbers{};
      std::size_t count{};
      const auto last{text.data() + text.size()};
      auto first{text.data()};
      while(first != last)
      {
        double value{};
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

    /** \brief `line` with its measured values set to zero, if `duration_summary`,
               with or without the suffix of `speedup_summary`, prints it
               exactly; otherwise `line` as it stands.
     */
    [[nodiscard]]
    std::string line_with_zeroed_measurements(std::string_view line)
    {
      constexpr std::string_view label{" Task duration: "};
      const auto labelPos{line.find(label)};
      if(labelPos == std::string_view::npos)
        return std::string{line};

      std::string_view prefix{line.substr(0, labelPos)};
      std::string_view afterLabel{line.substr(labelPos + label.size())};

      if(const auto numbers{extract_numbers_from<3>(afterLabel)})
      {
        const auto [mean, numSds, sd]{*numbers};
        if(duration_summary(prefix, mean, numSds, sd) == line)
          return duration_summary(prefix, 0, numSds, 0);
      }

      if(const auto numbers{extract_numbers_from<6>(afterLabel)})
      {
        const auto [mean, numSds, sd, speedup, minSpeedup, maxSpeedup]{*numbers};
        if(duration_summary(prefix, mean, numSds, sd) + speedup_summary(speedup, minSpeedup, maxSpeedup) == line)
          return duration_summary(prefix, 0, numSds, 0) + speedup_summary(0, minSpeedup, maxSpeedup);
      }

      return std::string{line};
    }

    /** \brief The lines of `text`, split at each newline, each as
               `line_with_zeroed_measurements` returns it.

        The view is lazy and refers to `text`, so it must not outlive `text`.
     */
    [[nodiscard]]
    auto lines_with_zeroed_measurements(std::string_view text)
    {
      auto lineWithZeroedMeasurements{
        [](auto line) { return line_with_zeroed_measurements(std::string_view{line}); }
      };

      return text | std::views::split('\n') | std::views::transform(lineWithZeroedMeasurements);
    }
  }

  [[nodiscard]]
  std::string duration_summary(std::string_view prefix, double mean, double numSds, double sd)
  {
    return std::format("{} Task duration: {:g}s +- {:g} * {:g}s", prefix, mean, numSds, sd);
  }

  [[nodiscard]]
  std::string speedup_summary(double speedup, double minSpeedup, double maxSpeedup)
  {
    return std::format(" [{:g}; ({:g}, {:g})]", speedup, minSpeedup, maxSpeedup);
  }

  [[nodiscard]]
  std::string_view postprocess(std::string_view testOutput, std::string_view referenceOutput)
  {
    return std::ranges::equal(lines_with_zeroed_measurements(testOutput),
                              lines_with_zeroed_measurements(referenceOutput))
      ? referenceOutput : testOutput;
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
