////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Utilities for running tests from the command line.
*/

#include "sequoia/TestFramework/DependencyAnalyzer.hpp"
#include "sequoia/TestFramework/PerformanceTestCore.hpp"
#include "sequoia/TestFramework/TestLogger.hpp"
#include "sequoia/TestFramework/VersionedOutput.hpp"

#include "sequoia/Core/Logic/Bitmask.hpp"
#include "sequoia/Core/Object/Factory.hpp"
#include "sequoia/Maths/Graph/DynamicTree.hpp"
#include "sequoia/PlatformSpecific/Helpers.hpp"
#include "sequoia/TextProcessing/Indent.hpp"

#include <chrono>
#include <format>
#include <iostream>
#include <optional>
#include <span>
#include <string>

namespace sequoia::testing
{
  enum class runner_mode : unsigned { none=0, help=1, test=2, create=4, init=8};

  enum class update_mode { none = 0, soft };

  enum class recovery_mode : unsigned { none = 0, recovery = 1, dump = 2 };

  enum class prune_outcome { not_attempted, no_time_stamp, success };

  enum class concurrency_mode {
    serial,    /// serial execution
    dynamic,   /// determined implicitly by the stl
    fixed      /// fixed-size thread pool
  };

  enum class return_code : unsigned { success=0, versioned_output_diffs=1, soft_failures=2, critical_failures=4, incomplete_run=8};

  [[nodiscard]]
  std::string to_string(return_code code);
}

NAMESPACE_SEQUOIA_AS_BITMASK
{
  template<>
  struct as_bitmask<sequoia::testing::runner_mode> : std::true_type {};

  template<>
  struct as_bitmask<sequoia::testing::recovery_mode> : std::true_type {};

  template<>
  struct as_bitmask<sequoia::testing::return_code> : std::true_type {};
}

namespace std
{
  template<>
  struct formatter<sequoia::testing::return_code>
  {
    constexpr auto parse(auto& ctx) { return ctx.begin(); }

    auto format(sequoia::testing::return_code code, auto& ctx) const -> decltype(ctx.out())
    {
      return std::format_to(ctx.out(), "{}", sequoia::testing::to_string(code));
    }
  };
}

namespace sequoia::testing
{
  [[nodiscard]]
  return_code to_return_code(const log_summary& summary) noexcept;

  /** Maps the exit status of a process which ran a sequoia test runner back to the code it
      reported, throwing if the status is not one a runner can produce. */
  [[nodiscard]]
  return_code child_return_code(int exitStatus);

  [[nodiscard]]
  int to_exit_code(return_code code) noexcept;

  individual_materials_paths set_materials(const std::filesystem::path& sourceFile, std::string_view testName, const project_paths& projPaths, std::vector<std::filesystem::path>& materialsPaths);

  [[nodiscard]]
  active_recovery_files make_active_recovery_paths(recovery_mode mode, const project_paths& projPaths);

  class test_vessel
  {
  public:
    template<class Test>
      requires (!std::is_same_v<Test, test_vessel> && concrete_test<Test>)
    test_vessel(Test&& t)
      : m_pTest{std::make_unique<essence<Test>>(std::forward<Test>(t))}
    {
      if constexpr(!is_parallelizable_v<Test>)
        m_Parallelizable = parallelizable_candidate::no;
    }

    test_vessel(const test_vessel&)     = delete;
    test_vessel(test_vessel&&) noexcept = default;

    test_vessel& operator=(const test_vessel&)     = delete;
    test_vessel& operator=(test_vessel&&) noexcept = default;

    [[nodiscard]]
    const std::string& name() const noexcept
    {
      return m_pTest->name();
    }

    [[nodiscard]]
    const test_summary_path& summary_file_path() const noexcept
    {
      return m_pTest->summary_file_path();
    }

    [[nodiscard]]
    std::filesystem::path source_file() const
    {
      return m_pTest->source_file();
    }

    [[nodiscard]]
    std::filesystem::path working_materials() const
    {
      return m_pTest->working_materials();
    }

    [[nodiscard]]
    std::filesystem::path predictive_materials() const
    {
      return m_pTest->predictive_materials();
    }

