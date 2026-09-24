////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TestRunnerProjectCreation.hpp"
#include "TestRunnerDiagnosticsUtilities.hpp"
#include "Parsing/CommandLineArgumentsTestingUtilities.hpp"
#include "Utilities/TestUtilities.hpp"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path test_runner_project_creation::source_file()
  {
    return std::source_location::current().file_name();
  }

  void test_runner_project_creation::run_tests()
  {
    test_exceptions();
    test_project_creation();
    test_init_failures();
  }

  void test_runner_project_creation::test_exceptions()
  {
    check_exception_thrown<std::runtime_error>(
      reporter{"Project name with space"},
      [this]() {
        commandline_arguments args{{zeroth_arg(), "init", "Oliver Jacob Rosten", (working_materials() /= "Generated Project").string(), "  "}};

        std::stringstream outputStream{};
        test_runner tr{args.size(), args.get(), "Oliver J. Rosten", "  ",  make_project_paths(), outputStream};

        return tr.execute();
      });

    check_exception_thrown<std::runtime_error>(
      reporter{"Project name with $"},
      [this]() {
        commandline_arguments args{{zeroth_arg(), "init", "Oliver Jacob Rosten", (working_materials() /= "Generated$Project").string(), "  "}};

        std::stringstream outputStream{};
        test_runner tr{args.size(), args.get(), "Oliver J. Rosten", "  ", make_project_paths(), outputStream};

        return tr.execute();
      });

    check_exception_thrown<std::runtime_error>(
      reporter{"Project path that is not absolute"},
      [this]() {
        commandline_arguments args{{zeroth_arg(), "init", "Oliver Jacob Rosten", "Generated_Project", "  "}};

        std::stringstream outputStream{};
        test_runner tr{args.size(), args.get(), "Oliver J. Rosten", "  ",  make_project_paths(), outputStream};

        return tr.execute();
      });

    check_exception_thrown<std::runtime_error>(
      reporter{"Empty project path"},
      [this]() {
        commandline_arguments args{{zeroth_arg(), "init", "Oliver Jacob Rosten", "", "  "}};

        std::stringstream outputStream{};
        test_runner tr{args.size(), args.get(), "Oliver J. Rosten", "  ",  make_project_paths(), outputStream};

        return tr.execute();
      });

    check_exception_thrown<std::runtime_error>(
      reporter{"Project name clash"},
      [this]() {
        commandline_arguments args{{zeroth_arg(), "init", "Oliver Jacob Rosten", working_materials().string(), "  "}};

        std::stringstream outputStream{};
        test_runner tr{args.size(), args.get(), "Oliver J. Rosten", "  ",  make_project_paths(), outputStream};

        return tr.execute();
      });

    check_exception_thrown<std::runtime_error>(
      reporter{"Illegal indent"},
      [this]() {
        commandline_arguments args{{zeroth_arg(), "init", "Oliver Jacob Rosten", (working_materials() /= "GeneratedProject").string(), "  "}};

        std::stringstream outputStream{};
        test_runner tr{args.size(), args.get(), "Oliver J. Rosten", "\t  x ",  make_project_paths(), outputStream};

        return tr.execute();
      });

    check_exception_thrown<std::runtime_error>(
      reporter{"Illegal init indent"},
      [this]() {
        commandline_arguments args{{zeroth_arg(), "init", "Oliver Jacob Rosten", (working_materials() /= "GeneratedProject").string(), "\n"}};

        std::stringstream outputStream{};
        test_runner tr{args.size(), args.get(), "Oliver J. Rosten", "  ",  make_project_paths(), outputStream};

        return tr.execute();
      });
  }

  [[nodiscard]]
  project_paths::customizer test_runner_project_creation::make_project_paths() const
  {
    return {.main_cpp{"TestSandbox/TestSandbox.cpp"}, .common_includes{"TestShared/SharedIncludes.hpp"}};
  }

  [[nodiscard]]
  std::filesystem::path test_runner_project_creation::fake_project() const
  {
    return working_materials() /= "FakeProject";
  }

  [[nodiscard]]
  std::string test_runner_project_creation::zeroth_arg() const
  {
    return (fake_project() / "build/CMade/FakeExe.txt").generic_string();
  }

  void test_runner_project_creation::test_project_creation()
  {
    namespace fs = std::filesystem;
    fs::copy(auxiliary_paths::repo(get_project_paths().project_root()), auxiliary_paths::repo(fake_project()), fs::copy_options::recursive);

    {
      const auto hostDir{working_materials() /= "GeneratedProject"};
      commandline_arguments args{{zeroth_arg(), "init", "Oliver Jacob Rosten", hostDir.string(), "  ", "--no-git", "--no-build"}};

      std::stringstream outputStream{};
      test_runner tr{args.size(), args.get(), "Oliver J. Rosten", "\t ",  make_project_paths(), outputStream};

      check(equality, "Project creation return code", tr.execute(), return_code::success);

      if(std::ofstream file{fake_project() / "output" / "io.txt"})
      {
        file << outputStream.rdbuf();
      }

      check(equivalence, "", hostDir, predictive_materials() /= "GeneratedProject");
      check(equivalence, "", fake_project(), predictive_materials() /= "FakeProject");
    }

    {
      const auto hostDir{working_materials() /= "Another_Generated-Project"};
      commandline_arguments args{{zeroth_arg(), "init", "Oliver Jacob Rosten", hostDir.generic_string(), "  ", "--no-git", "--no-build"}};

      std::stringstream outputStream{};
      test_runner tr{args.size(), args.get(), "Oliver J. Rosten", "\t ",  make_project_paths(), outputStream};

      check(equality, "Second project creation return code", tr.execute(), return_code::success);

      check(equivalence, "", hostDir, predictive_materials() /= "Another_Generated-Project");
    }
  }

  void test_runner_project_creation::test_init_failures()
  {
    namespace fs = std::filesystem;

    // Runs init into a directory which is removed afterwards, expecting it to throw; returns what
    // it threw, so that which step failed can be checked, not only that one did.
    auto initFailure{
      [this](std::string_view description, const fs::path& hostDir, std::vector<std::string> options) {
        std::string message{};
        check_exception_thrown<std::runtime_error>(
          reporter{description},
          [&]() {
            std::vector<std::string> arguments{zeroth_arg(),
                                               "init", "Oliver Jacob Rosten", hostDir.generic_string(), "  ",
                                               "--to-files", "GenerationOutput.txt"};
            arguments.append_range(options);

            commandline_arguments args{std::move(arguments)};
            std::stringstream outputStream{};
            test_runner tr{args.size(), args.get(), "Oliver J. Rosten", "\t ", make_project_paths(), outputStream};
          },
          [&message](const project_paths& projPaths, std::string thrown) {
            message = thrown;
            return relative_to_root(projPaths, std::move(thrown));
          });

        return message;
      }
    };

    // Each trigger goes in the fake project's template, which test_project_creation has copied in.
    const auto fakeTemplate{auxiliary_paths::project_template(fake_project())};

    {
      // A `.git` which is a file but not a gitfile makes `git init` fail; git's configuration
      // cannot rescue it, though a GIT_DIR in the environment would direct git past it.
      const transient_file bogusGit{fakeTemplate / ".git", "not a gitfile"};
      const transient_directory host{working_materials() /= "UnversionedProject"};

      const auto message{
        initFailure("git fails when placing the project under version control", host.path(), {"--no-build"})
      };
      check("The failure reported is the first git step's",
            message.starts_with("Placing the new project under version control failed"));

      check("The creation stopped before sequoia was copied",
            std::ranges::all_of(fs::directory_iterator{dependencies_paths{host.path()}.sequoia_root()},
                                [](const fs::directory_entry& entry) { return entry.path().filename() == ".keep"; }));
    }

    {
      // With everything under dependencies ignored, copying sequoia gives git nothing to commit.
      // The first commit must succeed, so this needs the git identity init always does.
      const transient_file ignoreDependencies{fakeTemplate / "dependencies" / ".gitignore", "*\n"};
      const transient_directory host{working_materials() /= "UncommittedProject"};

      const auto message{initFailure("git fails when committing sequoia", host.path(), {"--no-build"})};
      check("The failure reported is the second git step's",
            message.starts_with("Committing sequoia to the new project failed"));
    }

    {
      // The fake project's build tree is named for no preset, so the new project's configure fails
      const transient_directory host{working_materials() /= "UnbuiltProject"};

      const auto message{
        initFailure("CMake fails when configuring the project", host.path(), {"--no-git", "--no-ide"})
      };
      check("The failure reported is the configure and build step's",
            message.starts_with("Configuring and building the new project failed"));
    }
  }
}
