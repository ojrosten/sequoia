////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Utilities for summarizing results of tests.
*/

#include "sequoia/Core/Logic/Bitmask.hpp"
#include "sequoia/TestFramework/TestLogger.hpp"

namespace sequoia::testing
{
  /** bit mask for the level of detail */
  enum class summary_detail : unsigned { none=0, absent_checks=1, failure_messages=2, timings=4};
}

NAMESPACE_SEQUOIA_AS_BITMASK
{
  template<>
  struct as_bitmask<sequoia::testing::summary_detail> : std::true_type
  {};
}

namespace sequoia::testing
{
  template<std::forward_iterator Iter> void pad_right(Iter begin, Iter end, std::string_view suffix)
  {
    auto maxIter{std::ranges::max_element(begin, end, std::ranges::less{}, [](const std::string& s) { return s.size(); })};

    const auto maxChars{maxIter->size()};

    for(; begin != end; ++begin)
    {
      auto& s{*begin};
      s += std::string(maxChars - s.size(), ' ') += std::string{suffix};
    }
  }

  template<std::forward_iterator Iter> void pad_left(Iter begin, Iter end, const std::size_t minChars)
  {
    auto maxIter{std::ranges::max_element(begin, end, std::ranges::less{}, [](const std::string& s) { return s.size(); })};

    const auto maxChars{std::ranges::max(maxIter->size(), minChars)};

    for(; begin != end; ++begin)
    {
      auto& s{*begin};
      s = std::string(maxChars - s.size(), ' ') + s;
    }
  }

  /** \brief The execution duration and runner overhead of a test, or of
             several tests summed.
   */
  struct durations_of_tests
  {
    log_summary::duration
      execution_duration{},
      runner_overhead{};

    [[nodiscard]]
    log_summary::duration total() const noexcept { return execution_duration + runner_overhead; }

    [[nodiscard]]
    friend durations_of_tests operator+(const durations_of_tests& lhs, const durations_of_tests& rhs) noexcept
    {
      return {
        .execution_duration{lhs.execution_duration + rhs.execution_duration},
        .runner_overhead   {lhs.runner_overhead    + rhs.runner_overhead}
      };
    }
  };

  /** \brief The durations into which the critical path of an execution of
             tests divides the execution's wall clock.

      The critical path is a sequence of tests known to have run one after
      another. `spent_in_tests` sums its tests' durations, and
      `scheduling_overhead` is the rest of the wall clock, which the critical
      path spent outside its tests.
   */
  struct critical_path_durations
  {
    durations_of_tests    spent_in_tests{};
    log_summary::duration scheduling_overhead{};
  };

  /** \brief The durations reported for a run of tests.

      `total_run_time` also holds the work which follows the tests'
      execution, such as summarizing the tests.
   */
  struct durations_of_run
  {
    log_summary::duration   total_run_time{};
    critical_path_durations critical_path{};
  };

  struct stringified_duration
  {
    std::string time, unit;
  };

  [[nodiscard]]
  stringified_duration stringify_duration(const log_summary::duration& d);

  [[nodiscard]]
  std::string durations_report(const log_summary& log);

  [[nodiscard]]
  std::string durations_report(const durations_of_run& durationsOfRun);

  /** \brief Summarizes `log`, under a heading of its name and `namesuffix`.

      A log with no name has no heading.
   */
  [[nodiscard]]
  std::string summarize(const log_summary& log,
                        std::string_view namesuffix,
                        const summary_detail verbosity,
                        indentation ind_0,
                        indentation ind_1);
}
