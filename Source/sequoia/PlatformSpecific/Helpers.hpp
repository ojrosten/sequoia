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

namespace sequoia
{
  /** \brief Sets the timer resolution of the process to the finest that Windows offers, until the process exits.

      If Windows refuses, nothing changes. Later calls do nothing. On other platforms, the function does nothing.
   */
  void set_finest_windows_timer_resolution();

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
