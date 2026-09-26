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
  /** \brief RAII type holding a request to Windows for a timer resolution.

      Without a request, Windows ends a sleep only on a tick of its default timer, about every
      15.6 ms, so each sleep is rounded up to a whole number of ticks. A resolution which is not
      positive is not requested, and nothing is requested on other platforms.
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
