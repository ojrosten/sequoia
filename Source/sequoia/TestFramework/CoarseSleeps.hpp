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

#include "sequoia/PlatformSpecific/TimerRequests.hpp"
#include "sequoia/TestFramework/FloatingPointDurations.hpp"

#include <string>

namespace sequoia::testing
{
  /** \brief Whether `slept`, compared to `target`, indicates sleeps rounded
             up to a coarse timer tick.
   */
  [[nodiscard]]
  bool is_coarse_sleep(double_precision_milliseconds slept, double_precision_milliseconds target);

  /** \brief Combines the outcomes of two requests for more precise sleeps.

      \returns Whichever of `lhs` and `rhs` comes first in the order
               `failed`, `granted`, `not_observed`, `not_made`.
   */
  [[nodiscard]]
  timer_request_outcome combine_timer_request_outcomes(timer_request_outcome lhs, timer_request_outcome rhs);

  /** \brief A warning that sleeps of `target` lasted `slept` or more, so
             timings built on sleeps are unreliable.

      If `outcome` is `failed`, the warning names the failure of `request` as
      the likely cause. Otherwise, the warning states the outcome of
      `request`, and names no cause.

      \throws std::logic_error if `request` is `timer_request::none` and
              `outcome` is not `timer_request_outcome::not_made`.
   */
  [[nodiscard]]
  std::string coarse_sleep_message(double_precision_milliseconds slept,
                                   double_precision_milliseconds target,
                                   timer_request request,
                                   timer_request_outcome outcome);
}
