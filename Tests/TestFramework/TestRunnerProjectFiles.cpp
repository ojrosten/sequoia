////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TestRunnerProjectFiles.hpp"
#include "Parsing/CommandLineArgumentsTestingUtilities.hpp"

#include "sequoia/Streaming/Streaming.hpp"
#include "sequoia/TestFramework/CMakeCache.hpp"
#include "sequoia/TestFramework/Commands.hpp"
#include "sequoia/TestFramework/FileSystemUtilities.hpp"
#include "sequoia/TestFramework/ProjectCreator.hpp"
#include "sequoia/TestFramework/SumTypeCheckers.hpp"
#include "sequoia/TestFramework/TestRunner.hpp"

#include <sstream>
#include <stdexcept>

namespace sequoia::testing
{
  using namespace runtime;
  namespace fs = std::filesystem;

  [[nodiscard]]
  std::filesystem::path test_runner_project_files::source_file()
  {
    return std::source_location::current().file_name();
  }

  [[nodiscard]]
  std::filesystem::path test_runner_project_files::generated_project() const
  {
    return working_materials().parent_path() /= "GeneratedProject";
  }

  [[nodiscard]]
  std::string test_runner_project_files::summary_discriminator(const cmake_cache& cache)
  {
    switch(cache.generator_family())
    {
    case cmake_generator_family::visual_studio: return "visual_studio";
    case cmake_generator_family::ninja:         return "ninja";
    case cmake_generator_family::other:         return "";
    }

    throw std::logic_error{"Unrecognized case for cmake_generator_family"};
  }

  void test_runner_project_files::run_tests()
  {
    const cmake_cache cache{get_project_paths().build()};
    switch(cache.generator_family())
    {
    case cmake_generator_family::visual_studio:
      check_visual_studio_project_files(configure_generated_project(cache));
      break;
    case cmake_generator_family::ninja:
      check_ninja_project_files(configure_generated_project(cache));
      break;
    case cmake_generator_family::other:
      break;
    }
  }

  [[nodiscard]]
  build_paths test_runner_project_files::configure_generated_project(const cmake_cache& cache)
  {
    // --no-build because only the *generated* project files are under test, and cmake
    // generates those at configure time. Compiling the new project as well would cost
    // a great deal for nothing this test looks at; the end-to-end test pays that price
    // deliberately, for its own reasons.
    commandline_arguments args{{get_project_paths().discovered().executable().generic_string(),
                                "init",
                                "Oliver Jacob Rosten",
                                generated_project().string(),
                                "\t",
                                "--no-build",
                                "--to-files", "GenerationOutput.txt"}};

    std::stringstream outputStream{};

    const auto relativeMainCppPath{rebase_from(get_project_paths().main().file(), get_project_paths().project_root())};
    test_runner tr{args.size(),
                   args.get(),
                   "Oliver J. Rosten",
                   "  ",
                   {.main_cpp{relativeMainCppPath.generic_string()}, .common_includes{"TestCommon/TestIncludes.hpp"}},
                   outputStream};

    const auto build{make_new_build_paths(generated_project(), get_project_paths().build())};
    const main_paths main{generated_project() / main_paths::default_main_cpp_from_root()};

    invoke(cd_cmd(main.dir()) && cmake_cmd(build, generated_project() / "CMakeOutput.txt"));

    // The generated project is configured with the preset this build tree is named after, so it
    // must have been written by the same generator - which is what the checks below assume.
    if(check("CMake cache existence", fs::exists(build.cmake_cache_dir() / "CMakeCache.txt")))
      check(equality, "Generated project's generator", cmake_cache{build}.variable("CMAKE_GENERATOR"), cache.variable("CMAKE_GENERATOR"));

    return build;
  }

  void test_runner_project_files::check_visual_studio_project_files(const build_paths& build)
  {
    const fs::path subdirs{"ProjectFiles" / back(get_project_paths().build().cmake_cache_dir())};
    fs::create_directories(working_materials() /= subdirs);
    fs::copy(build.cmake_cache_dir() / "TestAll.vcxproj", working_materials() /= subdirs);

    check(equivalence, report("Project files"), working_materials() /= subdirs, predictive_materials() /= subdirs);
  }

  void test_runner_project_files::check_ninja_project_files(const build_paths& build)
  {
    // A build.ninja could be predicted the way a .vcxproj is, but it would take one
    // prediction per Ninja preset per platform. What can be asked of every one of them
    // is whether the graph has an edge for the test target - the one `ninja TestAll` builds.
    const auto buildFile{read_to_string(build.cmake_cache_dir() / "build.ninja", std::ios_base::in)};
    check("build.ninja existence", buildFile.has_value());
    check("build.ninja has an edge for TestAll", buildFile.has_value() && buildFile->contains("\nbuild TestAll:"));
  }
}
