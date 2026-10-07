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

#include <format>
#include <regex>

namespace sequoia::testing
{
  namespace
  {
    /** \brief `text` with each measured value replaced by `#`, in every line
               of the shape `duration_summary` writes, and in any suffix of
               the shape `speed_up_summary` appends to it.
     */
    [[nodiscard]]
    std::string without_measurements(std::string_view text)
    {
      constexpr std::string_view number{R"((?:-?(?:\d+(?:\.\d+)?(?:e[-+]\d+)?|inf|nan)))"};
      const auto duration{std::format("{}s", number)};

      // A value replaced by `#`, rather than removed, cannot compare equal to a value which is missing
      const std::regex durations{std::format(R"((Task duration: ){1}( \+- {0} \* ){1})", number, duration)};
      const std::regex speedUp{std::format(R"((Task duration: # \+- {0} \* #)( \[){0}(; \({0}, {0}\)\]))", number)};

      return std::regex_replace(std::regex_replace(std::string{text}, durations, "$1#$2#"), speedUp, "$1$2#$3");
    }
  }

  [[nodiscard]]
  std::string duration_summary(std::string_view prefix, double mean, double num_sds, double sig)
  {
    return std::format("{} Task duration: {:g}s +- {:g} * {:g}s", prefix, mean, num_sds, sig);
  }

  [[nodiscard]]
  std::string speed_up_summary(double speedUp, double minSpeedUp, double maxSpeedUp)
  {
    return std::format(" [{:g}; ({:g}, {:g})]", speedUp, minSpeedUp, maxSpeedUp);
  }

  [[nodiscard]]
  std::string_view postprocess(std::string_view testOutput, std::string_view referenceOutput)
  {
    return without_measurements(testOutput) == without_measurements(referenceOutput) ? referenceOutput : testOutput;
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