    [[nodiscard]]
    bool parallelizable() const noexcept
    {
      return m_Parallelizable == parallelizable_candidate::yes;
    }

    [[nodiscard]]
    log_summary execute(std::optional<std::size_t> index)
    {
      return m_pTest->execute(index);
    }

    void reset(const project_paths& projPaths, std::vector<std::filesystem::path>& materialsPaths)
    {
      m_pTest->reset(projPaths, materialsPaths);
    }

    /** \brief Replaces the held test with one which knows where its files are. */

    void initialize(const project_paths& projPaths, std::vector<std::filesystem::path>& materialsPaths, recovery_mode mode)
    {
      m_pTest->initialize(projPaths, materialsPaths, mode);
    }
  private:
    static void versioned_write(const std::filesystem::path& file, const failure_output& output);
    static void versioned_write(const std::filesystem::path& file, std::string_view text);

    struct soul
    {
      virtual ~soul() = default;

      virtual const std::string& name() const noexcept                    = 0;
      virtual const test_summary_path& summary_file_path() const noexcept = 0;
      virtual std::filesystem::path source_file() const                   = 0;
      virtual std::filesystem::path working_materials() const             = 0;
      virtual std::filesystem::path predictive_materials() const          = 0;

      virtual log_summary execute(std::optional<std::size_t> index) = 0;
      virtual void reset(const project_paths& projPaths, std::vector<std::filesystem::path>& materialsPaths) = 0;
      virtual void initialize(const project_paths& projPaths, std::vector<std::filesystem::path>& materialsPaths, recovery_mode mode) = 0;
    };

    template<concrete_test Test>
    class essence final : public soul
    {
    public:
      essence(Test&& t) : m_Test{std::forward<Test>(t)}
      {}

      [[nodiscard]]
      std::filesystem::path source_file() const final
      {
        return m_Test.source_file();
      }

      [[nodiscard]]
      const std::string& name() const noexcept final
      {
        return m_Test.name();
      }

      [[nodiscard]]
      const test_summary_path& summary_file_path() const noexcept final
      {
        return m_Test.summary_file_path();
      }

      [[nodiscard]]
      std::filesystem::path working_materials() const final
      {
        return m_Test.working_materials();
      }

      [[nodiscard]]
      std::filesystem::path predictive_materials() const final
      {
        return m_Test.predictive_materials();
      }

      [[nodiscard]]
      log_summary execute(std::optional<std::size_t> index) final
      {
        const timer t{};

        try
        {
          m_Test.run_tests();
        }
        catch(const std::exception& e)
        {
          m_Test.log_critical_failure(m_Test.source_file(), "Unexpected", e.what());
        }
        catch(...)
        {
          m_Test.log_critical_failure(m_Test.source_file(), "Unknown", "");
        }

        m_Test.write_instability_analysis_output(m_Test.source_file(), index);

        return write_versioned_output(t);
      }

      void reset(const project_paths& projPaths, std::vector<std::filesystem::path>& materialsPaths) final
      {
        m_Test.reset_results();
        set_materials(m_Test.source_file(), m_Test.name(), projPaths, materialsPaths);
      }

      void initialize(const project_paths& projPaths, std::vector<std::filesystem::path>& materialsPaths, recovery_mode mode) final
      {
        auto name{test_name<Test>()};
        const auto source{m_Test.source_file()};

        m_Test = Test{name,
                      source,
                      projPaths,
                      set_materials(source, name, projPaths, materialsPaths),
                      make_active_recovery_paths(mode, projPaths),
                      get_output_discriminator(m_Test),
                      get_reduction_discriminator(m_Test)};
      }
    private:
      log_summary write_versioned_output(const timer& t) const
      {
        auto summary{m_Test.summarize(t.time_elapsed())};

        if(!m_Test.has_critical_failures())
        {
          versioned_write(m_Test.diagnostics_file_paths().false_positive_or_negative_file_path(), summary.diagnostics_output());
          versioned_write(m_Test.diagnostics_file_paths().caught_exceptions_file_path(), summary.caught_exceptions_output());
        }

        return summary;
      }

      Test m_Test;
    };

