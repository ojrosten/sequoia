////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TestRunnerPerformanceTest.hpp"
#include "Parsing/CommandLineArgumentsTestingUtilities.hpp"

#include "sequoia/TestFramework/TestRunner.hpp"
#include "sequoia/Streaming/Streaming.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <format>
#include <fstream>
#include <ranges>
#include <span>
#include <vector>

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  namespace
  {
    template<class T>
    [[nodiscard]]
    T to_number(std::string_view timing)
    {
      if constexpr(!with_clang_v)
      {
        T x{};
        if(std::from_chars(timing.data(), std::ranges::next(timing.data(), timing.size()), x).ec == std::errc{})
          return x;
      }
      else
      {
        T x{};
        std::stringstream ss{std::string{timing}};
        if(ss >> x) return x;
      }

      throw std::runtime_error{"Unable to extract timing from: " + std::string{timing}};
    }
    
    /** A time as the runner prints it, a number followed by its unit, converted to milliseconds. */
    [[nodiscard]]
    double to_milliseconds(std::string_view time)
    {
      constexpr std::array<std::pair<std::string_view, double>, 4> millisecondsPerUnit{
        {{"s", 1e3}, {"ms", 1.0}, {"us", 1e-3}, {"ns", 1e-6}}
      };

      const auto unitStart{std::ranges::min(time.find_first_not_of("0123456789.e+"), time.size())};
      auto unitOf{[](const auto& entry){ return entry.first; }};
      const auto conversion{std::ranges::find(millisecondsPerUnit, time.substr(unitStart), unitOf)};
      if(conversion == millisecondsPerUnit.end())
        throw std::runtime_error{std::format("Unable to read the unit of the time {}", time)};

      return to_number<double>(time.substr(0, unitStart)) * conversion->second;
    }

    /** The run's time labelled `label`, such as "Execution Time", in milliseconds. It is read from the grand
        totals, since every test reports times of its own and the run's are the only ones these checks measure.
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

    /** The time labelled `label`, such as "execution time", in the execution record `runner` keeps for `Test`, in
        milliseconds.
     */
    template<concrete_test Test>
    [[nodiscard]]
    double get_recorded_time(const test_runner& runner, std::string_view label)
    {
      const test_execution_record_path record{Test::source_file(), test_name<Test>(), runner.proj_paths()};
      std::ifstream file{record.file_path()};
      for(std::string line{}; std::getline(file, line);)
      {
        if(line.starts_with(label))
          return to_milliseconds(std::string_view{line}.substr(label.size() + 1));
      }

      throw std::runtime_error{
        std::format("Unable to extract the {} from: {}", label, record.file_path().generic_string())
      };
    }

    /** When a test's tests began and ended running. */
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
      using change_type = std::pair<std::chrono::steady_clock::time_point, std::ptrdiff_t>;
      auto endpoints{
        [](const execution_interval& interval){
          return std::array{change_type{interval.start, 1}, change_type{interval.end, -1}};
        }
      };

      // At one instant an end, -1, sorts before a start, +1
      auto changes{
          intervals
        | std::views::transform(endpoints)
        | std::views::join
        | std::ranges::to<std::vector>()
      };

      std::ranges::sort(changes);

      std::ptrdiff_t overlap{}, peak{};
      for(const auto& change : changes)
      {
        overlap += change.second;
        peak = std::ranges::max(peak, overlap);
      }

      return peak;
    }

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
        check(equality, {"Integer equality"}, index, index);
      }
    private:
      // Each test writes only its own element, and the checks read them once the run has joined its threads
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

    /** The runner prints a time to three significant figures, so it prints one below a second to within half a
        millisecond.
     */
    constexpr double printed_time_tolerance_ms{1.0};

    /** Sleeps while its tests run, and sleeps again while the runner summarizes it. The runner summarizes a test after
        its tests have run, so the first sleep falls in the test's execution time and the second in the runner's
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
    if(std::ofstream file{filePath})
    {
      file << output.str();
    }

    output.str("");

    return filePath;
  }

  void test_runner_performance_test::run_tests()
  {
    test_parallel_acceleration();
    test_thread_pool_acceleration();
    test_serial_execution();
    test_runner_overhead_reported_apart();
    test_execution_time_of_busiest_thread();
  }

  void test_runner_performance_test::test_parallel_acceleration()
  {
    std::stringstream outputStream{};
    auto runner{make_slow_suite({{(minimal_fake_path()).generic_string()}}, outputStream)};
    check(equality, "Parallel acceleration return code", runner.execute(), return_code::success);

    auto outputFile{check_output(report({"Parallel Acceleration Output"}), "ParallelAccelerationOutput", outputStream)};
    check(within_tolerance{35.0}, "", get_grand_total(outputFile, "Execution Time"), 60.0);
    check(std::ranges::greater_equal{},
          "Tests execute at once in parallel",
          peak_overlap(slow_test_base::execution_intervals()),
          std::ptrdiff_t{2});
  }

  void test_runner_performance_test::test_thread_pool_acceleration()
  {
    {
      std::stringstream outputStream{};
      auto runner{make_slow_suite({{(minimal_fake_path()).generic_string(), "--thread-pool", "8"}}, outputStream)};
      check(equality, "Thread pool (8) return code", runner.execute(), return_code::success);

      auto outputFile{check_output(report({"Thread Pool (8) Acceleration Output"}), "ThreadPool8AccelerationOutput", outputStream)};
      check(within_tolerance{30.0}, "", get_grand_total(outputFile, "Execution Time"), 55.0);
      // One test may start after the others have finished, delayed by its execution record, without failing this
      check(std::ranges::greater_equal{},
            "At least seven of the pool's eight threads execute tests at once",
            peak_overlap(slow_test_base::execution_intervals()),
            std::ptrdiff_t{7});
    }

    {
      std::stringstream outputStream{};
      auto runner{make_slow_suite({{(minimal_fake_path()).generic_string(), "--thread-pool", "2"}}, outputStream)};
      check(equality, "Thread pool (2) return code", runner.execute(), return_code::success);

      auto outputFile{check_output(report({"Thread Pool (2) Acceleration Output"}), "ThreadPool2AccelerationOutput", outputStream)};
      check(within_tolerance{40.0}, "", get_grand_total(outputFile, "Execution Time"), 140.0);
      check(equality,
            "Both threads of the pool, and no more, execute tests at once",
            peak_overlap(slow_test_base::execution_intervals()),
            std::ptrdiff_t{2});
    }
  }

  void test_runner_performance_test::test_serial_execution()
  {
    std::stringstream outputStream{};
    auto runner{make_slow_suite({{(minimal_fake_path()).generic_string(), "--serial"}}, outputStream)};
    check(equality, "Serial execution return code", runner.execute(), return_code::success);

    auto outputFile{check_output(report({"Serial Output"}), "Serial Output", outputStream)};
    check(within_tolerance{55.0}, "", get_grand_total(outputFile, "Execution Time"), 255.0);
    check(equality,
          "Tests execute one at a time in a serial run",
          peak_overlap(slow_test_base::execution_intervals()),
          std::ptrdiff_t{1});
  }

  /** One test sleeps while its tests run, and again while the runner summarizes it. Its execution time must lie
      between the first sleep and the sum of the two sleeps. The sum is what the execution time would be if it
      included the runner's overhead. The printed output and the test's execution record must both say so.
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
          "The printed execution time excludes the sleep while summarizing",
          get_grand_total(outputFile, "Execution Time"),
          execution_sleep_ms + summarizing_sleep_ms / 2);

    check(std::ranges::greater_equal{},
          "The printed runner overhead includes the sleep while summarizing",
          get_grand_total(outputFile, "Runner Overhead"),
          summarizing_sleep_ms);

    check(within_tolerance{summarizing_sleep_ms / 2},
          "The recorded execution time excludes the sleep while summarizing",
          get_recorded_time<slow_to_summarize_test_0>(runner, "execution time"),
          execution_sleep_ms + summarizing_sleep_ms / 2);

    check(std::ranges::greater_equal{},
          "The recorded runner overhead includes the sleep while summarizing",
          get_recorded_time<slow_to_summarize_test_0>(runner, "runner overhead"),
          summarizing_sleep_ms);
  }

  /** Four tests, each sleeping while its tests run and again while the runner summarizes it. One is not
      parallelizable, so it runs first, alone. The other three then run over a pool of two threads, so one thread runs
      two of them. The run's execution time must therefore be the first test's recorded execution time plus those of
      two others. Each rival misses by at least a test's execution time, far more than the printed time's rounding:
      -# summing every test's execution time adds the third of the others;
      -# leaving out the first test subtracts its execution time;
      -# taking the longest test rather than the busiest thread subtracts the execution time of one of a pair;
      -# a wall clock adds the runner's overhead.
   */
  void test_runner_performance_test::test_execution_time_of_busiest_thread()
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

    const auto firstTime{get_recorded_time<unparallelizable_slow_to_summarize_test>(runner, "execution time")};
    const std::array othersTimes{
      get_recorded_time<slow_to_summarize_test_0>(runner, "execution time"),
      get_recorded_time<slow_to_summarize_test_1>(runner, "execution time"),
      get_recorded_time<slow_to_summarize_test_2>(runner, "execution time")
    };

    const std::array pairedTimes{
      othersTimes[0] + othersTimes[1],
      othersTimes[0] + othersTimes[2],
      othersTimes[1] + othersTimes[2]
    };

    const auto executionTime{get_grand_total(outputFile, "Execution Time")};
    auto distanceFromExecutionTime{
      [executionTime, firstTime](double paired){ return std::abs(executionTime - (firstTime + paired)); }
    };

    const auto nearestPairedTime{std::ranges::min(pairedTimes, std::ranges::less{}, distanceFromExecutionTime)};
    check(within_tolerance{printed_time_tolerance_ms},
          "The execution time is the unparallelizable test's plus those of the two others one thread ran",
          executionTime,
          firstTime + nearestPairedTime);

    check(std::ranges::greater_equal{},
          "The runner overhead includes the summarizing sleeps of the unparallelizable test and the busiest thread",
          get_grand_total(outputFile, "Runner Overhead"),
          3 * summarizing_sleep_ms);
  }
}
