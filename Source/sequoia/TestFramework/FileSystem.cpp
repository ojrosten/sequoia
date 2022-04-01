////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for FileSystem.hpp
 */

#include "sequoia/TestFramework/FileSystem.hpp"

#include "sequoia/TestFramework/Output.hpp"
#include "sequoia/TextProcessing/Substitutions.hpp"

namespace sequoia::testing
{
  namespace fs = std::filesystem;

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
  fs::path working_path()
  {
    return working_path_v;
  }

  [[nodiscard]]
  fs::path project_root(int argc, char** argv, const fs::path& fallback)
  {
    if(argc)
    {
      std::string_view zeroth{argv[0]};
      auto p{fs::canonical(fs::path(zeroth)).parent_path()};
      if(!p.empty())
      {
        auto back{[](const fs::path& p) { return *(--p.end()); }};

        while((std::distance(p.begin(), p.end()) > 1))
        {
          const auto last{back(p)};
          const auto parent{p.parent_path()};

          if(p == parent)
            throw std::runtime_error{"Unable to locate project root; please ensure that the build directory is a subdirectory of sequoia/build!"};

          p = parent;
          if(last == "build") break;
        }

        return p;
      }
    }

    return fallback;
  }

  project_paths::project_paths(const fs::path& projectRoot)
    : project_paths{projectRoot, projectRoot / "TestAll" / "TestAllMain.cpp", projectRoot / "TestAll" / "TestAllMain.cpp"}
  {}

  project_paths::project_paths(const fs::path& projectRoot, fs::path mainCpp, fs::path includePath)
    : m_ProjectRoot{projectRoot}
    , m_Source{source_path(projectRoot)}
    , m_SourceRoot{m_Source.parent_path()}
    , m_Tests{projectRoot / "Tests"}
    , m_TestMaterials{projectRoot / "TestMaterials"}
    , m_Output{projectRoot / "output"}
    , m_MainCpp{std::move(mainCpp)}
    , m_MainCppDir{m_MainCpp.parent_path()}
    , m_IncludeTarget{std::move(includePath)}
    , m_CMadeBuildDir{cmade_build_dir(m_ProjectRoot, m_MainCppDir)}
  {
    throw_unless_directory(m_ProjectRoot, "\nTest repository not found");
    throw_unless_regular_file(m_MainCpp, "\nTry ensuring that the application is run from the appropriate directory");
    throw_unless_regular_file(m_IncludeTarget, "\nInclude target not found");
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
  fs::path project_paths::source_path(const fs::path& projectRoot)
  {
    if(projectRoot.empty())
      throw std::runtime_error{"Project root should not be empty"};

    return projectRoot / "Source" / uncapitalize((--projectRoot.end())->generic_string());
  }

  [[nodiscard]]
  const fs::path& project_paths::project_root() const noexcept
  {
    return m_ProjectRoot;
  }

  [[nodiscard]]
  const fs::path& project_paths::source() const noexcept
  {
    return m_Source;
  }

  [[nodiscard]]
  const fs::path& project_paths::source_root() const noexcept
  {
    return m_SourceRoot;
  }

  [[nodiscard]]
  const fs::path& project_paths::tests() const noexcept
  {
    return m_Tests;
  }

  [[nodiscard]]
  const fs::path& project_paths::test_materials() const noexcept
  {
    return m_TestMaterials;
  }

  [[nodiscard]]
  const fs::path& project_paths::output() const noexcept
  {
    return m_Output;
  }

  [[nodiscard]]
  const fs::path& project_paths::main_cpp() const noexcept
  {
    return m_MainCpp;
  }

  [[nodiscard]]
  const fs::path& project_paths::main_cpp_dir() const noexcept
  {
    return m_MainCppDir;
  }

  [[nodiscard]]
  const fs::path& project_paths::include_target() const noexcept
  {
    return m_IncludeTarget;
  }

  [[nodiscard]]
  const fs::path& project_paths::cmade_build_dir() const noexcept
  {
    return m_CMadeBuildDir;
  }

  [[nodiscard]]
  fs::path aux_files_path(fs::path projectRoot)
  {
    return projectRoot/"aux_files";
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

  [[nodiscard]]
  fs::path prune_path(const project_paths& projPaths)
  {
    const auto& cmadeBuildDir{projPaths.cmade_build_dir()};
    if(cmadeBuildDir.empty())
      throw std::runtime_error{"Build directory required for pruning"};

    const auto relPath{fs::relative(cmadeBuildDir, projPaths.project_root())};
    auto outputDir{projPaths.output() / relPath};

    fs::create_directories(outputDir);

    return (outputDir /= *(--cmadeBuildDir.end())).concat(".prune");
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