    enum class parallelizable_candidate : bool { no, yes };

    std::unique_ptr<soul> m_pTest{};
    parallelizable_candidate m_Parallelizable{parallelizable_candidate::yes};
  };

  template<concrete_test T>
  [[nodiscard]]
  std::optional<std::string> get_output_discriminator(const T& test){
    if constexpr(has_discriminated_output_v<T>)
      return test.output_discriminator();
    else
      return std::nullopt;
  }

  template<concrete_test T>
  [[nodiscard]]
  std::optional<std::string> get_reduction_discriminator(const T& test){
    if constexpr(has_discriminated_summary_v<T>)
      return test.summary_discriminator();
    else
      return std::nullopt;
  }

  /** \brief Consumes command-line arguments and holds all test suites.

      If no arguments are specified, all tests are run; run with --help
      for information on the various options.
   */

  class test_runner
  {
  public:
    test_runner(int argc,
                char** argv,
                std::string copyright,
                std::string codeIndent="  ",
                const project_paths::customizer& projectPathsCustomization = {},
                std::ostream& stream=std::cout);

    test_runner(const test_runner&)     = delete;
    test_runner(test_runner&&) noexcept = default;

    test_runner& operator=(const test_runner&)     = delete;
    test_runner& operator=(test_runner&&) noexcept = default;

    /** \brief Registers a test, which is grouped by where its source file lives. */

    template<concrete_test T>
    void register_test()
    {
      m_Factory.register_product<T>(test_name<T>());
    }

    [[nodiscard]]
    return_code execute([[maybe_unused]] timer_resolution r={});

    [[nodiscard]]
    std::ostream& stream() noexcept { return *m_Stream; }

    [[nodiscard]]
    const project_paths& proj_paths() const noexcept { return m_ProjPaths; }

    [[nodiscard]]
    const std::string& copyright() const noexcept { return m_Copyright; }

    [[nodiscard]]
    const indentation& code_indent() const noexcept { return m_CodeIndent; }

  private:
    enum class verbosity { standard = 0, verbose = 1 };
    enum class instability_mode { none = 0, single_instance, coordinator, sandbox };
    enum class versioned_output_mode { unchecked = 0, checked = 1 };

    struct prune_info
    {
      prune_mode mode{prune_mode::passive};
      std::string include_cutoff{};
    };

    class path_equivalence
    {
    public:
      explicit path_equivalence(const std::filesystem::path& repo)
        : m_Repo{&repo}
      {}

      [[nodiscard]]
      bool operator()(const normal_path& selectedSource, const normal_path& filepath) const;

    private:
      const std::filesystem::path* m_Repo;
    };

    /** \brief Selection by source file, or by the name of a directory containing it. */

    class test_filter
    {
    public:
      using items_map_type  = std::vector<std::pair<normal_path, bool>>;
      using suites_map_type = std::vector<std::pair<std::string, bool>>;

      explicit test_filter(path_equivalence equivalent) : m_Equivalent{equivalent} {}

      void add_selected_suite(std::string name) { add(m_SelectedSuites, std::move(name)); }

      void add_selected_item(normal_path source) { add(m_SelectedItems, std::move(source)); }

      /** \brief Selects nothing, which is not the same as selecting everything. */

      void select_nothing()
      {
        m_SelectedItems.emplace();
        m_SelectedSuites.emplace();
      }

      [[nodiscard]]
      bool operator()(const normal_path& source, std::span<const std::string> groups)
      {
        if(!m_SelectedItems && !m_SelectedSuites) return true;

        // Both are evaluated: an unreported selection is one nobody can be warned about.
        const std::array<bool, 2> found{
          mark(m_SelectedItems,  [this, &source](const normal_path& selected){ return m_Equivalent(selected, source); }),
          mark(m_SelectedSuites, [groups](const std::string& selected){ return std::ranges::find(groups, selected) != groups.end(); })
        };

        return std::ranges::any_of(found, [](bool b){ return b; });
      }

      [[nodiscard]]
      std::optional<std::ranges::subrange<items_map_type::const_iterator>> selected_items() const noexcept
      {
        return as_range(m_SelectedItems);
      }

