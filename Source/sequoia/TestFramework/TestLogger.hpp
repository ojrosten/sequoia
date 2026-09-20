////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
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
#include "sequoia/TestFramework/FailureInfo.hpp"
#include "sequoia/TestFramework/Output.hpp"
#include "sequoia/TestFramework/TestMode.hpp"

#include <chrono>
#include <exception>
#include <filesystem>
#include <memory>

namespace sequoia::testing
{
  /** \brief Holds paths to files where recovery information will be written if the path is not empty

   */
  struct active_recovery_files
  {
    std::filesystem::path recovery_file{};
    std::filesystem::path dump_file{};
  };

  /// Recovery information is written only at run time; these are called only behind a live pointer
  void record_check_started(const active_recovery_files& files, std::string_view message);
  void record_check_ended(const active_recovery_files& files);
  void record_dump_started(const active_recovery_files& files, std::string_view message);
  void record_dump_ended(const active_recovery_files& files);
  void record_critical_failure(const active_recovery_files& files, std::string_view message);

  struct test_results
  {
    failure_output
      failure_messages,
      diagnostics_output,
      caught_exception_messages;

    uncaught_exception_info exception_info{};

    std::size_t
      failures{},
      top_level_failures{},
      performance_failures{},
      critical_failures{},
      top_level_checks{},
      deep_checks{},
      performance_checks{};
  };

  /** \brief Helper class for safe interaction with test_logger.

      \anchor sentinel_base_primary
   */

  class sentinel_base;

  /** \brief Helper class for logging of results.

      \anchor test_logger_base_primary
   */

  class test_logger_base
  {
    friend sentinel_base;
  public:
    test_logger_base(const test_logger_base&)            = delete;
    test_logger_base& operator=(const test_logger_base&) = delete;

    [[nodiscard]]
    constexpr const test_results& results() const noexcept { return m_Results; }

    constexpr void reset_results() noexcept { m_Results = {}; }

    /// Null where no recovery information is written
    [[nodiscard]]
    constexpr const active_recovery_files* recovery() const noexcept { return m_Recovery.get(); }

    void recovery(active_recovery_files paths) { m_Recovery = std::make_unique<active_recovery_files>(std::move(paths)); }

    [[nodiscard]]
    constexpr std::string_view top_level_message() const noexcept
    {
      return !m_SentinelDepth.empty() ? std::string_view{m_SentinelDepth.front().message} : "";
    }

    [[nodiscard]]
    constexpr const uncaught_exception_info& exceptions_detected_by_sentinel() const noexcept
    {
      return m_Results.exception_info;
    }
  protected:
    constexpr test_logger_base() = default;

    explicit test_logger_base(active_recovery_files recovery)
      : m_Recovery{std::make_unique<active_recovery_files>(std::move(recovery))}
    {}

    constexpr test_logger_base(test_logger_base&&)            noexcept = default;
    constexpr test_logger_base& operator=(test_logger_base&&) noexcept = default;
  private:
    struct level_message
    {
      constexpr explicit level_message(std::string_view m)
        : message{m}
      {}

      std::string message;
      bool written{};
    };

    enum class is_critical{yes, no};

    test_results m_Results;
    std::vector<level_message> m_SentinelDepth;
    // Held indirectly: a path is not a literal type, and a logger must be creatable in a constant evaluation
    std::unique_ptr<active_recovery_files> m_Recovery{};

    [[nodiscard]]
    constexpr std::size_t depth() const noexcept { return m_SentinelDepth.size(); }

    constexpr void log_check() noexcept { ++m_Results.deep_checks; }

    constexpr void log_top_level_check() noexcept { ++m_Results.top_level_checks; }

    constexpr void log_performance_check() noexcept
    {
      log_check();
      ++m_Results.performance_checks;
    }

    constexpr void failure_message(test_mode mode, std::string_view message, const is_critical isCritical)
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

      auto& output{add_to_output(output_channel(mode, isCritical), msg)};
      end_block(output.back().message, 1_linebreaks, "");
    }

    constexpr void log_failure(test_mode mode, std::string_view message)
    {
      ++m_Results.failures;
      failure_message(mode, message, is_critical::no);
    }

    constexpr void log_performance_failure(test_mode mode, std::string_view message)
    {
      ++m_Results.performance_failures;
      log_failure(mode, message);
    }

    constexpr void log_critical_failure(test_mode mode, std::string_view message)
    {
      ++m_Results.critical_failures;
      failure_message(mode, message, is_critical::yes);
      if(m_Recovery)
        record_critical_failure(*m_Recovery, message);
    }

    constexpr void log_top_level_failure(test_mode mode, std::string message)
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

