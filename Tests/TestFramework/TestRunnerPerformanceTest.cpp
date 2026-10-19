////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TestRunnerPerformanceTest.hpp"
#include "PerformanceTestingUtilities.hpp"
#include "TestRunnerTestingUtilities.hpp"

#include "sequoia/TestFramework/TestExecutionRecord.hpp"
#include "sequoia/TestFramework/TestRunner.hpp"
#include "sequoia/PlatformSpecific/Helpers.hpp"
#include "sequoia/Streaming/Streaming.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <format>
#include <ranges>
#include <span>
#include <stdexcept>
#include <vector>

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  namespace
  {
    /** Returns the summary `runner` wrote for `Test`, passed through
        `text_with_zeroed_measurements`.
     */
    template<concrete_test Test>
    [[nodiscard]]
    std::string get_summary_with_zeroed_measurements(const test_runner& runner)
    {
      const test_summary_path summary{Test::source_file(), test_name<Test>(), runner.proj_paths(), null_discriminator};
      if(const auto optContents{read_to_string(summary.file_path(), std::ios_base::in)})
        return text_with_zeroed_measurements(optContents.value(), attempt_count_stability::typically_stable);

      throw std::runtime_error{report_failed_read(summary.file_path())};
    }

    /** When a slow test's sleep began and ended. */
    struct execution_interval
    {
      std::chrono::steady_clock::time_point start{}, end{};
    };

    /** The largest number of `intervals` which overlap at any instant. An interval contains its start but not its
        end, so one which ends as another starts does not overlap it, and an empty interval overlaps nothing.
     */
    [[nodiscard]]
    std::size_t peak_overlap(std::span<const execution_interval> intervals)
    {
      auto numContaining{
        [intervals](std::chrono::steady_clock::time_point instant) {
          auto contains{
            [instant](const execution_interval& interval) {
              return (interval.start <= instant) && (instant < interval.end);
            }
          };

          return static_cast<std::size_t>(std::ranges::count_if(intervals, contains));
        }
      };

      // The overlap rises only at a start, so its peak is reached at one
      auto overlapsAtStarts{
          intervals
        | std::views::transform(&execution_interval::start)
        | std::views::transform(numContaining)
      };

      return std::ranges::fold_left(overlapsAtStarts, std::size_t{}, std::ranges::max);
    }

    /** Returns the line which reports `peak`. CI's step reporting the
        performance test's timings parses it from every run, passing or not,
        so a change to its form breaks that step.
     */
    [[nodiscard]]
    std::string peak_overlap_line(std::size_t peak)
    {
      return std::format("[Peak Overlap: {}]\n", peak);
    }

    /** More than half of eight threads. A test which starts only after the
        others have finished lowers the peak by one. So the floor tolerates
        three such tests, and still fails if half the threads are lost.
     */
    constexpr std::size_t eight_thread_peak_floor{5};

    /** The number of slow tests. Each sleeps for the same duration, so that
        the checks below can measure how the runner spreads the tests over
        threads, and how many it runs at once.

        Each slow test is a class of its own, since the runner names a test
        after its class. A test may not be a class template, and the runner
        refuses two registrations of one class.
     */
    constexpr std::size_t slow_test_count{8};

    class slow_test_base : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static fs::path source_file()
      {
        return std::source_location::current().file_name();
      }

      /** Returns each slow test's `execution_interval`, at the index the
          test passes to `sleep_and_check`.
       */
      [[nodiscard]]
      static std::span<const execution_interval, slow_test_count> execution_intervals() noexcept
      {
        return st_ExecutionIntervals;
      }

      static void clear_execution_intervals() noexcept { st_ExecutionIntervals = {}; }

      static void set_sleep_duration(std::chrono::milliseconds duration) noexcept { st_SleepDuration = duration; }
    protected:
      ~slow_test_base() = default;

      slow_test_base(slow_test_base&&)            noexcept = default;
      slow_test_base& operator=(slow_test_base&&) noexcept = default;

      /** Sleeps for the duration last passed to `set_sleep_duration`,
          records the sleep's `execution_interval` at `index`, and makes one
          check.
       */
      void sleep_and_check(std::size_t index)
      {
        const auto start{std::chrono::steady_clock::now()};
        precise_sleep_for(st_SleepDuration);
        st_ExecutionIntervals[index] = {start, std::chrono::steady_clock::now()};
        check(equality, "Integer equality", index, index);
      }
    private:
      // Each test writes only its own element, and the checks read them once execute has returned, which waits for
      // every test
      inline static std::array<execution_interval, slow_test_count> st_ExecutionIntervals{};

      // Set before a run begins, and read only by its tests
      inline static std::chrono::milliseconds st_SleepDuration{};
    };

    class slow_test_0 final : public slow_test_base
    {
    public:
      using slow_test_base::slow_test_base;

      void run_tests() { sleep_and_check(0); }
    };

    class slow_test_1 final : public slow_test_base
    {
    public:
      using slow_test_base::slow_test_base;

      void run_tests() { sleep_and_check(1); }
    };

    class slow_test_2 final : public slow_test_base
    {
    public:
      using slow_test_base::slow_test_base;

      void run_tests() { sleep_and_check(2); }
    };

    class slow_test_3 final : public slow_test_base
    {
    public:
      using slow_test_base::slow_test_base;

      void run_tests() { sleep_and_check(3); }
    };

    class slow_test_4 final : public slow_test_base
    {
    public:
      using slow_test_base::slow_test_base;

      void run_tests() { sleep_and_check(4); }
    };

    class slow_test_5 final : public slow_test_base
    {
    public:
      using slow_test_base::slow_test_base;

      void run_tests() { sleep_and_check(5); }
    };

    class slow_test_6 final : public slow_test_base
    {
    public:
      using slow_test_base::slow_test_base;

      void run_tests() { sleep_and_check(6); }
    };

    class slow_test_7 final : public slow_test_base
    {
    public:
      using slow_test_base::slow_test_base;

      void run_tests() { sleep_and_check(7); }
    };

    constexpr std::size_t too_few_trials{relative_performance_minimum_trials_floor - 1};

    constexpr std::chrono::microseconds spin_unit{250};

    /** Makes five performance checks. A check stops at its first passing
        attempt, so the tasks' durations set how many attempts each makes:
        -# Two: the slow task is as fast as the fast one for the first
           attempt's trials, and eight times slower after them.
        -# None: the check is refused before its first attempt.
        -# None recorded: the fast task throws during the first attempt.
        -# One: a speed-up of 8 passes at once.
        -# Three, the most allowed: a speed-up of 1 never passes.
     */
    class attempt_counting_performance_test final : public performance_test
    {
    public:
      using performance_test::performance_test;

      [[nodiscard]]
      static fs::path source_file()
      {
        return std::source_location::current().file_name();
      }

      void run_tests()
      {
        const relative_performance_parameters parameters{.prediction{.lower{2}, .upper{40}}};

        check_relative_performance("A speed-up of 1, then of 8",
                                   counted_spinner{spin_unit},
                                   counted_spinner{spin_unit, trials_for_minimum_10[0], 8 * spin_unit},
                                   parameters);

        check_exception_thrown<std::invalid_argument>(
          "Too few trials are refused",
          [this, &parameters]() {
            return check_relative_performance("Too few trials",
                                              counted_spinner{spin_unit},
                                              counted_spinner{8 * spin_unit},
                                              {.prediction{parameters.prediction}, .minimum_trials{too_few_trials}});
          }
        );

        check_exception_thrown<std::runtime_error>(
          "A task which throws ends the check",
          [this, &parameters]() {
            return check_relative_performance("A throwing task",
                                              []() { throw std::runtime_error{"Fast task failure"}; },
                                              counted_spinner{8 * spin_unit},
                                              parameters);
          }
        );

        check_relative_performance("A speed-up of 8", counted_spinner{spin_unit}, counted_spinner{8 * spin_unit}, parameters);
        check_relative_performance("A speed-up of 1", counted_spinner{spin_unit}, counted_spinner{spin_unit}, parameters);
      }
    };

    /** Makes a performance check, and two more, each within a check of an
        exception:
        -# A speed-up of 8 passes at once.
        -# A check of too few trials is refused, which is the exception
           expected.
        -# A speed-up of 0.25 never passes; a check, then the exception
           expected, follow.
     */
    class performance_checks_within_other_checks_test final : public performance_test
    {
    public:
      using performance_test::performance_test;

      [[nodiscard]]
      static fs::path source_file()
      {
        return std::source_location::current().file_name();
      }

      void run_tests()
      {
        const relative_performance_parameters parameters{.prediction{.lower{2}, .upper{40}}};

        check_relative_performance("A speed-up of 8", counted_spinner{spin_unit}, counted_spinner{8 * spin_unit}, parameters);

        check_exception_thrown<std::invalid_argument>(
          "Too few trials are refused",
          [this, &parameters]() {
            return check_relative_performance("Too few trials",
                                              counted_spinner{spin_unit},
                                              counted_spinner{8 * spin_unit},
                                              {.prediction{parameters.prediction}, .minimum_trials{too_few_trials}});
          }
        );

        check_exception_thrown<std::runtime_error>(
          "A nested performance check's failure is the enclosing check's failure, and a check may follow it",
          [this, &parameters]() {
            check_relative_performance("A speed-up of 0.25", counted_spinner{4 * spin_unit}, counted_spinner{spin_unit}, parameters);
            check("A check after the performance check", true);
            throw std::runtime_error{"After the performance check"};
          }
        );
      }
    };

    /** In false-positive mode, makes a performance check, and another within
        a check of an exception:
        -# A speed-up of 8 passes at once.
        -# A check of too few trials is refused, which is the exception
           expected.
     */
    class performance_checks_within_other_checks_false_positive_test final : public performance_false_positive_test
    {
    public:
      using performance_false_positive_test::performance_false_positive_test;

      [[nodiscard]]
      static fs::path source_file()
      {
        return std::source_location::current().file_name();
      }

      void run_tests()
      {
        const relative_performance_parameters parameters{.prediction{.lower{2}, .upper{40}}};

        check_relative_performance("A speed-up of 8", counted_spinner{spin_unit}, counted_spinner{8 * spin_unit}, parameters);

        check_exception_thrown<std::invalid_argument>(
          "Too few trials are refused",
          [this, &parameters]() {
            return check_relative_performance("Too few trials",
                                              counted_spinner{spin_unit},
                                              counted_spinner{8 * spin_unit},
                                              {.prediction{parameters.prediction}, .minimum_trials{too_few_trials}});
          }
        );
      }
    };

    /** In false-negative mode, makes a performance check, and another within
        a check of an exception, each meant to fail:
        -# A speed-up of 1 fails its first attempt.
        -# A check of too few trials is refused, which is not the exception
           expected.
     */
    class performance_checks_within_other_checks_false_negative_test final : public performance_false_negative_test
    {
    public:
      using performance_false_negative_test::performance_false_negative_test;

      [[nodiscard]]
      static fs::path source_file()
      {
        return std::source_location::current().file_name();
      }

      void run_tests()
      {
        const relative_performance_parameters parameters{.prediction{.lower{2}, .upper{40}}};

        check_relative_performance("A speed-up of 1", counted_spinner{spin_unit}, counted_spinner{spin_unit}, parameters);

        check_exception_thrown<std::runtime_error>(
          "Too few trials are refused, but not with the exception expected",
          [this, &parameters]() {
            return check_relative_performance("Too few trials",
                                              counted_spinner{spin_unit},
                                              counted_spinner{8 * spin_unit},
                                              {.prediction{parameters.prediction}, .minimum_trials{too_few_trials}});
          }
        );
      }
    };

    /** A performance test which makes no performance check. */
    class performance_test_without_performance_checks final : public performance_test
    {
    public:
      using performance_test::performance_test;

      [[nodiscard]]
      static fs::path source_file()
      {
        return std::source_location::current().file_name();
      }

      void run_tests()
      {
        check(equality, "Not a performance check", 42, 42);
      }
    };

    test_runner make_slow_suite(commandline_arguments args,
                                std::stringstream& outputStream,
                                std::chrono::milliseconds sleepDuration)
    {
      auto runner{make_fake_project_runner(args, outputStream)};
      slow_test_base::clear_execution_intervals();
      slow_test_base::set_sleep_duration(sleepDuration);

      runner.register_test<slow_test_0>();
      runner.register_test<slow_test_1>();
      runner.register_test<slow_test_2>();
      runner.register_test<slow_test_3>();
      runner.register_test<slow_test_4>();
      runner.register_test<slow_test_5>();
      runner.register_test<slow_test_6>();
      runner.register_test<slow_test_7>();

      return runner;
    }

    /** The sleep of each slow test in the runs whose printed output and
        peak overlap this test checks. The shorter the sleep, the more often
        a test which starts late fails to overlap the others.
     */
    constexpr std::chrono::milliseconds sleep_duration_for_overlap_checks{10};

    /** The sleep of each slow test in a run timed by a speed-up check. */
    constexpr std::chrono::milliseconds sleep_duration_for_speedup_checks{20};

    /** Runs the slow tests with the command line `arguments`, which begins
        with the fake project's executable, each test sleeping for
        `sleep_duration`.

        \throws std::runtime_error if the run does not succeed
     */
    struct run_of_slow_tests
    {
      std::vector<std::string> arguments;
      std::chrono::milliseconds sleep_duration;

      void operator()() const
      {
        std::stringstream outputStream{};
        auto runner{make_slow_suite(arguments, outputStream, sleep_duration)};
        if(runner.execute() != return_code::success)
          throw std::runtime_error{"A run of the slow tests did not succeed"};
      }
    };
  }

  [[nodiscard]]
  fs::path test_runner_performance_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  [[nodiscard]]
  fs::path test_runner_performance_test::fake_project() const
  {
    return auxiliary_materials() /= "FakeProject";
  }

  [[nodiscard]]
  fs::path test_runner_performance_test::minimal_fake_path() const
  {
    return fake_project().append("build/CMade/FakeExe.txt");
  }

  void test_runner_performance_test::check_output(std::string_view description,
                                                  std::string_view dirName,
                                                  const std::stringstream& output)
  {
    write(dirName, output.view());
    check(equivalence, description, working_materials() /= dirName, predictive_materials() /= dirName);
  }

  void test_runner_performance_test::write(std::string_view dirName, std::string_view printedOutput) const
  {
    const auto outputDir{working_materials() /= dirName};
    fs::create_directory(outputDir);
    write_to_file(outputDir / "io.txt", printedOutput, std::ios_base::out);
  }

  void test_runner_performance_test::run_tests()
  {
    test_parallel_acceleration();
    test_thread_pool_acceleration();
    test_serial_execution();
    test_recording_of_performance_check_attempts();
    test_summary_of_performance_checks_within_other_checks();
  }

  void test_runner_performance_test::test_parallel_acceleration()
  {
    std::stringstream outputStream{};
    auto runner{make_slow_suite({{minimal_fake_path().generic_string()}},
                                outputStream,
                                sleep_duration_for_overlap_checks)};
    check(equality, "Parallel acceleration return code", runner.execute(), return_code::success);

    const auto peak{peak_overlap(slow_test_base::execution_intervals())};
    outputStream << peak_overlap_line(peak);

    check_output(report({"Parallel Acceleration Output"}), "ParallelAccelerationOutput", outputStream);

    // Without parallel algorithms, the runner runs the tests on a pool of eight threads instead
    check(std::ranges::greater_equal{},
          "Tests execute at once in parallel",
          peak,
          has_parallel_algorithms_v ? std::size_t{2} : eight_thread_peak_floor);

    check_speedup_over_serial_execution("Parallel execution is faster than serial execution",
                                        {},
                                        has_parallel_algorithms_v ? relative_performance_interval{2, 9}
                                                                  : relative_performance_interval{6, 9});
  }

  void test_runner_performance_test::test_thread_pool_acceleration()
  {
    {
      std::stringstream outputStream{};
      auto runner{make_slow_suite({{minimal_fake_path().generic_string(), "--thread-pool", "8"}},
                                  outputStream,
                                  sleep_duration_for_overlap_checks)};
      check(equality, "Thread pool (8) return code", runner.execute(), return_code::success);

      const auto peak{peak_overlap(slow_test_base::execution_intervals())};
      outputStream << peak_overlap_line(peak);

      check_output(report({"Thread Pool (8) Acceleration Output"}), "ThreadPool8AccelerationOutput", outputStream);
      check(std::ranges::greater_equal{},
            "More than half of the pool's eight threads execute tests at once",
            peak,
            eight_thread_peak_floor);

      check_speedup_over_serial_execution("A pool of eight threads is about eight times faster than serial execution",
                                          {"--thread-pool", "8"},
                                          {6, 9});
    }

    {
      std::stringstream outputStream{};
      auto runner{make_slow_suite({{minimal_fake_path().generic_string(), "--thread-pool", "2"}},
                                  outputStream,
                                  sleep_duration_for_overlap_checks)};
      check(equality, "Thread pool (2) return code", runner.execute(), return_code::success);

      const auto peak{peak_overlap(slow_test_base::execution_intervals())};
      outputStream << peak_overlap_line(peak);

      check_output(report({"Thread Pool (2) Acceleration Output"}), "ThreadPool2AccelerationOutput", outputStream);
      check(equality, "Both threads of the pool, and no more, execute tests at once", peak, std::size_t{2});

      check_speedup_over_serial_execution("A pool of two threads is about twice as fast as serial execution",
                                          {"--thread-pool", "2"},
                                          {1.8, 2.1});
    }
  }

  void test_runner_performance_test::test_serial_execution()
  {
    std::stringstream outputStream{};
    auto runner{make_slow_suite({{minimal_fake_path().generic_string(), "--serial"}},
                                outputStream,
                                sleep_duration_for_overlap_checks)};
    check(equality, "Serial execution return code", runner.execute(), return_code::success);

    const auto peak{peak_overlap(slow_test_base::execution_intervals())};
    outputStream << peak_overlap_line(peak);

    check_output(report({"Serial Output"}), "Serial Output", outputStream);
    check(equality, "Tests execute one at a time in a serial run", peak, std::size_t{1});
  }

  /** Checks the speed-up of a run of the slow tests with `modeArguments`
      over a serial run. Each run's baseline is a run in the same mode in
      which the tests do not sleep.
   */
  void test_runner_performance_test::check_speedup_over_serial_execution(std::string_view description,
                                                                         std::vector<std::string> modeArguments,
                                                                         relative_performance_interval prediction)
  {
    auto taskWithBaselineFor{
      [fakePath{minimal_fake_path().generic_string()}](std::vector<std::string> arguments) {
        arguments.insert(arguments.begin(), fakePath);
        return task_with_baseline<run_of_slow_tests, run_of_slow_tests>{
          .task{arguments, sleep_duration_for_speedup_checks},
          .baseline{arguments, {}}
        };
      }
    };

    check_relative_performance(description,
                               taskWithBaselineFor(std::move(modeArguments)),
                               taskWithBaselineFor({"--serial"}),
                               {.prediction{prediction}});
  }

  /** `attempt_counting_performance_test`'s checks make 2, 1 and 3 attempts:
      distinct numbers, in neither sorted order nor its reverse, and not the
      checks' numbers of trials. Its other two checks end by throwing, one
      before its first attempt and one during it, so the record has no entry
      for either. `performance_test_without_performance_checks` is the
      control: a performance test whose record gives no attempts.
   */
  void test_runner_performance_test::test_recording_of_performance_check_attempts()
  {
    std::stringstream outputStream{};
    commandline_arguments args{{minimal_fake_path().generic_string()}};
    auto runner{make_fake_project_runner(args, outputStream)};
    runner.register_test<attempt_counting_performance_test>();
    runner.register_test<performance_test_without_performance_checks>();
    check(equality, "Performance check records return code", runner.execute(), return_code::soft_failures);

    check(equality,
          "The record of a performance test gives the number of attempts each check made, in the order of the checks",
          get_recorded_completion<attempt_counting_performance_test>(runner).performance_check_attempts,
          std::vector<std::size_t>{2, 1, 3});

    check(equality,
          "The record of a performance test which makes no performance check gives no attempts",
          get_recorded_completion<performance_test_without_performance_checks>(runner).performance_check_attempts,
          std::vector<std::size_t>{});
  }

  void test_runner_performance_test::test_summary_of_performance_checks_within_other_checks()
  {
    using standard_test       = performance_checks_within_other_checks_test;
    using false_positive_test = performance_checks_within_other_checks_false_positive_test;
    using false_negative_test = performance_checks_within_other_checks_false_negative_test;

    std::stringstream outputStream{};
    commandline_arguments args{{minimal_fake_path().generic_string()}};
    auto runner{make_fake_project_runner(args, outputStream)};
    runner.register_test<standard_test>();
    runner.register_test<false_positive_test>();
    runner.register_test<false_negative_test>();
    check(equality,
          "Performance checks within other checks return code",
          runner.execute(),
          return_code::soft_failures);

    const auto outputDir{working_materials() /= "PerformanceChecksWithinOtherChecks"};
    fs::create_directory(outputDir);

    auto writeSummary{
      [&outputDir](std::string_view testName, const std::string& summary) {
        write_to_file(outputDir / std::format("{}.txt", testName), summary, std::ios_base::out);
      }
    };

    writeSummary(test_name<standard_test>(),       get_summary_with_zeroed_measurements<standard_test>(runner));
    writeSummary(test_name<false_positive_test>(), get_summary_with_zeroed_measurements<false_positive_test>(runner));
    writeSummary(test_name<false_negative_test>(), get_summary_with_zeroed_measurements<false_negative_test>(runner));

    check(equivalence,
          "A performance check within another check is counted as part of the other",
          outputDir,
          predictive_materials() /= "PerformanceChecksWithinOtherChecks");
  }
}
