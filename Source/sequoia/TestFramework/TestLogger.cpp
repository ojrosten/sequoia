////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/TestFramework/TestLogger.hpp"
#include "sequoia/TestFramework/SeparatorAfterCheck.hpp"

#include <format>
#include <fstream>
#include <stdexcept>

namespace sequoia::testing
{
  namespace
  {
    [[nodiscard]]
    std::string to_reduced_string(const failure_output& output)
    {
      std::string str{};
      for(const auto& info : output)
      {
        if(!info.message.empty())
          str.append(info.message).append("\n");
      }

      return str;
    }

    void record_check_started(const std::filesystem::path& file, std::string_view message)
    {
      if(!file.empty())
      {
        if(std::ofstream of{file})
          of << "Check started:\n" << message << "\n";
      }
    }

    void record_check_ended(const std::filesystem::path& file)
    {
      if(!file.empty())
      {
        if(std::ofstream of{file, std::ios_base::app})
          of << "Check ended\n";
      }
    }

    void recored_dump_started(const std::filesystem::path& file, std::string_view message)
    {
      if(!file.empty() && !message.empty())
      {
        if(std::ofstream of{file, std::ios_base::app})
          of << message << "\n";
      }
    }

    void recored_dump_ended(const std::filesystem::path& file)
    {
      if(!file.empty())
      {
        if(std::ofstream of{file, std::ios_base::app})
          of << separator_after_check;
      }
    }

    void recored_critical_failure(const std::filesystem::path& file, std::string_view message)
    {
      if(!file.empty())
      {
        if(std::ofstream of{file, std::ios_base::app})
          of << "\nCritical Failure:\n" << message << "\n";
      }
    }
  }

  //================================== sentinel_base ==================================//

  sentinel_base::sentinel_base(test_logger_base& logger, test_mode mode, std::string message)
    : m_pLogger{&logger}
    , m_Mode{mode}
    , m_Level{logger.depth()}
    , m_PriorFailures{logger.results().failures}
    , m_PriorCriticalFailures{logger.results().critical_failures}
    , m_PriorDeepChecks{logger.results().deep_checks}
  {
    if(logger.within_performance_check())
      throw std::logic_error{"Checks cannot be nested within a performance check"};

    if(!logger.depth())
    {
      logger.log_top_level_check();
      record_check_started(get().recovery().recovery_file, message);
    }

    recored_dump_started(get().recovery().dump_file, message);
    logger.increment_depth(message);
  }

  sentinel_base::~sentinel_base()
  {
    auto& logger{get()};

    if(logger.depth() == 1)
    {
      if(critical_failure_detected())
      {
        logger.end_message(m_Mode, test_logger_base::is_critical::yes);
      }
      else
      {
        if(failure_detected()) logger.end_message(m_Mode, test_logger_base::is_critical::no);

        auto falseNegativeFailureMessage{
          [&logger](){
            return end_block(
                     append_lines("False Negative Failure:", logger.top_level_message()),
                     2_linebreaks,
                     footer()
                   );
          }
        };

        const bool modeSpecificFailure{
             ((m_Mode == test_mode::false_negative) && !failure_detected())
          || ((m_Mode != test_mode::false_negative) && failure_detected())
        };

        if(modeSpecificFailure)
        {
          logger.log_top_level_failure(
            m_Mode,
            (m_Mode == test_mode::false_negative) ? falseNegativeFailureMessage() : ""
          );
        }
        else if (m_Mode == test_mode::false_positive)
        {
          if(!critical_failure_detected())
            logger.append_to_diagnostics_output(falseNegativeFailureMessage());
        }

        record_check_ended(get().recovery().recovery_file);
      }

      recored_dump_ended(get().recovery().dump_file);
    }

    logger.decrement_depth();
  }

  //================================== test_logger ==================================//

  void test_logger_base::failure_message(test_mode mode, std::string_view message, const is_critical isCritical)
  {
    std::string msg{};
    auto build{
      [&msg](auto&& text, const indentation& ind){
        if(msg.empty())
        {
          msg = indent(std::forward<decltype(text)>(text), ind);
        }
        else
        {
          append_indented(msg, std::forward<decltype(text)>(text), ind);
        }
      }
    };

    indentation ind{no_indent};
    std::size_t activeLevels{};
    for(auto& info : m_SentinelLevels)
    {
      if(info.message.empty()) continue;

      if(activeLevels++ > 0) ind.append("  ");

      if(info.written) continue;

      build(info.message, ind);
      info.written = true;
    }

    build(message, ind);

    auto& output{add_to_output(output_channel(mode, isCritical), msg)};
    end_block(output.back().message, 1_linebreaks, "");
  }

  void test_logger_base::log_critical_failure(test_mode mode, std::string_view message)
  {
    ++m_Results.critical_failures;
    failure_message(mode, message, is_critical::yes);
    recored_critical_failure(m_Recovery.recovery_file, message);
  }

  void test_logger_base::log_performance_check()
  {
    if(m_SentinelLevels.empty())
      throw std::logic_error{"Cannot log a performance check outside a sentinel"};

    m_SentinelLevels.back().is_performance_check = true;
    if(top_level_is_performance_check())
      ++m_Results.top_level_performance_checks;

    m_Results.attempts_per_performance_check.emplace_back();
    log_check();
  }

  void test_logger_base::log_performance_check_attempts(std::size_t attempts)
  {
    if(m_Results.attempts_per_performance_check.empty())
      throw std::logic_error{"Cannot log the attempts of a performance check before the check"};

    m_Results.attempts_per_performance_check.back() = attempts;
  }

