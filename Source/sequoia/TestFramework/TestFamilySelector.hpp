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
      check_for_duplicates(name, test, tests...);
      
      const bool done{
        [this, name](Test&& test, Tests&&... tests) {
          if(!m_SelectedSources.empty())
          {
            using family_t = test_family<std::remove_cvref_t<Test>, std::remove_cvref_t<Tests>...>;
            family_t f{std::string{name}, m_Paths.tests(), m_Paths.test_materials(), m_Paths.output(), m_Recovery};
            auto setter{f.make_materials_setter()};
            
            add_tests(f, setter, std::forward<Test>(test), std::forward<Tests>(tests)...);
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
          m_Families.push_back(test_family{std::string{name}, m_Paths.tests(), m_Paths.test_materials(), m_Paths.output(), m_Recovery,
            std::forward<Test>(test), std::forward<Tests>(tests)...});
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

    template<std::input_or_output_iterator Iter>
    void update_prune_info(Iter startFailedTests, Iter endFailedTests);

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

    project_paths              m_Paths;
    recovery_paths             m_Recovery{};
    prune_info                 m_PruneInfo{};
    std::vector<family_vessel> m_Families{};
    family_map                 m_SelectedFamilies{};
    source_list                m_SelectedSources{};

    bool mark_family(std::string_view name);

    [[nodiscard]]
    auto find_filename(const std::filesystem::path& filename)->source_list::iterator;

    using duplicate_set = std::set<std::pair<std::string_view, std::filesystem::path>>;

    template<class Test, class... Tests>
    static void check_for_duplicates(std::string_view name, const Test& test, const Tests&... tests)
    {
       duplicate_set namesAndSources{};
       check_for_duplicates(namesAndSources, name, test, tests...);   
    }

    template<class Test, class... Tests>
    static void check_for_duplicates(duplicate_set& namesAndSources,
                                     std::string_view name,
                                     const Test& test,
                                     const Tests&... tests)
    {
      if(!namesAndSources.emplace(test.name(), test.source_filename()).second)
        throw std::runtime_error{duplication_message(name, test.name(), test.source_filename())};

      if constexpr(sizeof...(Tests) > 0)
      {
        check_for_duplicates(namesAndSources, name,tests...);
      }
    }

    template<concrete_test... AllTests, concrete_test Test, concrete_test... Tests>
    void add_tests(test_family<AllTests...>& f,
                   family_info::materials_setter& setter,
                   Test&& test,
                   Tests&&... tests)
    {
      auto i{find_filename(test.source_filename())};
      if(i != m_SelectedSources.end())
      {
        f.add_test(setter, std::forward<Test>(test));
        i->second = true;
      }

      if constexpr(sizeof...(Tests) > 0) add_tests(f, setter, std::forward<Tests>(tests)...);
    }

    [[nodiscard]]
    static 
    std::string duplication_message(std::string_view familyName,
                                    std::string_view testName,
                                    const std::filesystem::path& source);
  };
}
