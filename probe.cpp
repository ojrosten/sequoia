// Overshoot probe for roadmap item 374: std::this_thread::sleep_for against
// a high-resolution waitable timer (Windows), at 100 us, 1 ms, 5 ms, 20 ms.
// Usage: probe <default|period> [load]
//   period: timeBeginPeriod(wPeriodMin) first, as set_finest_windows_timer_resolution does
//   load:   hardware_concurrency threads spin throughout
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <string_view>
#include <thread>
#include <vector>

#ifdef _WIN32
  #define NOMINMAX
  #include <Windows.h>
  #pragma comment(lib, "winmm.lib")
#else
  #include <sys/prctl.h>
  #include <sys/utsname.h>
#endif

using namespace std::chrono;
using clk = steady_clock;

namespace
{
  long g_extra_waits{}; // waits after the first, i.e. early wake-ups re-waited

#ifdef _WIN32
  HANDLE make_timer(DWORD flags)
  {
    return CreateWaitableTimerExW(nullptr, nullptr, flags, TIMER_ALL_ACCESS);
  }

  // As a Windows body of try_precise_sleep_until would: wait on the timer until
  // the deadline has passed by steady_clock
  bool timer_sleep_until(HANDLE h, clk::time_point deadline)
  {
    bool first{true};
    for(auto now{clk::now()}; now < deadline; now = clk::now())
    {
      if(!first) ++g_extra_waits;
      first = false;
      const auto remaining{ceil<duration<long long, std::ratio<1, 10'000'000>>>(deadline - now)};
      LARGE_INTEGER due{};
      due.QuadPart = -remaining.count();
      if(!SetWaitableTimer(h, &due, 0, nullptr, nullptr, FALSE)) return false;
      if(WaitForSingleObject(h, INFINITE) != WAIT_OBJECT_0) return false;
    }
    return true;
  }

  void print_timer_resolution(const char* when)
  {
    using fn = LONG (NTAPI*)(PULONG, PULONG, PULONG);
    if(auto f{reinterpret_cast<fn>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtQueryTimerResolution"))})
    {
      ULONG coarsest{}, finest{}, current{};
      f(&coarsest, &finest, &current);
      std::printf("NtQueryTimerResolution %s: coarsest %.4f ms, finest %.4f ms, current %.4f ms\n",
                  when, coarsest / 1e4, finest / 1e4, current / 1e4);
    }
  }
#endif

  struct series
  {
    const char* label;
    std::vector<double> over{};
  };

  void report(const series& s, nanoseconds d)
  {
    auto v{s.over};
    std::ranges::sort(v);
    std::printf("%-10s %6lld us: n %4zu  min %9.1f  median %9.1f  p95 %9.1f  max %9.1f us over\n",
                s.label, static_cast<long long>(duration_cast<microseconds>(d).count()), v.size(),
                v.front(), v[v.size() / 2], v[v.size() * 95 / 100], v.back());
  }
}

int main(int argc, char** argv)
{
  const std::string_view mode{argc > 1 ? argv[1] : "default"};
  const bool load{argc > 2 && std::string_view{argv[2]} == "load"};

#ifdef _WIN32
  OSVERSIONINFOW os{sizeof(os)};
  using rtl = LONG (WINAPI*)(OSVERSIONINFOW*);
  if(auto f{reinterpret_cast<rtl>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlGetVersion"))}) f(&os);
  LARGE_INTEGER qpf{};
  QueryPerformanceFrequency(&qpf);
  std::printf("Windows %lu.%lu.%lu, _MSC_FULL_VER %d, _MSVC_STL_UPDATE %ld, QPC %lld Hz, cores %u\n",
              os.dwMajorVersion, os.dwMinorVersion, os.dwBuildNumber, _MSC_FULL_VER, static_cast<long>(_MSVC_STL_UPDATE),
              qpf.QuadPart, std::thread::hardware_concurrency());
  TIMECAPS caps{};
  timeGetDevCaps(&caps, sizeof(caps));
  std::printf("timeGetDevCaps: wPeriodMin %u ms, wPeriodMax %u ms\n", caps.wPeriodMin, caps.wPeriodMax);
  print_timer_resolution("at start");
  if(mode == "period")
    std::printf("timeBeginPeriod(%u) -> %u (0 is TIMERR_NOERROR)\n", caps.wPeriodMin, timeBeginPeriod(caps.wPeriodMin));
  print_timer_resolution("after request");

  const HANDLE hires{make_timer(CREATE_WAITABLE_TIMER_HIGH_RESOLUTION)};
  std::printf("high-resolution timer: %s (GetLastError %lu)\n", hires ? "created" : "FAILED", hires ? 0ul : GetLastError());
  const HANDLE plain{make_timer(0)};
  if(!hires || !plain) return 1;
#else
  utsname u{};
  uname(&u);
  std::printf("%s %s, cores %u, timer slack %d ns\n", u.sysname, u.release, std::thread::hardware_concurrency(),
              prctl(PR_GET_TIMERSLACK, 0, 0, 0, 0));
#endif
  std::printf("mode %.*s%s\n", static_cast<int>(mode.size()), mode.data(), load ? " + load" : "");

  std::atomic<bool> stop{};
  std::vector<std::thread> spinners;
  if(load)
    for(unsigned i{}; i < std::thread::hardware_concurrency(); ++i)
      spinners.emplace_back([&stop]{ while(!stop.load(std::memory_order_relaxed)) {} });

  constexpr int samples{300};
  for(nanoseconds d : {nanoseconds{microseconds{100}}, nanoseconds{milliseconds{1}}, nanoseconds{milliseconds{5}}, nanoseconds{milliseconds{20}}})
  {
    series sleep_for_s{"sleep_for"};
#ifdef _WIN32
    series hires_s{"hires"}, plain_s{"waitable"};
    g_extra_waits = 0;
    long hires_extra{};
#endif
    // Interleaved, so drift in the machine's state affects each method alike
    for(int i{}; i < samples; ++i)
    {
      auto t0{clk::now()};
      std::this_thread::sleep_for(d);
      sleep_for_s.over.push_back(duration<double, std::micro>(clk::now() - t0 - d).count());
#ifdef _WIN32
      t0 = clk::now();
      g_extra_waits = 0;
      if(!timer_sleep_until(hires, t0 + d)) { std::printf("hires wait FAILED\n"); return 1; }
      hires_s.over.push_back(duration<double, std::micro>(clk::now() - t0 - d).count());
      hires_extra += g_extra_waits;

      t0 = clk::now();
      if(!timer_sleep_until(plain, t0 + d)) { std::printf("plain wait FAILED\n"); return 1; }
      plain_s.over.push_back(duration<double, std::micro>(clk::now() - t0 - d).count());
#endif
    }
    report(sleep_for_s, d);
#ifdef _WIN32
    report(hires_s, d);
    report(plain_s, d);
    std::printf("           hires early wake-ups re-waited: %ld of %d\n", hires_extra, samples);
#endif
  }

  stop = true;
  for(auto& t : spinners) t.join();
}
