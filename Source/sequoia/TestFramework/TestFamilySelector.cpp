////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for TestFamilySelector.hpp"
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
  {}

  const project_paths& family_selector::proj_paths() const noexcept
  {
    return m_Paths;
  }

  [[nodiscard]]
  const std::filesystem::path& family_selector::recovery_file() const noexcept
  {
    return m_Recovery.recovery_file;
  }

  void family_selector::recovery_file(std::filesystem::path recovery)
  {
    m_Recovery.recovery_file = std::move(recovery);
  }

  [[nodiscard]]
  const std::filesystem::path& family_selector::dump_file() const noexcept
  {
    return m_Recovery.dump_file;
  }

  void family_selector::dump_file(std::filesystem::path dump)
  {
    m_Recovery.dump_file = std::move(dump);
  }

  void family_selector::select_family(std::string name)
  {
    m_SelectedFamilies.emplace(std::move(name), false);
  }

  void family_selector::select_source_file(std::filesystem::path file)
  {
    m_SelectedSources.emplace_back(std::move(file), false);
  }

  void family_selector::enable_prune()
  {
    m_PruneInfo.stamps.ondisk = time_stamps::from_file(prune_path(proj_paths().output(), proj_paths().main_cpp_dir()));
  }

  void family_selector::set_prune_cutoff(std::string cutoff)
  {
    m_PruneInfo.include_cutoff = std::move(cutoff);
  }

  void family_selector::prune(std::ostream& stream)
  {
    if(!pruned()) return;

    stream << "\nAnalyzing dependencies...\n";
    const auto start{std::chrono::steady_clock::now()};

    if(auto maybeToRun{tests_to_run(proj_paths().source_root(), proj_paths().tests(), proj_paths().test_materials(), m_PruneInfo.stamps.ondisk, m_PruneInfo.stamps.executable, m_PruneInfo.include_cutoff)})
    {
      auto& toRun{maybeToRun.value()};

      if(std::ifstream ifile{prune_path(proj_paths().output(), proj_paths().main_cpp_dir())})
      {
        while(ifile)
        {
          fs::path source{};
          ifile >> source;
          toRun.push_back(source);
        }
      }

      std::sort(toRun.begin(), toRun.end());
      auto last{std::unique(toRun.begin(), toRun.end())};
      toRun.erase(last, toRun.end());

      std::transform(toRun.begin(), toRun.end(), std::back_inserter(m_SelectedSources),
        [](const fs::path& file) -> std::pair<fs::path, bool> { return {file, false}; });
    }

    const auto end{std::chrono::steady_clock::now()};

    const auto [dur, unit] {testing::stringify(end - start)};
    stream << "[" << dur << unit << "]\n\n";
  }

  void family_selector::update_prune_info(const std::vector<std::filesystem::path>& failedTests)
  {
    if(!bespoke_selection() || pruned())
    {
      const auto stampFile{prune_path(proj_paths().output(), proj_paths().main_cpp_dir())};
      if(std::ofstream ostream{stampFile})
      {
        for(const auto& source : failedTests)
        {
          ostream << source.generic_string() << "\n";
        }
      }

      fs::last_write_time(stampFile, m_PruneInfo.stamps.current);
    }
  }

  void family_selector::executable_time_stamp(const std::filesystem::path& exe)
  {
    if(fs::exists(exe))
      m_PruneInfo.stamps.executable = fs::last_write_time(exe);
  }

  [[nodiscard]]
  std::string family_selector::check_argument_consistency(concurrency_mode mode)
  {
    using parsing::commandline::error;

    if((mode != concurrency_mode::serial) && (!recovery_file().empty() || !dump_file().empty()))
      throw std::runtime_error{error("Can't run asynchronously in recovery/dump mode\n")};

    if(pruned())
    {
      if((!m_SelectedFamilies.empty() || !m_SelectedSources.empty()))
      {
        using parsing::commandline::warning;
        m_PruneInfo.stamps.ondisk = std::nullopt;
        return warning("'prune' ignored if either test families or test source files are specified");
      }
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
    return m_PruneInfo.stamps.ondisk.has_value();
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
  auto family_selector::find_filename(const std::filesystem::path& filename) -> source_list::iterator
  {
    return std::find_if(m_SelectedSources.begin(), m_SelectedSources.end(),
      [&filename, &repo = m_Paths.tests(), &root = m_Paths.project_root()](const auto& element){
      const auto& source{element.first};

      if(filename.empty() || source.empty() || ((*--source.end()) != (*--filename.end())))
        return false;

      if(filename == source) return true;

      if(filename.is_absolute())
      {
        if(rebase_from(filename, root) == rebase_from(source, working_path()))
          return true;
      }

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
}