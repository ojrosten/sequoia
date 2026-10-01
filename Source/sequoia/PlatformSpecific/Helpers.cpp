////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/PlatformSpecific/Helpers.hpp"
#include "sequoia/PlatformSpecific/Macros.hpp"

#include "sequoia/Maths/Arithmetic/ArithmeticCasts.hpp"

#ifdef _WIN32
  #include "Windows.h"
#endif

#if defined(SEQUOIA_MSVC_DEBUG_RUNTIME)
  #include "crtdbg.h"

  #include <cstdio>
  #include <stdexcept>
  #include <string_view>
#endif

namespace sequoia
{
  #if defined(SEQUOIA_MSVC_DEBUG_RUNTIME)
    namespace
    {
      /** Ends the process with a fail-fast, as Release's abort does, so that Windows Error Reporting can leave a
          dump.
       */
      [[noreturn]]
      void fail_fast()
      {
        __fastfail(FAST_FAIL_FATAL_APP_EXIT);
      }

      int report_to_stderr(int type, char* message, int*)
      {
        // Returning false leaves the report to the runtime, which delivers it to an attached debugger
        if((type == _CRT_WARN) || IsDebuggerPresent())
          return false;

        std::string_view text{message};
        std::fputs(message, stderr);
        if(!text.ends_with('\n'))
          std::fputc('\n', stderr);

        std::fflush(stderr);
        fail_fast();
      }
    }
  #endif

  namespace
  {
    #ifdef _WIN32
      [[nodiscard]]
      unsigned int request(unsigned int resolution) noexcept
      {
        return timeBeginPeriod(resolution) == TIMERR_NOERROR ? resolution : 0;
      }

      void end_request(unsigned int resolution) noexcept
      {
        if(resolution > 0) timeEndPeriod(resolution);
      }
    #else
      [[nodiscard]]
      unsigned int request(unsigned int) noexcept { return 0; }

      void end_request(unsigned int) noexcept {}
    #endif
  }

  timer_resolution::timer_resolution(std::chrono::milliseconds t)
    : m_Resolution{request(maths::checked_conversion_to<unsigned int>(t.count()))}
  {}

  timer_resolution::~timer_resolution()
  {
    end_request(m_Resolution);
  }

  void hold_timer_resolution_of_one_millisecond()
  {
    // Without a request for a timer resolution, Windows ends a sleep only on a tick of its default timer.
    // The timer ticks about every 15.6 ms, so each sleep is rounded up to a whole number of ticks.
    static const timer_resolution resolution{std::chrono::milliseconds{1}};
  }

  debug_report_redirector::debug_report_redirector()
  {
    #if defined(SEQUOIA_MSVC_DEBUG_RUNTIME)
      if(_CrtSetReportHook2(_CRT_RPTHOOK_INSTALL, report_to_stderr) == -1)
        throw std::runtime_error{"Unable to install the debug runtime's report hook"};
    #endif
  }

  debug_report_redirector::~debug_report_redirector()
  {
    #if defined(SEQUOIA_MSVC_DEBUG_RUNTIME)
      _CrtSetReportHook2(_CRT_RPTHOOK_REMOVE, report_to_stderr);
    #endif
  }

  namespace
  {
    /// Returns the replaced error mode
    [[nodiscard]]
    unsigned int let_crashes_reach_error_reporting()
    {
      #ifdef _WIN32
        const auto replaced{GetErrorMode()};
        SetErrorMode(replaced & ~SEM_NOGPFAULTERRORBOX);
        return replaced;
      #else
        return 0;
      #endif
    }
  }

  windows_crash_report_enabler::windows_crash_report_enabler()
    : m_Replaced{let_crashes_reach_error_reporting()}
  {}

  windows_crash_report_enabler::~windows_crash_report_enabler()
  {
    #ifdef _WIN32
      SetErrorMode(m_Replaced);
    #endif
  }
}
