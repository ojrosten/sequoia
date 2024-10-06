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
  std::filesystem::path test_runner_project_creation::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void test_runner_project_creation::run_tests()
  {
    test_exceptions();
    test_project_creation();
  }

  void test_runner_project_creation::test_exceptions()
  {
    check_exception_thrown<std::runtime_error>(
      reporter{"Project name with space"},
      [this]() {
        commandline_arguments args{zeroth_arg(), "init", "Oliver Jacob Rosten", (working_materials() /= "Generated Project").string(), "  "};

        std::stringstream outputStream{};
        test_runner tr{args.size(), args.get(), "Oliver J. Rosten", make_project_paths(), "  ", outputStream};

        tr.execute();
      });

    check_exception_thrown<std::runtime_error>(
      reporter{"Project name with $"},
      [this]() {
        commandline_arguments args{zeroth_arg(), "init", "Oliver Jacob Rosten", (working_materials() /= "Generated$Project").string(), "  "};

        std::stringstream outputStream{};
        test_runner tr{args.size(), args.get(), "Oliver J. Rosten", make_project_paths(), "  ", outputStream};

        tr.execute();
      });

    check_exception_thrown<std::runtime_error>(
      reporter{"Project path that is not absolute"},
      [this]() {
        commandline_arguments args{zeroth_arg(), "init", "Oliver Jacob Rosten", "Generated_Project", "  "};

        std::stringstream outputStream{};
        test_runner tr{args.size(), args.get(), "Oliver J. Rosten", make_project_paths(), "  ", outputStream};

        tr.execute();
      });

    check_exception_thrown<std::runtime_error>(
      reporter{"Empty project path"},
      [this]() {
        commandline_arguments args{zeroth_arg(), "init", "Oliver Jacob Rosten", "", "  "};

        std::stringstream outputStream{};
        test_runner tr{args.size(), args.get(), "Oliver J. Rosten", make_project_paths(), "  ", outputStream};

        tr.execute();
      });

    check_exception_thrown<std::runtime_error>(
      reporter{"Project name clash"},
      [this]() {
        commandline_arguments args{zeroth_arg(), "init", "Oliver Jacob Rosten", working_materials().string(), "  "};

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
      reporter{"Illegal indent"},
      [this]() {
        commandline_arguments args{zeroth_arg(), "init", "Oliver Jacob Rosten", (working_materials() /= "GeneratedProject").string(), "  "};

        std::stringstream outputStream{};
        test_runner tr{args.size(), args.get(), "Oliver J. Rosten", make_project_paths(), "\t  x", outputStream};

        tr.execute();
      });

    check_exception_thrown<std::runtime_error>(
      reporter{"Illegal init indent"},
      [this]() {
        commandline_arguments args{zeroth_arg(), "init", "Oliver Jacob Rosten", (working_materials() /= "GeneratedProject").string(), "\n"};

        std::stringstream outputStream{};
        test_runner tr{args.size(), args.get(), "Oliver J. Rosten", make_project_paths(), "  ", outputStream};

        tr.execute();
      });
  }

  [[nodiscard]]
  project_paths::initializer test_runner_project_creation::make_project_paths() const
  {
    return {"TestSandbox/TestSandbox.cpp", {}, "TestShared/SharedIncludes.hpp"};
  }

  [[nodiscard]]
  std::filesystem::path test_runner_project_creation::fake_project() const
  {
    return working_materials() /= "FakeProject";
  }

  [[nodiscard]]
  std::string test_runner_project_creation::zeroth_arg() const
  {
    return (fake_project() / "build/CMade").generic_string();
  }

  void test_runner_project_creation::test_project_creation()
  {
    namespace fs = std::filesystem;
    fs::copy(auxiliary_paths::repo(project_root()), auxiliary_paths::repo(fake_project()), fs::copy_options::recursive);

    {
      const auto hostDir{working_materials() /= "GeneratedProject"};
      commandline_arguments args{zeroth_arg(), "init", "Oliver Jacob Rosten", hostDir.string(), "  ", "--no-git", "--no-build"};

      std::stringstream outputStream{};
      test_runner tr{args.size(), args.get(), "Oliver J. Rosten", make_project_paths(), "\t", outputStream};

      tr.execute();

      if(std::ofstream file{fake_project() / "output" / "io.txt"})
      {
        file << outputStream.rdbuf();
      }

      check(equivalence, "", hostDir, predictive_materials() /= "GeneratedProject");
      check(equivalence, "", fake_project(), predictive_materials() /= "FakeProject");
    }

    {
      const auto hostDir{working_materials() /= "Another_Generated-Project"};
      commandline_arguments args{zeroth_arg(), "init", "Oliver Jacob Rosten", hostDir.generic_string(), "  ", "--no-git", "--no-build"};

      std::stringstream outputStream{};
      test_runner tr{args.size(), args.get(), "Oliver J. Rosten", make_project_paths(), "\t", outputStream};

      tr.execute();

      check(equivalence, "", hostDir, predictive_materials() /= "Another_Generated-Project");
    }
  }
}
