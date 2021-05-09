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

namespace sequoia::testing
{
  namespace
  {    
    [[nodiscard]]
    std::string report_file_issue(const std::filesystem::path& file, std::string_view description)
    {
      auto mess{std::string{"Unable to open file "}.append(file.generic_string())};
      if(!description.empty()) mess.append(" ").append(description);
      mess.append("\n");

      return mess;
    }
  }

  [[nodiscard]]
  std::filesystem::path project_root(int argc, char** argv, const std::filesystem::path& fallback)
  {
    namespace fs = std::filesystem;

    static fs::path p{};

    if(p.empty())
    {
      if(argc)
      {
        std::string_view zeroth{argv[0]};
        p = fs::canonical(fs::path(zeroth)).parent_path();
        if(!p.empty())
        {
          auto back{[](const fs::path& p){ return *(--p.end()); }};

          while((std::distance(p.begin(), p.end()) > 1))
          {
            const auto last{back(p)};
            p = p.parent_path();
            if(last == "build") break;
          }

          return p;
        }
      }

      p = fallback;
    }

    return p;
  }

  [[nodiscard]]
  std::filesystem::path aux_files_path(std::filesystem::path projectRoot)
  {
    return projectRoot/"aux_files";
  }

  [[nodiscard]]
  std::filesystem::path code_templates_path(std::filesystem::path projectRoot)
  {
    return projectRoot/"aux_files"/"TestTemplates";
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

  [[nodiscard]]
  std::string report_failed_read(const std::filesystem::path& file)
  {
    return report_file_issue(file, " for reading");
  }

  [[nodiscard]]
  std::string report_failed_write(const std::filesystem::path& file)
  {
    return report_file_issue(file, " for writing");
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

    if(!fs::is_directory(dir))
      return p;

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
