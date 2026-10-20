////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief The outcome of a request for a timer finer than the default.
 */

namespace sequoia
{
  /** \brief The outcome of a request for a timer finer than the default, as
             observed by the function making the request.

      `not_made` means that the function observed neither a failure nor a
      grant: either it made no request, or it could not observe the outcome.
   */
  enum class timer_request_outcome { not_made, failed, granted };
}
