////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/PlatformSpecific/Helpers.hpp"
#include "sequoia/PlatformSpecific/Macros.hpp"

#ifdef _WIN32
  #include "Windows.h"
#endif

#ifdef __APPLE__
  #include "sys/event.h"
  #include "unistd.h"

  #include <cerrno>
  #include <cstdint>
#endif

#include <thread>

#if defined(SEQUOIA_MSVC_DEBUG_RUNTIME)
  #include "crtdbg.h"

  #include <cstdio>
  #include <stdexcept>
  #include <string_view>
#endif

namespace sequoia
{
  #ifdef __APPLE__
    namespace
    {
      /** \brief RAII type holding a kqueue.

          A kqueue is a queue, kept by the kernel, of events for which a
          process has registered, such as a timer firing. A process holds a
          kqueue as a file descriptor.
       */
      class [[nodiscard]] kqueue_descriptor
      {
      public:
        kqueue_descriptor()
          : m_Descriptor{kqueue()}
        {}

        kqueue_descriptor(const kqueue_descriptor&)            = delete;
        kqueue_descriptor& operator=(const kqueue_descriptor&) = delete;

        ~kqueue_descriptor()
        {
          if(m_Descriptor >= 0)
            close(m_Descriptor);
        }

        /** \returns A negative value if creating the kqueue failed. */
        [[nodiscard]]
        int get() const noexcept { return m_Descriptor; }
      private:
        int m_Descriptor;
      };

      /** \brief Sleeps until `deadline` on a kqueue timer with
                 `NOTE_CRITICAL`.

          Each call to `kevent64` registers a timer with the kqueue, then waits
          for an event. `NOTE_CRITICAL` asks the kernel not to defer the timer
          to coalesce it with others. A signal can end a wait early. The
          function then waits again for the time still remaining.

          Each thread holds a kqueue of its own. Every timer has the same
          identifier, so threads sharing a kqueue would replace each other's
          timers.

          \returns false, possibly before `deadline`, if the kqueue fails.
       */
      [[nodiscard]]
      bool critical_timer_sleep_until(std::chrono::steady_clock::time_point deadline)
      {
        static thread_local const kqueue_descriptor queue{};
        if(queue.get() < 0)
          return false;

        for(auto now{std::chrono::steady_clock::now()}; now < deadline; now = std::chrono::steady_clock::now())
        {
          const auto remaining{std::chrono::ceil<std::chrono::nanoseconds>(deadline - now)};
          struct kevent64_s request{}, fired{};
          // Timer 1 fires once, `remaining` nanoseconds from now, and the
          // kernel then deletes it
          EV_SET64(&request, 1, EVFILT_TIMER, EV_ADD | EV_ONESHOT, NOTE_NSECONDS | NOTE_CRITICAL,
                   static_cast<std::int64_t>(remaining.count()), 0, 0, 0);

          const int events{kevent64(queue.get(), &request, 1, &fired, 1, 0, nullptr)};
          if((events < 0) && (errno != EINTR))
            return false;

          // The kernel reports a refused registration as an event flagged
          // EV_ERROR
          if((events == 1) && (fired.flags & EV_ERROR))
            return false;
        }

        return true;
      }
    }
  #endif

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

  void precise_sleep_for(std::chrono::nanoseconds duration)
  {
    if(duration <= std::chrono::nanoseconds::zero())
      return;

    #ifdef __APPLE__
      using clock = std::chrono::steady_clock;
      const auto now{clock::now()};
      const auto deadline{duration < clock::time_point::max() - now ? now + duration : clock::time_point::max()};
      if(critical_timer_sleep_until(deadline))
        return;

      if(const auto remaining{deadline - std::chrono::steady_clock::now()}; remaining > remaining.zero())
        std::this_thread::sleep_for(remaining);
    #else
      std::this_thread::sleep_for(duration);
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
