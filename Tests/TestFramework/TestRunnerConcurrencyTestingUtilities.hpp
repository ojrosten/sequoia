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
  /** A half-open interval of `std::chrono::steady_clock` time points. The
      interval contains each time point from its start time up to, but not
      including, its end time.
   */
  class steady_clock_interval
  {
  public:
    using time_point = std::chrono::steady_clock::time_point;

    steady_clock_interval() = default;

    /** \throws std::invalid_argument if `end` is earlier than `start` */
    steady_clock_interval(time_point start, time_point end);

    [[nodiscard]]
    time_point start_time() const noexcept { return m_Start; }

    [[nodiscard]]
    time_point end_time() const noexcept { return m_End; }

    [[nodiscard]]
    friend bool operator==(const steady_clock_interval&, const steady_clock_interval&) noexcept = default;
  private:
    time_point m_Start{},
               m_End{};
  };

  /** Returns the largest number of `intervals` which contain the same
      instant.
   */
  [[nodiscard]]
  std::size_t peak_overlap(std::span<const steady_clock_interval> intervals);

  /** Prints `[Peak Overlap: <peak>]` and a newline to `stream`. CI's step
      "Report the performance test's timings" parses this text, and prints
      no peak overlap if it cannot.
   */
  void print_peak_overlap(std::ostream& stream, std::size_t peak);

  /** The base of each of `Family`'s tests which sleep. The `run_tests` of a
      test which sleeps must make one call to `sleep_and_check`. The tests
      registered together must pass it distinct indices, from 0 to one less
      than their number.

      Each test records the interval for which it slept, and a test's sleep
      lies within its total execution time. So tests whose sleeps overlap ran
      at the same time, though tests which ran at the same time need not have
      overlapping sleeps. The `peak_overlap` of a run's sleep intervals
      therefore never exceeds the number of the family's tests the runner
      executed at once.

      The tests of one family share one sleep duration and one set of sleep
      intervals. So from a call of `make_runner_of_tests_which_sleep` to the
      last read of `sleep_intervals` after the runner executes, nothing else
      may prepare a run of the same family or execute another runner of it. The
      repetitions of a run of tests execute one after another, so a test which
      makes runners of the family breaks this only in these cases:
      -# Two such tests are parallelizable.
      -# Such a test makes or executes a second runner of the family
         between the making of the first and the last read of its
         intervals.
      -# Such a test is registered with two runners which execute at the
         same time.

      Due to a current GCC bug (PR 126723), `Family` must be declared in an
      anonymous namespace, so that only the tests of one translation unit can
      use the family. If two units used one family, each would define the
      family's shared state as a weak symbol. gcc on macOS may address a unit's
      `std::source_location` data relative to such a symbol, and the linker may
      take the symbol from the other unit. A test may then report the other
      unit's file as its source.
   */
  template<class Family>
  class test_which_sleeps_base : public free_test
  {
  public:
    using free_test::free_test;

    [[nodiscard]]
    static std::filesystem::path source_file()
    {
      return std::source_location::current().file_name();
    }

    /** Returns the intervals for which the family's tests slept since the
        last `prepare_run`. Each test's interval is at the index the test
        passed to `sleep_and_check`. An index at which no test has recorded
        an interval holds an empty interval.
     */
    [[nodiscard]]
    static std::vector<steady_clock_interval> sleep_intervals()
    {
      return st_SleepIntervals;
    }

    /** Sets the sleep of each test of the family to `sleepDuration`, and
        replaces the sleep intervals with `numTests` empty ones.
     */
    static void prepare_run(std::size_t numTests, std::chrono::milliseconds sleepDuration)
    {
      st_SleepIntervals.assign(numTests, steady_clock_interval{});
      st_SleepDuration = sleepDuration;
    }
  protected:
    ~test_which_sleeps_base() = default;

    test_which_sleeps_base(test_which_sleeps_base&&)            noexcept = default;
    test_which_sleeps_base& operator=(test_which_sleeps_base&&) noexcept = default;

    /** Sleeps for the duration last passed to `prepare_run`, records the
        sleep's interval at `index` of the family's sleep intervals, and
        makes one check.

        \throws std::out_of_range if `index` is not less than the number of
                tests last passed to `prepare_run`
     */
    void sleep_and_check(std::size_t index)
    {
      const auto start{std::chrono::steady_clock::now()};
      precise_sleep_for(st_SleepDuration);
      st_SleepIntervals.at(index) = steady_clock_interval{start, std::chrono::steady_clock::now()};
      check(equality, "Integer equality", index, index);
    }
  private:
    // Each test writes only its own element. A runner's execute waits for
    // every test, so the elements may be read once it returns.
    inline static std::vector<steady_clock_interval> st_SleepIntervals{};

    // Set before a run begins, and read only by its tests
    inline static std::chrono::milliseconds st_SleepDuration{};
  };

  template<class Family, class... TestsWhichSleep>
  inline constexpr bool are_tests_which_sleep_of_v{
    (sizeof...(TestsWhichSleep) > 0) && (std::derived_from<TestsWhichSleep, test_which_sleeps_base<Family>> && ...)
  };

  /** Returns a runner made from `args`, with each of `TestsWhichSleep`
      registered to sleep for `sleepDuration`. On return, the family's sleep
      intervals are empty, one for each of `TestsWhichSleep`.
   */
  template<class Family, concrete_test... TestsWhichSleep>
    requires are_tests_which_sleep_of_v<Family, TestsWhichSleep...>
  [[nodiscard]]
  test_runner make_runner_of_tests_which_sleep(commandline_arguments args,
                                               std::stringstream& outputStream,
                                               std::chrono::milliseconds sleepDuration)
  {
    auto runner{make_fake_project_runner(args, outputStream)};
    test_which_sleeps_base<Family>::prepare_run(sizeof...(TestsWhichSleep), sleepDuration);
    (runner.register_test<TestsWhichSleep>(), ...);

    return runner;
  }

  inline constexpr std::chrono::milliseconds sleep_duration_for_overlap_checks{10};
}
