////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TestRunnerProjectCreation.hpp"
#include "TestRunnerDiagnosticsUtilities.hpp"
#include "Parsing/CommandLineArgumentsTestingUtilities.hpp"

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
      [&mat{working_materials()}]() { return mat / "FakeProject"; }
    };

    namespace fs = std::filesystem;
    fs::copy(aux_files_path(test_repository().parent_path()), aux_files_path(fake()), fs::copy_options::recursive);

    const auto testMain{fake().append("TestSandbox").append("TestSandbox.cpp")};
    const auto includeTarget{fake().append("TestShared").append("SharedIncludes.hpp")};

    const project_paths repos{fake()};

    auto generated{
      [&mat{working_materials()}]() { return mat / "GeneratedProject"; }
    };

    check_exception_thrown<std::runtime_error>(
      LINE(""),
      [&](){
        commandline_arguments args{"", "init", "Oliver Jacob Rosten", (working_materials() / "Generated Project").string()};

        std::stringstream outputStream{};
        test_runner tr{args.size(), args.get(), "Oliver J. Rosten", testMain, includeTarget, repos, outputStream};

        tr.execute();
      });

    commandline_arguments args{"", "init", "Oliver Jacob Rosten", generated().string()};

    std::stringstream outputStream{};
    test_runner tr{args.size(), args.get(), "Oliver J. Rosten", testMain, includeTarget, repos, outputStream};

    tr.execute();

    if(std::ofstream file{fake() / "output" / "io.txt"})
    {
      file << outputStream.rdbuf();
    }

    check_equivalence(LINE(""), generated(), predictive_materials() / "GeneratedProject");
    check_equivalence(LINE(""), fake(), predictive_materials() / "FakeProject");
  }
}
