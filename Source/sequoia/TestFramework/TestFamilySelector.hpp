////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "sequoia/TestFramework/FileSystem.hpp"
#include "sequoia/TestFramework/TestFamily.hpp"

#include <filesystem>
#include <optional>
#include <map>

namespace sequoia::testing
{
  class family_selector
  {
  public:
    explicit family_selector(project_paths paths);

    template<class Test, class... Tests>
    void add_test_family(std::string_view name, Test&& test, Tests&&... tests)
    {
      const bool done{
        [this, name](Test&& test, Tests&&... tests) {
          if(!m_SelectedSources.empty())
          {
            test_family f{name, m_Paths.tests(), m_Paths.test_materials(), m_Paths.output(), m_Recovery};
            add_tests(f, std::forward<Test>(test), std::forward<Tests>(tests)...);
            if(!f.empty())
            {
              m_Families.push_back(std::move(f));
              mark_family(name);
              return true;
            }
          }

          return false;
        }(std::forward<Test>(test), std::forward<Tests>(tests)...)
      };

      if(!done && ((m_SelectedSources.empty() && !pruned()) || !m_SelectedFamilies.empty()))
      {
        if(mark_family(name))
        {
          m_Families.emplace_back(name, m_Paths.tests(), m_Paths.test_materials(), m_Paths.output(), m_Recovery,
            std::forward<Test>(test), std::forward<Tests>(tests)...);
        }
      }
    }

    const project_paths& proj_paths() const noexcept;

    [[nodiscard]]
    const std::filesystem::path& recovery_file() const noexcept;

    void recovery_file(std::filesystem::path recovery);

    [[nodiscard]]
    const std::filesystem::path& dump_file() const noexcept;

    void dump_file(std::filesystem::path dump);

    void select_family(std::string name);

    void select_source_file(std::filesystem::path file);

    void enable_prune();

    void set_prune_cutoff(std::string cutoff);

    [[nodiscard]]
    bool pruned() const noexcept;

    void prune(std::ostream& stream);

    void update_prune_info(const std::vector<std::filesystem::path>& failedTests);

    void executable_time_stamp(const std::filesystem::path& exe);

    [[nodiscard]]
    std::string check_argument_consistency(concurrency_mode mode);

    [[nodiscard]]
    std::string check_for_missing_tests() const;

    [[nodiscard]]
    auto begin() noexcept
    {
      return m_Families.begin();
    }

    [[nodiscard]]
    auto end() noexcept
    {
      return m_Families.end();
    }

    [[nodiscard]]
    bool empty() const noexcept
    {
      return m_Families.empty();
    }

    [[nodiscard]]
    std::size_t size() const noexcept
    {
      return m_Families.size();
    }

    [[nodiscard]]
    bool bespoke_selection() const noexcept
    {
      return !m_SelectedFamilies.empty() || !m_SelectedSources.empty();
    }
  private:
    struct time_stamps
    {
      using time_type = std::filesystem::file_time_type;
      using stamp = std::optional<time_type>;

      static auto from_file(const std::filesystem::path& stampFile)->stamp;

      time_type current{std::chrono::file_clock::now()};
      stamp ondisk, executable;
    };

    struct prune_info
    {
      time_stamps stamps{};
      std::string include_cutoff{};
    };

    using family_map = std::map<std::string, bool, std::less<>>;
    using source_list = std::vector<std::pair<std::filesystem::path, bool>>;

    project_paths             m_Paths;
    recovery_paths            m_Recovery{};
    prune_info                m_PruneInfo{};
    std::vector<test_family>  m_Families{};
    family_map                m_SelectedFamilies{};
    source_list               m_SelectedSources{};

    bool mark_family(std::string_view name);

    [[nodiscard]]
    auto find_filename(const std::filesystem::path& filename)->source_list::iterator;

    template<class Test, class... Tests>
    void add_tests(test_family& f, Test&& test, Tests&&... tests)
    {
      auto i{find_filename(test.source_filename())};
      if(i != m_SelectedSources.end())
      {
        f.add_test(std::forward<Test>(test));
        i->second = true;
      }

      if constexpr(sizeof...(Tests) > 0) add_tests(f, std::forward<Tests>(tests)...);
    }
  };
}