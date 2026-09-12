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

#include <charconv>
#include <fstream>

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
    
    [[nodiscard]]
    double get_timing(const std::filesystem::path& file)
    {
      if(const auto optContents{read_to_string(file, std::ios_base::in)})
      {
        std::string_view contents{optContents.value()};
        constexpr std::string_view pattern{"Execution Time:"};

        // From the grand totals, since every test reports a time of its own and the run's total is
        // the only one this measures.
        if(auto pos{contents.find(pattern, contents.find("Grand Totals"))}; pos != std::string::npos)
        {
          auto start{pos + pattern.size() + 1};
          if(auto end{contents.find("ms]", start)}; end > start)
          {
            auto timing{contents.substr(start, end - start)};
            return to_number<double>(timing);
          }
        }
      }

      throw std::runtime_error{"Unable to extract timing from: " + file.generic_string()};
    }

    /** Eight tests are wanted, each sleeping the same amount, so that the timings below can
        measure how the runner schedules them. They are eight *classes* because a test's name is
        synthesized from its class: a class template cannot supply a file-system-safe name, and
        registering one class twice - which is what these used to do - would ask two tests to
        share it.
     */

    class slow_test_base : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static std::filesystem::path source_file()
      {
        return std::source_location::current().file_name();
      }
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
        std::this_thread::sleep_for(25ms);
        check(equality, {"Integer equality"}, index, index);
      }
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

    test_runner make_slow_suite(commandline_arguments args, std::stringstream& outputStream)
    {
      test_runner runner{args.size(),
                         args.get(),
                         "Oliver J. Rosten",
                         "  ",
                         {.main_cpp{"TestSandbox/TestSandbox.cpp"}, .common_includes{"TestShared/SharedIncludes.hpp"}},
                         outputStream};

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
  std::filesystem::path test_runner_performance_test::source_file()
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
  }

  void test_runner_performance_test::test_parallel_acceleration()
  {
    std::stringstream outputStream{};
    auto runner{make_slow_suite({{(minimal_fake_path()).generic_string()}}, outputStream)};
    check(equality, "Parallel acceleration return code", runner.execute(), return_code::success);

    auto outputFile{check_output(report({"Parallel Acceleration Output"}), "ParallelAccelerationOutput", outputStream)};
    check(within_tolerance{35.0}, "", get_timing(outputFile), 60.0);
  }

  void test_runner_performance_test::test_thread_pool_acceleration()
  {
    {
      std::stringstream outputStream{};
      auto runner{make_slow_suite({{(minimal_fake_path()).generic_string(), "--thread-pool", "8"}}, outputStream)};
      check(equality, "Thread pool (8) return code", runner.execute(), return_code::success);

      auto outputFile{check_output(report({"Thread Pool (8) Acceleration Output"}), "ThreadPool8AccelerationOutput", outputStream)};
      check(within_tolerance{30.0}, "", get_timing(outputFile), 55.0);
    }

    {
      std::stringstream outputStream{};
      auto runner{make_slow_suite({{(minimal_fake_path()).generic_string(), "--thread-pool", "2"}}, outputStream)};
      check(equality, "Thread pool (2) return code", runner.execute(), return_code::success);

      auto outputFile{check_output(report({"Thread Pool (2) Acceleration Output"}), "ThreadPool2AccelerationOutput", outputStream)};
      check(within_tolerance{40.0}, "", get_timing(outputFile), 140.0);
    }
  }

  void test_runner_performance_test::test_serial_execution()
  {
    std::stringstream outputStream{};
    auto runner{make_slow_suite({{(minimal_fake_path()).generic_string(), "--serial"}}, outputStream)};
    check(equality, "Serial execution return code", runner.execute(), return_code::success);

    auto outputFile{check_output(report({"Serial Output"}), "Serial Output", outputStream)};
    check(within_tolerance{55.0}, "", get_timing(outputFile), 255.0);
  }
}
