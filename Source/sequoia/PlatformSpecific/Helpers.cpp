////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

module;

#include "sequoia/PlatformSpecific/Macros.hpp"

#ifdef _WIN32
  #include "Windows.h"
#endif

// crtdbg.h is not a standard-library header, and stderr is a macro, so import std supplies neither
#if defined(SEQUOIA_MSVC_DEBUG_RUNTIME)
  #include "crtdbg.h"

  #include <cstdio>
#endif

module sequoia.platform_specific;

import std;

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

  #ifdef _WIN32
    namespace
    {
      /** \brief Requests the finest timer period that Windows offers.

          \returns The period granted, or 0 if none was.
       */
      [[nodiscard]]
      unsigned int request_finest_timer_period() noexcept
      {
        TIMECAPS capabilities{};
        if(timeGetDevCaps(&capabilities, sizeof(capabilities)) != MMSYSERR_NOERROR)
          return 0;

        return timeBeginPeriod(capabilities.wPeriodMin) == TIMERR_NOERROR ? capabilities.wPeriodMin : 0;
      }

      /** \brief RAII type holding a request for the finest timer resolution that Windows offers. */
      class [[nodiscard]] finest_timer_resolution
      {
      public:
        finest_timer_resolution()
          : m_Period{request_finest_timer_period()}
        {}

        finest_timer_resolution(const finest_timer_resolution&)            = delete;
        finest_timer_resolution& operator=(const finest_timer_resolution&) = delete;

        ~finest_timer_resolution()
        {
          if(m_Period > 0)
            timeEndPeriod(m_Period);
        }
      private:
        unsigned int m_Period{};
      };
    }
  #endif

  void set_finest_windows_timer_resolution()
  {
    #ifdef _WIN32
      // Without a request for a timer resolution, Windows ends a sleep only on a tick of its default timer.
      // The timer ticks about every 15.6 ms, so each sleep is rounded up to a whole number of ticks.
      static const finest_timer_resolution resolution{};
    #endif
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
