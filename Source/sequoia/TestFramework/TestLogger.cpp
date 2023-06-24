////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for TestLogger.hpp
 */

#include "sequoia/TestFramework/TestLogger.hpp"

#include <fstream>

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
      if(!file.empty())
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
          of << "\n\n";
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

  sentinel_base::sentinel_base(test_logger_base& logger, std::string_view message)
    : m_pLogger{&logger}
    , m_Message{message}
    , m_PriorFailures{logger.results().failures}
    , m_PriorCriticalFailures{logger.results().critical_failures}
    , m_PriorDeepChecks{logger.results().deep_checks}
  {
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
        logger.end_message(test_logger_base::is_critical::yes);
      }
      else
      {
        const auto mode{get().mode()};

        if(failure_detected()) logger.end_message(test_logger_base::is_critical::no);

        auto fpMessageMaker{
          [&logger](){
            
            auto mess{append_lines("False Positive Failure:", logger.top_level_message())};
            end_block(mess, 2_linebreaks, footer());

            return mess;
          }
        };

        const bool modeSpecificFailure{
             ((mode == test_mode::false_positive) && !failure_detected())
          || ((mode != test_mode::false_positive) && failure_detected())
        };

        if(modeSpecificFailure)
        {
          logger.log_top_level_failure((mode == test_mode::false_positive) ? fpMessageMaker() : "");
        }
        else if (mode == test_mode::false_negative)
        {
          if(!critical_failure_detected())
            logger.append_to_diagnostics_output(fpMessageMaker());
        }

        record_check_ended(get().recovery().recovery_file);
      }

      recored_dump_ended(get().recovery().dump_file);
    }

    logger.decrement_depth();
  }

  //================================== test_logger ==================================//

  void test_logger_base::failure_message(std::string_view message, const is_critical isCritical)
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
    for(auto& info : m_SentinelDepth)
    {
      if(info.message.empty()) continue;

      if(activeLevels++ > 0) ind.append("  ");

      if(info.written) continue;

      build(info.message, ind);
      info.written = true;
    }

    build(message, ind);

    auto& output{add_to_output(output_channel(isCritical), msg)};
    end_block(output.back().message, 1_linebreaks, "");
  }

  void test_logger_base::log_critical_failure(std::string_view message)
  {
    ++m_Results.critical_failures;
    failure_message(message, is_critical::yes);
    recored_critical_failure(m_Recovery.recovery_file, message);
  }

  void test_logger_base::log_top_level_failure(std::string message)
  {
    ++m_Results.top_level_failures;
    if(m_SentinelDepth.empty())
    {
      m_SentinelDepth.push_back(level_message{message});
    }
    else
    {
      m_SentinelDepth.back().message.append(std::move(message));
    }

    if(m_Mode == test_mode::false_positive)
    {
      m_Results.failure_messages.push_back(failure_info{m_Results.top_level_checks, std::string{message}});
    }
  }

  void test_logger_base::log_caught_exception_message(std::string_view message)
  {
    auto mess{std::string{top_level_message()}.append("\n").append(message)};
    end_block(mess, 2_linebreaks, footer());

    add_to_output(m_Results.caught_exception_messages, mess);
  }

  void test_logger_base::append_to_diagnostics_output(std::string message)
  {
    m_Results.diagnostics_output.push_back(failure_info{m_Results.top_level_checks, std::move(message)});
  }

  void test_logger_base::increment_depth(std::string_view message)
  {
    m_SentinelDepth.emplace_back(message);
  }

  void test_logger_base::decrement_depth()
  {
    if(m_SentinelDepth.empty())
      throw std::logic_error{"Cannot pop from TestLogger's empty stack"};

    if(depth() == 1)
    {
      m_Results.exception_info = {std::uncaught_exceptions(), std::move(m_SentinelDepth.front().message)}; 
    }
      
    m_SentinelDepth.pop_back();
  }

  void test_logger_base::end_message(const is_critical isCritical)
  {
    auto& output{output_channel(isCritical)};
    auto& mess{output.back().message};
    end_block(mess, 2_linebreaks, footer());
  }

  failure_output& test_logger_base::output_channel(const is_critical isCritical) noexcept
  {
    const bool toMessages{(m_Mode != test_mode::false_positive) || (isCritical == is_critical::yes)};
    return toMessages ? m_Results.failure_messages : m_Results.diagnostics_output;
  }

  failure_output& test_logger_base::add_to_output(failure_output& output, std::string_view message)
  {
    output.push_back(failure_info{m_Results.top_level_checks, std::string{message}});
    return output;
  }

  template class test_logger<test_mode::standard>;
  template class test_logger<test_mode::false_positive>;
  template class test_logger<test_mode::false_negative>;
  
  //================================== log_summary ==================================//

  log_summary::log_summary(std::string_view name) : m_Name{name} {}

  log_summary::log_summary(std::string_view name, const test_logger_base& logger, const duration delta)
    : m_Name{name}
    , m_FailureMessages{to_reduced_string(logger.results().failure_messages)}
    , m_DiagnosticsOutput{to_reduced_string(logger.results().diagnostics_output)}
    , m_CaughtExceptionMessages{to_reduced_string(logger.results().caught_exception_messages)}
    , m_CriticalFailures{logger.results().critical_failures}
    , m_Duration{delta}
  {
    switch(logger.mode())
    {
    case test_mode::standard:
      m_StandardTopLevelFailures    = logger.results().top_level_failures - logger.results().performance_failures;
      m_StandardDeepFailures        = logger.results().failures - logger.results().performance_failures;
      m_StandardPerformanceFailures = logger.results().performance_failures;
      m_StandardTopLevelChecks      = logger.results().top_level_checks - logger.results().performance_checks;
      m_StandardDeepChecks          = logger.results().deep_checks - logger.results().performance_checks;
      m_StandardPerformanceChecks   = logger.results().performance_checks;
      break;
    case test_mode::false_negative:
      m_FalseNegativeFailures            = logger.results().top_level_failures - logger.results().performance_failures;
      m_FalseNegativePerformanceFailures = logger.results().performance_failures;
      m_FalseNegativeChecks              = logger.results().top_level_checks - logger.results().performance_checks;
      m_FalseNegativePerformanceChecks   = logger.results().performance_checks;
      break;
    case test_mode::false_positive:
      m_FalsePositivePerformanceFailures = logger.results().performance_checks - logger.results().performance_failures;
      m_FalsePositiveFailures            = logger.results().top_level_failures - m_FalsePositivePerformanceFailures;
      m_FalsePositiveChecks              = logger.results().top_level_checks - logger.results().performance_checks;
      m_FalsePositivePerformanceChecks   = logger.results().performance_checks;
      break;
    }
  }
  
  void log_summary::clear() noexcept
  {
    *this = log_summary{""};
  }

  [[nodiscard]]
  std::size_t log_summary::soft_failures() const noexcept
  {
    return standard_top_level_failures()
         + false_positive_failures()
         + false_negative_failures()
         + standard_performance_failures()
         + false_positive_performance_failures()
         + false_negative_performance_failures();
  }

  log_summary& log_summary::operator+=(const log_summary& rhs)
  {
    m_FailureMessages.insert(m_FailureMessages.end(),
                             rhs.m_FailureMessages.begin(),
                             rhs.m_FailureMessages.end());

    m_DiagnosticsOutput.insert(m_DiagnosticsOutput.end(),
                              rhs.m_DiagnosticsOutput.begin(),
                              rhs.m_DiagnosticsOutput.end());

    m_StandardTopLevelChecks         += rhs.m_StandardTopLevelChecks;
    m_StandardDeepChecks             += rhs.m_StandardDeepChecks;
    m_StandardPerformanceChecks      += rhs.m_StandardPerformanceChecks;
    m_FalseNegativeChecks            += rhs.m_FalseNegativeChecks;
    m_FalsePositiveChecks            += rhs.m_FalsePositiveChecks;
    m_FalseNegativePerformanceChecks += rhs.m_FalseNegativePerformanceChecks;
    m_FalsePositivePerformanceChecks += rhs.m_FalsePositivePerformanceChecks;

    m_StandardTopLevelFailures         += rhs.m_StandardTopLevelFailures;
    m_StandardDeepFailures             += rhs.m_StandardDeepFailures;
    m_StandardPerformanceFailures      += rhs.m_StandardPerformanceFailures;
    m_FalseNegativeFailures            += rhs.m_FalseNegativeFailures;
    m_FalsePositiveFailures            += rhs.m_FalsePositiveFailures;
    m_FalseNegativePerformanceFailures += rhs.m_FalseNegativePerformanceFailures;
    m_FalsePositivePerformanceFailures += rhs.m_FalsePositivePerformanceFailures;

    m_CriticalFailures   += rhs.m_CriticalFailures;
    m_ExceptionsInFlight += rhs.m_ExceptionsInFlight;
    m_Duration           += rhs.m_Duration;

    return *this;
  }

  [[nodiscard]]
  log_summary operator+(const log_summary& lhs, const log_summary& rhs)
  {
    log_summary s{lhs};
    return s += rhs;
  }
}
