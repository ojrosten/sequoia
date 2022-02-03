////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "TestRunnerProjectCreation.hpp"
#include "TestRunnerDiagnosticsUtilities.hpp"
#include "Parsing/CommandLineArgumentsTestingUtilities.hpp"

#include <cstdlib>
#include <fstream>

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view test_runner_project_creation::source_file() const noexcept
  {
    return __FILE__;
  }

  void test_runner_project_creation::run_tests()
  {
    test_exceptions();
    test_project_creation();
  }

  void test_runner_project_creation::test_exceptions()
  {
    check_exception_thrown<std::runtime_error>(
      LINE("Project name with space"),
      [this]() {
        commandline_arguments args{"", "init", "Oliver Jacob Rosten", (working_materials() / "Generated Project").string(), "  "};

        std::stringstream outputStream{};
        test_runner tr{args.size(), args.get(), "Oliver J. Rosten", make_project_paths(), "  ", outputStream};

        tr.execute();
      });

    check_exception_thrown<std::runtime_error>(
      LINE("Project name with $"),
      [this]() {
        commandline_arguments args{"", "init", "Oliver Jacob Rosten", (working_materials() / "Generated$Project").string(), "  "};

        std::stringstream outputStream{};
        test_runner tr{args.size(), args.get(), "Oliver J. Rosten", make_project_paths(), "  ", outputStream};

        tr.execute();
      });

    check_exception_thrown<std::runtime_error>(
      LINE("Project path that is not absolute"),
      [this]() {
        commandline_arguments args{"", "init", "Oliver Jacob Rosten", "Generated_Project", "  "};

        std::stringstream outputStream{};
        test_runner tr{args.size(), args.get(), "Oliver J. Rosten", make_project_paths(), "  ", outputStream};

        tr.execute();
      });

    check_exception_thrown<std::runtime_error>(
      LINE("Empty project path"),
      [this]() {
        commandline_arguments args{"", "init", "Oliver Jacob Rosten", "", "  "};

        std::stringstream outputStream{};
        test_runner tr{args.size(), args.get(), "Oliver J. Rosten", make_project_paths(), "  ", outputStream};

        tr.execute();
      });

    check_exception_thrown<std::runtime_error>(
      LINE("Project name clash"),
      [this]() {
        commandline_arguments args{"", "init", "Oliver Jacob Rosten", working_materials().string(), "  "};

        std::stringstream outputStream{};
        test_runner tr{args.size(), args.get(), "Oliver J. Rosten", make_project_paths(), "  ", outputStream};

        tr.execute();
      },
      [](std::string mess){
        const auto pos{mess.find("output")};
        if(pos != std::string::npos)
          mess.erase(0, pos);
        
        return mess;
      });

    check_exception_thrown<std::runtime_error>(
      LINE("Illegal indent"),
      [this]() {
        commandline_arguments args{"", "init", "Oliver Jacob Rosten", (working_materials() / "GeneratedProject").string(), "  "};

        std::stringstream outputStream{};
        test_runner tr{args.size(), args.get(), "Oliver J. Rosten", make_project_paths(), "\t  x", outputStream};

        tr.execute();
      });

    check_exception_thrown<std::runtime_error>(
      LINE("Illegal init indent"),
      [this]() {
        commandline_arguments args{"", "init", "Oliver Jacob Rosten", (working_materials() / "GeneratedProject").string(), "\n"};

        std::stringstream outputStream{};
        test_runner tr{args.size(), args.get(), "Oliver J. Rosten", make_project_paths(), "  ", outputStream};

        tr.execute();
      });
  }


  project_paths test_runner_project_creation::make_project_paths() const
  {
    const auto testMain{fake_project().append("TestSandbox").append("TestSandbox.cpp")};
    const auto includeTarget{fake_project().append("TestShared").append("SharedIncludes.hpp")};

    return {fake_project(), testMain, includeTarget};
  }

  std::filesystem::path test_runner_project_creation::fake_project() const
  {
    return working_materials() / "FakeProject";
  }


  void test_runner_project_creation::test_project_creation()
  {
    namespace fs = std::filesystem;
    fs::copy(aux_files_path(test_repository().parent_path()), aux_files_path(fake_project()), fs::copy_options::recursive);

    {
      auto generated{
        [&mat{working_materials()}] () { return mat / "GeneratedProject"; }
      };

      commandline_arguments args{"", "init", "Oliver Jacob Rosten", generated().string(), "  ", "--no-build"};

      std::stringstream outputStream{};
      test_runner tr{args.size(), args.get(), "Oliver J. Rosten", make_project_paths(), "\t", outputStream};

      tr.execute();

      if(std::ofstream file{fake_project() / "output" / "io.txt"})
      {
        file << outputStream.rdbuf();
      }

      check(equivalence, LINE(""), generated(), predictive_materials() / "GeneratedProject");
      check(equivalence, LINE(""), fake_project(), predictive_materials() / "FakeProject");
    }

    {
      const auto hostDir{working_materials() / "AnotherGeneratedProject"};
      commandline_arguments args{"", "init", "Oliver Jacob Rosten", hostDir.generic_string(), "  ", "--no-build"};

      std::stringstream outputStream{};
      test_runner tr{args.size(), args.get(), "Oliver J. Rosten", make_project_paths(), "\t", outputStream};

      tr.execute();

      check(equivalence, LINE(""), hostDir, predictive_materials() / "AnotherGeneratedProject");
    }
  }
}
