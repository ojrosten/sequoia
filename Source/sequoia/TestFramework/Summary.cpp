////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/TestFramework/Summary.hpp"

#include <algorithm>
#include <array>
#include <format>
#include <optional>
#include <ranges>

namespace sequoia::testing
{
  namespace
  {
    template<class Period>
    [[nodiscard]]
    std::string to_string(const log_summary::duration& d)
    {
      using namespace std::chrono;
      return std::format("{:.3}", duration_cast<duration<double, Period>>(d).count());
    }

    [[nodiscard]]
    std::string timing_line(std::string_view label, const log_summary::duration& d)
    {
      const auto [dur, unit]{stringify_duration(d)};
      return std::format("[{}: {}{}]\n", label, dur, unit);
    }

    /** The label of a kind of check, how many checks of that kind ran, how
        many of those failed and, for standard top-level checks of which any
        ran, how many deep checks they made.
     */
    struct counts_of_checks
    {
      std::string_view label{};
      std::size_t
        checks{},
        failures{};
      std::optional<std::size_t> deep_checks{};
    };
  }

  [[nodiscard]]
  stringified_duration stringify_duration(const log_summary::duration& d)
  {
    using namespace std::chrono;
    const auto count{duration_cast<nanoseconds>(d).count()};
    // A count is written in the larger of two units once it rounds to a
    // thousand of the smaller, at three significant figures
    if(count >= 999'500'000) return {to_string<std::ratio<1>>(d), "s"};
    if(count >= 999'500)     return {to_string<std::milli>(d),   "ms"};
    if(count >= 1'000)       return {to_string<std::micro>(d),   "us"};

    return {std::to_string(count), "ns"};
  }

  [[nodiscard]]
  std::string durations_report(const durations_of_tests& durations)
  {
    return std::format("{}{}",
                       timing_line("Execution Time",  durations.execution_duration),
                       timing_line("Runner Overhead", durations.runner_overhead));
  }

  [[nodiscard]]
  std::string durations_report(const durations_of_run& durationsOfRun)
  {
    const auto& [totalRunTime, criticalPath]{durationsOfRun};
    return std::format("{}{}{}",
                       timing_line("Total Run Time",      totalRunTime),
                       durations_report(criticalPath.spent_in_tests),
                       timing_line("Scheduling Overhead", criticalPath.scheduling_overhead));
  }

  [[nodiscard]]
  std::string summarize(const log_summary& log,
                        std::string_view namesuffix,
                        const summary_detail verbosity,
                        indentation ind_0,
                        indentation ind_1)
  {
    auto verbosityIncludes{
      [verbosity](const summary_detail detail) { return (verbosity & detail) == detail; }
    };

    const std::array counts{
      counts_of_checks{
        .label      {"Standard Top Level Checks:"},
        .checks     {log.standard_top_level_checks()},
        .failures   {log.standard_top_level_failures()},
        .deep_checks{log.standard_top_level_checks() ? std::optional{log.standard_deep_checks()} : std::nullopt}
      },
      counts_of_checks{
        .label   {"Standard Performance Checks:"},
        .checks  {log.standard_performance_checks()},
        .failures{log.standard_performance_failures()}
      },
      counts_of_checks{
        .label   {"False Positive Checks:"},
        .checks  {log.false_positive_checks()},
        .failures{log.false_positive_failures()}
      },
      counts_of_checks{
        .label   {"False Positive Performance Checks:"},
        .checks  {log.false_positive_performance_checks()},
        .failures{log.false_positive_performance_failures()}
      },
      counts_of_checks{
        .label   {"False Negative Checks:"},
        .checks  {log.false_negative_checks()},
        .failures{log.false_negative_failures()}
      },
      counts_of_checks{
        .label   {"False Negative Performance Checks:"},
        .checks  {log.false_negative_performance_checks()},
        .failures{log.false_negative_performance_failures()}
      }
    };

    auto labelWidthOf   {[](const counts_of_checks& c) { return c.label.size(); }};
    auto checksWidthOf  {[](const counts_of_checks& c) { return std::formatted_size("{}", c.checks); }};
    auto failuresWidthOf{[](const counts_of_checks& c) { return std::formatted_size("{}", c.failures); }};

    auto widestOf{
      [&counts](auto widthOf) { return std::ranges::max(counts | std::views::transform(widthOf)); }
    };

    constexpr std::size_t minChecksWidth{8}, minFailuresWidth{2};
    const auto
      labelWidth   {widestOf(labelWidthOf)},
      checksWidth  {std::ranges::max(minChecksWidth,   widestOf(checksWidthOf))},
      failuresWidth{std::ranges::max(minFailuresWidth, widestOf(failuresWidthOf))};

    auto isReported{
      [absentChecksReported{verbosityIncludes(summary_detail::absent_checks)}](const counts_of_checks& c) {
        return absentChecksReported || (c.checks > 0);
      }
    };

    const indentation lineIndent{ind_0 + ind_1};

    auto lineOf{
      [&lineIndent, labelWidth, checksWidth, failuresWidth](const counts_of_checks& c) {
        const std::string deepChecksNote{c.deep_checks ? std::format("  [Deep checks: {}]", *c.deep_checks) : ""};
        return std::format("{}{:<{}}  {:>{}};  Failures: {:>{}}{}\n",
                           std::string_view{lineIndent},
                           c.label,
                           labelWidth,
                           c.checks,
                           checksWidth,
                           c.failures,
                           failuresWidth,
                           deepChecksNote);
      }
    };

    const auto checksReport{
        counts
      | std::views::filter(isReported)
      | std::views::transform(lineOf)
      | std::views::join
      | std::ranges::to<std::string>()
    };

    const std::string name{log.name().empty() ? "" : std::string{log.name()} += namesuffix};
    std::string summary{sequoia::indent(name, ind_0)};

    if(verbosityIncludes(summary_detail::timings))
    {
      append_indented(summary, durations_report(log.durations()), ind_0);
    }
    else if(!summary.empty())
    {
      summary.append("\n");
    }

    summary.append(checksReport);

    if(log.critical_failures())
    {
      summary.append(std::format("\n******  Critical Failures:  {}  ******\n\n", log.critical_failures()));
    }

    if(verbosityIncludes(summary_detail::failure_messages))
    {
      append_indented(summary, log.failure_messages(), lineIndent);
    }

    return summary;
  }
}
