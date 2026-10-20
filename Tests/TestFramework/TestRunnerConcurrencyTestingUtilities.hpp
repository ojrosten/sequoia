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

#include <array>
#include <chrono>
#include <cstddef>
#include <span>
#include <string>

namespace sequoia::testing
{
  /** When a slow test's sleep began and ended. */
  struct execution_interval
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
  std::size_t peak_overlap(std::span<const execution_interval> intervals);

  /** Returns the line which reports `peak`. CI's step reporting the
      performance test's timings parses this line from each of the
      performance test's runs, and prints no peak overlap if it cannot.
   */
  [[nodiscard]]
  std::string peak_overlap_line(std::size_t peak);

  /** The number of slow tests. Each sleeps for the same duration, so that a
      run of them shows how the runner spreads tests over threads, and how
      many it runs at once.

      Each slow test is a class of its own, since the runner names a test
      after its class. A test may not be a class template, and the runner
      refuses two registrations of one class.
   */
  inline constexpr std::size_t slow_test_count{8};

  class slow_test_base : public free_test
  {
  public:
    using free_test::free_test;

    [[nodiscard]]
    static std::filesystem::path source_file();

    /** Returns each slow test's `execution_interval`, at the index the
        test passes to `sleep_and_check`.
     */
    [[nodiscard]]
    static std::span<const execution_interval, slow_test_count> execution_intervals() noexcept;

    static void clear_execution_intervals() noexcept;

    static void set_sleep_duration(std::chrono::milliseconds duration) noexcept;
  protected:
    ~slow_test_base() = default;

    slow_test_base(slow_test_base&&)            noexcept = default;
    slow_test_base& operator=(slow_test_base&&) noexcept = default;

    /** Sleeps for the duration last passed to `set_sleep_duration`,
        records the sleep's `execution_interval` at `index`, and makes one
        check.
     */
    void sleep_and_check(std::size_t index);
  private:
    // Each slow test writes only its own element. A runner's execute waits
    // for every test, so the elements may be read once it returns.
    static std::array<execution_interval, slow_test_count> st_ExecutionIntervals;

    // Set before a run begins, and read only by its tests
    static std::chrono::milliseconds st_SleepDuration;
  };

  /** Clears the slow tests' execution intervals, and returns a runner made
      from `args` with the slow tests registered, each to sleep for
      `sleepDuration`.

      Every runner returned shares the slow tests' execution intervals and
      sleep duration. So callers must not let two of these runners execute
      at once.
   */
  [[nodiscard]]
  test_runner make_slow_suite(commandline_arguments args,
                              std::stringstream& outputStream,
                              std::chrono::milliseconds sleepDuration);

  /** The duration for which each slow test sleeps in a run whose printed
      output and peak overlap are checked. The shorter the sleep, the more
      often a test which starts late fails to overlap the others.
   */
  inline constexpr std::chrono::milliseconds sleep_duration_for_overlap_checks{10};
}
