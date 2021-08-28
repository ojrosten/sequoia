////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for Summary.hpp.
*/

#include "sequoia/TestFramework/Summary.hpp"

#include <array>

namespace sequoia::testing
{
  [[nodiscard]]
  stringified_duration stringify(const log_summary::duration& d)
  {
    using namespace std::chrono;
    const auto count{duration_cast<milliseconds>(d).count()};
    return {std::to_string(count), "ms"};
  }

  [[nodiscard]]
  std::string report_time(const log_summary& log, const opt_duration duration)
  {
    auto mess{std::string{"["}};
    if(duration) mess.append(stringify(*duration).time).append(" (");

    const auto[dur, unit]{stringify(log.execution_time())};
    mess.append(dur);

    if(duration) mess.append(")");

    return mess.append(unit).append("]\n");
  }

  [[nodiscard]]
  std::string summarize(const log_summary& log, const opt_duration duration, const summary_detail verbosity, indentation ind_0, indentation ind_1)
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
      indent().append("False Negative Checks:"),
      indent().append("False Negative Performance Checks:"),
      indent().append("False Positive Checks:"),
      indent().append("False Positive Performance Checks:")
    };

    pad_right(summaries.begin(), summaries.end(), "  ");

    std::array<std::string, entries> checkNums{
      std::to_string(log.standard_top_level_checks()),
      std::to_string(log.standard_performance_checks()),
      std::to_string(log.false_negative_checks()),
      std::to_string(log.false_negative_performance_checks()),
      std::to_string(log.false_positive_checks()),
      std::to_string(log.false_positive_performance_checks())
    };

    constexpr std::size_t minChars{8};
    pad_left(checkNums.begin(), checkNums.end(), minChars);

    const auto len{10u - std::min(std::size_t{minChars}, checkNums.front().size())};

    for(std::size_t i{}; i<entries; ++i)
    {
      summaries[i].append(checkNums[i] + ";").append(len, ' ').append("Failures: ");
    }

    std::array<std::string, entries> failures{
      std::to_string(log.standard_top_level_failures()),
      std::to_string(log.standard_performance_failures()),
      std::to_string(log.false_negative_failures()),
      std::to_string(log.false_negative_performance_failures()),
      std::to_string(log.false_positive_failures()),
      std::to_string(log.false_positive_performance_failures())
    };

    pad_left(failures.begin(), failures.end(), 2);

    for(std::size_t i{}; i < entries; ++i)
    {
      summaries[i] += failures[i];
    }

    if(log.standard_top_level_checks())
      summaries.front().append("  [Deep checks: " + std::to_string(log.standard_deep_checks()) + "]");

    std::string summary{sequoia::indent(log.name(), ind_0)};

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
      std::for_each(std::cbegin(summaries), std::cend(summaries), [&summary](const std::string& s){
          (summary += s) += "\n";
        }
      );
    }
    else
    {
      const std::array<std::size_t, entries> checks{
        log.standard_top_level_checks(),
        log.standard_performance_checks(),
        log.false_negative_checks(),
        log.false_negative_performance_checks(),
        log.false_positive_checks(),
        log.false_positive_performance_checks()
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
      append_lines(summary, log.failure_messages());
    }

    return summary;
  }

  [[nodiscard]]
  std::string summarize(const log_summary& log, const summary_detail verbosity, indentation ind_0, indentation ind_1)
  {
    return summarize(log, std::nullopt, verbosity, ind_0, ind_1);
  }
}
