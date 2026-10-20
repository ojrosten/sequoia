////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TestRunnerSerialExecutionFreeTest.hpp"
#include "FakeProjectTestingUtilities.hpp"
#include "TestRunnerConcurrencyTestingUtilities.hpp"

#include "sequoia/TestFramework/TestRunner.hpp"
#include "sequoia/Streaming/Streaming.hpp"

#include <cstddef>

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  [[nodiscard]]
  fs::path test_runner_serial_execution_free_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void test_runner_serial_execution_free_test::check_output(std::string_view description,
                                                           std::string_view dirName,
                                                           const std::stringstream& output)
  {
    const auto outputDir{working_materials() /= dirName};
    fs::create_directory(outputDir);
    write_to_file(outputDir / "io.txt", output.view(), std::ios_base::out);
    check(equivalence, description, outputDir, predictive_materials() /= dirName);
  }

  void test_runner_serial_execution_free_test::run_tests()
  {
    test_serial_execution();
  }

  void test_runner_serial_execution_free_test::test_serial_execution()
  {
    std::stringstream outputStream{};
    auto runner{make_slow_suite(fake_project_commandline(auxiliary_materials(), "--serial"),
                                outputStream,
                                sleep_duration_for_overlap_checks)};
    check(equality, "Serial execution return code", runner.execute(), return_code::success);

    const auto peak{peak_overlap(slow_test_base::execution_intervals())};
    outputStream << peak_overlap_line(peak);

    check_output(report({"Serial Output"}), "Serial Output", outputStream);
    check(equality, "Tests execute one at a time in a serial run", peak, std::size_t{1});
  }
}
