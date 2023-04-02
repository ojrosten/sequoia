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

#include <fstream>

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  namespace
  {
    double get_timing(const std::filesystem::path& file)
    {
      if(const auto optContents{read_to_string(file)})
      {
        const auto& contents{optContents.value()};
        constexpr std::string_view pattern{"Execution Time:"};
        if(auto pos{contents.find(pattern)}; pos != std::string::npos)
        {
          auto start{pos + pattern.size() + 1};
          if(auto end{contents.find("ms]", start)}; end > start)
          {
            auto timing{contents.substr(start, end - start)};
            double d{};
            std::from_chars(timing.data(), std::next(timing.data(), end - start), d);

            return d;
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
      std::string_view source_file() const noexcept final
      {
        return __FILE__;
      }
    private:
      void run_tests() final
      {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(10ms);
        check(equality, "Integer equality", I, I);
      }
    };

    test_runner make_slow_family(commandline_arguments args, std::stringstream& outputStream)
    {
      test_runner runner{args.size(),
                         args.get(),
                         "Oliver J. Rosten",
                         {"TestSandbox/TestSandbox.cpp", {}, "TestShared/SharedIncludes.hpp"},
                         "  ",
                         outputStream};

      runner.add_test_family(
        "Slow Family",
        slow_test<0>{"Slow test 0"},
        slow_test<1>{"Slow test 1"}
      );

      return runner;
    }
  }

  [[nodiscard]]
  std::string_view test_runner_performance_test::source_file() const noexcept
  {
    return __FILE__;
  }

  [[nodiscard]]
  fs::path test_runner_performance_test::fake_project() const
  {
    return auxiliary_materials() /= "FakeProject";
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
    test_serial_execution();
  }

  void test_runner_performance_test::test_parallel_acceleration()
  {
    std::stringstream outputStream{};
    auto runner{make_slow_family({(fake_project() / "build").generic_string()}, outputStream)};
    runner.execute();

    auto outputFile{check_output(LINE("Parallel Acceleration Output"), "ParallelAccelerationOutput", outputStream)};
    check(within_tolerance{2.0}, LINE(""), get_timing(outputFile), 10.0);
  }

  void test_runner_performance_test::test_serial_execution()
  {
    std::stringstream outputStream{};
    auto runner{make_slow_family({(fake_project() / "build").generic_string(), "--async-depth", "null"}, outputStream)};
    runner.execute();

    auto outputFile{check_output(LINE("Serial Output"), "Serial Output", outputStream)};
    check(within_tolerance{3.0}, LINE(""), get_timing(outputFile), 20.0);
  }
}
