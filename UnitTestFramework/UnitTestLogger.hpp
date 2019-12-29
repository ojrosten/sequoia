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
  
  template<class Iter> void pad_right(Iter begin, Iter end, std::string_view suffix)
  {
    auto maxIter{
      std::max_element(begin, end, [](const std::string& lhs, const std::string& rhs) {
          return lhs.size() < rhs.size();
        }
      )
    };

    const auto maxChars{maxIter->size()};

    for(; begin != end; ++begin)
    {
      auto& s{*begin};
      s += std::string(maxChars - s.size(), ' ') += std::string{suffix};
    }
  }  
      
  template<class Iter> void pad_left(Iter begin, Iter end, const std::size_t minChars)
  {
    auto maxIter{std::max_element(begin, end, [](const std::string& lhs, const std::string& rhs) {
          return lhs.size() < rhs.size();
        }
      )
    };

    const auto maxChars{std::max(maxIter->size(), minChars)};

    for(; begin != end; ++begin)
    {
      auto& s{*begin};
      s = std::string(maxChars - s.size(), ' ') + s;
    }
  }

  enum class test_mode { standard, false_positive, false_negative };

  [[nodiscard]]
  inline constexpr bool diagnostic(const test_mode mode) noexcept { return (mode != test_mode::standard ); }
    
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
        if(!logger.depth()) logger.current_message(message);
        
        if(diagnostic(Mode) && !logger.depth()) logger.log_check();
        
        logger.increment_depth();
      }

      ~sentinel()
      {
        auto& logger{m_Logger.get()};
        logger.decrement_depth();
        if constexpr (diagnostic(Mode))
        {
          if(!logger.depth())
          {
            const bool failure{((Mode == test_mode::false_positive) && !logger.failures()) || ((Mode == test_mode::false_negative) && logger.failures())};
            if(failure)
            {
              std::string message{
                (Mode == test_mode::false_positive) ? combine_messages("\tFalse Positive Failure:", logger.current_message(), "\n") : ""
              };

              logger.log_diagnostic_failure(std::move(message));
            }
            logger.reset_failures();
          }
        }

        if(!logger.depth()) logger.exceptions_detected_by_sentinel(std::uncaught_exceptions());
      }

      sentinel(const sentinel&) = delete;
      sentinel(sentinel&&)      = default;

      sentinel& operator=(const sentinel&) = delete;
      sentinel& operator=(sentinel&&)      = delete;

      void log_performance_check()
      {
        if(!diagnostic(Mode)) m_Logger.get().log_performance_check();
      }

      void log_check()
      {
        if(!diagnostic(Mode)) m_Logger.get().log_check();
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
    std::size_t performance_failures() const noexcept { return m_PerformanceFailures; }     

    [[nodiscard]]
    std::size_t diagnostic_failures() const noexcept { return m_DiagnosticFailures; }

    [[nodiscard]]
    std::size_t critical_failures() const noexcept { return m_CriticalFailures; }

    [[nodiscard]]
    std::size_t checks() const noexcept { return m_Checks; }

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
      m_PerformanceFailures{},
      m_Checks{},
      m_PerformanceChecks{},
      m_Depth{},
      m_DiagnosticFailures{},
      m_CriticalFailures{};

    [[nodiscard]]
    std::size_t depth() const noexcept { return m_Depth; }

    void increment_depth() noexcept { ++m_Depth; }

    void decrement_depth() noexcept { --m_Depth; }

    void log_check() noexcept { ++m_Checks; }

    void log_performance_check() noexcept
    {
      log_check();
      ++m_PerformanceChecks;
    }

    void reset_failures() noexcept { m_Failures = 0u; }

    void log_diagnostic_failure(std::string_view message)
    {
      ++m_DiagnosticFailures;
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
    log_summary(std::string_view name="") : m_Name{name}
    {
    }
      
    template<test_mode Mode> log_summary(std::string_view name, const unit_test_logger<Mode>& logger)
      : m_Name{name}
      , m_Message{logger.messages()}
      , m_CurrentMessage{logger.current_message()}
    {
      switch(Mode)
      {
      case test_mode::standard:
        m_StandardFailures    = logger.failures() - logger.performance_failures();
        m_PerformanceFailures = logger.performance_failures();
        m_Checks              = logger.checks() - logger.performance_checks();
        m_PerformanceChecks   = logger.performance_checks();
        break;
      case test_mode::false_negative:
        m_FalseNegativeFailures = logger.diagnostic_failures();
        m_FalseNegativeChecks   = logger.checks();
        break;
      case test_mode::false_positive:
        m_FalsePositiveFailures = logger.diagnostic_failures();
        m_FalsePositiveChecks   = logger.checks();
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
    std::size_t checks() const noexcept { return m_Checks; }

    [[nodiscard]]
    std::size_t performance_checks() const noexcept { return m_PerformanceChecks; }

    [[nodiscard]]
    std::size_t false_negative_checks() const noexcept { return m_FalseNegativeChecks; }

    [[nodiscard]]
    std::size_t false_positive_checks() const noexcept { return m_FalsePositiveChecks; }

    [[nodiscard]]
    std::size_t standard_failures() const noexcept { return m_StandardFailures; }

    [[nodiscard]]
    std::size_t performance_failures() const noexcept { return m_PerformanceFailures; }

    [[nodiscard]]
    std::size_t false_negative_failures() const noexcept { return m_FalseNegativeFailures; }

    [[nodiscard]]
    std::size_t false_positive_failures() const noexcept { return m_FalsePositiveFailures; }

    [[nodiscard]]
    std::size_t critical_failures() const noexcept { return m_CriticalFailures; }

    [[nodiscard]]
    std::string current_message() const
    {
      return m_Name.empty() ?  m_CurrentMessage : '[' + m_Name + "]\n\t" + m_CurrentMessage;
    }
      
    log_summary& operator+=(const log_summary& rhs)
    {
      m_Message               += rhs.m_Message;
      m_Checks                += rhs.m_Checks;
      m_PerformanceChecks     += rhs.m_PerformanceChecks;
      m_FalseNegativeChecks   += rhs.m_FalseNegativeChecks;
      m_FalsePositiveChecks   += rhs.m_FalsePositiveChecks;
      m_StandardFailures      += rhs.m_StandardFailures;
      m_PerformanceFailures   += rhs.m_PerformanceFailures;
      m_FalseNegativeFailures += rhs.m_FalseNegativeFailures;
      m_FalsePositiveFailures += rhs.m_FalsePositiveFailures;
      m_CriticalFailures      += rhs.m_CriticalFailures;
      m_ExceptionsInFlight    += rhs.m_ExceptionsInFlight;
        
      m_CurrentMessage         = rhs.m_CurrentMessage;

      return *this;
    }

    [[nodiscard]]
    const std::string& name() const noexcept { return m_Name; }
    
    [[nodiscard]]
    std::string summarize(std::string_view checkNumsPrefix, const log_verbosity suppression ) const
    {
      std::array<std::string, 4> summaries{
        std::string{checkNumsPrefix}.append("Standard Checks:"),
        std::string{checkNumsPrefix}.append("Performance Checks:"),
        std::string{checkNumsPrefix}.append("False Negative Checks:"),
        std::string{checkNumsPrefix}.append("False Positive Checks:")
      };

      pad_right(summaries.begin(), summaries.end(), "  ");

      std::array<std::string, 4> checkNums{
        std::to_string(checks()),
        std::to_string(performance_checks()),
        std::to_string(false_negative_checks()),
        std::to_string(false_positive_checks())
      };

      constexpr std::size_t minChars{8};
      pad_left(checkNums.begin(), checkNums.end(), minChars);

      const auto len{10u - std::min(std::size_t{minChars}, checkNums.front().size())};
        
      for(int i{}; i<4; ++i)
      {
        summaries[i].append(checkNums[i] + ";").append(len, ' ').append("Failures: ");
      }

      std::array<std::string, 4> failures{
        std::to_string(standard_failures()),
        std::to_string(performance_failures()),
        std::to_string(false_negative_failures()),
        std::to_string(false_positive_failures())
      };

      pad_left(failures.begin(), failures.end(), 2);

      for(int i{}; i<4; ++i)
      {
        summaries[i] += failures[i];
      }

      std::string summary{m_Name.empty() ? "" : m_Name + ":\n"};

      if((suppression & log_verbosity::absent_checks) == log_verbosity::absent_checks)
      {
        std::for_each(std::cbegin(summaries), std::cend(summaries), [&summary](const std::string& s){
            (summary += s) += "\n";
          }
        );
      }
      else
      {
        const std::array<std::size_t, 4> numbers{
          checks(),
          performance_checks(),
          false_negative_checks(),
          false_positive_checks()
        };

        for(int i{}; i<4; ++i)
        {
            
          if(numbers[i]) summary += summaries[i] += "\n";
        }
      }

      if(m_CriticalFailures)
      {
        (summary += "Critical Failures:  ") += std::to_string(critical_failures()) += "\n";
      }

      if((suppression & log_verbosity::failure_messages) == log_verbosity::failure_messages)
        summary += m_Message;

      return summary;
    }

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
      m_Checks{},
      m_PerformanceChecks{},
      m_FalseNegativeChecks{},
      m_FalsePositiveChecks{},
      m_StandardFailures{},
      m_PerformanceFailures{},
      m_FalseNegativeFailures{},
      m_FalsePositiveFailures{},
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
