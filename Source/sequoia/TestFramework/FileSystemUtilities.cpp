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

  [[nodiscard]]
  discoverable_paths discover_paths(int argc, char** argv)
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

  project_paths::project_paths(int argc, char** argv, const initializer& pathsFromRoot)
    : m_Discovered{discover_paths(argc, argv)}
    , m_MainCpp{project_root() / pathsFromRoot.mainCpp}
    , m_Source{source(project_root())}
    , m_SourceRoot{m_Source.parent_path()}
    , m_Tests{tests(project_root())}
    , m_TestMaterials{test_materials(project_root())}
    , m_Output{output(project_root())}
    , m_Build{build(project_root())}
    , m_AuxFiles{aux_files(project_root())}
    , m_CommonIncludes{project_root() / pathsFromRoot.commonIncludes}
    , m_CMadeBuildDir{cmade_build_dir(project_root(), m_MainCpp.dir())}
    , m_PruneDir{output() / fs::relative(cmade_build_dir(), project_root())}
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
  fs::path project_paths::output(fs::path projectRoot)
  {
    return projectRoot /= "output";
  }

  [[nodiscard]]
  fs::path project_paths::build(fs::path projectRoot)
  {
    return projectRoot /= "build";
  }

  [[nodiscard]]
  fs::path project_paths::aux_files(fs::path projectRoot)
  {
    return projectRoot /= "aux_files";
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

  [[nodiscard]]
  fs::path build_system_path(fs::path projectRoot)
  {
    return projectRoot / "build_system";
  }

  [[nodiscard]]
  fs::path code_templates_path(fs::path projectRoot)
  {
    return projectRoot/"aux_files"/"TestTemplates";
  }

  [[nodiscard]]
  fs::path source_templates_path(fs::path projectRoot)
  {
    return projectRoot / "aux_files" / "SourceTemplates";
  }

  [[nodiscard]]
  fs::path project_template_path(fs::path projectRoot)
  {
    return projectRoot/"aux_files"/"ProjectTemplate";
  }

  [[nodiscard]]
  fs::path recovery_path(fs::path outputDir)
  {
    return outputDir /= "Recovery";
  }

  [[nodiscard]]
  fs::path tests_temporary_data_path(fs::path outputDir)
  {
    return outputDir /= "TestsTemporaryData";
  }

  [[nodiscard]]
  fs::path diagnostics_output_path(fs::path outputDir)
  {
    return outputDir /= "DiagnosticsOutput";
  }

  [[nodiscard]]
  fs::path test_summaries_path(fs::path outputDir)
  {
    return outputDir /= "TestSummaries";
  }

  [[nodiscard]]
  fs::path temp_test_summaries_path(fs::path outputDir)
  {
    return tests_temporary_data_path(outputDir) /= "InstabilityAnalysis";
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
