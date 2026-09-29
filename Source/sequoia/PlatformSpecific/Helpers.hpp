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
  class [[nodiscard]] timer_resolution
  {
    unsigned int m_Resolution{};

    [[nodiscard]]
    static unsigned int resolution(std::chrono::milliseconds t) noexcept;
  public:
    timer_resolution() = default;

    explicit timer_resolution(std::chrono::milliseconds t);

    ~timer_resolution();
  };

  /** \brief An RAII wrapper to redirect the assertion and error reports of MSVC's debug runtime to standard error.

      The runtime would otherwise show a dialog, which blocks an unattended run until someone dismisses it. A
      redirected report ends the process.

      A report made while a debugger is attached is left to the runtime to handle.
      Under any other runtime, nothing changes.

      \throws std::runtime_error if the redirection cannot be installed
   */
  class [[nodiscard]] debug_report_redirection
  {
  public:
    debug_report_redirection();

    debug_report_redirection(const debug_report_redirection&)            = delete;
    debug_report_redirection& operator=(const debug_report_redirection&) = delete;

    ~debug_report_redirection();
  };
}
