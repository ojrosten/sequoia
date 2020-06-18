////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions used for summarizing results of tests.
*/

#include "Summary.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string stringify(const log_summary::duration& d)
  {
    using namespace std::chrono;
    const auto count{duration_cast<milliseconds>(d).count()};
    return std::to_string(count);
  }

  [[nodiscard]]
  std::string report_time(const log_summary& log, const opt_duration duration)
  {
    auto mess{std::string{"["}};
    if(duration) mess.append(stringify(*duration)).append(" (");

    mess.append(stringify(log.execution_time()));

    if(duration) mess.append(")");
      
    return mess.append("ms]\n");
  }

  [[nodiscard]]
  std::string summarize(const log_summary& log, const opt_duration duration, std::string_view indent, const log_verbosity suppression)
  {
    constexpr std::size_t entries{6};

    std::array<std::string, entries> summaries{
      std::string{indent}.append("\tStandard Top Level Checks:"),
      std::string{indent}.append("\tStandard Performance Checks:"),
      std::string{indent}.append("\tFalse Negative Checks:"),
      std::string{indent}.append("\tFalse Negative Performance Checks:"),
      std::string{indent}.append("\tFalse Positive Checks:"),
      std::string{indent}.append("\tFalse Positive Performance Checks:")
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

    auto dur{
      [&log, &duration](){
        return report_time(log, duration);
      }
    };
    
    std::string summary{log.name().empty() ? dur()
        : std::string{"\t"}.append(log.name()).append(":\n\t").append(dur())};

    if((suppression & log_verbosity::absent_checks) == log_verbosity::absent_checks)
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
      (summary += "Critical Failures:  ") += std::to_string(log.critical_failures()) += "\n";
    }

    if((suppression & log_verbosity::failure_messages) == log_verbosity::failure_messages)
    {
      auto mess{log.failure_messages()};
      if(!mess.empty())
        summary.append("\n").append(std::move(mess));
    }

    return summary;
  }

  [[nodiscard]]
  std::string summarize(const log_summary& log, std::string_view indent, const log_verbosity suppression)
  {
    return summarize(log, std::nullopt, indent, suppression);
  }
}
