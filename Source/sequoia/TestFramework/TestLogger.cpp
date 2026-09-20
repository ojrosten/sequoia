////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

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
  }

  void record_check_started(const active_recovery_files& files, std::string_view message)
  {
    if(!files.recovery_file.empty())
    {
      if(std::ofstream of{files.recovery_file})
        of << "Check started:\n" << message << "\n";
    }
  }

  void record_check_ended(const active_recovery_files& files)
  {
    if(!files.recovery_file.empty())
    {
      if(std::ofstream of{files.recovery_file, std::ios_base::app})
        of << "Check ended\n";
    }
  }

  void record_dump_started(const active_recovery_files& files, std::string_view message)
  {
    if(!files.dump_file.empty())
    {
      if(std::ofstream of{files.dump_file, std::ios_base::app})
        of << message << "\n";
    }
  }

  void record_dump_ended(const active_recovery_files& files)
  {
    if(!files.dump_file.empty())
    {
      if(std::ofstream of{files.dump_file, std::ios_base::app})
        of << "\n\n";
    }
  }

  void record_critical_failure(const active_recovery_files& files, std::string_view message)
  {
    if(!files.recovery_file.empty())
    {
      if(std::ofstream of{files.recovery_file, std::ios_base::app})
        of << "\nCritical Failure:\n" << message << "\n";
    }
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
    , m_CriticalFailures{logger.results().critical_failures}
    , m_Duration{delta}
  {
    switch(mode)
    {
    case test_mode::standard:
      m_StandardTopLevelFailures    = logger.results().top_level_failures - logger.results().performance_failures;
      m_StandardDeepFailures        = logger.results().failures - logger.results().performance_failures;
      m_StandardPerformanceFailures = logger.results().performance_failures;
      m_StandardTopLevelChecks      = logger.results().top_level_checks - logger.results().performance_checks;
      m_StandardDeepChecks          = logger.results().deep_checks - logger.results().performance_checks;
      m_StandardPerformanceChecks   = logger.results().performance_checks;
      break;
    case test_mode::false_positive:
      m_FalsePositiveFailures            = logger.results().top_level_failures - logger.results().performance_failures;
      m_FalsePositivePerformanceFailures = logger.results().performance_failures;
      m_FalsePositiveChecks              = logger.results().top_level_checks - logger.results().performance_checks;
      m_FalsePositivePerformanceChecks   = logger.results().performance_checks;
      break;
    case test_mode::false_negative:
      m_FalseNegativePerformanceFailures = logger.results().performance_checks - logger.results().performance_failures;
      m_FalseNegativeFailures            = logger.results().top_level_failures - m_FalseNegativePerformanceFailures;
      m_FalseNegativeChecks              = logger.results().top_level_checks - logger.results().performance_checks;
      m_FalseNegativePerformanceChecks   = logger.results().performance_checks;
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
         + false_negative_failures()
         + false_positive_failures()
         + standard_performance_failures()
         + false_negative_performance_failures()
         + false_positive_performance_failures();
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
    m_FalsePositiveChecks            += rhs.m_FalsePositiveChecks;
    m_FalseNegativeChecks            += rhs.m_FalseNegativeChecks;
    m_FalsePositivePerformanceChecks += rhs.m_FalsePositivePerformanceChecks;
    m_FalseNegativePerformanceChecks += rhs.m_FalseNegativePerformanceChecks;

    m_StandardTopLevelFailures         += rhs.m_StandardTopLevelFailures;
    m_StandardDeepFailures             += rhs.m_StandardDeepFailures;
    m_StandardPerformanceFailures      += rhs.m_StandardPerformanceFailures;
    m_FalsePositiveFailures            += rhs.m_FalsePositiveFailures;
    m_FalseNegativeFailures            += rhs.m_FalseNegativeFailures;
    m_FalsePositivePerformanceFailures += rhs.m_FalsePositivePerformanceFailures;
    m_FalseNegativePerformanceFailures += rhs.m_FalseNegativePerformanceFailures;

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
