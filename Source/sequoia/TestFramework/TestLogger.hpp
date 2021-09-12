////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
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

#include "sequoia/Core/Meta/TypeTraits.hpp"
#include "sequoia/TestFramework/Output.hpp"

#include <chrono>
#include <filesystem>
#include <vector>

namespace sequoia::testing
{
  namespace impl
  {
    void record_check_started(const std::filesystem::path& file, std::string_view message);
    void record_check_ended(const std::filesystem::path& file);
    void recored_dump_started(const std::filesystem::path& file, std::string_view message);
    void recored_dump_ended(const std::filesystem::path& file);
    void recored_critical_failure(const std::filesystem::path& file, std::string_view message);
  }

  /*! \brief Holds details of the file to which the last successfully completed test is registered.

      If a check causes a crash, the recovery file may be used to provide a clue as to where this
      happened.
   */
  struct recovery_paths
  {
    std::filesystem::path recovery_file{};
    std::filesystem::path dump_file{};
  };

  /*! \brief Specifies whether tests are run as standard tests or in false postive/negative mode.

      \anchor test_mode_enum
   */
  enum class test_mode { standard, false_positive, false_negative };

  /*! \class
      \brief Logs test results.

      \anchor test_logger_primary
   */

  template<test_mode Mode>
  class test_logger;

  /*! \class
      \brief Marshals the logging of checks and failures.

      The sentinel class template serves several purposes.

      1. This RAII class ensures robust treatment in the presence of exceptions.

      2. sentinel is befriended by test_logger and effectively provides access to a limited
      component of the the private, mutating part of the test_logger interface. The point of
      this is that while test_logger is exposed to clients of the test framework clients
      should use it, directly, only very sparingly. This is encouraged by the extra level
      of indirection provided by the sentinels.

      3. sentinel caters for some of the complexity of the testing framework in
      a localized and simple manner. In particular, a given check may actually be
      composed of many sub-checks. For standard checks, we want to count this as a
      single 'top-level' check. For false-positive checks success is counted as a failure
      of one or more of the sub-checks. The sentinels deal with all of this smoothly
      in a way which clients of the framework can generally ignore.

      4. sentinel may be used to help coordinate output.
  */

  template<test_mode Mode>
  class [[nodiscard]] sentinel
  {
  public:
    sentinel(test_logger<Mode>& logger, std::string_view message)
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
      logger.push_message(message);
      logger.increment_depth();
    }

