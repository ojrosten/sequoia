////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TestRunnerPerformanceTest.hpp"
#include "PerformanceTestingUtilities.hpp"
#include "Parsing/CommandLineArgumentsTestingUtilities.hpp"

#include "sequoia/TestFramework/TestExecutionRecord.hpp"
#include "sequoia/TestFramework/TestRunner.hpp"
#include "sequoia/Streaming/Streaming.hpp"
#include "sequoia/TextProcessing/Numbers.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
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
    /** A duration as the runner prints it, a number followed by its unit, converted to milliseconds. */
    [[nodiscard]]
    double to_milliseconds(std::string_view printedDuration)
    {
      constexpr std::array<std::pair<std::string_view, double>, 4> millisecondsPerUnit{
        {{"s", 1e3}, {"ms", 1.0}, {"us", 1e-3}, {"ns", 1e-6}}
      };

      const auto numberEnd{printedDuration.find_first_not_of("0123456789.e+")};
      const auto unitStart{std::ranges::min(numberEnd, printedDuration.size())};
      auto unitOf{[](const auto& entry){ return entry.first; }};
      const auto conversion{std::ranges::find(millisecondsPerUnit, printedDuration.substr(unitStart), unitOf)};
      const auto number{extract_numbers_from<double, 1>(printedDuration.substr(0, unitStart))};
      if(!number || (conversion == millisecondsPerUnit.end()))
        throw std::runtime_error{std::format("Unable to read the duration {}", printedDuration)};

      return number->front() * conversion->second;
    }

    /** The run's duration labelled `label`, such as "Execution Time", in milliseconds. It is read from the grand
        totals, since every test reports durations of its own and the run's are the only ones these checks measure.
     */
    [[nodiscard]]
    double get_grand_total(const fs::path& file, std::string_view label)
    {
      if(const auto optContents{read_to_string(file, std::ios_base::in)})
      {
        std::string_view contents{optContents.value()};
        const auto pattern{std::format("[{}: ", label)};

        if(const auto pos{contents.find(pattern, contents.find("Grand Totals"))}; pos != std::string::npos)
        {
          const auto start{pos + pattern.size()};
          if(const auto end{contents.find(']', start)}; end != std::string::npos)
            return to_milliseconds(contents.substr(start, end - start));
        }
      }

      throw std::runtime_error{std::format("Unable to extract the {} from: {}", label, file.generic_string())};
    }

    /** Reads the completion of the last record `runner` keeps for `Test`. */
    template<concrete_test Test>
    [[nodiscard]]
    test_execution_completion get_recorded_completion(const test_runner& runner)
    {
      const test_execution_record_path record{Test::source_file(), test_name<Test>(), runner.proj_paths()};
      if(auto records{read_test_execution_records(record.file_path())}; !records.empty() && records.back().completion)
        return std::move(records.back().completion).value();

      throw std::runtime_error{std::format("No completion in: {}", record.file_path().generic_string())};
    }

    [[nodiscard]]
    double in_milliseconds(std::chrono::microseconds duration)
    {
      return std::chrono::duration<double, std::milli>{duration}.count();
    }

    /** The summary `runner` writes for `Test`, with every measurement set to
        zero by `text_with_zeroed_measurements`.
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
    std::ptrdiff_t peak_overlap(std::span<const execution_interval> intervals)
    {
      using change_t = std::pair<std::chrono::steady_clock::time_point, std::ptrdiff_t>;
      auto endpoints{
        [](const execution_interval& interval){
          return std::array{change_t{interval.start, 1}, change_t{interval.end, -1}};
        }
      };

      auto changes{
          intervals
        | std::views::transform(endpoints)
        | std::views::join
        | std::ranges::to<std::vector>()
      };

      // At one instant an end, -1, sorts before a start, +1
      std::ranges::sort(changes);

      std::ptrdiff_t overlap{}, peak{};
      for(const auto& change : changes)
      {
        overlap += change.second;
        peak = std::ranges::max(peak, overlap);
      }

      return peak;
    }

    /** \brief A line for the run's output giving `peak`, which CI reports for every run, passing or not. */
    [[nodiscard]]
    std::string peak_overlap_line(std::ptrdiff_t peak)
    {
      return std::format("[Peak Overlap: {}]\n", peak);
    }

    /** More than half of eight threads. A test which starts only after the others have finished, delayed by its
        execution record perhaps, lowers the peak by one, so this tolerates three such tests and still fails if half
        the threads are lost.
     */
    constexpr std::ptrdiff_t eight_thread_peak_floor{5};

    /** Eight tests are wanted, each sleeping the same amount, so that the checks below can
        measure how the runner distributes them over threads, and how many it runs at once. They
        are eight *classes* because a test's name is synthesized from its class: a class template
        cannot supply a file-system-safe name, and registering one class twice - which is what
        these used to do - would ask two tests to share it.
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

      /** \brief When each test's tests ran, by the index the test passes to `sleep_and_check`. */
      [[nodiscard]]
      static std::span<const execution_interval, slow_test_count> execution_intervals() noexcept
      {
        return st_ExecutionIntervals;
      }

      static void clear_execution_intervals() noexcept { st_ExecutionIntervals = {}; }
    protected:
      ~slow_test_base() = default;

      slow_test_base(slow_test_base&&)            noexcept = default;
      slow_test_base& operator=(slow_test_base&&) noexcept = default;

      /** `sleep_for` overshoots by a few ms per call, and that cost is **per call and independent
          of the duration requested**: eight of them contribute ~25ms whether each asks for 25ms or
          for 100ms. Since the tolerances below are absolute, lengthening the sleeps would buy no
          margin whatsoever - it would only shrink the error as a *fraction* of a total nothing
          measures. So the sleeps are as short as the timings below can resolve, the overshoot is
          absorbed by the tolerances, and anyone tempted to stabilise these checks by sleeping
          longer will pay in run time and get nothing.

          The duration also fixes the lower bound of every timing check below. Each is centred so
          that its *lower* bound is exactly the nominal time for that execution model - 25ms for
          eight tasks run concurrently, 100ms for eight tasks over a pool of two, 200ms for eight
          run serially. So the lower half of each tolerance is not slack: it asserts that the
          acceleration being claimed is real, and a run finishing faster than physics allows is a
          failure rather than a bonus.
       */
      void sleep_and_check(std::size_t index)
      {
        using namespace std::chrono_literals;
        const auto start{std::chrono::steady_clock::now()};
        std::this_thread::sleep_for(25ms);
        st_ExecutionIntervals[index] = {start, std::chrono::steady_clock::now()};
        check(equality, "Integer equality", index, index);
      }
    private:
      // Each test writes only its own element, and the checks read them once execute has returned, which waits for
      // every test
      inline static std::array<execution_interval, slow_test_count> st_ExecutionIntervals{};
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

    constexpr double execution_sleep_ms{20.0}, summarizing_sleep_ms{60.0};

    /** The runner prints a duration to three significant figures, so it prints one below a second to within half a
        millisecond.
     */
    constexpr double printed_duration_tolerance_ms{1.0};

    /** Sleeps while its tests run, and sleeps again while the runner summarizes it. The runner summarizes a test after
        its tests have run, so the first sleep falls in the test's execution duration and the second in the runner's
        overhead.
     */
    class slow_to_summarize_test_base : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static fs::path source_file()
      {
        return std::source_location::current().file_name();
      }

      void run_tests()
      {
        std::this_thread::sleep_for(std::chrono::duration<double, std::milli>{execution_sleep_ms});
        check("Slept", true);
      }

      [[nodiscard]]
      log_summary summarize(duration delta) const
      {
        std::this_thread::sleep_for(std::chrono::duration<double, std::milli>{summarizing_sleep_ms});
        return free_test::summarize(delta);
      }
    protected:
      ~slow_to_summarize_test_base() = default;

      slow_to_summarize_test_base(slow_to_summarize_test_base&&)            noexcept = default;
      slow_to_summarize_test_base& operator=(slow_to_summarize_test_base&&) noexcept = default;
    };

    class slow_to_summarize_test_0 final : public slow_to_summarize_test_base
    {
    public:
      using slow_to_summarize_test_base::slow_to_summarize_test_base;
    };

    class slow_to_summarize_test_1 final : public slow_to_summarize_test_base
    {
    public:
      using slow_to_summarize_test_base::slow_to_summarize_test_base;
    };

    class slow_to_summarize_test_2 final : public slow_to_summarize_test_base
    {
    public:
      using slow_to_summarize_test_base::slow_to_summarize_test_base;
    };

    class unparallelizable_slow_to_summarize_test final : public slow_to_summarize_test_base
    {
    public:
      using parallelizable_type = std::false_type;

      using slow_to_summarize_test_base::slow_to_summarize_test_base;
    };

    constexpr std::size_t too_few_trials{relative_performance_minimum_trials_floor - 1};

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
        using namespace std::chrono_literals;

        const relative_performance_parameters parameters{.prediction{.lower{2}, .upper{40}}};

        check_relative_performance("A speed-up of 1, then of 8",
                                   counted_spinner{1ms},
                                   counted_spinner{1ms, trials_for_minimum_10[0], 8ms},
                                   parameters);

        check_exception_thrown<std::invalid_argument>(
          "Too few trials are refused",
          [this, &parameters]() {
            return check_relative_performance("Too few trials",
                                              counted_spinner{1ms},
                                              counted_spinner{8ms},
                                              {.prediction{parameters.prediction}, .minimum_trials{too_few_trials}});
          }
        );

        check_exception_thrown<std::runtime_error>(
          "A task which throws ends the check",
          [this, &parameters]() {
            return check_relative_performance("A throwing task",
                                              []() { throw std::runtime_error{"Fast task failure"}; },
                                              counted_spinner{8ms},
                                              parameters);
          }
        );

        check_relative_performance("A speed-up of 8", counted_spinner{1ms}, counted_spinner{8ms}, parameters);
        check_relative_performance("A speed-up of 1", counted_spinner{1ms}, counted_spinner{1ms}, parameters);
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
        using namespace std::chrono_literals;

        const relative_performance_parameters parameters{.prediction{.lower{2}, .upper{40}}};

        check_relative_performance("A speed-up of 8", counted_spinner{1ms}, counted_spinner{8ms}, parameters);

        check_exception_thrown<std::invalid_argument>(
          "Too few trials are refused",
          [this, &parameters]() {
            return check_relative_performance("Too few trials",
                                              counted_spinner{1ms},
                                              counted_spinner{8ms},
                                              {.prediction{parameters.prediction}, .minimum_trials{too_few_trials}});
          }
        );

        check_exception_thrown<std::runtime_error>(
          "A nested performance check's failure is the enclosing check's failure, and a check may follow it",
          [this, &parameters]() {
            check_relative_performance("A speed-up of 0.25", counted_spinner{4ms}, counted_spinner{1ms}, parameters);
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
        using namespace std::chrono_literals;

        const relative_performance_parameters parameters{.prediction{.lower{2}, .upper{40}}};

        check_relative_performance("A speed-up of 8", counted_spinner{1ms}, counted_spinner{8ms}, parameters);

        check_exception_thrown<std::invalid_argument>(
          "Too few trials are refused",
          [this, &parameters]() {
            return check_relative_performance("Too few trials",
                                              counted_spinner{1ms},
                                              counted_spinner{8ms},
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
        using namespace std::chrono_literals;

        const relative_performance_parameters parameters{.prediction{.lower{2}, .upper{40}}};

        check_relative_performance("A speed-up of 1", counted_spinner{1ms}, counted_spinner{1ms}, parameters);

        check_exception_thrown<std::runtime_error>(
          "Too few trials are refused, but not with the exception expected",
          [this, &parameters]() {
            return check_relative_performance("Too few trials",
                                              counted_spinner{1ms},
                                              counted_spinner{8ms},
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

    test_runner make_runner(commandline_arguments& args, std::stringstream& outputStream)
    {
      return test_runner{args.size(),
                         args.get(),
                         "Oliver J. Rosten",
                         "  ",
                         {.main_cpp{"TestSandbox/TestSandbox.cpp"}, .common_includes{"TestShared/SharedIncludes.hpp"}},
                         outputStream};
    }

    test_runner make_slow_suite(commandline_arguments args, std::stringstream& outputStream)
    {
      auto runner{make_runner(args, outputStream)};
      slow_test_base::clear_execution_intervals();

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

  fs::path test_runner_performance_test::check_output(std::string_view description, std::string_view dirName, std::stringstream& output)
  {
    fs::path filePath{write(dirName, output)};
    check(equivalence, description, working_materials() /= dirName, predictive_materials() /= dirName);

    return filePath;
  }

  fs::path test_runner_performance_test::write(std::string_view dirName, std::stringstream& output) const
  {
    const auto outputDir{working_materials() /= dirName};
    fs::create_directory(outputDir);

    const auto filePath{outputDir / "io.txt"};
    write_to_file(filePath, output.str(), std::ios_base::out);

    output.str("");

    return filePath;
  }

  void test_runner_performance_test::run_tests()
  {
    test_parallel_acceleration();
    test_thread_pool_acceleration();
    test_serial_execution();
    test_runner_overhead_reported_apart();
    test_execution_duration_of_busiest_thread();
    test_recording_of_performance_check_attempts();
    test_summary_of_performance_checks_within_other_checks();
  }

  void test_runner_performance_test::test_parallel_acceleration()
  {
    std::stringstream outputStream{};
    auto runner{make_slow_suite({{(minimal_fake_path()).generic_string()}}, outputStream)};
    check(equality, "Parallel acceleration return code", runner.execute(), return_code::success);

    const auto peak{peak_overlap(slow_test_base::execution_intervals())};
    outputStream << peak_overlap_line(peak);

    auto outputFile{check_output(report({"Parallel Acceleration Output"}), "ParallelAccelerationOutput", outputStream)};
    check(within_tolerance{35.0}, "", get_grand_total(outputFile, "Execution Time"), 60.0);

    // Without parallel algorithms, the runner runs the tests on a pool of eight threads instead
    check(std::ranges::greater_equal{},
          "Tests execute at once in parallel",
          peak,
          has_parallel_algorithms_v ? std::ptrdiff_t{2} : eight_thread_peak_floor);
  }

  void test_runner_performance_test::test_thread_pool_acceleration()
  {
    {
      std::stringstream outputStream{};
      auto runner{make_slow_suite({{(minimal_fake_path()).generic_string(), "--thread-pool", "8"}}, outputStream)};
      check(equality, "Thread pool (8) return code", runner.execute(), return_code::success);

      const auto peak{peak_overlap(slow_test_base::execution_intervals())};
      outputStream << peak_overlap_line(peak);

      auto outputFile{check_output(report({"Thread Pool (8) Acceleration Output"}), "ThreadPool8AccelerationOutput", outputStream)};
      check(within_tolerance{30.0}, "", get_grand_total(outputFile, "Execution Time"), 55.0);
      check(std::ranges::greater_equal{},
            "More than half of the pool's eight threads execute tests at once",
            peak,
            eight_thread_peak_floor);
    }

    {
      std::stringstream outputStream{};
      auto runner{make_slow_suite({{(minimal_fake_path()).generic_string(), "--thread-pool", "2"}}, outputStream)};
      check(equality, "Thread pool (2) return code", runner.execute(), return_code::success);

      const auto peak{peak_overlap(slow_test_base::execution_intervals())};
      outputStream << peak_overlap_line(peak);

      auto outputFile{check_output(report({"Thread Pool (2) Acceleration Output"}), "ThreadPool2AccelerationOutput", outputStream)};
      check(within_tolerance{40.0}, "", get_grand_total(outputFile, "Execution Time"), 140.0);
      check(equality, "Both threads of the pool, and no more, execute tests at once", peak, std::ptrdiff_t{2});
    }
  }

  void test_runner_performance_test::test_serial_execution()
  {
    std::stringstream outputStream{};
    auto runner{make_slow_suite({{(minimal_fake_path()).generic_string(), "--serial"}}, outputStream)};
    check(equality, "Serial execution return code", runner.execute(), return_code::success);

    const auto peak{peak_overlap(slow_test_base::execution_intervals())};
    outputStream << peak_overlap_line(peak);

    auto outputFile{check_output(report({"Serial Output"}), "Serial Output", outputStream)};
    check(within_tolerance{55.0}, "", get_grand_total(outputFile, "Execution Time"), 255.0);
    check(equality, "Tests execute one at a time in a serial run", peak, std::ptrdiff_t{1});
  }

  /** One test sleeps while its tests run, and again while the runner summarizes it. Its execution duration must
      lie between the first sleep and the sum of the two sleeps. The sum is what the execution duration would be if
      it included the runner's overhead. The printed output and the test's execution record must both say so.
   */
  void test_runner_performance_test::test_runner_overhead_reported_apart()
  {
    std::stringstream outputStream{};
    commandline_arguments args{{minimal_fake_path().generic_string(), "--serial"}};
    auto runner{make_runner(args, outputStream)};
    runner.register_test<slow_to_summarize_test_0>();
    check(equality, "Runner overhead return code", runner.execute(), return_code::success);

    const auto outputFile{check_output(report({"Runner Overhead Output"}), "RunnerOverheadOutput", outputStream)};
    check(within_tolerance{summarizing_sleep_ms / 2},
          "The printed execution duration excludes the sleep while summarizing",
          get_grand_total(outputFile, "Execution Time"),
          execution_sleep_ms + summarizing_sleep_ms / 2);

    check(std::ranges::greater_equal{},
          "The printed runner overhead includes the sleep while summarizing",
          get_grand_total(outputFile, "Runner Overhead"),
          summarizing_sleep_ms);

    check(within_tolerance{summarizing_sleep_ms / 2},
          "The recorded execution duration excludes the sleep while summarizing",
          in_milliseconds(get_recorded_completion<slow_to_summarize_test_0>(runner).execution_duration),
          execution_sleep_ms + summarizing_sleep_ms / 2);

    check(std::ranges::greater_equal{},
          "The recorded runner overhead includes the sleep while summarizing",
          in_milliseconds(get_recorded_completion<slow_to_summarize_test_0>(runner).runner_overhead),
          summarizing_sleep_ms);
  }

  /** Four tests, each sleeping while its tests run and again while the runner summarizes it. One is not
      parallelizable, so it runs first, alone. The other three then run over a pool of two threads, so one thread runs
      two of them. The run's execution duration must therefore be the first test's recorded execution duration plus
      those of two others. Each rival misses by at least a test's execution duration, far more than the printed
      duration's rounding:
      -# summing every test's execution duration adds the third of the others;
      -# leaving out the first test subtracts its execution duration;
      -# taking the longest test rather than the busiest thread subtracts the execution duration of one of a pair;
      -# a wall clock adds the runner's overhead.
   */
  void test_runner_performance_test::test_execution_duration_of_busiest_thread()
  {
    std::stringstream outputStream{};
    commandline_arguments args{{minimal_fake_path().generic_string(), "--thread-pool", "2"}};
    auto runner{make_runner(args, outputStream)};
    runner.register_test<unparallelizable_slow_to_summarize_test>();
    runner.register_test<slow_to_summarize_test_0>();
    runner.register_test<slow_to_summarize_test_1>();
    runner.register_test<slow_to_summarize_test_2>();
    check(equality, "Busiest thread return code", runner.execute(), return_code::success);

    const auto outputFile{check_output(report({"Busiest Thread Output"}), "BusiestThreadOutput", outputStream)};

    const auto firstDuration{
      in_milliseconds(get_recorded_completion<unparallelizable_slow_to_summarize_test>(runner).execution_duration)
    };
    const std::array othersDurations{
      in_milliseconds(get_recorded_completion<slow_to_summarize_test_0>(runner).execution_duration),
      in_milliseconds(get_recorded_completion<slow_to_summarize_test_1>(runner).execution_duration),
      in_milliseconds(get_recorded_completion<slow_to_summarize_test_2>(runner).execution_duration)
    };

    const std::array pairedDurations{
      othersDurations[0] + othersDurations[1],
      othersDurations[0] + othersDurations[2],
      othersDurations[1] + othersDurations[2]
    };

    const auto executionDuration{get_grand_total(outputFile, "Execution Time")};
    auto distanceFromExecutionDuration{
      [executionDuration, firstDuration](double paired){
        return std::abs(executionDuration - (firstDuration + paired));
      }
    };

    const auto nearestPairedDuration{
      std::ranges::min(pairedDurations, std::ranges::less{}, distanceFromExecutionDuration)
    };
    check(within_tolerance{printed_duration_tolerance_ms},
          "The execution duration is the unparallelizable test's plus those of the two others one thread ran",
          executionDuration,
          firstDuration + nearestPairedDuration);

    check(std::ranges::greater_equal{},
          "The runner overhead includes the summarizing sleeps of the unparallelizable test and the busiest thread",
          get_grand_total(outputFile, "Runner Overhead"),
          3 * summarizing_sleep_ms);
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
    auto runner{make_runner(args, outputStream)};
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
    auto runner{make_runner(args, outputStream)};
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
