////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for FileSystemUtilities.hpp
 */

#include "sequoia/FileSystem/FileSystem.hpp"
#include "sequoia/TestFramework/FileSystemUtilities.hpp"

#include "sequoia/TestFramework/Output.hpp"
#include "sequoia/TextProcessing/Substitutions.hpp"

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  namespace
  {
    [[nodiscard]]
    std::vector<file_info> make_ancillary_info(const fs::path& root, const project_paths::initializer& initializer)
    {
      std::vector<file_info> ancillaryInfo;

      std::transform(initializer.ancillaryMainCpps.begin(),
        initializer.ancillaryMainCpps.end(),
        std::back_inserter(ancillaryInfo),
        [&root](const fs::path& relPath) { return root / relPath; });

      return ancillaryInfo;
    }
  }

  [[nodiscard]]
  std::string serializer<fs::path>::make(const fs::path& p)
  {
    return p.generic_string();
  }

  [[nodiscard]]
  std::string serializer<fs::file_type>::make(const fs::file_type& val)
  {
    using ft = fs::file_type;
    switch(val)
    {
    case ft::none:
      return "none";
    case ft::not_found:
      return "not found";
    case ft::regular:
      return "regular";
    case ft::directory:
      return "directory";
    case ft::symlink:
      return "symlink";
    case ft::block:
      return "block";
    case ft::character:
      return "character";
    case ft::fifo:
      return "fifo";
    case ft::socket:
      return "socket";
    case ft::unknown:
      return "unknown";
    default:
      return "unrecognized";
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

  file_info::file_info(fs::path file)
    : m_File{std::move(file)}
    , m_Dir{m_File.parent_path()}
  {
    throw_unless_regular_file(m_File, "\nTry ensuring that the application is run from the appropriate directory");
  }

  //===================================== auxiliary_paths =====================================//

  auxiliary_paths::auxiliary_paths(const std::filesystem::path& projectRoot)
    : m_Dir{dir(projectRoot)}
    , m_TestTemplates{test_templates(projectRoot)}
    , m_SourceTemplates{source_templates(projectRoot)}
    , m_ProjectTemplate{project_template(projectRoot)}
  {}

  [[nodiscard]]
  fs::path auxiliary_paths::dir(fs::path projectRoot)
  {
    return projectRoot /= "aux_files";
  }

  [[nodiscard]]
  std::filesystem::path auxiliary_paths::test_templates(std::filesystem::path projectRoot)
  {
    return dir(projectRoot) /= "TestTemplates";
  }

  [[nodiscard]]
  std::filesystem::path auxiliary_paths::source_templates(std::filesystem::path projectRoot)
  {
    return dir(projectRoot) /= "SourceTemplates";
  }

  [[nodiscard]]
  std::filesystem::path auxiliary_paths::project_template(std::filesystem::path projectRoot)
  {
    return dir(projectRoot) /= "ProjectTemplate";
  }

  //===================================== recovery_paths =====================================//

  recovery_paths::recovery_paths(const fs::path& outputDir)
    : m_Dir{dir(outputDir)}
  {}

  [[nodiscard]]
  fs::path recovery_paths::dir(std::filesystem::path outputDir)
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
  fs::path output_paths::instability_analysis(fs::path projectRoot)
  {
    return tests_temporary_data(projectRoot) /= "InstabilityAnalysis";
  }

  //===================================== project_paths =====================================//

  project_paths::project_paths(int argc, char** argv, const initializer& pathsFromRoot)
    : m_Discovered{argc, argv}
    , m_MainCpp{project_root() / pathsFromRoot.mainCpp}
    , m_Source{source(project_root())}
    , m_SourceRoot{m_Source.parent_path()}
    , m_Tests{tests(project_root())}
    , m_TestMaterials{test_materials(project_root())}
    , m_Build{build(project_root())}
    , m_BuildSystem{build_system(project_root())}
    , m_Auxiliary{project_root()}
    , m_Output{project_root()}
    , m_CommonIncludes{project_root() / pathsFromRoot.commonIncludes}
    , m_CMadeBuildDir{cmade_build_dir(project_root(), m_MainCpp.dir())}
    , m_PruneDir{output().dir() / "Prune" / fs::relative(cmade_build_dir(), build())}
    , m_InstabilityAnalysisPruneDir{prune_dir() / "InstabilityAnalysis"}
    , m_AncillaryMainCpps{make_ancillary_info(project_root(), pathsFromRoot)}
  {
    throw_unless_directory(project_root(), "\nRepository root not found");
    throw_unless_regular_file(m_CommonIncludes, "\nCommon includes not found");
  }

  [[nodiscard]]
  fs::path project_paths::cmade_build_dir(const fs::path& projectRoot, const fs::path& mainCppDir)
  {
    fs::path dir{projectRoot / "build" / "CMade"};
    if constexpr(with_msvc_v)
    {
      dir /= "win";
    }
    else if constexpr(with_clang_v)
    {
      dir /= "clang";
    }
    else if constexpr(with_gcc_v)
    {
      dir /= "gcc";
    }

    return dir /= fs::relative(mainCppDir, projectRoot);
  }

  [[nodiscard]]
  fs::path project_paths::source(fs::path projectRoot)
  {
    return (projectRoot /= "Source") /= uncapitalize((--projectRoot.end())->generic_string());
  }

  [[nodiscard]]
  fs::path project_paths::tests(fs::path projectRoot)
  {
    return projectRoot /= "Tests";
  }

  [[nodiscard]]
  fs::path project_paths::test_materials(fs::path projectRoot)
  {
    return projectRoot /= "TestMaterials";
  }

  [[nodiscard]]
  fs::path project_paths::build(fs::path projectRoot)
  {
    return projectRoot /= "build";
  }

  [[nodiscard]]
  fs::path project_paths::build_system(fs::path projectRoot)
  {
    return projectRoot /= "build_system";
  }

  [[nodiscard]]
  fs::path project_paths::prune_file_path(std::optional<std::size_t> id) const
  {
    auto [dir, num] {
      [id,this]() {
        return (id == std::nullopt) ? std::make_pair(prune_dir(), std::string{})
                                    : std::make_pair(instability_analysis_prune_dir(), std::to_string(id.value()));
      }()
    };

    return (dir /= back(cmade_build_dir())).concat(num).concat(".prune");
  }

  void throw_unless_exists(const fs::path& p, std::string_view message)
  {
    throw_if(p, append_lines(p.empty() ? "File path is empty" :"not found", message),
             [](const fs::path& p){ return !fs::exists(p); });
  }

  void throw_unless_directory(const fs::path& p, std::string_view message)
  {
    throw_if(p, append_lines(p.empty() ? "File path is empty" : "is not a directory", message), [](const fs::path& p){ return !fs::is_directory(p); });
  }

  void throw_unless_regular_file(const fs::path& p, std::string_view message)
  {
    throw_unless_exists(p, message);
    throw_if(p, append_lines(" is not a regular file", message), [](const fs::path& p){ return !fs::is_regular_file(p); });
  }

  [[nodiscard]]
  fs::path find_in_tree(const fs::path& root, const fs::path& toFind)
  {
    throw_unless_directory(root, "");

    using dir_iter = fs::recursive_directory_iterator;

    if(const auto toFindLen{std::distance(toFind.begin(), toFind.end())}; toFindLen)
    {
      for(const auto& i : dir_iter{root})
      {
        const auto p{i.path()};
        const auto entryPathLen{std::distance(p.begin(), p.end())};
        if(entryPathLen >= toFindLen)
        {
          auto entryIter{p.end()}, toFindIter{toFind.begin()};

          // MSVC 16.9.4 objects to std::prev or std::advance as its impl of path::iterator
          // does not satisfy the requirements of a bi-directional iterator
          using diff_t = std::remove_const_t<decltype(toFindLen)>;
          for (diff_t n{}; n < toFindLen; ++n) --entryIter;

          while(entryIter != p.end())
          {
            if(*entryIter != *toFindIter++) break;

            ++entryIter;
          }

          if(entryIter == p.end()) return p;
        }
      }
    }

    return {};
  }

  [[nodiscard]]
  fs::path rebase_from(const fs::path& p, const fs::path& dir)
  {
    if(fs::exists(dir) && !fs::is_directory(dir))
      throw std::logic_error{"Trying to rebase from something other than a directory"};

    if(p.is_absolute() && dir.is_absolute())
      return fs::relative(p, dir);

    auto i{p.begin()};

    while((i != p.end()) && (*i == "..")) ++i;

    if(i != p.end())
    {
      auto dirIter{dir.end()};
      while(dirIter != dir.begin())
      {
        --dirIter;
        if(*dirIter == *i) break;
      }

      while((dirIter != dir.end()) && (i != p.end()) && (*dirIter == *i))
      {
        ++dirIter;
        ++i;
      }
    }

    fs::path rebased{};
    for(; i!= p.end(); ++i)
    {
      rebased /= *i;
    }

    return rebased;
  }
}
