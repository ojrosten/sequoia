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

#include <optional>

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

  /** \brief The durations into which the critical path of an execution of
             tests divides the execution's wall clock.

      The critical path is the longest sequence of tests which ran one after
      another. A test's length is its execution duration plus its runner
      overhead. `scheduling_overhead` is the rest of the wall clock, which
      the critical path spent outside its tests.
   */
  struct critical_path_durations
  {
    log_summary::duration
      execution_duration{},
      runner_overhead{},
      scheduling_overhead{};
  };

  /** \brief The durations reported for a run of tests. */
  struct durations_of_run
  {
    log_summary::duration   total{};
    critical_path_durations critical_path{};
  };

  using opt_durations_of_run = std::optional<durations_of_run>;

  struct stringified_duration
  {
    std::string time, unit;
  };

  [[nodiscard]]
  stringified_duration stringify_duration(const log_summary::duration& d);

  /** \brief Reports the run's durations if `durationsOfRun` holds them, and
             otherwise those of `log`.
   */
  [[nodiscard]]
  std::string report_time(const log_summary& log, const opt_durations_of_run& durationsOfRun);

  [[nodiscard]]
  std::string summarize(const log_summary& log, std::string_view namesuffix, const opt_durations_of_run& durationsOfRun, const summary_detail verbosity, indentation ind_0, indentation ind_1);

  [[nodiscard]]
  std::string summarize(const log_summary& log, std::string_view namesuffix, summary_detail verbosity, indentation ind_0, indentation ind_1);
}
