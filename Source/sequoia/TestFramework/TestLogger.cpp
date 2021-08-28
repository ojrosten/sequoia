////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file Definitions for TestLogger.hpp */

#include "sequoia/TestFramework/TestLogger.hpp"

#include <fstream>

namespace sequoia::testing
{
  namespace impl
  {
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

  void log_summary::clear() noexcept
  {
    log_summary clean{""};
    std::swap(*this, clean);
  }

  [[nodiscard]]
  std::size_t log_summary::soft_failures() const noexcept
  {
    return standard_top_level_failures()
        || false_positive_failures()
        || false_negative_failures()
        || standard_performance_failures()
        || false_positive_performance_failures()
        || false_negative_performance_failures();
  }

  log_summary& log_summary::operator+=(const log_summary& rhs)
  {
    m_FailureMessages += rhs.m_FailureMessages;

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
    m_DiagnosticsOutput  += rhs.m_DiagnosticsOutput;
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