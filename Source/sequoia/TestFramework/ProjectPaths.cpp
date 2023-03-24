////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for ProjectPaths.hpp
 */

#include "sequoia/TestFramework/ProjectPaths.hpp"

#include "sequoia/FileSystem/FileSystem.hpp"
#include "sequoia/TestFramework/FileSystemUtilities.hpp"
#include "sequoia/TestFramework/Output.hpp"

#include "sequoia/TextProcessing/Substitutions.hpp"

#include <algorithm>

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  namespace
  {
    [[nodiscard]]
    std::vector<main_paths> make_ancillary_info(const fs::path& root, const fs::path& commonIncludes, const project_paths::initializer& initializer)
    {
      std::vector<main_paths> ancillaryInfo;

      std::transform(initializer.ancillaryMainCpps.begin(),
                     initializer.ancillaryMainCpps.end(),
                     std::back_inserter(ancillaryInfo),
                     [&root,&commonIncludes](const fs::path& relPath) { return main_paths{root / relPath, commonIncludes}; });

      return ancillaryInfo;
    }
  }

  discoverable_paths::discoverable_paths(int argc, char** argv)
    : discoverable_paths{make(argc, argv)}
  {}

  discoverable_paths::discoverable_paths(fs::path rt, fs::path ex)
    : m_Root{std::move(rt)}
    , m_Executable{std::move(ex)}
  {}

  [[nodiscard]]
  discoverable_paths discoverable_paths::make(int argc, char** argv)
  {
    if(argc)
    {
      if(std::string_view zeroth{argv[0]}; !zeroth.empty())
      {
        if(const auto exectable{fs::canonical(fs::path(zeroth))}; !exectable.empty())
        {
          auto trialRoot{exectable};
          while((std::distance(trialRoot.begin(), trialRoot.end()) > 1))
          {
            const auto last{back(trialRoot)};
            const auto parent{trialRoot.parent_path()};

            if(trialRoot == parent) break;

            trialRoot = parent;
            if(last == "build") return {trialRoot, exectable};
          }
        }

        throw std::runtime_error{std::string{"Unable to locate project root from path:\n"}.append(zeroth)
                    .append("\nPlease ensure that the build directory is a subdirectory of <project>/build.")};
      }
    }

    throw std::runtime_error{"Unable to locate project root as no commandline arguments supplied."};
  }

  main_paths::main_paths(fs::path file, fs::path commonIncludes)
    : m_File{std::move(file)}
    , m_Dir{m_File.parent_path()}
    , m_CommonIncludes{std::move(commonIncludes)}
  {}

  main_paths::main_paths(fs::path file)
    : main_paths{file, file}
  {}

  [[nodiscard]]
  fs::path main_paths::cmake_lists() const
  {
    return dir() / "CMakeLists.txt";
  }

  [[nodiscard]]
  fs::path main_paths::default_main_cpp_from_root()
  {
    return "TestAll/TestAllMain.cpp";
  }

  [[nodiscard]]
  fs::path main_paths::default_cmake_from_root()
  {
    return "TestAll/CMakeLists.txt";
  }


  //===================================== source_paths =====================================//

  source_paths::source_paths(const fs::path& projectRoot)
    : m_Repo{repo(projectRoot)}
    , m_Project{repo() / uncapitalize(back(projectRoot).generic_string())}
  {}

  [[nodiscard]]
  fs::path source_paths::cmake_lists() const
  {
    return repo() / "CMakeLists.txt";
  }

  [[nodiscard]]
  fs::path source_paths::cmake_lists(std::filesystem::path projectRoot)
  {
    return repo(projectRoot) /= "CMakeLists.txt";
  }

  [[nodiscard]]
  fs::path source_paths::repo(std::filesystem::path projectRoot)
  {
    if(projectRoot.empty())
      throw std::runtime_error{"Project root required to construct source path"};

    return projectRoot /= "Source";
  }

  //===================================== tests_paths =====================================//

  tests_paths::tests_paths(fs::path projectRoot)
    : m_Repo{std::move(projectRoot /= "Tests")}
  {}

  [[nodiscard]]
  std::filesystem::path tests_paths::project_root() const
  {
    return m_Repo.parent_path();
  }

  //===================================== test_materials_paths =====================================//

  test_materials_paths::test_materials_paths(fs::path projectRoot)
    : m_Repo{std::move(projectRoot /= "TestMaterials")}
  {}

  //===================================== build_system_paths =====================================//

  build_system_paths::build_system_paths(fs::path projectRoot)
    : m_Repo{std::move(projectRoot /= "build_system")}
  {}

  //===================================== build_paths =====================================//

  build_paths::build_paths(fs::path projectRoot, const main_paths& main)
    : m_Dir{std::move(projectRoot /= "build")}
    , m_CMadeBuildDir{cmade_dir(main)}
  {}

  [[nodiscard]]
  fs::path build_paths::cmade_dir(const main_paths& main)
  {
    auto compilerDir{
      []() -> std::string {
        if      constexpr(with_msvc_v)  return "win";
        else if constexpr(with_clang_v) return "clang";
        else if constexpr(with_gcc_v)   return "gcc";
        else                            throw std::runtime_error{"Compiler not supported"};
      }
    };

    return (dir() / "CMade").append(compilerDir()) /= fs::relative(main.dir(), dir().parent_path());
  }

  [[nodiscard]]
  fs::path build_paths::cmake_cache() const
  {
    return cmade_dir() / "CMakeCache.txt";
  }

  //===================================== auxiliary_paths =====================================//

  auxiliary_paths::auxiliary_paths(const fs::path& projectRoot)
    : m_Dir{repo(projectRoot)}
    , m_TestTemplates{test_templates(projectRoot)}
    , m_SourceTemplates{source_templates(projectRoot)}
    , m_ProjectTemplate{project_template(projectRoot)}
  {}

  [[nodiscard]]
  fs::path auxiliary_paths::repo(fs::path projectRoot)
  {
    return projectRoot /= "aux_files";
  }

  [[nodiscard]]
  fs::path auxiliary_paths::test_templates(fs::path projectRoot)
  {
    return repo(projectRoot) /= "TestTemplates";
  }

  [[nodiscard]]
  fs::path auxiliary_paths::source_templates(fs::path projectRoot)
  {
    return repo(projectRoot) /= "SourceTemplates";
  }

  [[nodiscard]]
  fs::path auxiliary_paths::project_template(fs::path projectRoot)
  {
    return repo(projectRoot) /= "ProjectTemplate";
  }

  //===================================== recovery_paths =====================================//

  recovery_paths::recovery_paths(const fs::path& outputDir)
    : m_Dir{dir(outputDir)}
  {}

  [[nodiscard]]
  fs::path recovery_paths::dir(fs::path outputDir)
  {
    return outputDir /= "Recovery";
  }

  [[nodiscard]]
  fs::path recovery_paths::recovery_file() const
  {
    return fs::path{dir()} /= "Recovery.txt";
  }

  [[nodiscard]]
  fs::path recovery_paths::dump_file() const
  {
    return fs::path{dir()} /= "Dump.txt";
  }

  //===================================== prune_paths =====================================//

  prune_paths::prune_paths(fs::path outputDir, const fs::path& buildRoot, const fs::path& buildDir)
    : m_Dir{(outputDir /= "Prune") /= fs::relative(buildDir, buildRoot)}
    , m_Stem{make_stem(buildDir)}
  {}

  [[nodiscard]]
  fs::path prune_paths::stamp() const
  {
    return (m_Dir / m_Stem).concat(".prune");
  }

  [[nodiscard]]
  fs::path prune_paths::failures(std::optional<std::size_t> id) const
  {
    return make_path(id, ".failures");
  }

  [[nodiscard]]
  fs::path prune_paths::selected_passes(std::optional<std::size_t> id) const
  {
    return make_path(id, ".passes");
  }

  [[nodiscard]]
  std::filesystem::path prune_paths::external_dependencies() const
  {
    return make_path(std::nullopt, ".external");
  }

  [[nodiscard]]
  fs::path prune_paths::instability_analysis() const
  {
    return m_Dir / "InstabilityAnalysis";
  }

  [[nodiscard]]
  fs::path prune_paths::make_stem(const std::filesystem::path& buildDir)
  {
    if(buildDir.empty())
      throw std::logic_error{"Build Dir required for pruning"};

    return back(buildDir);
  }

  [[nodiscard]]
  fs::path prune_paths::make_path(std::optional<std::size_t> id, std::string_view extension) const
  {
    auto [directory, num] {(id == std::nullopt) ? std::make_pair(dir(), std::string{})
                                                : std::make_pair(instability_analysis(), std::to_string(id.value()))
    };

    return (directory /= m_Stem).concat(num).concat(extension);
  }

  //===================================== output_paths =====================================//

  output_paths::output_paths(const fs::path& projectRoot)
    : m_Dir{dir(projectRoot)}
    , m_TestsTemporaryData{tests_temporary_data(projectRoot)}
    , m_Diagnostics{diagnostics(projectRoot)}
    , m_TestSummaries{test_summaries(projectRoot)}
    , m_InstabilityAnalysis{instability_analysis(projectRoot)}
  {}

  [[nodiscard]]
  fs::path output_paths::dir(fs::path projectRoot)
  {
    return projectRoot /= "output";
  }

  [[nodiscard]]
  fs::path output_paths::tests_temporary_data(fs::path projectRoot)
  {
    return dir(projectRoot) /= "TestsTemporaryData";
  }

  [[nodiscard]]
  fs::path output_paths::diagnostics(fs::path projectRoot)
  {
    return dir(projectRoot) /= "DiagnosticsOutput";
  }

  [[nodiscard]]
  fs::path output_paths::test_summaries(fs::path projectRoot)
  {
    return dir(projectRoot) /= "TestSummaries";
  }

  [[nodiscard]]
  fs::path output_paths::instability_analysis_file(fs::path projectRoot, fs::path source, std::string_view name, std::size_t index)
  {
    const auto ext{replace(source.filename().extension().string(), ".", "_")};

    return (instability_analysis(std::move(projectRoot)) / source.filename().replace_extension().concat(ext))
      .append(replace_all(name, " ", "_")).append("Output_" + std::to_string(index) + ".txt");
  }

  [[nodiscard]]
  fs::path output_paths::instability_analysis(fs::path projectRoot)
  {
    return tests_temporary_data(projectRoot) /= "InstabilityAnalysis";
  }

  //===================================== project_paths =====================================//

  project_paths::project_paths(int argc, char** argv, const initializer& pathsFromRoot)
    : m_Discovered{argc, argv}
    , m_Main{project_root() / pathsFromRoot.mainCpp, project_root() / pathsFromRoot.commonIncludes}
    , m_Source{project_root()}
    , m_Build{project_root(), main()}
    , m_Auxiliary{project_root()}
    , m_Output{project_root()}
    , m_Tests{project_root()}
    , m_Materials{project_root()}
    , m_BuildSystem{project_root()}
    , m_AncillaryMainCpps{make_ancillary_info(project_root(), main().common_includes(), pathsFromRoot)}
  {
    throw_unless_directory(project_root(), "\nRepository root not found");
    throw_unless_regular_file(main().file(), "\nTry ensuring that the application is run from the appropriate directory");
    throw_unless_regular_file(main().common_includes(), "\nCommon includes not found");
  }

  [[nodiscard]]
  prune_paths project_paths::prune() const
  {
    return output().prune(build().dir(), build().cmade_dir());
  }
}
