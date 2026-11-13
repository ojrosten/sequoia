////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TestRunnerPerformanceTest.hpp"
#include "FakeProjectTestingUtilities.hpp"
#include "TestRunnerConcurrencyTestingUtilities.hpp"

#include "sequoia/Core/Meta/TypeList.hpp"
#include "sequoia/Streaming/Streaming.hpp"

#include <algorithm>
#include <chrono>
#include <functional>
#include <memory>
#include <stdexcept>
#include <vector>

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  namespace
  {
    /** More than half of eight threads. A test which starts only after the
        others have finished lowers the peak by one. So the floor tolerates
        three such tests, and still fails if half the threads are lost.
     */
    constexpr std::size_t eight_thread_peak_floor{5};

    struct acceleration_family {};

    using acceleration_family_base = test_which_sleeps_base<acceleration_family>;

    class test_which_sleeps_0 final : public acceleration_family_base
    {
    public:
      using test_which_sleeps_base::test_which_sleeps_base;

      void run_tests() { sleep_and_check(0); }
    };

    class test_which_sleeps_1 final : public acceleration_family_base
    {
    public:
      using test_which_sleeps_base::test_which_sleeps_base;

      void run_tests() { sleep_and_check(1); }
    };

    class test_which_sleeps_2 final : public acceleration_family_base
    {
    public:
      using test_which_sleeps_base::test_which_sleeps_base;

      void run_tests() { sleep_and_check(2); }
    };

    class test_which_sleeps_3 final : public acceleration_family_base
    {
    public:
      using test_which_sleeps_base::test_which_sleeps_base;

      void run_tests() { sleep_and_check(3); }
    };

    class test_which_sleeps_4 final : public acceleration_family_base
    {
    public:
      using test_which_sleeps_base::test_which_sleeps_base;

      void run_tests() { sleep_and_check(4); }
    };

    class test_which_sleeps_5 final : public acceleration_family_base
    {
    public:
      using test_which_sleeps_base::test_which_sleeps_base;

      void run_tests() { sleep_and_check(5); }
    };

    class test_which_sleeps_6 final : public acceleration_family_base
    {
    public:
      using test_which_sleeps_base::test_which_sleeps_base;

      void run_tests() { sleep_and_check(6); }
    };

    class test_which_sleeps_7 final : public acceleration_family_base
    {
    public:
      using test_which_sleeps_base::test_which_sleeps_base;

      void run_tests() { sleep_and_check(7); }
    };

    using acceleration_family_test_types = type_list<test_which_sleeps_0,
                                                     test_which_sleeps_1,
                                                     test_which_sleeps_2,
                                                     test_which_sleeps_3,
                                                     test_which_sleeps_4,
                                                     test_which_sleeps_5,
                                                     test_which_sleeps_6,
                                                     test_which_sleeps_7>;

    [[nodiscard]]
    test_runner make_acceleration_family_runner(const std::vector<std::string>& arguments,
                                                std::stringstream& outputStream,
                                                std::chrono::milliseconds sleepDuration)
    {
      return [&]<class... Tests>(type_list<Tests...>) {
        return make_runner_of_tests_which_sleep<acceleration_family, Tests...>(arguments, outputStream, sleepDuration);
      }(acceleration_family_test_types{});
    }

    [[nodiscard]]
    std::vector<test_to_run> make_acceleration_family_tests_to_run(const project_paths& projPaths)
    {
      return [&projPaths]<class... Tests>(type_list<Tests...>) {
        return make_tests_which_sleep<acceleration_family, Tests...>(projPaths);
      }(acceleration_family_test_types{});
    }

    constexpr std::chrono::milliseconds sleep_duration_for_speedup_checks{20};

    /** \throws std::runtime_error if the run does not succeed */
    struct run_of_acceleration_family
    {
      std::vector<std::string> arguments;
      std::chrono::milliseconds sleep_duration;

      void operator()() const
      {
        std::stringstream outputStream{};
        auto runner{make_acceleration_family_runner(arguments, outputStream, sleep_duration)};
        if(runner.execute() != return_code::success)
          throw std::runtime_error{"A run of the acceleration family's tests did not succeed"};
      }
    };

    using acceleration_family_run_with_baseline_t = task_with_baseline<run_of_acceleration_family,
                                                                       run_of_acceleration_family>;

    /** The baseline runs the same tests with the same command line, and no
        test sleeps.
     */
    [[nodiscard]]
    acceleration_family_run_with_baseline_t make_run_with_baseline(const std::vector<std::string>& arguments)
    {
      return {
        .task{arguments, sleep_duration_for_speedup_checks},
        .baseline{arguments, {}}
      };
    }

    /** Executes the acceleration family's tests with `execute_tests`, and
        nothing else of a run, with each test sleeping for `sleep_duration`.

        \throws std::runtime_error if a test fails, or if a summary is not
                its test's
     */
    struct execution_of_acceleration_family
    {
      // Shared, since a performance task must be copy constructible, and a
      // `test_to_run` is move-only
      std::shared_ptr<std::vector<test_to_run>> tests;
      concurrency_mode mode;
      std::size_t pool_size;
      std::chrono::milliseconds sleep_duration;

      void operator()() const
      {
        acceleration_family_base::prepare_run(tests->size(), sleep_duration);
        std::ranges::for_each(*tests, &test_to_run::reset_results);

        const auto execution{execute_tests(references_to(*tests), mode, pool_size, std::nullopt)};

        auto succeeded{[](const log_summary& summary) { return to_return_code(summary) == return_code::success; }};
        const bool summarizesEachTest{
          std::ranges::equal(execution.summaries, *tests, {}, &log_summary::name, &test_to_run::name)
        };

        if(!summarizesEachTest || !std::ranges::all_of(execution.summaries, succeeded))
          throw std::runtime_error{"An execution of the acceleration family's tests did not succeed"};
      }
    };

    using acceleration_family_execution_with_baseline_t = task_with_baseline<execution_of_acceleration_family,
                                                                             execution_of_acceleration_family>;

    /** The baseline executes tests of the same types under the same
        concurrency, and none of them sleeps.
     */
    [[nodiscard]]
    acceleration_family_execution_with_baseline_t
      make_execution_with_baseline(const project_paths& projPaths, concurrency_mode mode, std::size_t poolSize)
    {
      auto testsToRun{
        [&projPaths]() {
          return std::make_shared<std::vector<test_to_run>>(make_acceleration_family_tests_to_run(projPaths));
        }
      };

      return {
        .task    {testsToRun(), mode, poolSize, sleep_duration_for_speedup_checks},
        .baseline{testsToRun(), mode, poolSize, {}}
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
  }

  void test_runner_performance_test::test_parallel_acceleration()
  {
    const auto arguments{fake_project_commandline(auxiliary_materials())};
    std::stringstream outputStream{};
    auto runner{make_acceleration_family_runner(arguments, outputStream, sleep_duration_for_overlap_checks)};
    check(equality, "Parallel acceleration return code", runner.execute(), return_code::success);

    const auto peak{peak_overlap(acceleration_family_base::sleep_intervals())};
    print_peak_overlap(outputStream, peak);

    check_output(report({"Parallel Acceleration Output"}), "ParallelAccelerationOutput", outputStream);

    // Without parallel algorithms, the runner runs the tests on a pool of eight threads instead
    check(std::ranges::greater_equal{},
          "Tests execute at once in parallel",
          peak,
          has_parallel_algorithms_v ? std::size_t{2} : eight_thread_peak_floor);

    const auto parallelSpeedup{
      has_parallel_algorithms_v ? relative_performance_interval{2, 9} : relative_performance_interval{6, 9}
    };

    check_run_speedup_over_serial_run("A parallel run is faster than a serial run", arguments, parallelSpeedup);

    check_execution_speedup_over_serial_execution(
      "Executing the tests in parallel is faster than executing them serially",
      concurrency_mode::dynamic,
      {},
      parallelSpeedup);
  }

  void test_runner_performance_test::test_thread_pool_acceleration()
  {
    {
      const auto arguments{fake_project_commandline(auxiliary_materials(), "--thread-pool", "8")};
      std::stringstream outputStream{};
      auto runner{make_acceleration_family_runner(arguments, outputStream, sleep_duration_for_overlap_checks)};
      check(equality, "Thread pool (8) return code", runner.execute(), return_code::success);

      const auto peak{peak_overlap(acceleration_family_base::sleep_intervals())};
      print_peak_overlap(outputStream, peak);

      check_output(report({"Thread Pool (8) Acceleration Output"}), "ThreadPool8AccelerationOutput", outputStream);
      check(std::ranges::greater_equal{},
            "More than half of the pool's eight threads execute tests at once",
            peak,
            eight_thread_peak_floor);

      check_run_speedup_over_serial_run(
        "A run on a pool of eight threads is about eight times faster than a serial run",
        arguments,
        {6, 9});

      check_execution_speedup_over_serial_execution(
        "Executing the tests on a pool of eight threads is about eight times faster than executing them serially",
        concurrency_mode::fixed,
        8,
        {6, 9});
    }

    {
      const auto arguments{fake_project_commandline(auxiliary_materials(), "--thread-pool", "2")};
      std::stringstream outputStream{};
      auto runner{make_acceleration_family_runner(arguments, outputStream, sleep_duration_for_overlap_checks)};
      check(equality, "Thread pool (2) return code", runner.execute(), return_code::success);

      const auto peak{peak_overlap(acceleration_family_base::sleep_intervals())};
      print_peak_overlap(outputStream, peak);

      check_output(report({"Thread Pool (2) Acceleration Output"}), "ThreadPool2AccelerationOutput", outputStream);
      check(equality, "Two tests' sleeps overlap, and no three do", peak, std::size_t{2});

      check_run_speedup_over_serial_run("A run on a pool of two threads is about twice as fast as a serial run",
                                        arguments,
                                        {1.8, 2.1});

      check_execution_speedup_over_serial_execution(
        "Executing the tests on a pool of two threads is about twice as fast as executing them serially",
        concurrency_mode::fixed,
        2,
        {1.8, 2.1});
    }
  }

  /** Checks the speed-up of a run of the acceleration family's tests with
      the command line `arguments` over a serial run of the same tests.
      Callers must build `arguments` by calling `fake_project_commandline`
      with `auxiliary_materials()`; the check builds the serial run's command
      line the same way.
   */
  void test_runner_performance_test::check_run_speedup_over_serial_run(std::string_view description,
                                                                       const std::vector<std::string>& arguments,
                                                                       relative_performance_interval prediction)
  {
    check_relative_performance(description,
                               make_run_with_baseline(arguments),
                               make_run_with_baseline(fake_project_commandline(auxiliary_materials(), "--serial")),
                               {.prediction{prediction}});
  }

  /** Checks the speed-up of executing the acceleration family's tests under
      `concurrencyMode`, with a pool of `poolSize` threads if that mode is
      `concurrency_mode::fixed`, over executing them serially.

      The check times calls of `execute_tests` alone: it makes the tests
      outside the timed calls, and constructs no runner.
   */
  void test_runner_performance_test::check_execution_speedup_over_serial_execution(
    std::string_view description,
    concurrency_mode concurrencyMode,
    std::size_t poolSize,
    relative_performance_interval prediction)
  {
    commandline_arguments args{fake_project_commandline(auxiliary_materials())};
    const project_paths projPaths{args.size(), args.get(), fake_project_customization()};

    check_relative_performance(description,
                               make_execution_with_baseline(projPaths, concurrencyMode, poolSize),
                               make_execution_with_baseline(projPaths, concurrency_mode::serial, {}),
                               {.prediction{prediction}});
  }
}
