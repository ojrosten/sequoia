////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for Summary.hpp
*/

#include "sequoia/TestFramework/Summary.hpp"

#include <algorithm>
#include <array>

namespace sequoia::testing
{
  namespace
  {
    template<class Period>
    [[nodiscard]]
    std::string to_string(const log_summary::duration& d)
    {
      using namespace std::chrono;
      std::stringstream ss{};
      ss << std::setprecision(3) << duration_cast<duration<double, Period>>(d).count();
      return ss.str();
    }
  }

  [[nodiscard]]
  stringified_duration stringify(const log_summary::duration& d)
  {
    using namespace std::chrono;
    const auto count{duration_cast<nanoseconds>(d).count()};
    if(count >= 1'000'000'000) return {to_string<std::ratio<1>>(d), "s"};
    if(count >= 1'000'000)     return {to_string<std::milli>(d),   "ms"};
    if(count >= 1'000)         return {to_string<std::micro>(d),   "us"};

    return {std::to_string(count), "ns"};
  }

  [[nodiscard]]
  std::string report_time(const log_summary& log, const opt_duration duration)
  {
    std::string mess{};
    if(duration)
    {
      const auto [dur, unit]{stringify(*duration)};
      mess.append("[Total Run Time: ").append(dur).append(unit).append("]\n");
    }

    const auto[dur, unit]{stringify(log.execution_time())};
    mess.append("[Execution Time: ").append(dur).append(unit).append("]\n");

    return mess;
  }

  [[nodiscard]]
  std::string summarize(const log_summary& log, std::string_view namesuffix, const opt_duration duration, const summary_detail verbosity, indentation ind_0, indentation ind_1)
  {
    constexpr std::size_t entries{6};

    auto indent{
      [ind_0, ind_1](){
        return std::string{ind_0}.append(ind_1);
      }
    };

    std::array<std::string, entries> summaries{
      indent().append("Standard Top Level Checks:"),
      indent().append("Standard Performance Checks:"),
      indent().append("False Positive Checks:"),
      indent().append("False Positive Performance Checks:"),
      indent().append("False Negative Checks:"),
      indent().append("False Negative Performance Checks:")
    };

    pad_right(summaries.begin(), summaries.end(), "  ");

    std::array<std::string, entries> checkNums{
      std::to_string(log.standard_top_level_checks()),
      std::to_string(log.standard_performance_checks()),
      std::to_string(log.false_positive_checks()),
      std::to_string(log.false_positive_performance_checks()),
      std::to_string(log.false_negative_checks()),
      std::to_string(log.false_negative_performance_checks())
    };

    constexpr std::size_t minChars{8};
    pad_left(checkNums.begin(), checkNums.end(), minChars);

    const auto len{10u - std::ranges::min(std::size_t{minChars}, checkNums.front().size())};

    for(std::size_t i{}; i<entries; ++i)
    {
      summaries[i].append(checkNums[i] + ";").append(len, ' ').append("Failures: ");
    }

    std::array<std::string, entries> failures{
      std::to_string(log.standard_top_level_failures()),
      std::to_string(log.standard_performance_failures()),
      std::to_string(log.false_positive_failures()),
      std::to_string(log.false_positive_performance_failures()),
      std::to_string(log.false_negative_failures()),
      std::to_string(log.false_negative_performance_failures())
    };

    pad_left(failures.begin(), failures.end(), 2);

    for(std::size_t i{}; i < entries; ++i)
    {
      summaries[i] += failures[i];
    }

    if(log.standard_top_level_checks())
      summaries.front().append("  [Deep checks: " + std::to_string(log.standard_deep_checks()) + "]");

    const std::string name{log.name().empty() ? "" : std::string{log.name()} += namesuffix};
    std::string summary{sequoia::indent(name, ind_0)};

    if((verbosity & summary_detail::timings) == summary_detail::timings)
    {
      append_indented(summary, report_time(log, duration), ind_0);
    }
    else
    {
      summary.append("\n");
    }

    if((verbosity & summary_detail::absent_checks) == summary_detail::absent_checks)
    {
      std::ranges::for_each(summaries, [&summary](const std::string& s){ (summary += s) += "\n"; } );
    }
    else
    {
      const std::array<std::size_t, entries> checks{
        log.standard_top_level_checks(),
        log.standard_performance_checks(),
        log.false_positive_checks(),
        log.false_positive_performance_checks(),
        log.false_negative_checks(),
        log.false_negative_performance_checks()
      };

      for(std::size_t i{}; i<entries; ++i)
      {
        if(checks[i]) summary += summaries[i] += "\n";
      }
    }

    if(log.critical_failures())
    {
      summary.append("\n******  Critical Failures:  ")
        .append(std::to_string(log.critical_failures()))
        .append("  ******\n\n");
    }

    if((verbosity & summary_detail::failure_messages) == summary_detail::failure_messages)
    {
      append_indented(summary, log.failure_messages(), ind_0 + ind_1);
    }

    return summary;
  }

  [[nodiscard]]
  std::string summarize(const log_summary& log, std::string_view namesuffix, const summary_detail verbosity, indentation ind_0, indentation ind_1)
  {
    return summarize(log, namesuffix, std::nullopt, verbosity, ind_0, ind_1);
  }
}
