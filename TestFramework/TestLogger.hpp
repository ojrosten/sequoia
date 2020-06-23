////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Utilities for recording the outcome of tests.

    This is the header where the enum class, test_mode, is defined. This ultimately determines
    whether a given test is standard, false-positive or false-negative. See
    \ref sec_robustness for more information.
    The test_logger class
    template employs test_mode as a non-type template parameter. It is via the test_logger that
    other building blocks of the testing framework, such as checkers and concrete test classes
    depend on the test_mode.
 */

#include "TypeTraits.hpp"
#include "Format.hpp"

#include <sstream>
#include <fstream>
#include <array>
#include <chrono>
#include <functional>
#include <filesystem>

namespace sequoia::testing
{
  /*! \brief Holds details of the file to which the last successfully completed test is registered.

      If a check causes a crash, the recovery file may be used to provide a clue as to where this
      happened.
   */
  class output_manager
  {
    inline static std::filesystem::path st_RecoveryFile{};
  public:
    static void recovery_file(const std::filesystem::path& recoveryFile) { st_RecoveryFile = std::string{recoveryFile}; }

    static const std::filesystem::path& recovery_file() noexcept { return st_RecoveryFile; }
  };

  /*! \brief Specifies whether tests are run as standard tests or in false postive/negative mode .
      
      \anchor test_mode_enum
   */
  enum class test_mode { standard, false_positive, false_negative };

  /*! \class
      \brief logs checks and failures within a test.

      The process of logging checks and failures is marshalled by sentinels. At a basic
      level, this RAII class ensures robust treatment in the presence of exceptions.
      But beyond this, it caters for some of the complexity of the testing framework in
      a localized and simple manner. In particular, a given check may actually be
      composed of many sub-checks. For standard checks, we want to count this as a
      single 'top-level' check. For false-positive checks success is counted as a failure
      of one or more of the sub-checks.

      The sentinels deal with all of this in a way which clients of the framework can
      generally ignore. For example, when composing a new specialization of the 
      detailed_equality_checker, there is no need for clients to add sentinels of their
      own.

      The sentinels also take care of writing to the auxiliary file associated with
      false-positive/negative tests.

      \anchor test_logger_primary
   */

  template<test_mode Mode>
  class test_logger;

  template<test_mode Mode>
  class [[nodiscard]] sentinel
  {
  public:
    sentinel(test_logger<Mode>& logger, std::string_view message)
      : m_Logger{logger}
      , m_PriorFailures{logger.failures()}
      , m_PriorDeepChecks{logger.deep_checks()}
    {
      if(!logger.depth())
      {
        logger.current_message(message);
        logger.log_top_level_check();

        if(auto file{output_manager::recovery_file()}; !file.empty())
        {
          if(std::ofstream of{file})
            of << "Check started:\n" << message << "\n";
        }
      }

      logger.increment_depth();
    }

    ~sentinel()
    {
      auto& logger{m_Logger.get()};
      logger.decrement_depth();

      if(!logger.depth())
      {        
        const bool failure{
               ((Mode == test_mode::false_positive) && !failure_detected())
            || ((Mode != test_mode::false_positive) &&  failure_detected())
        };

        if(failure_detected()) logger.end_message();

        auto messageMaker{
          [&logger](){
            auto mess{indent("False Positive Failure:")};
            append_indented(mess, logger.current_message());
            end_block(mess, 3, footer());

            return mess;
          }
        };

        if(failure)
        {
          logger.log_top_level_failure((Mode == test_mode::false_positive) ? messageMaker() : "");
        }
        else if constexpr(Mode == test_mode::false_negative)
        {
          logger.append_to_diagnostics_output(messageMaker());
        }

        if(auto file{output_manager::recovery_file()}; !file.empty())
        {
          if(std::ofstream of{file, std::ios_base::app})
            of << "Check ended\n";
        }
      }

      if(!logger.depth())
        logger.exceptions_detected_by_sentinel(std::uncaught_exceptions());
    }

    sentinel(const sentinel&) = delete;
    sentinel(sentinel&&)      = default;

    sentinel& operator=(const sentinel&) = delete;
    sentinel& operator=(sentinel&&)      = delete;

    [[nodiscard]]
    std::string add_details(std::string_view description, std::string_view details) const
    {
      std::string_view desc{checks_registered() && failure_detected() ? "" : description};
      return append_indented(desc, details);
    }

    void log_performance_check()
    {
      m_Logger.get().log_performance_check();
    }

    void log_check()
    {
      m_Logger.get().log_check();
    }

    [[nodiscard]]
    bool failure_detected() const noexcept
    {
      return m_Logger.get().failures() != m_PriorFailures;
    }

    [[nodiscard]]
    bool checks_registered() const noexcept
    {
      return m_Logger.get().deep_checks() != m_PriorDeepChecks;
    }

    [[nodiscard]]
      test_logger<Mode>& logger() noexcept { return m_Logger.get(); }
  private:
    std::reference_wrapper<test_logger<Mode>> m_Logger;
    std::size_t m_PriorFailures{}, m_PriorDeepChecks{};
  };

  

