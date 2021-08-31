////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Utilities for running tests from the command line.
*/

#include "sequoia/TestFramework/TestFamily.hpp"

#include "sequoia/TestFramework/TestCreator.hpp"
#include "sequoia/TestFramework/ProjectCreator.hpp"

#include "sequoia/Parsing/CommandLineArguments.hpp"
#include "sequoia/PlatformSpecific/Helpers.hpp"
#include "sequoia/TextProcessing/Indent.hpp"
#include "sequoia/Runtime/Factory.hpp"

#include <map>
#include <iostream>
#include <chrono>

namespace sequoia::testing
{
  enum class runner_mode { none=0, help=1, test=2, create=4, init=8};

  template<>
  struct as_bitmask<runner_mode> : std::true_type
  {};

  [[nodiscard]]
  std::string report_time(const test_family::summary& s);

  /*! \brief Consumes command-line arguments and holds all test families

      If no arguments are specified, all tests are run, in serial, with the diagnostic
      files generated; run with --help for information on the various options

   */

  class test_runner
  {
  public:
    test_runner(int argc, char** argv, std::string copyright, project_paths paths, std::string codeIndent="  ", std::ostream& stream=std::cout);

    test_runner(const test_runner&)     = delete;
    test_runner(test_runner&&) noexcept = default;

    test_runner& operator=(const test_runner&)     = delete;
    test_runner& operator=(test_runner&&) noexcept = default;

    template<class Test, class... Tests>
    void add_test_family(std::string_view name, Test&& test, Tests&&... tests)
    {
      const bool done{
        [this, name](Test&& test, Tests&&... tests){
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

      if(!done && ( (m_SelectedSources.empty() && !pruned()) || !m_SelectedFamilies.empty() ))
      {
        if(mark_family(name))
        {
          m_Families.emplace_back(name, m_Paths.tests(), m_Paths.test_materials(), m_Paths.output(), m_Recovery,
                                  std::forward<Test>(test), std::forward<Tests>(tests)...);
        }
      }
    }

    void execute([[maybe_unused]] timer_resolution r={});

    [[nodiscard]]
    concurrency_mode concurrency() const noexcept { return m_ConcurrencyMode; }

  private:
    enum class output_mode { standard = 0, verbose = 1 };
    using creation_factory = runtime::factory<nascent_semantics_test, nascent_allocation_test, nascent_behavioural_test>;
    using vessel = typename creation_factory::vessel;

    struct nascent_test_data
    {
      nascent_test_data(std::string type, std::string subType, test_runner& r, std::vector<vessel>& nascentTests);

      void operator()(const parsing::commandline::arg_list& args);

      std::string genus, species;
      test_runner& runner;
      std::vector<vessel>& nascent_tests;
    };

    struct time_stamps
    {
      using time_type = std::filesystem::file_time_type;
      using stamp = std::optional<time_type>;

      static auto from_file(const std::filesystem::path& stampFile) -> stamp;

      time_type current{std::chrono::file_clock::now()};
      stamp ondisk, executable;
    };

    struct prune_info
    {
      time_stamps stamps{};
      std::string include_cutoff{};
    };

    using family_map        = std::map<std::string, bool, std::less<>>;
    using source_list       = std::vector<std::pair<std::filesystem::path, bool>>;

    std::string               m_Copyright{};
    project_paths             m_Paths;
    indentation               m_CodeIndent{"  "};
    std::ostream*             m_Stream;

    prune_info                m_PruneInfo{};
    std::vector<test_family>  m_Families{};
    family_map                m_SelectedFamilies{};
    source_list               m_SelectedSources{};
    std::vector<project_data> m_NascentProjects{};
    recovery_paths            m_Recovery{};
    runner_mode               m_RunnerMode{runner_mode::none};
    output_mode               m_OutputMode{output_mode::standard};
    update_mode               m_UpdateMode{update_mode::none};
    concurrency_mode          m_ConcurrencyMode{concurrency_mode::serial};

    std::vector<std::filesystem::path> m_FailedTestSourceFiles;

    std::ostream& stream() noexcept { return *m_Stream; }

    [[nodiscard]]
    bool pruned() const noexcept;

    bool mark_family(std::string_view name);

    void process_args(int argc, char** argv);

    void finalize_concurrency_mode();

    [[nodiscard]]
    test_family::summary process_family(const test_family::results& results);

    [[nodiscard]]
    bool concurrent_execution() const noexcept { return m_ConcurrencyMode != concurrency_mode::serial; }

    void check_argument_consistency();

    void run_tests();

    void check_for_missing_tests();

    [[nodiscard]]
    auto find_filename(const std::filesystem::path& filename) -> source_list::iterator;

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

    [[nodiscard]]
    static std::string stringify(concurrency_mode mode);

    void finalize_nascent_tests();

    void init_projects();

    [[nodiscard]]
    bool mode(runner_mode m) const noexcept
    {
      return (m_RunnerMode & m) == m;
    }
 };
}
