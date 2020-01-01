////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file UnitTestLogger.hpp
    \brief Utilities for recording the outcome of the unit tests

 */

#include "TypeTraits.hpp"

#include <sstream>
#include <fstream>
#include <array>

#ifndef _MSC_VER_
  #include <cxxabi.h>
#endif

namespace sequoia::unit_testing
{
  [[nodiscard]]
  inline std::string combine_messages(std::string_view s1, std::string_view s2, std::string_view sep=" ")
  {
    std::string mess{};
    if(s1.empty())
    {
      if(!s2.empty()) mess.append("\t").append(s2);
    }
    else
    {
      mess.append(s1);
      if(!s2.empty())
      {
        if((mess.back() == '\n') && (sep.empty() || sep == " "))
          mess.append("\t");
        else
        {
          mess.append(sep);
          if(sep.back() == '\n') mess.append("\t");
        }
          
        mess.append(s2);
      }
    }
        
    return mess;
  }
  
  template<class T> std::string demangle()
  {
    #ifndef _MSC_VER_
      int status;
      return abi::__cxa_demangle(typeid(T).name(), 0, 0, &status);
    #else        
      return typeid(T).name();
    #endif
  }

  enum class test_mode { standard, false_positive, false_negative };

  template<test_mode Mode>
  class unit_test_logger
  {
  public:
    unit_test_logger()  = default;
    ~unit_test_logger() = default;
      
    unit_test_logger(const unit_test_logger&)            = delete;
    unit_test_logger(unit_test_logger&&) noexcept        = default;
    unit_test_logger& operator=(const unit_test_logger&) = delete;
    unit_test_logger& operator=(unit_test_logger&&)      = delete;

    constexpr static test_mode mode{Mode};
      
    class [[nodiscard]] sentinel
    {
    public:
      sentinel(unit_test_logger& logger, std::string_view message)
        : m_Logger{logger}
      {
        if(!logger.depth())
        {
          logger.current_message(message);
          logger.log_top_level_check();
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
            ((Mode == test_mode::false_positive) && !logger.failures()) || ((Mode != test_mode::false_positive) && logger.failures())
          };

          if(failure)
          {
            auto message{
              [&logger](){
                return (Mode == test_mode::false_positive)
                  ? combine_messages("\tFalse Positive Failure:", logger.current_message(), "\n") : "";                
              }
            };

            logger.log_top_level_failure(message());
          }
          logger.reset_failures();
        }

        if(!logger.depth()) logger.exceptions_detected_by_sentinel(std::uncaught_exceptions());
      }

      sentinel(const sentinel&) = delete;
      sentinel(sentinel&&)      = default;

      sentinel& operator=(const sentinel&) = delete;
      sentinel& operator=(sentinel&&)      = delete;

      void log_performance_check()
      {
        m_Logger.get().log_performance_check();
      }

      void log_check()
      {
        m_Logger.get().log_check();
      }
    private:
      std::reference_wrapper<unit_test_logger> m_Logger;
    };       
      
    void log_failure(std::string_view message)
    {
      ++m_Failures;
      m_CurrentMessage = message;
      if constexpr (Mode != test_mode::false_positive) m_Messages.append(m_CurrentMessage).append("\n");
      if(std::ofstream of{m_RecoveryFile}) of << m_Messages;
    }

    void log_performance_failure(std::string_view message)
    {
      ++m_PerformanceFailures;
      log_failure(message);
    }

    void log_critical_failure(std::string_view message)
    {
      ++m_CriticalFailures;
      m_Messages.append(message).append("\n");
      if(std::ofstream of{m_RecoveryFile}) of << m_Messages;
    }

    void post_message(std::string_view message)
    {
      m_Messages.append(message);
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
    const std::string& messages() const noexcept{ return m_Messages; }

    [[nodiscard]]
    const std::string& current_message() const noexcept { return m_CurrentMessage; }

    void exceptions_detected_by_sentinel(const int n) { m_ExceptionsInFlight = n; }

    [[nodiscard]]
    int exceptions_detected_by_sentinel() const noexcept { return m_ExceptionsInFlight; }
  private:
    std::string
      m_Messages,
      m_CurrentMessage;

    // TO DO: use std::filesystem once supported!
    std::string m_RecoveryFile{"../output/Recovery.txt"};

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

    void reset_failures() noexcept { m_Failures = 0u; }

    void log_top_level_failure(std::string_view message)
    {
      ++m_TopLevelFailures;
      m_Messages.append(message).append("\n");
    }

    void current_message(std::string_view message)
    {
      m_CurrentMessage = message;
      if(std::ofstream of{m_RecoveryFile}) of << m_CurrentMessage;            
    }
  };

  enum class log_verbosity { terse=0, absent_checks=1, failure_messages=2};

  [[nodiscard]]
  constexpr log_verbosity operator&(log_verbosity lhs, log_verbosity rhs) noexcept
  {
    return static_cast<log_verbosity>(static_cast<int>(lhs) & static_cast<int>(rhs));
  }

  class log_summary
  {
  public:
    log_summary(std::string_view name="") : m_Name{name} {}
      
    template<test_mode Mode> log_summary(std::string_view name, const unit_test_logger<Mode>& logger)
      : m_Name{name}
      , m_Message{logger.messages()}
      , m_CurrentMessage{logger.current_message()}
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
    std::string current_message() const
    {
      return m_Name.empty() ?  m_CurrentMessage : '[' + m_Name + "]\n\t" + m_CurrentMessage;
    }
      
    log_summary& operator+=(const log_summary& rhs)
    {
      m_Message                        += rhs.m_Message;

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
        
      m_CurrentMessage      = rhs.m_CurrentMessage;

      return *this;
    }

    [[nodiscard]]
    const std::string& name() const noexcept { return m_Name; }

    [[nodiscard]]
    const std::string& message() const noexcept { return m_Message; }

    [[nodiscard]]
    friend log_summary operator+(const log_summary& lhs, const log_summary& rhs)
    {
      log_summary s{lhs};
      s += rhs;
      return s;
    }
  private:
    std::string m_Name, m_Message, m_CurrentMessage;
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

    [[nodiscard]]
    static std::string pluralize(const std::size_t n, std::string_view sv)
    {
      const std::string s{sv};
      return (n==1) ? " " + s : " " + s + "s";
    }
  };
}