  template<test_mode Mode>
  class test_logger
  {
    friend class sentinel<Mode>;
  public:
    test_logger() = default;

    test_logger(const test_logger&)            = delete;
    test_logger(test_logger&&) noexcept        = default;
    test_logger& operator=(const test_logger&) = delete;
    test_logger& operator=(test_logger&&)      = delete;

    constexpr static test_mode mode{Mode};
     
    void log_failure(std::string_view message)
    {
      ++m_Failures;
      m_CurrentMessage = message;

      update_output(m_CurrentMessage, 2, "");
    }

    void log_performance_failure(std::string_view message)
    {
      ++m_PerformanceFailures;
      log_failure(message);
    }

    void log_critical_failure(std::string_view message)
    {
      ++m_CriticalFailures;
      m_FailureMessages.append(message).append("\n");
      if(auto file{output_manager::recovery_file()}; !file.empty())
      {
        if(std::ofstream of{file, std::ios_base::app})
          of << "\nCritical Failure:\n" << message << "\n";
      }
    }

    [[nodiscard]]
    std::size_t failures() const noexcept { return m_Failures; }

    [[nodiscard]]
    std::size_t top_level_failures() const noexcept { return m_TopLevelFailures; }

    [[nodiscard]]
    std::size_t performance_failures() const noexcept { return m_PerformanceFailures; }

    [[nodiscard]]
    std::size_t critical_failures() const noexcept { return m_CriticalFailures; }

    [[nodiscard]]
    std::size_t top_level_checks() const noexcept { return m_TopLevelChecks; }
    
    [[nodiscard]]
    std::size_t deep_checks() const noexcept { return m_Checks; }

    [[nodiscard]]
    std::size_t performance_checks() const noexcept { return m_PerformanceChecks; } 

    [[nodiscard]]
    std::string_view failure_messages() const noexcept{ return m_FailureMessages; }

    [[nodiscard]]
    std::string_view current_message() const noexcept { return m_CurrentMessage; }

    [[nodiscard]]
    std::string_view diagnostics_output() const noexcept { return m_DiagnosticsOutput; }

    void exceptions_detected_by_sentinel(const int n) { m_ExceptionsInFlight = n; }

    [[nodiscard]]
    int exceptions_detected_by_sentinel() const noexcept { return m_ExceptionsInFlight; }
  private:
    std::string
      m_FailureMessages,
      m_CurrentMessage,
      m_DiagnosticsOutput;

    int m_ExceptionsInFlight{};
      
    std::size_t
      m_Failures{},
      m_TopLevelFailures{},
      m_PerformanceFailures{},
      m_CriticalFailures{},
      m_TopLevelChecks{},
      m_Checks{},
      m_PerformanceChecks{},
      m_Depth{};

    [[nodiscard]]
    std::size_t depth() const noexcept { return m_Depth; }

    void increment_depth() noexcept { ++m_Depth; }

    void decrement_depth() noexcept { --m_Depth; }

    void log_check() noexcept { ++m_Checks; }

    void log_top_level_check() noexcept { ++m_TopLevelChecks; }

    void log_performance_check() noexcept
    {
      log_check();
      ++m_PerformanceChecks;
    }

    void log_top_level_failure(std::string_view message)
    {
      ++m_TopLevelFailures;
      m_FailureMessages.append(message).append("\n");
    }

    void current_message(std::string_view message)
    {
      m_CurrentMessage = message;
    }

    void append_to_diagnostics_output(std::string message)
    {
      m_DiagnosticsOutput.append(message);
    }

    void end_message()
    {
      if(!m_CurrentMessage.empty()) update_output("", 2, footer());
    }

    void update_output(std::string_view message, std::size_t newLines, std::string foot)
    {
      auto append{
        [message, newLines, ft{std::move(foot)}](std::string& output) {
          output.append(message);
          end_block(output, newLines, ft);
        }
      };

      if constexpr (Mode != test_mode::false_positive)
      {
        append(m_FailureMessages);
      }
      else
      {
        append(m_DiagnosticsOutput);
      }
    }
  };

  enum class log_verbosity { terse=0, absent_checks=1, failure_messages=2};

  [[nodiscard]]
  constexpr log_verbosity operator&(log_verbosity lhs, log_verbosity rhs) noexcept
  {
    return static_cast<log_verbosity>(static_cast<int>(lhs) & static_cast<int>(rhs));
  }

  /*! \brief Summaries data generated by the logger, for the purposes of reporting.
     
      Summaries may be combined by using either operator+= or operator+
   */

  class log_summary
  {
  public:
    using duration = std::chrono::steady_clock::duration;
    
    explicit log_summary(std::string_view name="") : m_Name{name} {}

