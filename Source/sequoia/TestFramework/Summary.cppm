////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

module;

#include "sequoia/PlatformSpecific/Macros.hpp"

export module sequoia.test_framework:Summary;

import std;

import :CoreInfrastructure;
import :FailureInfo;
import :Output;
import :TestLogger;
import :TestMode;
export import sequoia.core.logic;
export import sequoia.core.meta;
export import sequoia.platform_specific;
export import sequoia.text_processing;

/** \file
    \brief Utilities for summarizing results of tests.
*/

export namespace sequoia::testing
{
  /** bit mask for the level of detail */
  enum class summary_detail : unsigned { none=0, absent_checks=1, failure_messages=2, timings=4};
}

export NAMESPACE_SEQUOIA_AS_BITMASK
{
  template<>
  struct as_bitmask<sequoia::testing::summary_detail> : std::true_type
  {};
}

export namespace sequoia::testing
{
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

    [[nodiscard]]
    friend constexpr bool operator==(const critical_path_durations&, const critical_path_durations&) noexcept = default;
  };

  /** \brief The durations reported for a run of tests.

      `total_run_time` also holds the work which follows the tests'
      execution, such as summarizing the tests.
   */
  struct durations_of_run
  {
    log_summary::duration   total_run_time{};
    critical_path_durations critical_path{};

    [[nodiscard]]
    friend constexpr bool operator==(const durations_of_run&, const durations_of_run&) noexcept = default;
  };

  struct stringified_duration
  {
    std::string time, unit;
  };

  [[nodiscard]]
  stringified_duration stringify_duration(const log_summary::duration& d);

  [[nodiscard]]
  std::string durations_report(const durations_of_tests& durations);

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
