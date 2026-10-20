////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include "TestRunnerTestingUtilities.hpp"

#include "sequoia/TestFramework/FreeTestCore.hpp"

#include <chrono>
#include <cstddef>
#include <span>
#include <string>

namespace sequoia::testing
{
  /** When a slow test's sleep began and ended. */
  struct sleep_interval
  {
    std::chrono::steady_clock::time_point start{};
    std::chrono::steady_clock::time_point end{};
  };

  /** The largest number of `intervals` which overlap at any instant. An
      interval contains its start but not its end, so one which ends as
      another starts does not overlap it, and an empty interval overlaps
      nothing.
   */
  [[nodiscard]]
  std::size_t peak_overlap(std::span<const sleep_interval> intervals);

  /** Returns the line which reports `peak`. CI's step "Report the
      performance test's timings" parses this line from each of the
      performance test's runs, and prints no peak overlap if it cannot.
   */
  [[nodiscard]]
  std::string peak_overlap_line(std::size_t peak);

  /** The number of slow tests. Each sleeps for the same duration, so that a
      run of them shows how the runner spreads tests over threads, and how
      many it runs at once.
   */
  inline constexpr std::size_t slow_test_count{8};

  /** Returns each slow test's `sleep_interval` in the latest run of the
      slow tests, at the slow test's index.
   */
  [[nodiscard]]
  std::span<const sleep_interval, slow_test_count> slow_test_sleep_intervals() noexcept;

  namespace impl
  {
    [[nodiscard]]
    test_runner make_slow_test_runner(commandline_arguments& args,
                                      std::stringstream& outputStream,
                                      std::chrono::milliseconds sleepDuration);
  }

  /** Clears the slow tests' sleep intervals, and returns a runner made from
      `args` with the slow tests registered, each to sleep for
      `sleepDuration`.

      Every runner returned shares one sleep duration and one set of sleep
      intervals with the others. A caller's window runs from this call to
      its last read of `slow_test_sleep_intervals`, and no two callers'
      windows may overlap. The constraint on `CallingTest`, the test making
      the call, ensures this: tests which are not parallelizable execute
      one at a time.
   */
  template<concrete_test CallingTest>
    requires (!is_parallelizable_v<CallingTest>)
  [[nodiscard]]
  test_runner make_slow_test_runner(commandline_arguments args,
                                    std::stringstream& outputStream,
                                    std::chrono::milliseconds sleepDuration)
  {
    return impl::make_slow_test_runner(args, outputStream, sleepDuration);
  }

  /** The sleep of each slow test in a run whose peak overlap is checked. */
  inline constexpr std::chrono::milliseconds sleep_duration_for_overlap_checks{10};
}
