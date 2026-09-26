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
  /** \brief Requests a finer timer resolution from Windows for the lifetime of the object.

      Windows otherwise ends a sleep only on a tick of its default timer, about every 15.6 ms, so
      each sleep is rounded up to a whole number of ticks.
   */
  class [[nodiscard]] timer_resolution
  {
    unsigned int m_Resolution{};

    [[nodiscard]]
    static unsigned int resolution(std::chrono::milliseconds t) noexcept;
  public:
    /** Requests a resolution of `t`; requests nothing on other platforms, or when `t` is not
        positive.
     */
    explicit timer_resolution(std::chrono::milliseconds t);

    // The destructor ends the request, so a copy would end it a second time
    timer_resolution(const timer_resolution&)            = delete;
    timer_resolution& operator=(const timer_resolution&) = delete;

    ~timer_resolution();
  };
}
