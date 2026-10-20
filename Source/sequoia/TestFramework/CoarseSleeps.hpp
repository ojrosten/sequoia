////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Detection of sleeps rounded up to a coarse timer tick, and
           warnings about them.
 */

#include "sequoia/PlatformSpecific/TimerRequestOutcome.hpp"
#include "sequoia/TestFramework/FloatingPointDurations.hpp"

#include <string>

namespace sequoia::testing
{
  /** \brief The outcomes of the two requests a process makes for timers
             finer than the default.
   */
  struct outcomes_of_timer_requests
  {
    timer_request_outcome finest_timer_resolution{};
    timer_request_outcome precise_timer{};
  };

  /** \brief The observations a process can make of its requests for timers
             finer than the default.
   */
  enum class sleep_timing_evidence
  {
    finest_timer_resolution_request_failed,
    precise_timer_request_failed,
    finest_timer_resolution_request_granted,
    precise_timer_request_granted,
    finest_timer_resolution_request_not_observed,
    precise_timer_request_not_observed,
    no_timer_request_made
  };

  /** \brief Whether `slept`, compared to `target`, indicates sleeps rounded
             up to a coarse timer tick.
   */
  [[nodiscard]]
  bool is_coarse_sleep(double_precision_milliseconds slept, double_precision_milliseconds target);

  /** \brief Combines the outcomes of two requests for timers.

      \returns Whichever of `lhs` and `rhs` comes first in the order
               `failed`, `granted`, `not_observed`, `not_made`.
   */
  [[nodiscard]]
  timer_request_outcome combine_timer_request_outcomes(timer_request_outcome lhs, timer_request_outcome rhs);

  /** \brief Chooses what `outcomes` show about the timing of sleeps.

      An outcome takes precedence in the order `failed`, `granted`,
      `not_observed`. Where both requests have the same outcome, the request
      for the finest timer resolution takes precedence.
   */
  [[nodiscard]]
  sleep_timing_evidence choose_sleep_timing_evidence(outcomes_of_timer_requests outcomes);

  /** \brief A warning that sleeps of `target` lasted `slept` or more, so
             timings built on sleeps are unreliable.

      If `evidence` is of a failed request, the warning names the failure as
      the likely cause. Otherwise, the warning states what `evidence` shows,
      and names no cause.
   */
  [[nodiscard]]
  std::string coarse_sleep_message(double_precision_milliseconds slept,
                                   double_precision_milliseconds target,
                                   sleep_timing_evidence evidence);
}
