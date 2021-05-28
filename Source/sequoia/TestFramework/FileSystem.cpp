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
  [[nodiscard]]
  std::filesystem::path working_path()
  {
    return working_path_v;
  }

  [[nodiscard]]
  std::filesystem::path project_root(int argc, char** argv, const std::filesystem::path& fallback)
  {
    namespace fs = std::filesystem;

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
          p = p.parent_path();
          if(last == "build") break;
        }

        return p;
      }
    }

    return fallback;
  }

  project_paths::project_paths(const std::filesystem::path& projectRoot)
    : project_paths{projectRoot, projectRoot / "TestAll" / "TestMain.cpp", projectRoot / "TestAll" / "TestMain.cpp"}
  {}

  project_paths::project_paths(const std::filesystem::path& projectRoot, std::filesystem::path mainCpp, std::filesystem::path includePath)
    : m_ProjectRoot{projectRoot}
    , m_Source{source_path(projectRoot)}
    , m_SourceRoot{m_Source.parent_path()}
    , m_Tests{projectRoot / "Tests"}
    , m_TestMaterials{projectRoot / "TestMaterials"}
    , m_Output{projectRoot / "output"}
    , m_MainCpp{std::move(mainCpp)}
    , m_MainCppDir{m_MainCpp.parent_path()}
    , m_IncludeTarget{std::move(includePath)}
  {
    throw_unless_directory(m_ProjectRoot, "\nTest repository not found");
    throw_unless_regular_file(m_MainCpp, "\nTry ensuring that the application is run from the appropriate directory");
    throw_unless_regular_file(m_IncludeTarget, "\nInclude target not found");
  }

  [[nodiscard]]
  std::filesystem::path project_paths::source_path(const std::filesystem::path& projectRoot)
  {
    if(projectRoot.empty())
      throw std::runtime_error{"Project root should not be empty"};

    return projectRoot / "Source" / uncapitalize((--projectRoot.end())->generic_string());
  }

  [[nodiscard]]
  const std::filesystem::path& project_paths::project_root() const noexcept
  {
    return m_ProjectRoot;
  }

  [[nodiscard]]
  const std::filesystem::path& project_paths::source() const noexcept
  {
    return m_Source;
  }

  [[nodiscard]]
  const std::filesystem::path& project_paths::source_root() const noexcept
  {
    return m_SourceRoot;
  }

  [[nodiscard]]
  const std::filesystem::path& project_paths::tests() const noexcept
  {
    return m_Tests;
  }

  [[nodiscard]]
  const std::filesystem::path& project_paths::test_materials() const noexcept
  {
    return m_TestMaterials;
  }

  [[nodiscard]]
  const std::filesystem::path& project_paths::output() const noexcept
  {
    return m_Output;
  }

  [[nodiscard]]
  const std::filesystem::path& project_paths::main_cpp() const noexcept
  {
    return m_MainCpp;
  }

  [[nodiscard]]
  const std::filesystem::path& project_paths::main_cpp_dir() const noexcept
  {
    return m_MainCppDir;
  }

  [[nodiscard]]
  const std::filesystem::path& project_paths::include_target() const noexcept
  {
    return m_IncludeTarget;
  }

  [[nodiscard]]
  std::filesystem::path aux_files_path(std::filesystem::path projectRoot)
  {
    return projectRoot/"aux_files";
  }

  [[nodiscard]]
  std::filesystem::path build_system_path(std::filesystem::path projectRoot)
  {
    return projectRoot / "build_system";
  }

  [[nodiscard]]
  std::filesystem::path code_templates_path(std::filesystem::path projectRoot)
  {
    return projectRoot/"aux_files"/"TestTemplates";
  }

  [[nodiscard]]
  std::filesystem::path source_templates_path(std::filesystem::path projectRoot)
  {
    return projectRoot / "aux_files" / "SourceTemplates";
  }

  [[nodiscard]]
  std::filesystem::path project_template_path(std::filesystem::path projectRoot)
  {
    return projectRoot/"aux_files"/"ProjectTemplate";
  }

  [[nodiscard]]
  std::filesystem::path recovery_path(std::filesystem::path outputDir)
  {
    return outputDir /= "Recovery";
  }

  [[nodiscard]]
  std::filesystem::path tests_temporary_data_path(std::filesystem::path outputDir)
  {
    return outputDir /= "TestsTemporaryData";
  }

  [[nodiscard]]
  std::filesystem::path diagnostics_output_path(std::filesystem::path outputDir)
  {
    return outputDir /= "DiagnosticsOutput";
  }

  [[nodiscard]]
  std::filesystem::path test_summaries_path(std::filesystem::path outputDir)
  {
    return outputDir /= "TestSummaries";
  }

  void throw_unless_exists(const std::filesystem::path& p, std::string_view message)
  {
    namespace fs = std::filesystem;

    throw_if(p, append_lines(p.empty() ? "File path is empty" :"not found", message),
             [](const fs::path& p){ return !fs::exists(p); });
  }

  void throw_unless_directory(const std::filesystem::path& p, std::string_view message)
  {
    namespace fs = std::filesystem;

    throw_if(p, append_lines(p.empty() ? "File path is empty" : "is not a directory", message), [](const fs::path& p){ return !fs::is_directory(p); });
  }

  void throw_unless_regular_file(const std::filesystem::path& p, std::string_view message)
  {
    namespace fs = std::filesystem;

    throw_unless_exists(p, message);
    throw_if(p, append_lines(" is not a regular file", message), [](const fs::path& p){ return !fs::is_regular_file(p); });
  }

  [[nodiscard]]
  std::filesystem::path find_in_tree(const std::filesystem::path& root, const std::filesystem::path& toFind)
  {
    throw_unless_directory(root, "");

    using dir_iter = std::filesystem::recursive_directory_iterator;

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
  std::filesystem::path rebase_from(const std::filesystem::path& p, const std::filesystem::path& dir)
  {
    namespace fs = std::filesystem;

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

      while((*dirIter == *i) && (dirIter != dir.end()) && (i != p.end()))
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
