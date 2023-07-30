////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "TestRunnerPerformanceTest.hpp"
#include "Parsing/CommandLineArgumentsTestingUtilities.hpp"

#include "sequoia/TestFramework/TestRunner.hpp"

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
      if(const auto optContents{read_to_string(file)})
      {
        std::string_view contents{optContents.value()};
        constexpr std::string_view pattern{"Execution Time:"};
        if(auto pos{contents.find(pattern)}; pos != std::string::npos)
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

    template<std::size_t I>
    class slow_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      std::filesystem::path source_file() const
      {
        return std::source_location::current().file_name();
      }

      void run_tests()
      {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(25ms);
        check(equality, "Integer equality", I, I);
      }
    };

    test_runner make_slow_suite(commandline_arguments args, std::stringstream& outputStream)
    {
      test_runner runner{args.size(),
                         args.get(),
                         "Oliver J. Rosten",
                         {"TestSandbox/TestSandbox.cpp", {}, "TestShared/SharedIncludes.hpp"},
                         "  ",
                         outputStream};

      runner.add_test_suite(
        "Slow Suite",
        slow_test<0>{"Slow test 0"},
        slow_test<1>{"Slow test 1"},
        slow_test<2>{"Slow test 2"},
        slow_test<3>{"Slow test 3"},
        slow_test<0>{"Slow test 4"},
        slow_test<1>{"Slow test 5"},
        slow_test<2>{"Slow test 6"},
        slow_test<3>{"Slow test 7"}
      );

      return runner;
    }
  }

  [[nodiscard]]
  std::filesystem::path test_runner_performance_test::source_file() const
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
    return fake_project().append("build/CMade");
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
    auto runner{make_slow_suite({(minimal_fake_path()).generic_string()}, outputStream)};
    runner.execute();

    auto outputFile{check_output(report_line("Parallel Acceleration Output"), "ParallelAccelerationOutput", outputStream)};
    check(within_tolerance{35.0}, report_line(""), get_timing(outputFile), 60.0);
  }

  void test_runner_performance_test::test_thread_pool_acceleration()
  {
    {
      std::stringstream outputStream{};
      auto runner{make_slow_suite({(minimal_fake_path()).generic_string(), "--thread-pool", "8"}, outputStream)};
      runner.execute();

      auto outputFile{check_output(report_line("Thread Pool (8) Acceleration Output"), "ThreadPool8AccelerationOutput", outputStream)};
      check(within_tolerance{15.0}, report_line(""), get_timing(outputFile), 40.0);
    }

    {
      std::stringstream outputStream{};
      auto runner{make_slow_suite({(minimal_fake_path()).generic_string(), "--thread-pool", "2"}, outputStream)};
      runner.execute();

      auto outputFile{check_output(report_line("Thread Pool (2) Acceleration Output"), "ThreadPool2AccelerationOutput", outputStream)};
      check(within_tolerance{25.0}, report_line(""), get_timing(outputFile), 125.0);
    }
  }

  void test_runner_performance_test::test_serial_execution()
  {
    std::stringstream outputStream{};
    auto runner{make_slow_suite({(minimal_fake_path()).generic_string(), "--serial"}, outputStream)};
    runner.execute();

    auto outputFile{check_output(report_line("Serial Output"), "Serial Output", outputStream)};
    check(within_tolerance{20.0}, report_line(""), get_timing(outputFile), 220.0);
  }
}
