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
#include <format>
#include <fstream>
#include <string>
#include <vector>

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  [[nodiscard]]
  fs::path test_runner_project_creation::source_file()
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
  fs::path test_runner_project_creation::fake_project() const
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

      // The .git of a worktree or of a submodule checkout is a file. It is made here because git cannot track a path
      // named .git, and before the runner because constructing the runner performs the init.
      const transient_file gitFile{fake_project() / "dependencies/sequoia/.git",
                                   "gitdir: ../../.git/modules/dependencies/sequoia\n"};

      std::stringstream outputStream{};
      test_runner tr{args.size(), args.get(), "Oliver J. Rosten", "\t ",  make_project_paths(), outputStream};

      check(equality, "Second project creation return code", tr.execute(), return_code::success);

      check(equivalence, "", hostDir, predictive_materials() /= "Another_Generated-Project");
    }
  }

  void test_runner_project_creation::test_init_failures()
  {
    // Runs init into `hostDir`, and checks that init throws. Returns the message thrown, so that
    // the caller can check which step failed, not only that one did.
    auto initFailure{
      [this](std::string_view description, const fs::path& hostDir, std::initializer_list<std::string> options) {
        std::string message{};
        check_exception_thrown<std::runtime_error>(
          reporter{description},
          [&]() {
            const auto argList{
              [&]() {
                std::vector<std::string> list{zeroth_arg(),
                                              "init", "Oliver Jacob Rosten", hostDir.generic_string(), "  ",
                                              "--to-files", "GenerationOutput.txt"};
                list.append_range(options);
                return list;
              }()
            };

            commandline_arguments args{argList};
            std::stringstream outputStream{};
            test_runner tr{args.size(), args.get(), "Oliver J. Rosten", "\t ", make_project_paths(), outputStream};
          },
          [&message](const project_paths& projPaths, std::string thrown) {
            message = thrown;
            return relative_to_root(projPaths, std::move(thrown));
          }
        );

        return message;
      }
    };

    // Each trigger goes in the fake project's template. test_project_creation has copied the
    // template in.
    const auto fakeTemplate{auxiliary_paths::project_template(fake_project())};

    {
      // A `.git` which is a file but not a gitfile makes `git init` fail. git's configuration
      // cannot prevent the failure, but a GIT_DIR in the environment would direct git past the file.
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
      // A failure in the first project ends the run, so init never reaches the second project.
      const transient_file bogusGit{fakeTemplate / ".git", "not a gitfile"};
      const transient_directory host{working_materials() /= "FailingProject"};
      const transient_directory abandoned{working_materials() /= "AbandonedProject"};

      const auto message{
        initFailure("A failure ends the run before a later project",
                    host.path(),
                    {"--no-build",
                     "init", "Oliver Jacob Rosten", abandoned.path().generic_string(), "  ", "--no-build"})
      };

      check("The message names the project the failure abandoned",
            message.contains(std::format("Not attempted, since this failure ended the run: {}",
                                         abandoned.path().generic_string())));
    }

    {
      // Copying sequoia gives git nothing to commit, since everything under dependencies is
      // ignored. The first commit must succeed, so this case needs a git identity, as init always
      // needs one.
      const transient_file ignoreDependencies{fakeTemplate / "dependencies" / ".gitignore", "*\n"};
      const transient_directory host{working_materials() /= "UncommittedProject"};

      const auto message{initFailure("git fails when committing sequoia", host.path(), {"--no-build"})};
      check("The failure reported is the second git step's",
            message.starts_with("Committing sequoia to the new project failed"));
    }

    {
      // The new project's configure fails, since the fake project's build tree is named for no preset
      const transient_directory host{working_materials() /= "UnbuiltProject"};

      const auto message{
        initFailure("CMake fails when configuring the project", host.path(), {"--no-git", "--no-ide"})
      };
      check("The failure reported is the configure and build step's",
            message.starts_with("Configuring and building the new project failed"));
    }
  }
}
