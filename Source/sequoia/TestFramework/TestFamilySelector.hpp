////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Mechanism for selecting which test families to run.
 */

#include "sequoia/TestFramework/DependencyAnalyzer.hpp"
#include "sequoia/TestFramework/ProjectPaths.hpp"
#include "sequoia/TestFramework/TestFamily.hpp"

#include "sequoia/FileSystem/FileSystem.hpp"

#include <optional>
#include <map>
#include <set>

namespace sequoia::testing
{
  enum class prune_outcome { not_attempted, no_time_stamp, success};

  class family_selector
  {
  public:
    using time_type = std::filesystem::file_time_type;

    explicit family_selector(project_paths paths);

    template<concrete_test... Tests>
      requires (sizeof...(Tests) > 0)
    void add_test_family(std::string_view name, recovery_mode recoveryMode, Tests... tests)
    {
      m_FamiliesPresented = true;

      check_for_duplicates(name, tests...);

      const bool done{
        [this, name, recoveryMode](Tests&... tests) {
          if(!m_SelectedSources.empty())
          {
            using family_t = test_family<std::remove_cvref_t<Tests>...>;
            family_t f{std::string{name}, m_Paths, recoveryMode};
            std::vector<std::filesystem::path> materialsPath{};

            add_tests(f, materialsPath, std::move(tests)...);
            if(!f.empty())
            {
              m_Families.push_back(std::move(f));
              mark_family(name);
              return true;
            }
          }

          return false;
        }(tests...)
      };

      if(!done && (((pruned() == prune_mode::passive) && m_SelectedSources.empty()) || !m_SelectedFamilies.empty()))
      {
        if(mark_family(name))
        {
          m_Families.push_back(test_family{std::string{name}, m_Paths, recoveryMode, std::move(tests)...});
        }
      }
    }

    const project_paths& proj_paths() const noexcept;

    void select_family(std::string name);

    void select_source_file(const normal_path& file);

    void enable_prune();

    void set_prune_cutoff(std::string cutoff);

    [[nodiscard]]
    prune_mode pruned() const noexcept
    {
      return m_PruneInfo.mode;
    }

    [[nodiscard]]
    prune_outcome prune();

    [[nodiscard]]
    time_type current_time_stamp() const noexcept
    {
      return m_PruneInfo.stamps.current;
    }

    void update_prune_info(std::vector<std::filesystem::path> failedTests, std::optional<std::size_t> id) const;

    [[nodiscard]]
    std::string check_for_missing_tests() const;

    [[nodiscard]]
    std::string check_argument_consistency();

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
    bool families_presented() const noexcept
    {
      return m_FamiliesPresented;
    }


    [[nodiscard]]
    auto begin_selected_sources() const noexcept
    {
      return m_SelectedSources.begin();
    }

    [[nodiscard]]
    auto end_selected_sources() const noexcept
    {
      return m_SelectedSources.end();
    }

    [[nodiscard]]
    auto begin_selected_families() const noexcept
    {
      return m_SelectedFamilies.begin();
    }

    [[nodiscard]]
    auto end_selected_families() const noexcept
    {
      return m_SelectedFamilies.end();
    }
  private:

    struct time_stamps
    {
      using stamp = std::optional<time_type>;

      static auto from_file(const std::filesystem::path& stampFile)->stamp;

      stamp ondisk, executable;
      time_type current{std::chrono::file_clock::now()};
    };

    struct prune_info
    {
      time_stamps stamps{};
      prune_mode mode{prune_mode::passive};
      std::string include_cutoff{};
    };

    using family_map  = std::map<std::string, bool, std::less<>>;
    using source_list = std::vector<std::pair<normal_path, bool>>;

    project_paths              m_Paths;
    prune_info                 m_PruneInfo{};
    std::vector<family_vessel> m_Families{};
    family_map                 m_SelectedFamilies{};
    source_list                m_SelectedSources{};
    bool                       m_FamiliesPresented{};

    bool mark_family(std::string_view name);

    [[nodiscard]]
    auto find_filename(const normal_path& filename) -> source_list::iterator;

    [[nodiscard]]
    std::vector<std::filesystem::path> get_executed_tests() const;

    using duplicate_set = std::set<std::pair<std::string_view, std::filesystem::path>>;

    template<concrete_test... Tests>
      requires (sizeof...(Tests) > 0)
    static void check_for_duplicates(std::string_view name, const Tests&... tests)
    {
       duplicate_set namesAndSources{};

       auto check{
         [&,name](concrete_test auto const& test) {
           if(!namesAndSources.emplace(test.name(), test.source_filename()).second)
             throw std::runtime_error{duplication_message(name, test.name(), test.source_filename())};
         }
       };

       (check(tests),...);
    }

    template<concrete_test... AllTests, concrete_test... Tests>
      requires (sizeof...(Tests) > 0)
    void add_tests(test_family<AllTests...>& f,
                   std::vector<std::filesystem::path>& materialsPath,
                   Tests&&... tests)
    {
      auto add{
        [&]<concrete_test T>(T&& test) {
          const normal_path src{test.source_filename()};
          auto i{find_filename(src)};
          if(i != m_SelectedSources.end())
          {
            f.add_test(materialsPath, std::forward<T>(test));
            i->first = rebase_from(src, proj_paths().tests().repo());
            i->second = true;
          }
        }
      };

      (add(std::forward<Tests>(tests)), ...);
    }

    [[nodiscard]]
    bool bespoke_selection() const noexcept
    {
      return !m_SelectedFamilies.empty() || !m_SelectedSources.empty();
    }

    [[nodiscard]]
    static std::string duplication_message(std::string_view familyName,
                                           std::string_view testName,
                                           const std::filesystem::path& source);
  };
}