    ~sentinel()
    {
      auto& logger{get()};
      logger.decrement_depth();

      if(!logger.depth())
      {
        if(critical_failure_detected())
        {
          logger.end_message(test_logger<Mode>::critical::yes);
        }
        else
        {
          if(failure_detected()) logger.end_message(test_logger<Mode>::critical::no);

          auto fpMessageMaker{
            [&logger](){
              auto mess{indent("False Positive Failure:", logger.top_level_message(), tab)};
              end_block(mess, 3, indent(footer(), tab));

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

      logger.pop_message();
    }

    sentinel(const sentinel&)     = delete;
    sentinel(sentinel&&) noexcept = default;

    sentinel& operator=(const sentinel&)     = delete;
    sentinel& operator=(sentinel&&) noexcept = default;

    void log_performance_check()
    {
      get().log_performance_check();
    }

    void log_check()
    {
      get().log_check();
    }

    void log_failure(std::string_view message)
    {
      get().log_failure(message);
    }

    void log_performance_failure(std::string_view message)
    {
      get().log_performance_failure(message);
    }

    void log_critical_failure(std::string_view message)
    {
      get().log_critical_failure(message);
    }

    void log_caught_exception_message(std::string_view message)
    {
      get().log_caught_exception_message(message);
    }

    [[nodiscard]]
    bool critical_failure_detected() const noexcept
    {
      return get().critical_failures() != m_PriorCriticalFailures;
    }

    [[nodiscard]]
    bool failure_detected() const noexcept
    {
      return get().failures() != m_PriorFailures;
    }

    [[nodiscard]]
    bool checks_registered() const noexcept
    {
      return get().deep_checks() != m_PriorDeepChecks;
    }
  private:
    [[nodiscard]]
    test_logger<Mode>& get() noexcept { return *m_pLogger; }

    [[nodiscard]]
    const test_logger<Mode>& get() const noexcept { return *m_pLogger; }

    test_logger<Mode>* m_pLogger;
    std::string m_Message;
    std::size_t
      m_PriorFailures{},
      m_PriorCriticalFailures{},
      m_PriorDeepChecks{};
  };

  struct failure_info
  {
    std::size_t check_index;
    std::string message;
  };

  using failure_output = std::vector<failure_info>;

  [[nodiscard]]
  std::string to_string(const failure_output& output);

  template<test_mode Mode>
  class test_logger
  {
    friend class sentinel<Mode>;
  public:
    test_logger() = default;

    test_logger(const test_logger&)     = delete;
    test_logger(test_logger&&) noexcept = default;

    test_logger& operator=(const test_logger&)     = delete;
    test_logger& operator=(test_logger&&) noexcept = default;

    constexpr static test_mode mode{Mode};

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
    const failure_output& failure_messages() const noexcept{ return m_FailureMessages; }

    [[nodiscard]]
    const failure_output& diagnostics_output() const noexcept { return m_DiagnosticsOutput; }    

    [[nodiscard]]
    const failure_output& caught_exceptions_output() const noexcept { return m_CaughtExceptionMessages; }

    [[nodiscard]]
    const uncaught_exception_info& exceptions_detected_by_sentinel() const
    {
      return m_UncaughtExceptionInfo;
    }

    [[nodiscard]]
    std::string_view top_level_message() const noexcept
    {
      return !m_LevelMessages.empty() ? std::string_view{m_LevelMessages.front().message} : "";
    }

    [[nodiscard]]
    const recovery_paths& recovery() const noexcept { return m_Recovery; }

    void recovery(recovery_paths paths)
    {
      m_Recovery = std::move(paths);
    }
  private:
    struct level_message
    {
      level_message(std::string_view m)
        : message{m}
      {}

      std::string message;
      bool written{};
    };

    enum class critical{yes, no};
    enum class update_mode{fresh, app};

    failure_output
      m_FailureMessages,
      m_DiagnosticsOutput,
      m_CaughtExceptionMessages;

    std::vector<level_message> m_LevelMessages;

    uncaught_exception_info m_UncaughtExceptionInfo{};

    std::size_t
      m_Failures{},
      m_TopLevelFailures{},
      m_PerformanceFailures{},
      m_CriticalFailures{},
      m_TopLevelChecks{},
      m_Checks{},
      m_PerformanceChecks{},
      m_Depth{};

    recovery_paths m_Recovery{};

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

    void failure_message(std::string_view message, const critical isCritical)
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

      auto indenter{
        [](std::size_t depth) -> indentation {
          return indentation{tab}.append(2*depth, ' ');
        }
      };

      indentation ind{tab};
      std::size_t activeLevels{};
      for(auto& lm : m_LevelMessages)
      {
        if(lm.message.empty()) continue;

        if(activeLevels++ > 0) ind.append("  ");

        if(lm.written) continue;

        build(lm.message, ind);
        lm.written = true;
      }

      build(message, ind);

      update_output(msg, 1, "", isCritical, update_mode::fresh);
    }

    void log_failure(std::string_view message)
    {
      ++m_Failures;
      failure_message(message, critical::no);
    }

    void log_performance_failure(std::string_view message)
    {
      ++m_PerformanceFailures;
      log_failure(message);
    }

    void log_critical_failure(std::string_view message)
    {
      ++m_CriticalFailures;
      failure_message(message, critical::yes);
      impl::recored_critical_failure(m_Recovery.recovery_file, message);
    }

    void log_top_level_failure(std::string message)
    {
      ++m_TopLevelFailures;
      if(m_LevelMessages.empty())
      {
        m_LevelMessages.push_back(level_message{""});
      }
      else
      {
        m_LevelMessages.back().message.append(std::move(message));
      }
    }

    void log_caught_exception_message(std::string_view message)
    {
      append_to_output(m_CaughtExceptionMessages,
                       std::string{top_level_message()}.append("\n").append(message),
                       3,
                       footer(),
                       update_mode::fresh);
    }

    void append_to_diagnostics_output(std::string message)
    {
      m_DiagnosticsOutput.push_back(failure_info{m_TopLevelChecks, std::move(message)});
    }

    void push_message(std::string_view message)
    {
      m_LevelMessages.emplace_back(message);
    }

    void pop_message()
    {
      if(m_LevelMessages.empty())
        throw std::logic_error{"Cannot pop from TestLogger's empty stack"};

      if(!depth())
      {
        m_UncaughtExceptionInfo = {std::uncaught_exceptions(), std::move(m_LevelMessages.front().message)}; 
      }
      
      m_LevelMessages.pop_back();
    }

    void end_message(const critical isCritical)
    {
      update_output("", 2, footer(), isCritical, update_mode::app);
    }

    void append_to_output(failure_output& output,
                          std::string_view message,
                          std::size_t newLines,
                          std::string_view foot,
                          update_mode mode)
    {
      switch(mode)
      {
      case update_mode::fresh:
        output.push_back(failure_info{m_TopLevelChecks, std::string{message}});
        end_block(output.back().message, newLines, indent(foot, tab));
        break;
      case update_mode::app:
        if(!output.empty())
        {
          // This is really cryptic! Note that message is unused here!
          end_block(output.back().message, newLines, indent(foot, tab));
        }
      }
    }

    void update_output(std::string_view message,
                       std::size_t newLines,
                       std::string_view foot,
                       critical isCritical,
                       update_mode mode)
    {
      auto toMessages{
        []([[maybe_unused]] critical cr){
          if constexpr(Mode != test_mode::false_positive)
          {
            return true;
          }
          else
          {
            return cr == critical::yes;
          }
        }
      };

      if(toMessages(isCritical))
      {
        append_to_output(m_FailureMessages, message, newLines, foot, mode);
      }
      else
      {
        append_to_output(m_DiagnosticsOutput, message, newLines, foot, mode);
      }
    }

    void add_failure_message(failure_output& output, std::string message)
    {
      if(output.empty())
      {
        output.emplace_back(m_TopLevelChecks, std::move(message));
      }
      else
      {
        output.back().message.append(std::move(message));
      }
    }
  };

  /*! \brief Summaries data generated by the logger, for the purposes of reporting.

      Summaries may be combined by using either operator+= or operator+
   */

  class log_summary
  {
  public:
    using duration = std::chrono::steady_clock::duration;

    log_summary() = default;

    explicit log_summary(std::string_view name) : m_Name{name} {}

    template<test_mode Mode>
    log_summary(std::string_view name, const test_logger<Mode>& logger, const duration delta)
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

    void clear() noexcept;

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
    std::size_t soft_failures() const noexcept;

    [[nodiscard]]
    const std::string& diagnostics_output() const noexcept { return m_DiagnosticsOutput; }

    void diagnostics_output(std::string output)
    {
      m_DiagnosticsOutput = std::move(output);
    }

    [[nodiscard]]
    const std::string& caught_exceptions_output() const noexcept { return m_CaughtExceptionMessages; }

    [[nodiscard]]
    duration execution_time() const noexcept { return m_Duration; }

    log_summary& operator+=(const log_summary& rhs);

    [[nodiscard]]
    const std::string& name() const noexcept { return m_Name; }

    [[nodiscard]]
    const std::string& failure_messages() const noexcept { return m_FailureMessages; }

    friend log_summary operator+(const log_summary& lhs, const log_summary& rhs);
  private:
    std::string
      m_Name,
      m_FailureMessages,
      m_DiagnosticsOutput,
      m_CaughtExceptionMessages;

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
