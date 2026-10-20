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

#include "sequoia/PlatformSpecific/TimerRequestOutcome.hpp"

#include <chrono>

namespace sequoia
{
  /** \brief Sleeps for at least `duration`, as measured by
             `std::chrono::steady_clock`, and returns the outcome of the
             sleep's requests for a precise timer.

      On macOS, the kernel may defer the wake-up from a sleep, to coalesce
      the wake-up with others and so save power. So a
      `std::this_thread::sleep_for` may overshoot by a quarter of its length
      or more. On macOS, this function requests a precise timer: one which
      the kernel is asked not to defer. If a request fails, the rest of the
      sleep is a `std::this_thread::sleep_for`. Elsewhere the whole sleep is
      a `std::this_thread::sleep_for`.

      \returns
      -# `timer_request_outcome::failed` if a request failed;
      -# `timer_request_outcome::granted` if a request was granted, and none
         failed;
      -# `timer_request_outcome::not_made` otherwise, as away from macOS.
   */
  timer_request_outcome precise_sleep_for(std::chrono::nanoseconds duration);

  /** \brief Sets the timer resolution of the process to the finest that Windows offers, until the process exits.

      If Windows refuses, nothing changes. Later calls do nothing. On other platforms, the function does nothing.

      \returns The outcome of the first call's request; on other platforms,
               `timer_request_outcome::not_made`.
   */
  [[nodiscard]]
  timer_request_outcome set_finest_windows_timer_resolution();

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
