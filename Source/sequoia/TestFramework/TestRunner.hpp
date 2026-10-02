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
#include "sequoia/TestFramework/FailureReporting.hpp"
#include "sequoia/TestFramework/PerformanceTestCore.hpp"
#include "sequoia/TestFramework/TestLogger.hpp"
#include "sequoia/TestFramework/VersionedOutput.hpp"

#include "sequoia/Core/Logic/Bitmask.hpp"
#include "sequoia/Maths/Graph/DynamicTree.hpp"
#include "sequoia/TextProcessing/Indent.hpp"

#include <chrono>
#include <format>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <span>
#include <thread>

namespace sequoia::testing
{
  enum class runner_mode : unsigned { none=0, test=1, create=2, init=4};

  enum class update_mode { none = 0, soft };

  enum class recovery_mode : unsigned { none = 0, recovery = 1, dump = 2 };

  enum class concurrency_mode {
    serial,    /// serial execution
    dynamic,   /// determined implicitly by the stl
    fixed      /// fixed-size thread pool
  };

  enum class return_code : unsigned {
    success                = 0,
    versioned_output_diffs = 1 << 0,
    soft_failures          = 1 << 1,
    critical_failures      = 1 << 2,
    incomplete_run         = 1 << 3,
    post_run_failures      = 1 << 4
  };

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

