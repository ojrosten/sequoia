////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Requests for more precise sleeps, and their outcomes.
 */

namespace sequoia
{
  /** \brief The request a platform makes for more precise sleeps, if any. */
  enum class timer_request { none, finest_timer_resolution, precise_timer };

  /** \brief The outcome of a request for more precise sleeps. */
  enum class timer_request_outcome { not_made, not_observed, failed, granted };
}
