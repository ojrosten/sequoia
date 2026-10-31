////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "RunnerPerformanceAgainstReferenceTest.hpp"

#include "sequoia/PlatformSpecific/Preprocessor.hpp"
#include "sequoia/Runtime/ShellCommands.hpp"
#include "sequoia/TestFramework/CMakeCache.hpp"
#include "sequoia/TestFramework/Commands.hpp"
#include "sequoia/TestFramework/ProjectCreator.hpp"

#include <algorithm>
#include <array>
#include <format>
#include <limits>
#include <stdexcept>

namespace sequoia::testing
{
  namespace fs = std::filesystem;
  using namespace runtime;

  namespace
  {
    constexpr std::string_view reference_root_variable{"SEQUOIA_PERFORMANCE_REFERENCE_ROOT"};

    /** The largest multiple of the reference's time which the candidate's
        runner may take to run the workload's tests, net of a request for
        help.

        The bound suits Linux. On hosted Linux runners, a mutant of
        `prepare_materials` which removes the previous run's materials on each
        test's own thread took 1.08 to 1.25 times as long, and a bound of 1.1
        let it pass on some hosts. On hosted macOS runners, the same mutant
        passed at 1.1 on every host.
     */
    constexpr double slowdown_bound{1.05};

    /** \throws std::runtime_error if the cache of `build` has no entry for
        the reference tree's root
     */
    [[nodiscard]]
    fs::path reference_root(const build_paths& build)
    {
      const auto root{cmake_cache{build}.variable(reference_root_variable)};
      if(!root)
        throw std::runtime_error{
          std::format("{} has no entry for {}",
                      (build.cmake_cache_dir() / "CMakeCache.txt").generic_string(),
                      reference_root_variable)
        };

      return *root;
    }

    /** Copies the directories of the sequoia at `sequoiaRoot` which a build
        reads into the project at `projectRoot`, as its dependency.
     */
    void vendor_sequoia(const fs::path& sequoiaRoot, const fs::path& projectRoot)
    {
      const auto destination{dependencies_paths{projectRoot}.sequoia_root()};
      fs::create_directories(destination);

      auto copySubdirectory{
        [&sequoiaRoot, &destination](std::string_view dir) {
          fs::copy(sequoiaRoot / dir, destination / dir, fs::copy_options::recursive);
        }
      };

      constexpr std::array<std::string_view, 3> buildInputs{"build_system", "resources", "Source"};
      std::ranges::for_each(buildInputs, copySubdirectory);
    }

    /** A command which runs the workload's executable, its output going to
        the file `output`.
     */
    struct workload_command
    {
      shell_command command;
      fs::path      output;

      /** \throws std::runtime_error if the command's exit status is not
          zero
       */
      void operator()() const
      {
        throw_unless_succeeded(invoke(command),
                               "A run of the workload",
                               std::format("The output is {}", describe_output_location({}, output)));
      }
    };

    /** The workload, built against one sequoia. */
    class workload_build
    {
    public:
      /** Copies the workload at `workload` to `projectRoot`, with the sequoia
          at `sequoiaRoot`, and builds it with the preset of `parentBuild`.

          \throws std::runtime_error if the configuration or the build fails
       */
      workload_build(const fs::path& workload,
                     const fs::path& sequoiaRoot,
                     const fs::path& projectRoot,
                     const build_paths& parentBuild)
        : m_Build{make_new_build_paths(projectRoot, parentBuild)}
      {
        fs::copy(workload, projectRoot, fs::copy_options::recursive);
        vendor_sequoia(sequoiaRoot, projectRoot);

        const auto mainDir{main_paths{projectRoot / main_paths::default_main_cpp_from_root()}.dir()};
        const fs::path configureOutput{"CMakeOutput.txt"},
                       buildOutput{"BuildOutput.txt"};

        throw_unless_succeeded(invoke(cd_cmd(mainDir) && cmake_cmd(m_Build, configureOutput, "CODE_COVERAGE=OFF")),
                               std::format("Configuring the workload against {}", sequoiaRoot.generic_string()),
                               std::format("The output is {}", describe_output_location(mainDir, configureOutput)));

        throw_unless_succeeded(invoke(cd_cmd(mainDir) && build_cmd(m_Build, buildOutput)),
                               std::format("Building the workload against {}", sequoiaRoot.generic_string()),
                               std::format("The output is {}", describe_output_location(mainDir, buildOutput)));
      }

      /** A serial run of the workload's tests.

          In a serial run, one test starts only once the previous test has
          finished. So a cost which the runner adds to a test's preparation,
          execution or recording delays every later test. In a concurrent run,
          other threads' tests can hide the cost.
       */
      [[nodiscard]]
      workload_command serial_tests_command() const
      {
        return command("--serial", "TestsOutput.txt");
      }

      /** A request for help, which constructs a runner and runs no test. */
      [[nodiscard]]
      workload_command help_command() const
      {
        return command("--help", "HelpOutput.txt");
      }
    private:
      build_paths m_Build;

      [[nodiscard]]
      workload_command command(std::string_view options, std::string_view outputName) const
      {
        const auto executable{m_Build.executable_dir() / (with_windows_v ? "TestAll.exe" : "TestAll")},
                   output{m_Build.executable_dir() / outputName};

        return {.command{"", std::format("{} {}", quote_for_shell(executable.string()), options), output},
                .output{output}};
      }
    };

    /** The task is a serial run of the workload's tests, and the baseline a
        request for help. So the task's net duration excludes the costs which
        both share: starting the process, constructing the runner and stopping
        the process.
     */
    [[nodiscard]]
    task_with_baseline<workload_command, workload_command> make_task_with_baseline(const workload_build& build)
    {
      return {.task{build.serial_tests_command()}, .baseline{build.help_command()}};
    }
  }

  [[nodiscard]]
  fs::path runner_performance_against_reference_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void runner_performance_against_reference_test::run_tests()
  {
    const auto& build{get_project_paths().build()};
    const auto referenceRoot{reference_root(build)},
               workload{auxiliary_materials() / "Workload"},
               scratchpad{scratchpad_materials()};

    const workload_build candidate{workload, get_project_paths().project_root(), scratchpad / "Candidate", build},
                         reference{workload, referenceRoot,                       scratchpad / "Reference", build};

    // Every timed run then prepares materials which a previous run left, as
    // every run of a user's suite but the first does.
    candidate.serial_tests_command()();
    reference.serial_tests_command()();

    check_relative_performance(
      std::format("Net of a request for help, the candidate's runner takes at most {} times as long as the "
                  "reference's to run the tests serially",
                  slowdown_bound),
      make_task_with_baseline(candidate),
      make_task_with_baseline(reference),
      {.prediction{1 / slowdown_bound, std::numeric_limits<double>::infinity()}}
    );
  }
}