      [[nodiscard]]
      std::optional<std::ranges::subrange<suites_map_type::const_iterator>> selected_suites() const noexcept
      {
        return as_range(m_SelectedSuites);
      }

      [[nodiscard]]
      operator bool() const noexcept { return m_SelectedItems.has_value() || m_SelectedSuites.has_value(); }
    private:
      template<class Map>
      static void add(std::optional<Map>& map, typename Map::value_type::first_type key)
      {
        if(!map) map = Map{};

        map->emplace_back(std::move(key), false);
      }

      template<class Map, class Predicate>
      static bool mark(std::optional<Map>& map, Predicate pred)
      {
        if(!map) return false;

        auto found{std::ranges::find_if(*map, [&pred](const auto& e){ return pred(e.first); })};
        if(found == map->end()) return false;

        found->second = true;
        return true;
      }

      template<class Map>
      [[nodiscard]]
      static std::optional<std::ranges::subrange<typename Map::const_iterator>> as_range(const std::optional<Map>& map) noexcept
      {
        if(!map) return std::nullopt;

        return std::ranges::subrange{map->begin(), map->end()};
      }

      std::optional<items_map_type>  m_SelectedItems{};
      std::optional<suites_map_type> m_SelectedSuites{};
      path_equivalence m_Equivalent;
    };

    struct suite_node
    {
      log_summary summary{};
      std::optional<test_vessel> optTest{};
    };

    using suite_type = maths::directed_tree<maths::tree_link_direction::forward, maths::null_weight, suite_node>;

    std::string      m_Copyright{};
    project_paths    m_ProjPaths;
    indentation      m_CodeIndent{"  "};
    std::ostream*    m_Stream;

    suite_type m_Suites{};
    object::erasing_factory<test_vessel> m_Factory{};
    test_filter m_Filter{path_equivalence{proj_paths().tests().repo()}};
    prune_info m_PruneInfo{};

    runner_mode           m_RunnerMode{runner_mode::none};
    verbosity             m_Verbosity{verbosity::standard};
    update_mode           m_UpdateMode{update_mode::none};
    recovery_mode         m_RecoveryMode{recovery_mode::none};
    concurrency_mode      m_ConcurrencyMode{concurrency_mode::dynamic};
    instability_mode      m_InstabilityMode{instability_mode::none};
    versioned_output_mode m_VersionedOutputMode{versioned_output_mode::unchecked};

    std::size_t m_NumReps{1},
                m_RunnerID{},
                m_PoolSize{8};

    void process_args(int argc, char** argv);

    void check_argument_consistency();

    void check_for_missing_tests();

    [[nodiscard]]
    bool concurrent_execution() const noexcept { return m_ConcurrencyMode != concurrency_mode::serial; }

    void sort_tests();

    void reset_tests();

    void run_tests(std::optional<std::size_t> id);

    /** The `select`/`test` options which reproduce this run's filter, for handing to a child process. */
    [[nodiscard]]
    std::string selection_options() const;

    /** Runs the tests in child processes, one per repetition, each isolated from the others. */
    [[nodiscard]]
    return_code run_tests_in_sandboxes();

    /** Runs the tests here: once if this process is itself a sandbox, otherwise once per repetition. */
    [[nodiscard]]
    return_code run_tests_in_this_process();

    [[nodiscard]]
    const log_summary& root_summary() const;

    /** The state to compare the run's writes against, or nothing at all if the versioned output is
        not being checked - which is a different thing from an empty snapshot, since a project with
        no versioned output yet legitimately has one of those. This is the sole place the mode is
        consulted, and the files are read only when something will be done with them.
     */
    [[nodiscard]]
    std::optional<versioned_output_snapshot> versioned_output_baseline() const;

    [[nodiscard]]
    return_code report_versioned_output_changes(const std::optional<versioned_output_snapshot>& baseline);

    [[nodiscard]]
    bool nothing_to_do();

    [[nodiscard]]
    bool in_mode(runner_mode m) const noexcept { return (m_RunnerMode & m) == m; }

    void prune();

    [[nodiscard]]
    prune_outcome do_prune();

    void build_suite_tree();

 };
}