  void test_logger_base::log_top_level_failure(test_mode mode, std::string message)
  {
    if(m_SentinelLevels.empty())
      throw std::logic_error{"Cannot log a top-level failure outside a sentinel"};

    ++m_Results.top_level_failures;
    m_SentinelLevels.back().message.append(message);

    if(mode == test_mode::false_negative)
    {
      m_Results.failure_messages.push_back(failure_info{m_Results.top_level_checks, std::move(message)});
    }
  }

  void test_logger_base::log_caught_exception_message(std::string_view message)
  {
    add_to_output(
      m_Results.caught_exception_messages,
      end_block(std::format("{}\n{}", top_level_message(), message), 2_linebreaks, footer())
    );
  }

  void test_logger_base::append_to_diagnostics_output(std::string message)
  {
    m_Results.diagnostics_output.push_back(failure_info{m_Results.top_level_checks, std::move(message)});
  }

  void test_logger_base::increment_depth(std::string_view message)
  {
    m_SentinelLevels.emplace_back(message);
  }

  void test_logger_base::append_to_level_message(std::size_t level, std::string_view lines)
  {
    if(level >= m_SentinelLevels.size())
      throw std::logic_error{"Cannot append to a message absent from TestLogger's stack"};

    append_lines(m_SentinelLevels[level].message, lines);
  }

  void test_logger_base::decrement_depth()
  {
    if(m_SentinelLevels.empty())
      throw std::logic_error{"Cannot pop from TestLogger's empty stack"};

    if(depth() == 1)
    {
      m_Results.last_check_exit_info = top_level_check_exit_info{
                                         .via_exception{std::uncaught_exceptions() > 0},
                                         .message{std::move(m_SentinelLevels.front().message)}
                                       };
    }

    m_SentinelLevels.pop_back();
  }

  void test_logger_base::end_message(test_mode mode, const is_critical isCritical)
  {
    auto& output{output_channel(mode, isCritical)};
    auto& mess{output.back().message};
    end_block(mess, 2_linebreaks, footer());
  } 

  failure_output& test_logger_base::output_channel(test_mode mode, const is_critical isCritical) noexcept
  {
    const bool toMessages{(mode != test_mode::false_negative) || (isCritical == is_critical::yes)};
    return toMessages ? m_Results.failure_messages : m_Results.diagnostics_output;
  }

  failure_output& test_logger_base::add_to_output(failure_output& output, std::string_view message)
  {
    output.push_back(failure_info{m_Results.top_level_checks, std::string{message}});
    return output;
  }

  template class test_logger<test_mode::standard>;
  template class test_logger<test_mode::false_negative>;
  template class test_logger<test_mode::false_positive>;
  
  //================================== log_summary ==================================//

  log_summary::log_summary(std::string_view name) : m_Name{name} {}

  log_summary::log_summary(std::string_view name, const test_logger_base& logger, test_mode mode, const duration delta)
    : m_Name{name}
    , m_FailureMessages{to_reduced_string(logger.results().failure_messages)}
    , m_DiagnosticsOutput{to_reduced_string(logger.results().diagnostics_output)}
    , m_CaughtExceptionMessages{to_reduced_string(logger.results().caught_exception_messages)}
    , m_Counts{counts_of(logger.results(), mode)}
    , m_CriticalFailures{logger.results().critical_failures}
    , m_Durations{.execution_duration{delta}}
  {}

  [[nodiscard]]
  log_summary::top_level_counts log_summary::top_level_counts_of(const test_results& results,
                                                                 const std::size_t performanceFailures) noexcept
  {
    return {
      .checks              {results.top_level_checks   - results.top_level_performance_checks},
      .performance_checks  {results.top_level_performance_checks},
      .failures            {results.top_level_failures - performanceFailures},
      .performance_failures{performanceFailures}
    };
  }

  [[nodiscard]]
  log_summary::counts_by_mode log_summary::counts_of(const test_results& results, const test_mode mode)
  {
    switch(mode)
    {
    case test_mode::standard:
      return {
        .standard{
          .top_level  {top_level_counts_of(results, results.top_level_performance_failures)},
          .deep_checks{results.deep_checks - results.top_level_performance_checks}
        }
      };
    case test_mode::false_positive:
      return {.false_positive{top_level_counts_of(results, results.top_level_performance_failures)}};
    case test_mode::false_negative:
    {
      const auto falseNegativePerformanceFailures{
        results.top_level_performance_checks - results.top_level_performance_failures
      };

      return {.false_negative{top_level_counts_of(results, falseNegativePerformanceFailures)}};
    }
    }

    throw std::logic_error{"Unrecognized case for test_mode"};
  }

  [[nodiscard]]
  std::size_t log_summary::soft_failures() const noexcept
  {
    return standard_top_level_failures()
         + false_negative_failures()
         + false_positive_failures()
         + standard_performance_failures()
         + false_negative_performance_failures()
         + false_positive_performance_failures();
  }

  log_summary& log_summary::operator+=(const log_summary& rhs)
  {
    m_FailureMessages.append(rhs.m_FailureMessages);
    m_DiagnosticsOutput.append(rhs.m_DiagnosticsOutput);
    m_CaughtExceptionMessages.append(rhs.m_CaughtExceptionMessages);

    m_Counts           += rhs.m_Counts;
    m_CriticalFailures += rhs.m_CriticalFailures;
    m_Durations        += rhs.m_Durations;

    return *this;
  }
}
