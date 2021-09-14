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

  [[nodiscard]]
  std::string to_string(const failure_output& output)
  {
    std::string str{};
    for(const auto& info : output)
    {
      if(!info.message.empty())
        str.append(info.message).append("\n");
    }

    return str;
  }

  //================================== sentinel ==================================//

  template<test_mode Mode>
  sentinel<Mode>::sentinel(test_logger<Mode>& logger, std::string_view message)
    : m_pLogger{&logger}
    , m_Message{message}
    , m_PriorFailures{logger.failures()}
    , m_PriorCriticalFailures{logger.critical_failures()}
    , m_PriorDeepChecks{logger.deep_checks()}
  {
    if(!logger.depth())
    {
      logger.log_top_level_check();
      impl::record_check_started(get().recovery().recovery_file, message);
    }

    impl::recored_dump_started(get().recovery().dump_file, message);
    logger.increment_depth(message);
  }

  template<test_mode Mode>
  sentinel<Mode>::~sentinel()
  {
    auto& logger{get()};

    if(logger.depth() == 1)
    {
      if(critical_failure_detected())
      {
        logger.end_message(test_logger<Mode>::is_critical::yes);
      }
      else
      {
        if(failure_detected()) logger.end_message(test_logger<Mode>::is_critical::no);

        auto fpMessageMaker{
          [&logger](){
            auto mess{indent("False Positive Failure:", logger.top_level_message(), tab)};
            end_block(mess, 3_linebreaks, indent(footer(), tab));

            return mess;
          }
        };

        const bool modeSpecificFailure{
          ((Mode == test_mode::false_positive) && !failure_detected())
          || ((Mode != test_mode::false_positive) &&  failure_detected())
        };

        if(modeSpecificFailure)
        {
          logger.log_top_level_failure((Mode == test_mode::false_positive) ? fpMessageMaker() : "");
        }
        else if constexpr(Mode == test_mode::false_negative)
        {
          if(!critical_failure_detected())
            logger.append_to_diagnostics_output(fpMessageMaker());
        }

        impl::record_check_ended(get().recovery().recovery_file);
      }

      impl::recored_dump_ended(get().recovery().dump_file);
    }
      
    logger.decrement_depth();
  }

  template<test_mode Mode>
  void sentinel<Mode>::log_performance_check()
  {
    get().log_performance_check();
  }

  template<test_mode Mode>
  void sentinel<Mode>::log_check()
  {
    get().log_check();
  }

  template<test_mode Mode>
  void sentinel<Mode>::log_failure(std::string_view message)
  {
    get().log_failure(message);
  }

  template<test_mode Mode>
  void sentinel<Mode>::log_performance_failure(std::string_view message)
  {
    get().log_performance_failure(message);
  }

  template<test_mode Mode>
  void sentinel<Mode>::log_critical_failure(std::string_view message)
  {
    get().log_critical_failure(message);
  }

  template<test_mode Mode>
  void sentinel<Mode>::log_caught_exception_message(std::string_view message)
  {
    get().log_caught_exception_message(message);
  }

  template<test_mode Mode>
  [[nodiscard]]
  bool sentinel<Mode>::critical_failure_detected() const noexcept
  {
    return get().critical_failures() != m_PriorCriticalFailures;
  }

  template<test_mode Mode>
  [[nodiscard]]
  bool sentinel<Mode>::failure_detected() const noexcept
  {
    return get().failures() != m_PriorFailures;
  }

  template<test_mode Mode>
  [[nodiscard]]
  bool sentinel<Mode>::checks_registered() const noexcept
  {
    return get().deep_checks() != m_PriorDeepChecks;
  }

  template<test_mode Mode>
  [[nodiscard]]
  test_logger<Mode>& sentinel<Mode>::get() noexcept { return *m_pLogger; }

  template<test_mode Mode>
  [[nodiscard]]
  const test_logger<Mode>& sentinel<Mode>::get() const noexcept { return *m_pLogger; }

  template class sentinel<test_mode::standard>;
  template class sentinel<test_mode::false_positive>;
  template class sentinel<test_mode::false_negative>;
  
  //================================== test_logger ==================================//

  //================================== public ==================================//
  template<test_mode Mode>
  [[nodiscard]]
  std::size_t test_logger<Mode>::failures() const noexcept { return m_Failures; }

  template<test_mode Mode>
  [[nodiscard]]
  std::size_t test_logger<Mode>::top_level_failures() const noexcept { return m_TopLevelFailures; }

  template<test_mode Mode>
  [[nodiscard]]
  std::size_t test_logger<Mode>::performance_failures() const noexcept { return m_PerformanceFailures; }

  template<test_mode Mode>
  [[nodiscard]]
  std::size_t test_logger<Mode>::critical_failures() const noexcept { return m_CriticalFailures; }

  template<test_mode Mode>
  [[nodiscard]]
  std::size_t test_logger<Mode>::top_level_checks() const noexcept { return m_TopLevelChecks; }

  template<test_mode Mode>
  [[nodiscard]]
  std::size_t test_logger<Mode>::deep_checks() const noexcept { return m_Checks; }

  template<test_mode Mode>
  [[nodiscard]]
  std::size_t test_logger<Mode>::performance_checks() const noexcept { return m_PerformanceChecks; }

  template<test_mode Mode>
  [[nodiscard]]
  const failure_output& test_logger<Mode>::failure_messages() const noexcept { return m_FailureMessages; }

  template<test_mode Mode>
  [[nodiscard]]
  const failure_output& test_logger<Mode>::diagnostics_output() const noexcept { return m_DiagnosticsOutput; }    

  template<test_mode Mode>
  [[nodiscard]]
  const failure_output& test_logger<Mode>::caught_exceptions_output() const noexcept { return m_CaughtExceptionMessages; }

  template<test_mode Mode>
  [[nodiscard]]
  const uncaught_exception_info& test_logger<Mode>::exceptions_detected_by_sentinel() const
  {
    return m_UncaughtExceptionInfo;
  }
  
  template<test_mode Mode>
  [[nodiscard]]
  std::string_view test_logger<Mode>::top_level_message() const noexcept
  {
    return !m_SentinelDepth.empty() ? std::string_view{m_SentinelDepth.front().message} : "";
  }

  template<test_mode Mode>
  [[nodiscard]]
  const recovery_paths& test_logger<Mode>::recovery() const noexcept { return m_Recovery; }

  template<test_mode Mode>
  void test_logger<Mode>::recovery(recovery_paths paths)
  {
    m_Recovery = std::move(paths);
  }

  //================================== private ==================================//

  template<test_mode Mode>
  [[nodiscard]]
  std::size_t test_logger<Mode>::depth() const noexcept { return m_SentinelDepth.size(); }

  template<test_mode Mode>
  void test_logger<Mode>::log_check() noexcept { ++m_Checks; }

  template<test_mode Mode>
  void test_logger<Mode>::log_top_level_check() noexcept { ++m_TopLevelChecks; }

  template<test_mode Mode>
  void test_logger<Mode>::log_performance_check() noexcept
  {
    log_check();
    ++m_PerformanceChecks;
  }

  template<test_mode Mode>
  void test_logger<Mode>::failure_message(std::string_view message, const is_critical isCritical)
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

    indentation ind{tab};
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

    auto& output{update_output(msg, isCritical, update_mode::fresh)};
    end_block(output.back().message, 1_linebreaks, "");
  }

  template<test_mode Mode>
  void test_logger<Mode>::log_failure(std::string_view message)
  {
    ++m_Failures;
    failure_message(message, is_critical::no);
  }

  template<test_mode Mode>
  void test_logger<Mode>::log_performance_failure(std::string_view message)
  {
    ++m_PerformanceFailures;
    log_failure(message);
  }

  template<test_mode Mode>
  void test_logger<Mode>::log_critical_failure(std::string_view message)
  {
    ++m_CriticalFailures;
    failure_message(message, is_critical::yes);
    impl::recored_critical_failure(m_Recovery.recovery_file, message);
  }

  template<test_mode Mode>
  void test_logger<Mode>::log_top_level_failure(std::string message)
  {
    ++m_TopLevelFailures;
    if(m_SentinelDepth.empty())
    {
      m_SentinelDepth.push_back(level_message{""});
    }
    else
    {
      m_SentinelDepth.back().message.append(std::move(message));
    }
  }

  template<test_mode Mode>
  void test_logger<Mode>::log_caught_exception_message(std::string_view message)
  {
    auto mess{std::string{top_level_message()}.append("\n").append(message)};
    end_block(mess, 2_linebreaks, indent(footer(), tab));

    add_to_output(m_CaughtExceptionMessages, mess, update_mode::fresh);
  }

  template<test_mode Mode>
  void test_logger<Mode>::append_to_diagnostics_output(std::string message)
  {
    m_DiagnosticsOutput.push_back(failure_info{m_TopLevelChecks, std::move(message)});
  }

  template<test_mode Mode>
  void test_logger<Mode>::increment_depth(std::string_view message)
  {
    m_SentinelDepth.emplace_back(message);
  }

  template<test_mode Mode>
  void test_logger<Mode>::decrement_depth()
  {
    if(m_SentinelDepth.empty())
      throw std::logic_error{"Cannot pop from TestLogger's empty stack"};

    if(depth() == 1)
    {
      m_UncaughtExceptionInfo = {std::uncaught_exceptions(), std::move(m_SentinelDepth.front().message)}; 
    }
      
    m_SentinelDepth.pop_back();
  }

  template<test_mode Mode>
  void test_logger<Mode>::end_message(const is_critical isCritical)
  {
    auto& output{output_channel(isCritical)};
    auto& mess{output.back().message};
    end_block(mess, 2_linebreaks, indent(footer(), tab));
  }

  template<test_mode Mode>
  failure_output& test_logger<Mode>::output_channel(is_critical isCritical) noexcept
  {
    const bool toMessages{(Mode != test_mode::false_positive) || (isCritical == is_critical::yes)};
    return toMessages ? m_FailureMessages : m_DiagnosticsOutput;
  }

  template<test_mode Mode>
  failure_output& test_logger<Mode>::update_output(std::string_view message, is_critical isCritical, update_mode uMode)
  {
    return add_to_output(output_channel(isCritical), message, uMode);
  }

  template<test_mode Mode>
  failure_output& test_logger<Mode>::add_to_output(failure_output& output, std::string_view message, update_mode uMode)
  {
    switch(uMode)
    {
    case update_mode::fresh:
      output.push_back(failure_info{m_TopLevelChecks, std::string{message}});
      break;
    case update_mode::app:
      if(!output.empty())
      {
        output.back().message.append(message);
      }
      break;
    }

    return output;
  }

  template class test_logger<test_mode::standard>;
  template class test_logger<test_mode::false_positive>;
  template class test_logger<test_mode::false_negative>;
  
  //================================== log_summary ==================================//

  log_summary::log_summary(std::string_view name) : m_Name{name} {}

  template<test_mode Mode>
  log_summary::log_summary(std::string_view name, const test_logger<Mode>& logger, const duration delta)
    : m_Name{name}
    , m_FailureMessages{to_string(logger.failure_messages())}
    , m_DiagnosticsOutput{to_string(logger.diagnostics_output())}
    , m_CaughtExceptionMessages{to_string(logger.caught_exceptions_output())}
    , m_CriticalFailures{logger.critical_failures()}
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

  template log_summary::log_summary(std::string_view, const test_logger<test_mode::standard>&, const duration);
  template log_summary::log_summary(std::string_view, const test_logger<test_mode::false_positive>&, const duration);
  template log_summary::log_summary(std::string_view, const test_logger<test_mode::false_negative>&, const duration);
}
