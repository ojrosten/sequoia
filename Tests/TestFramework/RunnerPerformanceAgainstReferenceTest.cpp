////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "RunnerPerformanceAgainstReferenceTest.hpp"

#include "sequoia/PlatformSpecific/Preprocessor.hpp"
#include "sequoia/Runtime/ShellCommands.hpp"
#include "sequoia/TestFramework/Commands.hpp"
#include "sequoia/TestFramework/FileSystemUtilities.hpp"
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
    /** The largest multiple of the reference's time which the candidate's
        runner may take to run the workload's tests, net of a request for
        help, and to answer a request for help alone.

        On hosted Linux runners, a mutant of `prepare_materials` which removes
        the previous run's materials on each test's own thread took 1.08 to
        1.25 times as long, and a bound of 1.1 let it pass on some hosts. On
        hosted macOS runners, the same mutant passed at 1.1 on every host, so
        macOS is not gated. No measured regression stands behind the bound for
        a request for help alone.
     */
    constexpr double slowdown_bound{1.05};

    /** \throws std::runtime_error if `projPaths` holds no reference root, or
        if the reference root holds no regular file
        `Source/sequoia/CMakeLists.txt`
     */
    [[nodiscard]]
    fs::path reference_sequoia_root(const project_paths& projPaths)
    {
      const auto& root{projPaths.reference_root()};
      if(!root)
        throw std::runtime_error{
          "No reference root given: run with --reference-root <path>, the root of a sequoia tree to compare against"
        };

      throw_unless_regular_file(source_paths{*root, "sequoia"}.cmake_lists(),
                                "\nThe reference root must be the root of a sequoia tree");

      return *root;
    }

    void copy_sequoia_build_inputs(const fs::path& sequoiaRoot, const fs::path& projectRoot)
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
        copy_sequoia_build_inputs(sequoiaRoot, projectRoot);

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

    /** The net duration excludes the costs which both runs share: starting
        the process, constructing the runner and stopping the process.
     */
    [[nodiscard]]
    task_with_baseline<workload_command, workload_command> serial_run_net_of_help(const workload_build& build)
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
    const auto referenceRoot{reference_sequoia_root(get_project_paths())},
               workload{auxiliary_materials() / "Workload"},
               scratchpad{scratchpad_materials()};

    const workload_build candidate{workload, get_project_paths().project_root(), scratchpad / "Candidate", build},
                         reference{workload, referenceRoot,                       scratchpad / "Reference", build};

    // Every timed run then prepares materials which a previous run left, as
    // every run of a user's suite but the first does.
    candidate.serial_tests_command()();
    reference.serial_tests_command()();

    const relative_performance_parameters boundedSlowdownPrediction{
      .prediction{1 / slowdown_bound, std::numeric_limits<double>::infinity()}
    };

    check_relative_performance(
      std::format("Net of a request for help, the candidate's runner takes at most {} times as long as the "
                  "reference's to run the tests serially",
                  slowdown_bound),
      serial_run_net_of_help(candidate),
      serial_run_net_of_help(reference),
      boundedSlowdownPrediction
    );

    // The check above is net of a request for help, so this one times a
    // request for help alone.
    check_relative_performance(
      std::format("The candidate's runner takes at most {} times as long as the reference's to answer a request "
                  "for help",
                  slowdown_bound),
      candidate.help_command(),
      reference.help_command(),
      boundedSlowdownPrediction
    );
  }
}
