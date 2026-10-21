////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TestRunnerPerformanceTest.hpp"
#include "FakeProjectTestingUtilities.hpp"
#include "PerformanceTestingUtilities.hpp"
#include "TestRunnerConcurrencyTestingUtilities.hpp"
#include "TestRunnerTestingUtilities.hpp"

#include "sequoia/TestFramework/TestExecutionRecord.hpp"
#include "sequoia/Streaming/Streaming.hpp"

#include <chrono>
#include <format>
#include <functional>
#include <stdexcept>
#include <utility>
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

    /** More than half of eight threads. A test which starts only after the
        others have finished lowers the peak by one. So the floor tolerates
        three such tests, and still fails if half the threads are lost.
     */
    constexpr std::size_t eight_thread_peak_floor{5};

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
                                   counted_spinner{spin_unit, trials_per_attempt_for_minimum_10[0], 8 * spin_unit},
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

    /** The family of the slow tests whose runs the performance test checks. */
    struct acceleration_family {};

    using acceleration_slow_test_base = slow_test_base<acceleration_family>;

    class slow_test_0 final : public acceleration_slow_test_base
    {
    public:
      using slow_test_base::slow_test_base;

      void run_tests() { sleep_and_check(0); }
    };

    class slow_test_1 final : public acceleration_slow_test_base
    {
    public:
      using slow_test_base::slow_test_base;

      void run_tests() { sleep_and_check(1); }
    };

    class slow_test_2 final : public acceleration_slow_test_base
    {
    public:
      using slow_test_base::slow_test_base;

      void run_tests() { sleep_and_check(2); }
    };

    class slow_test_3 final : public acceleration_slow_test_base
    {
    public:
      using slow_test_base::slow_test_base;

      void run_tests() { sleep_and_check(3); }
    };

    class slow_test_4 final : public acceleration_slow_test_base
    {
    public:
      using slow_test_base::slow_test_base;

      void run_tests() { sleep_and_check(4); }
    };

    class slow_test_5 final : public acceleration_slow_test_base
    {
    public:
      using slow_test_base::slow_test_base;

      void run_tests() { sleep_and_check(5); }
    };

    class slow_test_6 final : public acceleration_slow_test_base
    {
    public:
      using slow_test_base::slow_test_base;

      void run_tests() { sleep_and_check(6); }
    };

    class slow_test_7 final : public acceleration_slow_test_base
    {
    public:
      using slow_test_base::slow_test_base;

      void run_tests() { sleep_and_check(7); }
    };

    /** Returns a runner made from `arguments`, with the acceleration
        family's eight slow tests registered, each to sleep for
        `sleepDuration`.
     */
    [[nodiscard]]
    test_runner make_acceleration_family_runner(const std::vector<std::string>& arguments,
                                                std::stringstream& outputStream,
                                                std::chrono::milliseconds sleepDuration)
    {
      return make_slow_test_runner<acceleration_family,
                                   slow_test_0,
                                   slow_test_1,
                                   slow_test_2,
                                   slow_test_3,
                                   slow_test_4,
                                   slow_test_5,
                                   slow_test_6,
                                   slow_test_7>(arguments, outputStream, sleepDuration);
    }

    /** The sleep of each slow test in a run timed by a speed-up check. */
    constexpr std::chrono::milliseconds sleep_duration_for_speedup_checks{20};

    /** Runs the slow tests with the command line `arguments`, each test
        sleeping for `sleep_duration`.

        \throws std::runtime_error if the run does not succeed
     */
    struct run_of_slow_tests
    {
      std::vector<std::string> arguments;
      std::chrono::milliseconds sleep_duration;

      void operator()() const
      {
        std::stringstream outputStream{};
        auto runner{make_acceleration_family_runner(arguments, outputStream, sleep_duration)};
        if(runner.execute() != return_code::success)
          throw std::runtime_error{"A run of the slow tests did not succeed"};
      }
    };

    using run_of_slow_tests_with_baseline_t = task_with_baseline<run_of_slow_tests, run_of_slow_tests>;

    /** Makes a run of the slow tests with the command line `arguments`, each
        test sleeping for `sleep_duration_for_speedup_checks`, and pairs the
        run with its baseline. The baseline runs the slow tests with the same
        command line, and no test sleeps.
     */
    [[nodiscard]]
    run_of_slow_tests_with_baseline_t make_task_with_baseline(const std::vector<std::string>& arguments)
    {
      return {
        .task{arguments, sleep_duration_for_speedup_checks},
        .baseline{arguments, {}}
      };
    }
  }

  [[nodiscard]]
  fs::path test_runner_performance_test::source_file()
  {
    return std::source_location::current().file_name();
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
    test_recording_of_performance_check_attempts();
    test_summary_of_performance_checks_within_other_checks();
  }

  void test_runner_performance_test::test_parallel_acceleration()
  {
    const auto arguments{fake_project_commandline(auxiliary_materials())};
    std::stringstream outputStream{};
    auto runner{make_acceleration_family_runner(arguments, outputStream, sleep_duration_for_overlap_checks)};
    check(equality, "Parallel acceleration return code", runner.execute(), return_code::success);

    const auto peak{peak_overlap(acceleration_slow_test_base::sleep_intervals())};
    append_peak_overlap(outputStream, peak);

    check_output(report({"Parallel Acceleration Output"}), "ParallelAccelerationOutput", outputStream);

    // Without parallel algorithms, the runner runs the tests on a pool of eight threads instead
    check(std::ranges::greater_equal{},
          "Tests execute at once in parallel",
          peak,
          has_parallel_algorithms_v ? std::size_t{2} : eight_thread_peak_floor);

    check_speedup_over_serial_execution("Parallel execution is faster than serial execution",
                                        arguments,
                                        has_parallel_algorithms_v ? relative_performance_interval{2, 9}
                                                                  : relative_performance_interval{6, 9});
  }

  void test_runner_performance_test::test_thread_pool_acceleration()
  {
    {
      const auto arguments{fake_project_commandline(auxiliary_materials(), "--thread-pool", "8")};
      std::stringstream outputStream{};
      auto runner{make_acceleration_family_runner(arguments, outputStream, sleep_duration_for_overlap_checks)};
      check(equality, "Thread pool (8) return code", runner.execute(), return_code::success);

      const auto peak{peak_overlap(acceleration_slow_test_base::sleep_intervals())};
      append_peak_overlap(outputStream, peak);

      check_output(report({"Thread Pool (8) Acceleration Output"}), "ThreadPool8AccelerationOutput", outputStream);
      check(std::ranges::greater_equal{},
            "More than half of the pool's eight threads execute tests at once",
            peak,
            eight_thread_peak_floor);

      check_speedup_over_serial_execution("A pool of eight threads is about eight times faster than serial execution",
                                          arguments,
                                          {6, 9});
    }

    {
      const auto arguments{fake_project_commandline(auxiliary_materials(), "--thread-pool", "2")};
      std::stringstream outputStream{};
      auto runner{make_acceleration_family_runner(arguments, outputStream, sleep_duration_for_overlap_checks)};
      check(equality, "Thread pool (2) return code", runner.execute(), return_code::success);

      const auto peak{peak_overlap(acceleration_slow_test_base::sleep_intervals())};
      append_peak_overlap(outputStream, peak);

      check_output(report({"Thread Pool (2) Acceleration Output"}), "ThreadPool2AccelerationOutput", outputStream);
      check(equality, "Both threads of the pool, and no more, execute tests at once", peak, std::size_t{2});

      check_speedup_over_serial_execution("A pool of two threads is about twice as fast as serial execution",
                                          arguments,
                                          {1.8, 2.1});
    }
  }

  /** Checks the speed-up of a run of the slow tests with the command line
      `arguments` over a serial run of the slow tests. Callers must build
      `arguments` by calling `fake_project_commandline` with
      `auxiliary_materials()`; the check builds the serial run's command
      line the same way.
   */
  void test_runner_performance_test::check_speedup_over_serial_execution(std::string_view description,
                                                                         const std::vector<std::string>& arguments,
                                                                         relative_performance_interval prediction)
  {
    check_relative_performance(description,
                               make_task_with_baseline(arguments),
                               make_task_with_baseline(fake_project_commandline(auxiliary_materials(), "--serial")),
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
    commandline_arguments args{fake_project_commandline(auxiliary_materials())};
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
    commandline_arguments args{fake_project_commandline(auxiliary_materials())};
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