      if(mode == test_mode::false_negative)
      {
        m_Results.failure_messages.push_back(failure_info{m_Results.top_level_checks, std::string{message}});
      }
    }

    constexpr void log_caught_exception_message(std::string_view message)
    {
      auto mess{std::string{top_level_message()}.append("\n").append(message)};
      end_block(mess, 2_linebreaks, footer());

      add_to_output(m_Results.caught_exception_messages, mess);
    }

    constexpr void append_to_diagnostics_output(std::string message)
    {
      m_Results.diagnostics_output.push_back(failure_info{m_Results.top_level_checks, std::move(message)});
    }

    constexpr void increment_depth(std::string_view message)
    {
      m_SentinelDepth.emplace_back(message);
    }

    constexpr void decrement_depth()
    {
      if(m_SentinelDepth.empty())
        throw std::logic_error{"Cannot pop from TestLogger's empty stack"};

      if(depth() == 1)
      {
        m_Results.exception_info = {uncaught_exceptions(), std::move(m_SentinelDepth.front().message)};
      }

      m_SentinelDepth.pop_back();
    }

    constexpr void end_message(test_mode mode, const is_critical isCritical)
    {
      auto& output{output_channel(mode, isCritical)};
      auto& mess{output.back().message};
      end_block(mess, 2_linebreaks, footer());
    }

    [[nodiscard]]
    constexpr failure_output& output_channel(test_mode mode, const is_critical isCritical) noexcept
    {
      const bool toMessages{(mode != test_mode::false_negative) || (isCritical == is_critical::yes)};
      return toMessages ? m_Results.failure_messages : m_Results.diagnostics_output;
    }

    constexpr failure_output& add_to_output(failure_output& output, std::string_view message)
    {
      output.push_back(failure_info{m_Results.top_level_checks, std::string{message}});
      return output;
    }

    // No exception is in flight during a constant evaluation, and the query is not constexpr
    [[nodiscard]]
    constexpr static int uncaught_exceptions()
    {
      if consteval
      {
        return 0;
      }
      else
      {
        return std::uncaught_exceptions();
      }
    }
  };

  /** \brief Logs test results.

      \anchor test_logger_primary
   */

  template<test_mode Mode>
  class test_logger : public test_logger_base
  {
  public:
    constexpr static test_mode mode{Mode};

    constexpr test_logger() = default;

    explicit test_logger(active_recovery_files recoveryFiles)
      : test_logger_base{recoveryFiles}
    {}

    test_logger(const test_logger&)     = delete;
    test_logger(test_logger&&) noexcept = default;

    test_logger& operator=(const test_logger&)     = delete;
    test_logger& operator=(test_logger&&) noexcept = default;

  };


  class sentinel_base
  {
  public:
    sentinel_base(const sentinel_base&)            = delete;
    sentinel_base& operator=(const sentinel_base&) = delete;

    constexpr void log_performance_check() { get().log_performance_check(); }

    constexpr void log_check() { get().log_check(); }

    constexpr void log_failure(std::string_view message) { get().log_failure(m_Mode, message); }

    constexpr void log_performance_failure(std::string_view message) { get().log_performance_failure(m_Mode, message); }

    constexpr void log_critical_failure(std::string_view message) { get().log_critical_failure(m_Mode, message); }

    constexpr void log_caught_exception_message(std::string_view message) { get().log_caught_exception_message(message); }

    [[nodiscard]]
    constexpr bool critical_failure_detected() const noexcept { return get().results().critical_failures != m_PriorCriticalFailures; }

    [[nodiscard]]
    constexpr bool failure_detected() const noexcept { return get().results().failures != m_PriorFailures; }

    [[nodiscard]]
    constexpr bool checks_registered() const noexcept { return get().results().deep_checks != m_PriorDeepChecks; }
  protected:
    constexpr sentinel_base(test_logger_base& logger, test_mode mode, std::string message)
      : m_pLogger{&logger}
      , m_Mode{mode}
      , m_Message{std::move(message)}
      , m_PriorFailures{logger.results().failures}
      , m_PriorCriticalFailures{logger.results().critical_failures}
      , m_PriorDeepChecks{logger.results().deep_checks}
    {
      if(!logger.depth())
      {
        logger.log_top_level_check();
        if(const auto* files{get().recovery()})
          record_check_started(*files, m_Message);
      }

      if(const auto* files{get().recovery()})
        record_dump_started(*files, m_Message);

      logger.increment_depth(m_Message);
    }

    constexpr ~sentinel_base()
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

          auto fpMessageMaker{
            [&logger](){
              auto mess{append_lines("False Negative Failure:", logger.top_level_message())};
              end_block(mess, 2_linebreaks, footer());

              return mess;
            }
          };

          const bool modeSpecificFailure{
               ((m_Mode == test_mode::false_negative) && !failure_detected())
            || ((m_Mode != test_mode::false_negative) && failure_detected())
          };

          if(modeSpecificFailure)
          {
            logger.log_top_level_failure(m_Mode, (m_Mode == test_mode::false_negative) ? fpMessageMaker() : "");
          }
          else if (m_Mode == test_mode::false_positive)
          {
            if(!critical_failure_detected())
              logger.append_to_diagnostics_output(fpMessageMaker());
          }

          if(const auto* files{get().recovery()})
            record_check_ended(*files);
        }

        if(const auto* files{get().recovery()})
          record_dump_ended(*files);
      }

      logger.decrement_depth();
    }

    sentinel_base(sentinel_base&&)            noexcept = default;
    sentinel_base& operator=(sentinel_base&&) noexcept = default;
  private:
    [[nodiscard]]
    constexpr test_logger_base& get() noexcept { return *m_pLogger; }

    [[nodiscard]]
    constexpr const test_logger_base& get() const noexcept { return *m_pLogger; }

    test_logger_base* m_pLogger;
    test_mode m_Mode;
    std::string m_Message;
    std::size_t
      m_PriorFailures{},
      m_PriorCriticalFailures{},
      m_PriorDeepChecks{};
  };

  /** \brief Marshals the logging of checks and failures.

      The sentinel class template serves several purposes.

      1. This RAII class ensures robust treatment in the presence of exceptions.

      2. sentinel is befriended by test_logger_base and effectively provides access to a limited
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
  class [[nodiscard]] sentinel : public sentinel_base
  {
  public:
    constexpr static test_mode mode{Mode};

    constexpr sentinel(test_logger<Mode>& logger, std::string message)
      : sentinel_base{logger, Mode, std::move(message)}
    {}

    sentinel(const sentinel&)     = delete;
    sentinel(sentinel&&) noexcept = default;

    sentinel& operator=(const sentinel&)     = delete;
    sentinel& operator=(sentinel&&) noexcept = default;

  };

  /** \brief Summaries data generated by the logger, for the purposes of reporting.

      Summaries may be combined by using either operator+= or operator+
   */

  class log_summary
  {
  public:
    using duration = std::chrono::steady_clock::duration;

    // `{}` rather than `= default`, to stay identical to `modules-native`, where the
    // defaulted constructor of this class is emitted in no translation unit at all and
    // the link fails. See gcc-bugs/F in the sequoia-LLM repository; fixed upstream in
    // gcc 15.3. The two spellings differ in value-initialisation - `{}` is
    // user-provided, so members without initialisers would not be zero-initialised - but
    // every member below has one, so the object state is identical.
    log_summary() {}

    explicit log_summary(std::string_view name);

    template<test_mode Mode>
    log_summary(std::string_view name, const test_logger<Mode>& logger, const duration delta)
      : log_summary(name, logger, Mode, delta)
    {}

    void clear() noexcept;

    [[nodiscard]]
    std::size_t standard_top_level_checks() const noexcept { return m_StandardTopLevelChecks; }

    [[nodiscard]]
    std::size_t standard_deep_checks() const noexcept { return m_StandardDeepChecks; }

    [[nodiscard]]
    std::size_t standard_performance_checks() const noexcept { return m_StandardPerformanceChecks; }

    [[nodiscard]]
    std::size_t false_positive_checks() const noexcept { return m_FalsePositiveChecks; }

    [[nodiscard]]
    std::size_t false_positive_performance_checks() const noexcept { return m_FalsePositivePerformanceChecks; }

    [[nodiscard]]
    std::size_t false_negative_checks() const noexcept { return m_FalseNegativeChecks; }

    [[nodiscard]]
    std::size_t false_negative_performance_checks() const noexcept { return m_FalseNegativePerformanceChecks; }

    [[nodiscard]]
    std::size_t standard_deep_failures() const noexcept { return m_StandardDeepFailures; }

    [[nodiscard]]
    std::size_t standard_top_level_failures() const noexcept { return m_StandardTopLevelFailures; }

    [[nodiscard]]
    std::size_t standard_performance_failures() const noexcept { return m_StandardPerformanceFailures; }

    [[nodiscard]]
    std::size_t false_positive_failures() const noexcept { return m_FalsePositiveFailures; }

    [[nodiscard]]
    std::size_t false_positive_performance_failures() const noexcept { return m_FalsePositivePerformanceFailures; }

    [[nodiscard]]
    std::size_t false_negative_failures() const noexcept { return m_FalseNegativeFailures; }

    [[nodiscard]]
    std::size_t false_negative_performance_failures() const noexcept { return m_FalseNegativePerformanceFailures; }

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

    void execution_time(const duration delta) { m_Duration = delta; }

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
      m_FalsePositiveChecks{},
      m_FalseNegativeChecks{},
      m_FalsePositivePerformanceChecks{},
      m_FalseNegativePerformanceChecks{},
      m_StandardTopLevelFailures{},
      m_StandardDeepFailures{},
      m_StandardPerformanceFailures{},
      m_FalsePositiveFailures{},
      m_FalseNegativeFailures{},
      m_FalsePositivePerformanceFailures{},
      m_FalseNegativePerformanceFailures{},
      m_CriticalFailures{};

    int m_ExceptionsInFlight{};

    duration m_Duration{};

    log_summary(std::string_view name, const test_logger_base& logger, test_mode mode, const duration delta);
  };
}
