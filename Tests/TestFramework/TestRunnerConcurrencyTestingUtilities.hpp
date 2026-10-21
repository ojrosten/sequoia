////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include "TestRunnerTestingUtilities.hpp"

#include "sequoia/PlatformSpecific/Helpers.hpp"
#include "sequoia/TestFramework/FreeTestCore.hpp"

#include <chrono>
#include <concepts>
#include <cstddef>
#include <ostream>
#include <source_location>
#include <span>
#include <vector>

namespace sequoia::testing
{
  /** The interval of time over which a slow test sleeps. The interval
      contains its start time but not its end time.
   */
  class sleep_interval
  {
  public:
    using time_point = std::chrono::steady_clock::time_point;

    sleep_interval() = default;

    /** \throws std::invalid_argument if `end` is earlier than `start` */
    sleep_interval(time_point start, time_point end);

    [[nodiscard]]
    time_point start_time() const noexcept { return m_Start; }

    [[nodiscard]]
    time_point end_time() const noexcept { return m_End; }

    [[nodiscard]]
    friend bool operator==(const sleep_interval&, const sleep_interval&) noexcept = default;
  private:
    time_point m_Start{};
    time_point m_End{};
  };

  /** Returns the largest number of `intervals` which contain the same
      instant.
   */
  [[nodiscard]]
  std::size_t peak_overlap(std::span<const sleep_interval> intervals);

  /** Appends `[Peak Overlap: <peak>]` and a newline to `stream`. CI's step
      "Report the performance test's timings" parses this text, and prints
      no peak overlap if it cannot.
   */
  void append_peak_overlap(std::ostream& stream, std::size_t peak);

  /** The base of each slow test of `Family`. A slow test's `run_tests` must
      make one call to `sleep_and_check`. The tests registered together must
      pass it distinct indices, from 0 to one less than their number. The
      sleep intervals of a run give a lower bound on how many of the
      family's tests the runner executed at the same time.

      The tests of one family share one sleep duration and one set of sleep
      intervals. So from a call of `make_slow_test_runner` to the last read
      of `sleep_intervals` after the runner executes, nothing else may
      prepare a run of the family or execute another runner of it. The
      repetitions of a run of tests execute one after another, so a test
      which makes runners of the family breaks this only in these cases:
      -# Two such tests are parallelizable.
      -# Such a test makes or executes a second runner of the family
         between the making of the first and the last read of its
         intervals.
      -# Such a test is registered with two runners which execute at the
         same time.

      `Family` must be declared in an anonymous namespace, so that only the
      tests of one translation unit can use the family. If two units used
      one family, each would define the family's shared state as a weak
      symbol. gcc on macOS may address a unit's `std::source_location` data
      relative to such a symbol, and the linker may take the symbol from the
      other unit (GCC PR 126723). A test may then report the other unit's
      file as its source.
   */
  template<class Family>
  class slow_test_base : public free_test
  {
  public:
    using free_test::free_test;

    [[nodiscard]]
    static std::filesystem::path source_file()
    {
      return std::source_location::current().file_name();
    }

    /** Returns the `sleep_interval` each test of the family recorded since
        the last `prepare_run`, at the index the test passed to
        `sleep_and_check`. A test which has recorded none has an empty
        interval.
     */
    [[nodiscard]]
    static std::vector<sleep_interval> sleep_intervals()
    {
      return st_SleepIntervals;
    }

    /** Sets the sleep of each test of the family to `sleepDuration`, and
        replaces the sleep intervals with `numTests` empty ones.
     */
    static void prepare_run(std::size_t numTests, std::chrono::milliseconds sleepDuration)
    {
      st_SleepIntervals.assign(numTests, sleep_interval{});
      st_SleepDuration = sleepDuration;
    }
  protected:
    ~slow_test_base() = default;

    slow_test_base(slow_test_base&&)            noexcept = default;
    slow_test_base& operator=(slow_test_base&&) noexcept = default;

    /** Sleeps for the duration last passed to `prepare_run`, records the
        sleep's `sleep_interval` at `index`, and makes one check.

        \throws std::out_of_range if `index` is not less than the number of
                tests last passed to `prepare_run`
     */
    void sleep_and_check(std::size_t index)
    {
      const auto start{std::chrono::steady_clock::now()};
      precise_sleep_for(st_SleepDuration);
      st_SleepIntervals.at(index) = sleep_interval{start, std::chrono::steady_clock::now()};
      check(equality, "Integer equality", index, index);
    }
  private:
    // Each test writes only its own element. A runner's execute waits for
    // every test, so the elements may be read once it returns.
    inline static std::vector<sleep_interval> st_SleepIntervals{};

    // Set before a run begins, and read only by its tests
    inline static std::chrono::milliseconds st_SleepDuration{};
  };

  /** Whether `SlowTests` holds at least one type, and each derives from
      `Family`'s `slow_test_base`.
   */
  template<class Family, class... SlowTests>
  inline constexpr bool are_slow_tests_of_v{
    (sizeof...(SlowTests) > 0) && (std::derived_from<SlowTests, slow_test_base<Family>> && ...)
  };

  /** Returns a runner made from `args`, with each of `SlowTests` registered
      to sleep for `sleepDuration`. On return, the family's sleep intervals
      are empty, one for each of `SlowTests`.
   */
  template<class Family, concrete_test... SlowTests>
    requires are_slow_tests_of_v<Family, SlowTests...>
  [[nodiscard]]
  test_runner make_slow_test_runner(commandline_arguments args,
                                    std::stringstream& outputStream,
                                    std::chrono::milliseconds sleepDuration)
  {
    auto runner{make_fake_project_runner(args, outputStream)};
    slow_test_base<Family>::prepare_run(sizeof...(SlowTests), sleepDuration);
    (runner.register_test<SlowTests>(), ...);

    return runner;
  }

  /** The sleep of each slow test in a run whose peak overlap is checked. */
  inline constexpr std::chrono::milliseconds sleep_duration_for_overlap_checks{10};
}