    template<test_mode Mode> log_summary(std::string_view name, const test_logger<Mode>& logger, const duration delta)
      : m_Name{name}
      , m_FailureMessages{logger.failure_messages()}
      , m_CurrentMessage{logger.current_message()}
      , m_Duration{delta}
    {
      switch(Mode)
      {
      case test_mode::standard:
        m_StandardTopLevelFailures    = logger.top_level_failures() - logger.performance_failures();
        m_StandardDeepFailures        = logger.failures() - logger.performance_failures();
        m_StandardPerformanceFailures = logger.performance_failures();
        m_StandardTopLevelChecks      = logger.top_level_checks() - logger.performance_checks();
        m_StandardDeepChecks          = logger.deep_checks() - logger.performance_checks();
        m_StandardPerformanceChecks   = logger.performance_checks();
        break;
      case test_mode::false_negative:
        m_FalseNegativeFailures            = logger.top_level_failures() - logger.performance_failures();
        m_FalseNegativePerformanceFailures = logger.performance_failures();
        m_FalseNegativeChecks              = logger.top_level_checks() - logger.performance_checks();
        m_FalseNegativePerformanceChecks   = logger.performance_checks();
        break;
      case test_mode::false_positive:        
        m_FalsePositivePerformanceFailures = logger.performance_checks() - logger.performance_failures();
        m_FalsePositiveFailures            = logger.top_level_failures() - m_FalsePositivePerformanceFailures;
        m_FalsePositiveChecks              = logger.top_level_checks() - logger.performance_checks();
        m_FalsePositivePerformanceChecks   = logger.performance_checks();
        break;
      }

      m_DiagnosticsOutput = logger.diagnostics_output();
      m_CriticalFailures = logger.critical_failures();
    }

    void clear()
    {
      log_summary clean{""};
      std::swap(*this, clean);
    }

    [[nodiscard]]
    std::size_t standard_top_level_checks() const noexcept { return m_StandardTopLevelChecks; }

    [[nodiscard]]
    std::size_t standard_deep_checks() const noexcept { return m_StandardDeepChecks; }

    [[nodiscard]]
    std::size_t standard_performance_checks() const noexcept { return m_StandardPerformanceChecks; }

    [[nodiscard]]
    std::size_t false_negative_checks() const noexcept { return m_FalseNegativeChecks; }

    [[nodiscard]]
    std::size_t false_negative_performance_checks() const noexcept { return m_FalseNegativePerformanceChecks; }

    [[nodiscard]]
    std::size_t false_positive_checks() const noexcept { return m_FalsePositiveChecks; }

    [[nodiscard]]
    std::size_t false_positive_performance_checks() const noexcept { return m_FalsePositivePerformanceChecks; }

    [[nodiscard]]
    std::size_t standard_deep_failures() const noexcept { return m_StandardDeepFailures; }

    [[nodiscard]]
    std::size_t standard_top_level_failures() const noexcept { return m_StandardTopLevelFailures; }

    [[nodiscard]]
    std::size_t standard_performance_failures() const noexcept { return m_StandardPerformanceFailures; }

    [[nodiscard]]
    std::size_t false_negative_failures() const noexcept { return m_FalseNegativeFailures; }

    [[nodiscard]]
    std::size_t false_negative_performance_failures() const noexcept { return m_FalseNegativePerformanceFailures; }

    [[nodiscard]]
    std::size_t false_positive_failures() const noexcept { return m_FalsePositiveFailures; }

    [[nodiscard]]
    std::size_t false_positive_performance_failures() const noexcept { return m_FalsePositivePerformanceFailures; }
 
    [[nodiscard]]
    std::size_t critical_failures() const noexcept { return m_CriticalFailures; }

    [[nodiscard]]
    std::string_view diagnostics_output() const noexcept { return m_DiagnosticsOutput; }

    [[nodiscard]]
    std::string current_message() const
    {
      return m_Name.empty()
        ?  m_CurrentMessage
        : append_indented(std::string{"["}.append(m_Name).append("]"), m_CurrentMessage);
    }

    [[nodiscard]]
    duration execution_time() const noexcept { return m_Duration; }
      
    log_summary& operator+=(const log_summary& rhs)
    {
      m_FailureMessages                += rhs.m_FailureMessages;

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
        
      m_CurrentMessage      = rhs.m_CurrentMessage;


      return *this;
    }

    [[nodiscard]]
    std::string_view name() const noexcept { return m_Name; }

    [[nodiscard]]
    std::string_view failure_messages() const noexcept { return m_FailureMessages; }

    [[nodiscard]]
    friend log_summary operator+(const log_summary& lhs, const log_summary& rhs)
    {
      log_summary s{lhs};
      s += rhs;
      return s;
    }
  private:
    std::string m_Name, m_FailureMessages, m_CurrentMessage, m_DiagnosticsOutput;
    std::size_t
      m_StandardTopLevelChecks{},
      m_StandardDeepChecks{},
      m_StandardPerformanceChecks{},
      m_FalseNegativeChecks{},
      m_FalsePositiveChecks{},
      m_FalseNegativePerformanceChecks{},
      m_FalsePositivePerformanceChecks{},
      m_StandardTopLevelFailures{},
      m_StandardDeepFailures{},
      m_StandardPerformanceFailures{},
      m_FalseNegativeFailures{},
      m_FalsePositiveFailures{},
      m_FalseNegativePerformanceFailures{},
      m_FalsePositivePerformanceFailures{},
      m_CriticalFailures{};

    int m_ExceptionsInFlight{};

    duration m_Duration{};
  };
}
