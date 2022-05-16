////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for TestFamilySelector.hpp
 */

#include "sequoia/TestFramework/TestFamilySelector.hpp"

#include "sequoia/TestFramework/DependencyAnalyzer.hpp"
#include "sequoia/Parsing/CommandLineArguments.hpp"

#include <fstream>

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  namespace
  {
    const std::string& convert(const std::string& s) { return s; }
    std::string convert(const std::filesystem::path& p) { return p.generic_string(); }

    [[nodiscard]]
    std::vector<fs::path> read_failures(const fs::path& file)
    {
      std::vector<fs::path> failures{};
      if(fs::exists(file))
      {
        if(std::ifstream ifile{file})
        {
          while(ifile)
          {
            fs::path filePath{};
            ifile >> filePath;
            if(!filePath.empty())
              failures.push_back(std::move(filePath));
          }
        }
      }

      return failures;
    }

    template<std::input_or_output_iterator Iter, std::sentinel_for<Iter> Sentinel>
      requires std::same_as<typename Iter::value_type, fs::path>
    void write_failures(const fs::path& file, Iter first, Sentinel last)
    {
      if(std::ofstream ostream{file})
      {
        while(first != last)
        {
          ostream << (first++)->generic_string() << "\n";
        }
      }
    }
  }

  auto family_selector::time_stamps::from_file(const std::filesystem::path& stampFile) -> stamp
  {
    if(fs::exists(stampFile))
    {
      return fs::last_write_time(stampFile);
    }

    return std::nullopt;
  }

  family_selector::family_selector(project_paths paths)
    : m_Paths{std::move(paths)}
    , m_PruneInfo{time_stamps::from_file(proj_paths().prune_file_path(std::nullopt)),
                  time_stamps::from_file(proj_paths().executable())}
  {}

  const project_paths& family_selector::proj_paths() const noexcept
  {
    return m_Paths;
  }

  void family_selector::select_family(std::string name)
  {
    m_SelectedFamilies.emplace(std::move(name), false);
  }

  void family_selector::select_source_file(const normal_path& file)
  {
    m_SelectedSources.emplace_back(file, false);
  }

  void family_selector::enable_prune()
  {
    m_PruneInfo.mode = prune_mode::active;
  }

  void family_selector::set_prune_cutoff(std::string cutoff)
  {
    m_PruneInfo.include_cutoff = std::move(cutoff);
  }

  void family_selector::prune(std::ostream& stream)
  {
    if(!pruned()) return;

    stream << "\nAnalyzing dependencies...\n";
    const timer t{};

    if(auto maybeToRun{tests_to_run(proj_paths().source_root(), proj_paths().tests(), proj_paths().test_materials(), proj_paths().prune_file_path(std::nullopt), m_PruneInfo.stamps.ondisk, m_PruneInfo.stamps.executable, m_PruneInfo.include_cutoff)})
    {
      auto& toRun{maybeToRun.value()};

      std::transform(toRun.begin(), toRun.end(), std::back_inserter(m_SelectedSources),
        [](const fs::path& file) -> std::pair<fs::path, bool> { return {file, false}; });
    }

    const auto [dur, unit] {testing::stringify(t.time_elapsed())};
    stream << "[" << dur << unit << "]\n\n";
  }

  template<std::input_or_output_iterator Iter, std::sentinel_for<Iter> Sentinel>
  void family_selector::update_prune_info(Iter startFailedTests, Sentinel endFailedTests, const std::optional<std::size_t> id)
  {
    std::sort(startFailedTests, endFailedTests);
    if(!pruned() && bespoke_selection())
    {
      if(m_PruneInfo.stamps.ondisk.has_value())
      {
        std::vector<fs::path> executedTests{};
        for(const auto& src : m_SelectedSources)
        {
          if(src.second) executedTests.push_back(src.first);
        }

        std::sort(executedTests.begin(), executedTests.end());

        std::vector<fs::path> passingTests{};
        std::set_difference(executedTests.begin(),
                            executedTests.end(),
                            startFailedTests,
                            endFailedTests,
                            std::back_inserter(passingTests));

        if(!passingTests.empty())
        {
          const auto pruneFile{proj_paths().prune_file_path(id)};
          auto previousFailures{read_failures(pruneFile)};

          std::vector<fs::path> remainingFailures{};
          std::set_difference(previousFailures.begin(),
                              previousFailures.end(),
                              passingTests.begin(),
                              passingTests.end(),
                              std::back_inserter(remainingFailures));

          write_failures(pruneFile, remainingFailures.begin(), remainingFailures.end());
          fs::last_write_time(pruneFile, m_PruneInfo.stamps.ondisk.value());
        }
      }
    }
    else
    {
      const auto pruneFile{proj_paths().prune_file_path(id)};
      write_failures(pruneFile, startFailedTests, endFailedTests);
      fs::last_write_time(pruneFile, m_PruneInfo.stamps.current);
    }
  }

  void family_selector::aggregate_instability_analysis_prune_files(const std::size_t numReps) const
  {
    if(!bespoke_selection() || m_PruneInfo.stamps.ondisk.has_value())
    {
      std::vector<fs::path> failures{};
      for(std::size_t i{}; i < numReps; ++i)
      {
        const auto pruneFile{proj_paths().prune_file_path(i)};

        if(fs::exists(pruneFile))
        {
          if(std::ifstream ifile{pruneFile})
          {
            while(ifile)
            {
              fs::path filePath{};
              ifile >> filePath;
              if(!filePath.empty())
                failures.push_back(std::move(filePath));
            }
          }
        }
      }

      std::sort(failures.begin(), failures.end());
      auto last{std::unique(failures.begin(), failures.end())};
      failures.erase(last, failures.end());

      const auto grandPruneFile{proj_paths().prune_file_path(std::nullopt)};
      if(std::ofstream ofile{grandPruneFile})
      {
        for(const auto& p : failures)
          ofile << p.generic_string() << '\n';
      }

      fs::last_write_time(grandPruneFile, !bespoke_selection() ? m_PruneInfo.stamps.current : m_PruneInfo.stamps.ondisk.value());
    }
  }

  [[nodiscard]]
  std::string family_selector::check_argument_consistency()
  {
    if(pruned() && bespoke_selection())
    {
      using parsing::commandline::warning;
      m_PruneInfo.mode = prune_mode::passive;
      return warning("'prune' ignored if either test families or test source files are specified");
    }

    return "";
  }

  [[nodiscard]]
  std::string family_selector::check_for_missing_tests() const
  {
    if(pruned()) return "";

    std::string messages{};

    auto check{
      [&messages] (const auto& tests, std::string_view type, auto fn) {
        for(const auto& test : tests)
        {
          if(!test.second)
          {
            using namespace parsing::commandline;
            messages += warning(std::string{"Test "}.append(type)
                                                    .append(" '")
                                                    .append(convert(test.first))
                                                    .append("' not found\n")
                                                    .append(fn(test.first)));
          }
        }
      }
    };

    check(m_SelectedFamilies, "Family", [](const std::string& name) -> std::string {
        if(auto pos{name.rfind('.')}; pos < std::string::npos)
        {
          return "--If trying to select a source file use 'select' rather than 'test'\n";
        }
        
        return "";
      }
    );

    check(m_SelectedSources, "File", [](const std::filesystem::path& p) -> std::string {
        if(!p.has_extension())
        {
          return "--If trying to test a family use 'test' rather than 'select'\n";
        }
        
        return "";
      }
    );

    return messages;
  }

  [[nodiscard]]
  bool family_selector::pruned() const noexcept
  {
    return m_PruneInfo.mode == prune_mode::active;
  }

  bool family_selector::mark_family(std::string_view name)
  {
    if(m_SelectedFamilies.empty()) return true;

    auto i{m_SelectedFamilies.find(name)};
    if(i != m_SelectedFamilies.end())
    {
      i->second = true;
      return true;
    }

    return false;
  }

  [[nodiscard]]
  auto family_selector::find_filename(const normal_path& filename) -> source_list::iterator
  {
    return std::find_if(m_SelectedSources.begin(), m_SelectedSources.end(),
      [&filename, &repo = m_Paths.tests(), &root = m_Paths.project_root()](const auto& element){
      const auto& source{element.first};

      if(filename.path().empty() || source.path().empty() || (back(source) != back(filename)))
        return false;

      if(filename == source) return true;

      // filename is relative to where compilation was performed which
      // cannot be known here. Therefore fallback to assuming the 'selected sources'
      // live in the test repository

      if(!repo.empty())
      {
        if(rebase_from(source, repo) == rebase_from(filename, repo))
          return true;

        if(const auto path{find_in_tree(repo, source)}; !path.empty())
        {
          if(rebase_from(path, repo) == rebase_from(filename, repo))
            return true;
        }
      }

      return false;
    });
  }

  [[nodiscard]]
  std::string family_selector::duplication_message(std::string_view familyName,
                                                   std::string_view testName,
                                                   const std::filesystem::path& source)
  {
    using namespace parsing::commandline;
    
    return error(std::string{"Family/Test: \""}
             .append(familyName).append("/").append(testName).append("\"\n")
             .append("Source file: \"").append(source.generic_string()).append("\"\n")
             .append("Please do not include tests in the same family"
                     " which both have the same name and are defined"
                     " in the same source file.\n"));
  }

  template void family_selector::update_prune_info<std::vector<fs::path>::iterator>(std::vector<fs::path>::iterator, std::vector<fs::path>::iterator, std::optional<std::size_t>);
}