  /** \brief Wipes a test's temporary materials root and copies its materials afresh.

      The `WorkingCopy` and `Auxiliary` in the original root are copied beneath the temporary root.
      If the original root exists but holds no `WorkingCopy`, an empty `WorkingCopy` is made beneath
      the temporary root instead. A test with no original root is left an empty temporary root, which
      is its scratchpad.

      \throws std::logic_error if `materials` names no test
      \throws std::runtime_error if the original root holds anything but `WorkingCopy`, `Prediction`
               and `Auxiliary`, besides a `.keep` or `.DS_Store`, naming what else it holds
      \throws std::runtime_error if the test declares a materials discriminator, and one of these holds:
               -# The discriminator is not one portable directory name;
               -# The discriminator names a kind of material, ignoring case;
               -# The discriminator differs only in case from the name of an entry in the test's own
                  directory;
               -# The test's own directory holds a directory named for a kind of material, ignoring case;
               -# The test's own directory holds an entry which is not a directory, other than a `.keep`
                  or a `.DS_Store`.
   */
  void prepare_materials(const individual_materials_paths& materials);

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
    std::string_view name() const noexcept
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
    const individual_materials_paths& materials_paths() const noexcept
    {
      return m_pTest->materials_paths();
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

    void reset()
    {
      m_pTest->reset();
    }

    /** \brief Replaces the held test with one which knows where its files are. */

    void initialize(const project_paths& projPaths, const cmake_cache& cache, recovery_mode mode)
    {
      m_pTest->initialize(projPaths, cache, mode);
    }
  private:
    static void versioned_write(const std::filesystem::path& file, std::string_view text);

    /** \brief Times a test's execution apart from the runner's overhead.

        The execution duration is the time spent in the calls to `time_execution`. The runner's overhead is the rest
        of the time since construction.
     */
    class execution_timer
    {
    public:
      template<std::invocable Fn>
      void time_execution(Fn fn)
      {
        const timer t{};
        fn();
        m_ExecutionDuration += t.time_elapsed();
      }

      [[nodiscard]]
      log_summary::duration execution_duration() const noexcept { return m_ExecutionDuration; }

      [[nodiscard]]
      log_summary::duration runner_overhead() const { return m_Timer.time_elapsed() - m_ExecutionDuration; }
    private:
      timer m_Timer{};
      log_summary::duration m_ExecutionDuration{};
    };

    /** \brief An RAII wrapper to write a test's execution record: when the test started and, on destruction, its
               execution duration and the runner's overhead so far, as `executionTimer` gives them.

        A record which cannot be written is skipped rather than reported; an allocation failure is not caught.
     */
    class [[nodiscard]] scoped_execution_record
    {
    public:
      scoped_execution_record(std::filesystem::path file, const execution_timer& executionTimer);

      scoped_execution_record(const scoped_execution_record&)            = delete;
      scoped_execution_record& operator=(const scoped_execution_record&) = delete;

      ~scoped_execution_record();
    private:
      std::filesystem::path m_File{};
      std::chrono::system_clock::time_point m_Start{};
      const execution_timer& m_ExecutionTimer;
    };

    struct soul
    {
      virtual ~soul() = default;

      virtual std::string_view name() const noexcept                      = 0;
      virtual const test_summary_path& summary_file_path() const noexcept = 0;
      virtual std::filesystem::path source_file() const                   = 0;
      virtual const individual_materials_paths& materials_paths() const noexcept = 0;

      virtual log_summary execute(std::optional<std::size_t> index) = 0;
      virtual void reset() = 0;
      virtual void initialize(const project_paths& projPaths, const cmake_cache& cache, recovery_mode mode) = 0;
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
        return Test::source_file();
      }

      [[nodiscard]]
      std::string_view name() const noexcept final
      {
        return m_Name;
      }

      [[nodiscard]]
      const test_summary_path& summary_file_path() const noexcept final
      {
        return m_Test.summary_file_path();
      }

      [[nodiscard]]
      const individual_materials_paths& materials_paths() const noexcept final
      {
        return m_Test.materials_paths();
      }

      [[nodiscard]]
      log_summary execute(std::optional<std::size_t> index) final
      {
        execution_timer executionTimer{};
        auto summary{execute_and_record(index, executionTimer)};
        summary.runner_overhead(executionTimer.runner_overhead());

        return summary;
      }

      void reset() final
      {
        m_Test.reset_results();
      }

      void initialize(const project_paths& projPaths, const cmake_cache& cache, recovery_mode mode) final
      {
        const auto source{Test::source_file()};

        m_Test = Test{m_Name,
                      source,
                      projPaths,
                      individual_materials_paths{
                        source,
                        m_Name,
                        projPaths,
                        get_discriminator<materials_discriminator_probe, Test>(cache)
                      },
                      make_active_recovery_paths(mode, projPaths),
                      get_discriminator<output_discriminator_probe, Test>(cache),
                      get_discriminator<summary_discriminator_probe, Test>(cache)};

        m_ExecutionRecord = test_execution_record_path{source, m_Name, projPaths};
      }
    private:
      static constexpr std::string_view m_Name{test_name<Test>()};

      /** Returns the test's summary but for the runner's overhead. The overhead includes finishing the record, and the
          record finishes only after this function has made the summary.
       */
      [[nodiscard]]
      log_summary execute_and_record(std::optional<std::size_t> index, execution_timer& executionTimer)
      {
        // Also installed per test, since under MSVC each thread has its own terminate handler
        const scoped_terminate_handler terminationReported{report_termination};
        const scoped_execution_record record{m_ExecutionRecord.file_path(), executionTimer};

        if(try_prepare_materials())
          executionTimer.time_execution([this](){ try_run_tests(); });

        return write_output(executionTimer.execution_duration(), index);
      }

      void try_run_tests()
      {
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
      }

      [[nodiscard]]
      log_summary write_output(const log_summary::duration executionDuration, std::optional<std::size_t> index)
      {
        try
        {
          m_Test.write_instability_analysis_output(m_Test.source_file(), index);
          return write_versioned_output(executionDuration);
        }
        catch(const std::exception& e)
        {
          m_Test.log_critical_failure(m_Test.source_file(), "Output Writing", e.what());
        }
        catch(...)
        {
          m_Test.log_critical_failure(m_Test.source_file(), "Output Writing", "Unknown exception");
        }

        return m_Test.summarize(executionDuration);
      }

      [[nodiscard]]
      bool try_prepare_materials()
      {
        try
        {
          prepare_materials(m_Test.materials_paths());
          return true;
        }
        catch(const std::exception& e)
        {
          m_Test.log_critical_failure(m_Test.source_file(), "Materials Preparation", e.what());
          return false;
        }
      }

      log_summary write_versioned_output(const log_summary::duration executionDuration) const
      {
        auto summary{m_Test.summarize(executionDuration)};

        if(!m_Test.has_critical_failures())
        {
          versioned_write(m_Test.diagnostics_file_paths().false_positive_or_negative_file_path(), summary.diagnostics_output());
          versioned_write(m_Test.diagnostics_file_paths().caught_exceptions_file_path(), summary.caught_exceptions_output());
        }

        return summary;
      }

      Test m_Test;
      test_execution_record_path m_ExecutionRecord{};
    };

    enum class parallelizable_candidate : bool { no, yes };

