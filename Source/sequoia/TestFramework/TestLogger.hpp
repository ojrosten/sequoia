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
    sentinel(test_logger<Mode>& logger, std::string_view message);

    ~sentinel();

    sentinel(const sentinel&)     = delete;
    sentinel(sentinel&&) noexcept = default;

    sentinel& operator=(const sentinel&)     = delete;
    sentinel& operator=(sentinel&&) noexcept = default;

    void log_performance_check();

    void log_check();

    void log_failure(std::string_view message);

    void log_performance_failure(std::string_view message);

    void log_critical_failure(std::string_view message);

    void log_caught_exception_message(std::string_view message);

    [[nodiscard]]
    bool critical_failure_detected() const noexcept;

    [[nodiscard]]
    bool failure_detected() const noexcept;

    [[nodiscard]]
    bool checks_registered() const noexcept;
  private:
    [[nodiscard]]
    test_logger<Mode>& get() noexcept;

    [[nodiscard]]
    const test_logger<Mode>& get() const noexcept;

    test_logger<Mode>* m_pLogger;
    std::string m_Message;
    std::size_t
      m_PriorFailures{},
      m_PriorCriticalFailures{},
      m_PriorDeepChecks{};
  };
  
  extern template class sentinel<test_mode::standard>;
  extern template class sentinel<test_mode::false_positive>;
  extern template class sentinel<test_mode::false_negative>;

  struct failure_info
  {
    std::size_t check_index;
    std::string message;
  };

  using failure_output = std::vector<failure_info>;

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
    std::size_t failures() const noexcept;

    [[nodiscard]]
    std::size_t top_level_failures() const noexcept;

    [[nodiscard]]
    std::size_t performance_failures() const noexcept;

    [[nodiscard]]
    std::size_t critical_failures() const noexcept;

    [[nodiscard]]
    std::size_t top_level_checks() const noexcept;

    [[nodiscard]]
    std::size_t deep_checks() const noexcept;

    [[nodiscard]]
    std::size_t performance_checks() const noexcept;

    [[nodiscard]]
    const failure_output& failure_messages() const noexcept ;

    [[nodiscard]]
    const failure_output& diagnostics_output() const noexcept ;    

    [[nodiscard]]
    const failure_output& caught_exceptions_output() const noexcept ;

    [[nodiscard]]
    const uncaught_exception_info& exceptions_detected_by_sentinel() const;

    [[nodiscard]]
    std::string_view top_level_message() const noexcept;

    [[nodiscard]]
    const recovery_paths& recovery() const noexcept;

    void recovery(recovery_paths paths);
  private:
    struct level_message
    {
      level_message(std::string_view m)
        : message{m}
      {}

      std::string message;
      bool written{};
    };

    enum class is_critical{yes, no};
    enum class update_mode{fresh, app};

    failure_output
      m_FailureMessages,
      m_DiagnosticsOutput,
      m_CaughtExceptionMessages;

    std::vector<level_message> m_SentinelDepth;

    uncaught_exception_info m_UncaughtExceptionInfo{};

    std::size_t
      m_Failures{},
      m_TopLevelFailures{},
      m_PerformanceFailures{},
      m_CriticalFailures{},
      m_TopLevelChecks{},
      m_Checks{},
      m_PerformanceChecks{};

    recovery_paths m_Recovery{};

    [[nodiscard]]
    std::size_t depth() const noexcept;

    void log_check() noexcept;

    void log_top_level_check() noexcept;

    void log_performance_check() noexcept;

    void failure_message(std::string_view message, is_critical isCritical);
    
    void log_failure(std::string_view message);

    void log_performance_failure(std::string_view message);

    void log_critical_failure(std::string_view message);

    void log_top_level_failure(std::string message);

    void log_caught_exception_message(std::string_view message);

    void append_to_diagnostics_output(std::string message);

    void increment_depth(std::string_view message);

    void decrement_depth();

    void end_message(is_critical isCritical);

    [[nodiscard]]
    failure_output& output_channel(is_critical isCritical) noexcept;

    failure_output& update_output(std::string_view message, is_critical isCritical, update_mode uMode);

    failure_output& add_to_output(failure_output& output, std::string_view message, update_mode uMode);
  };

  extern template class test_logger<test_mode::standard>;
  extern template class test_logger<test_mode::false_positive>;
  extern template class test_logger<test_mode::false_negative>;

  /*! \brief Summaries data generated by the logger, for the purposes of reporting.

      Summaries may be combined by using either operator+= or operator+
   */

  class log_summary
  {
  public:
    using duration = std::chrono::steady_clock::duration;

    log_summary() = default;

    explicit log_summary(std::string_view name);

    template<test_mode Mode>
    log_summary(std::string_view name, const test_logger<Mode>& logger, const duration delta);

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
