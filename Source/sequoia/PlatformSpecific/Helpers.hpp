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

  /** \brief An RAII wrapper to redirect the assertion and error reports of MSVC's debug runtime to standard error.

      The runtime would otherwise show a dialog, which blocks an unattended run until someone dismisses it. A
      redirected report ends the process.

      A report made while a debugger is attached is left to the runtime to handle.
      Under any other runtime, nothing changes.

      \throws std::runtime_error if the redirection cannot be installed
   */
  class [[nodiscard]] debug_report_redirector
  {
  public:
    debug_report_redirector();

    debug_report_redirector(const debug_report_redirector&)            = delete;
    debug_report_redirector& operator=(const debug_report_redirector&) = delete;

    ~debug_report_redirector();
  };

  /** \brief An RAII wrapper to ensure a crash reaches Windows Error Reporting.

      A parent process can stop a crash reaching it, as the GitHub Actions runner does for every process of a job,
      and the crash then leaves no dump. The wrapper clears `SEM_NOGPFAULTERRORBOX` from the process's error mode,
      which child processes inherit, and restores the replaced mode on destruction. Instances may be nested. Away
      from Windows, nothing changes.
   */
  class [[nodiscard]] windows_crash_report_enabler
  {
  public:
    windows_crash_report_enabler();

    windows_crash_report_enabler(const windows_crash_report_enabler&)            = delete;
    windows_crash_report_enabler& operator=(const windows_crash_report_enabler&) = delete;

    ~windows_crash_report_enabler();
  private:
    [[maybe_unused]] unsigned int m_Replaced;
  };
}