    std::unique_ptr<soul> m_pTest{};
    parallelizable_candidate m_Parallelizable{parallelizable_candidate::yes};
  };

  /** \brief Calls the hook of `T` that `Probe` probes for.

      \returns
      -# The hook's result for `cache`, as a `std::string`, if `T` declares the hook;
      -# `null_discriminator` otherwise.
   */
  template<template<class> class Probe, concrete_test T>
  [[nodiscard]]
  std::optional<std::string> get_discriminator(const cmake_cache& cache)
  {
    static_assert(!Probe<T>::declared_v || Probe<T>::conforming_v,
                  "A discriminator hook must be a public static member function taking const cmake_cache& "
                  "and returning something convertible to std::string");

    if constexpr(Probe<T>::conforming_v)
      return Probe<T>::discriminator(cache);
    else
      return null_discriminator;
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

    template<concrete_test T>
    void register_test()
    {
      ++m_Registered;

      register_name(test_name<T>(), T::source_file());
      register_source(T::source_file());

      constexpr auto isPerformanceTest{is_performance_test_v<T> ? is_performance_test::yes : is_performance_test::no};

      if(m_Filter(T::source_file(), enclosing_suites(T::source_file()), isPerformanceTest))
        m_Tests.emplace_back(T{});
    }

    /** \brief Runs the tests, as the command line asked.

        `report_termination` is the terminate handler for the run, and for each test on the thread running it. Under
        MSVC's debug runtime, reports are redirected as `debug_report_redirector` describes, and under Windows a
        crash reaches Windows Error Reporting, as `windows_crash_report_enabler` describes. On Windows, the tests
        run under the finest timer resolution, as `set_finest_windows_timer_resolution` describes.
     */
    [[nodiscard]]
    return_code execute();

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
    enum class performance_mode { included = 0, excluded = 1 };
    enum class is_performance_test : bool { no, yes };


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

    /** \brief Selection by source file, or by the name of a directory containing it; exclusion by source file.

        -# Every source listed, whether selected or excluded, is marked found when a registered test
           matches it, so that those which matched nothing can be reported.
        -# A selection has a third state, absent, which is not the same as empty: with no selection
           every test runs, with an empty one none. Hence the `std::optional`s. An exclusion has no
           such state, an empty list excluding nothing.
     */

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

      void exclude_performance_tests() noexcept { m_PerformanceMode = performance_mode::excluded; }

      void exclude_item(normal_path source) { m_ExcludedItems.emplace_back(std::move(source), false); }

      /** \brief Whether the test defined in `source`, in the nested `suites`, is to run. */
      [[nodiscard]]
      bool operator()(const normal_path& source,
                      std::span<const std::string> suites,
                      is_performance_test isPerformanceTest);

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
      std::ranges::subrange<items_map_type::const_iterator> excluded_items() const noexcept
      {
        return as_range(m_ExcludedItems);
      }

      /** \brief Whether tests have been selected, which is not the same as whether any matched. */
      [[nodiscard]]
      bool selects() const noexcept { return m_SelectedItems.has_value() || m_SelectedSuites.has_value(); }

      [[nodiscard]]
      bool excludes_performance_tests() const noexcept { return m_PerformanceMode == performance_mode::excluded; }

      /** \brief The sources of the tests left out by an exclusion, in the order they were offered. */
      [[nodiscard]]
      const std::vector<std::filesystem::path>& tests_left_out() const noexcept { return m_TestsLeftOut; }
    private:
      template<class Map>
      static void add(std::optional<Map>& map, typename Map::value_type::first_type key)
      {
        if(!map) map = Map{};

        map->emplace_back(std::move(key), false);
      }

      /** \brief Marks the first entry `pred` accepts as found, and says whether there was one. */
      template<class Map, class Predicate>
      static bool mark_found(Map& map, Predicate pred)
      {
        auto found{std::ranges::find_if(map, [&pred](const auto& e){ return pred(e.first); })};
        if(found == map.end())
          return false;

        found->second = true;
        return true;
      }

      template<class Map>
      [[nodiscard]]
      static std::ranges::subrange<typename Map::const_iterator> as_range(const Map& map) noexcept
      {
        return std::ranges::subrange{map.begin(), map.end()};
      }

      template<class Map>
      [[nodiscard]]
      static std::optional<std::ranges::subrange<typename Map::const_iterator>> as_range(const std::optional<Map>& map) noexcept
      {
        if(!map) return std::nullopt;

        return as_range(*map);
      }

      items_map_type                     m_ExcludedItems{};
      std::vector<std::filesystem::path> m_TestsLeftOut{};
      std::optional<items_map_type>      m_SelectedItems{};
      std::optional<suites_map_type>     m_SelectedSuites{};
      path_equivalence m_Equivalent;
      performance_mode m_PerformanceMode{performance_mode::included};
    };

    struct suite_node
    {
      log_summary summary{};
      std::optional<test_vessel> optTest{};
      std::thread::id executing_thread_id{};
    };

    using suite_type = maths::directed_tree<maths::tree_link_direction::forward, maths::null_weight, suite_node>;
    using suite_node_index = suite_type::size_type;

    std::string      m_Copyright{};
    project_paths    m_ProjPaths;
    cmake_cache      m_CMakeCache;
    indentation      m_CodeIndent{"  "};
    std::ostream*    m_Stream;

    suite_type m_Suites{};
    std::vector<test_vessel> m_Tests{};
    std::set<std::string> m_LowerCaseTestNames{};
    std::map<std::string, std::filesystem::path> m_SourcesByLowerCasePrefix{};
    std::size_t m_Registered{};
    test_filter m_Filter{path_equivalence{proj_paths().tests().repo()}};
    prune_mode m_PruneMode{prune_mode::passive};

    runner_mode           m_RunnerMode{runner_mode::none};
    verbosity             m_Verbosity{verbosity::standard};
    update_mode           m_UpdateMode{update_mode::none};
    recovery_mode         m_RecoveryMode{recovery_mode::none};
    std::string           m_KeepDumpAs{}, m_CompareDumpAgainst{};
    concurrency_mode      m_ConcurrencyMode{concurrency_mode::dynamic};
    instability_mode      m_InstabilityMode{instability_mode::none};
    versioned_output_mode m_VersionedOutputMode{versioned_output_mode::unchecked};

    std::size_t m_NumReps{1},
                m_RunnerID{},
                m_PoolSize{8};

    void process_args(int argc, char** argv);

    void check_argument_consistency();

    void check_for_missing_tests();

    void check_for_coarse_sleeps();

    [[nodiscard]]
    bool concurrent_execution() const noexcept { return m_ConcurrencyMode != concurrency_mode::serial; }

    void sort_tests();

    void reset_tests();

    return_code run_tests(std::optional<std::size_t> id);

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

    return_code run();

    [[nodiscard]]
    static std::string dump_name(std::string name);

    void compare_dump();

    void keep_dump();

    [[nodiscard]]
    bool nothing_to_do();

    [[nodiscard]]
    bool in_mode(runner_mode m) const noexcept { return (m_RunnerMode & m) == m; }

    void prune();

    /// The reason prune selected every test, if it did; the filter holds the selection otherwise
    [[nodiscard]]
    std::optional<prune_fallback_reason> do_prune();

    void build_suite_tree();

    /** \brief The suites enclosing a test's source: the names of the directories beneath the tests
        repository which hold the source, outermost first.

        A source outside the repository belongs to the suites named by its directories below the
        deepest one it shares with the repository, so two directories with a common tail share a suite.
     */
    [[nodiscard]]
    std::vector<std::string> enclosing_suites(const std::filesystem::path& source) const;

    [[nodiscard]]
    static std::string duplication_message(std::string_view testName, const std::filesystem::path& source);

    /** \brief Admits the name of a test being registered.

        \throws std::logic_error naming `source`, if `name` contains anything non-ASCII
        \throws std::logic_error naming both, if a test of the same name, ignoring case, was admitted
     */
    void register_name(std::string_view name, const std::filesystem::path& source);

    /** \brief Admits the source of a test being registered.

        \throws std::logic_error naming `source`, if its materials prefix is empty
        \throws std::logic_error naming `source`, if its materials prefix contains anything non-ASCII
        \throws std::logic_error naming both sources, if the materials prefix of `source` lies beneath
        that of a source already admitted, or has one beneath it, ignoring ASCII case
     */
    void register_source(const std::filesystem::path& source);

    [[nodiscard]]
    static std::string nesting_message(const std::filesystem::path& source, const std::filesystem::path& nestedWith);

    [[nodiscard]]
    static std::string non_ascii_name_message(const std::filesystem::path& source);

    [[nodiscard]]
    static std::string non_ascii_source_message(const std::filesystem::path& source);

    [[nodiscard]]
    static std::string unplaceable_source_message(const std::filesystem::path& source);

 };
}
