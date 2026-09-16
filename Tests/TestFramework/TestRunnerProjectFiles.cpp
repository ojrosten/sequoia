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

#include <algorithm>
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
      generate_project();
      check_visual_studio_project_files(cache);
      break;
    case cmake_generator_family::ninja:
      generate_project();
      check_ninja_project_files(configure_generated_project(cache, back(get_project_paths().build().cmake_cache_dir())));
      break;
    case cmake_generator_family::other:
      break;
    }
  }

  void test_runner_project_files::generate_project()
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
  }

  [[nodiscard]]
  build_paths test_runner_project_files::configure_generated_project(const cmake_cache& cache, const fs::path& preset)
  {
    const auto cacheDir{generated_project() / "build" / "TestAll" / preset};
    const build_paths build{generated_project(), cacheDir, cacheDir};
    const main_paths main{generated_project() / main_paths::default_main_cpp_from_root()};

    invoke(cd_cmd(main.dir()) && cmake_cmd(build, generated_project() / std::format("CMakeOutput_{}.txt", preset.generic_string())));

    // The generated project carries this project's presets, so a preset names the same
    // generator in both - which is what the checks on its project files assume.
    if(check(std::format("CMake cache existence for {}", preset.generic_string()), fs::exists(cacheDir / "CMakeCache.txt")))
      check(equality, std::format("Generator for {}", preset.generic_string()), cmake_cache{build}.variable("CMAKE_GENERATOR"), cache.variable("CMAKE_GENERATOR"));

    return build;
  }

  [[nodiscard]]
  std::vector<std::filesystem::path> test_runner_project_files::predicted_presets() const
  {
    std::vector<fs::path> presets{};
    for(const auto& entry : fs::directory_iterator{predictive_materials() /= "ProjectFiles"})
    {
      if(entry.is_directory()) presets.push_back(entry.path().filename());
    }

    std::ranges::sort(presets);
    return presets;
  }

  void test_runner_project_files::check_visual_studio_project_files(const cmake_cache& cache)
  {
    // Every prediction is checked, not only the one for the preset this build tree was
    // configured with: a prediction is named after the preset that produces it, and that
    // name is what the generated project is configured with. So no choice of column
    // leaves a prediction unverified, and a prediction naming no preset fails to configure.
    const fs::path projectFiles{"ProjectFiles"};
    for(const auto& preset : predicted_presets())
    {
      const auto build{configure_generated_project(cache, preset)};

      // Absent when the configure failed, which its checks have reported; the comparison
      // below then reports the prediction with nothing to match it.
      if(const auto vcxproj{build.cmake_cache_dir() / "TestAll.vcxproj"}; fs::exists(vcxproj))
      {
        fs::create_directories(working_materials() /= projectFiles / preset);
        fs::copy(vcxproj, working_materials() /= projectFiles / preset);
      }
    }

    check(equivalence, report("Project files"), working_materials() /= projectFiles, predictive_materials() /= projectFiles);
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
