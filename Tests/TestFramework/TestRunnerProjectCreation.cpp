////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TestRunnerProjectCreation.hpp"
#include "TestRunnerDiagnosticsUtilities.hpp"
#include "CommandLineArgumentsTestingUtilities.hpp"

#include <cstdlib>

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view test_runner_project_creation::source_file() const noexcept
  {
    return __FILE__;
  }

  void test_runner_project_creation::run_tests()
  {
    test_project_creation();
  }

  void test_runner_project_creation::test_project_creation()
  {
    auto fake{
      [&mat{working_materials()}]() {
        return mat / "FakeProject";
      }
    };
    
    const auto testMain{fake().append("TestSandbox").append("TestSandbox.cpp")};
    const auto includeTarget{fake().append("TestShared").append("SharedIncludes.hpp")};

    const repositories repos{fake()};

    auto generated{
      [&mat{working_materials()}]() {
        return mat / "GeneratedProject";
      }
    };

    // The first argument is set to ensure that the project root is deduced as
    // Sequoia, in order that the aux_files are correctly located

    commandline_arguments args{"../../build/foo/bar", "init", "Oliver Jacob Rosten", generated().string()};

    std::stringstream outputStream{};
    test_runner tr{args.size(), args.get(), "Oliver J. Rosten", testMain, includeTarget, repos, outputStream};

    tr.execute();

    if(std::ofstream file{fake() / "output" / "io.txt"})
    {
      file << outputStream.rdbuf();
    }

    check_equivalence(LINE(""), generated(), predictive_materials() / "GeneratedProject");
    check_equivalence(LINE(""), fake(), predictive_materials() / "FakeProject");

    /*const auto cd{std::string{"cd "}.append((generated() / "TestAll").string()).append("\n")};
    const auto cmake{"cmake -S . -B ../build/CMade/TestAll -D CMAKE_CXX_COMPILER=/usr/local/opt/llvm/bin/clang++ > CMakeOutput.txt\n"};
    const auto make{std::string{"cd "}.append((generated() / "build" / "CMade" / "TestAll").string()).append("\nmake > BuildOutput.txt")};

    std::system((cd + cmake + make).c_str());
    */
  }
}
