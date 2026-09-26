////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Platform-dependent utilities
 */

#include <chrono>

namespace sequoia
{
  /** \brief Requests a timer resolution of `t` from Windows for the lifetime of the object; a no-op
             on other platforms, and when `t` is not positive.

      Windows otherwise ends a sleep only on a tick of its default timer, about every 15.6 ms, so
      each sleep is rounded up to a whole number of ticks. The request ends on destruction, so
      the object is not copyable: a copy would end the request twice.
   */
  class [[nodiscard]] timer_resolution
  {
    unsigned int m_Resolution{};

    [[nodiscard]]
    static unsigned int resolution(std::chrono::milliseconds t) noexcept;
  public:
    explicit timer_resolution(std::chrono::milliseconds t);

    timer_resolution(const timer_resolution&)            = delete;
    timer_resolution& operator=(const timer_resolution&) = delete;

    ~timer_resolution();
  };
}
