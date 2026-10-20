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
  /** \brief Whether a request for a timer finer than the default was made,
             and if so, whether it was granted.
   */
  enum class timer_request_outcome { not_made, failed, granted };
}
