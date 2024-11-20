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

#include "sequoia/TestFramework/DependencyAnalyzer.hpp"
#include "sequoia/TestFramework/PerformanceTestCore.hpp"
#include "sequoia/TestFramework/TestLogger.hpp"

#include "sequoia/Core/Logic/Bitmask.hpp"
#include "sequoia/Core/Object/Suite.hpp"
#include "sequoia/Maths/Graph/DynamicTree.hpp"
#include "sequoia/PlatformSpecific/Helpers.hpp"
#include "sequoia/TextProcessing/Indent.hpp"

#include <iostream>
#include <chrono>
#include <set>

namespace sequoia::testing
{
  enum class runner_mode { none=0, help=1, test=2, create=4, init=8};

  enum class update_mode { none = 0, soft };

  enum class recovery_mode { none = 0, recovery = 1, dump = 2 };

  enum class prune_outcome { not_attempted, no_time_stamp, success };

  enum class concurrency_mode {
    serial,    /// serial execution
    dynamic,   /// determined implicitly by the stl
    fixed      /// fixed-size thread pool
  };
}

NAMESPACE_SEQUOIA_AS_BITMASK
{
  template<>
  struct as_bitmask<sequoia::testing::runner_mode> : std::true_type {};

  template<>
  struct as_bitmask<sequoia::testing::recovery_mode> : std::true_type {};
}

namespace sequoia::testing
{
  individual_materials_paths set_materials(const std::filesystem::path& sourceFile, const project_paths& projPaths, std::vector<std::filesystem::path>& materialsPaths);

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
        set_materials(m_Test.source_file(), projPaths, materialsPaths);
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

    enum class parallelizable_candidate { no, yes };

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
    if constexpr(has_reduced_output_v<T>)
      return test.reduction_discriminator();
    else
      return std::nullopt;
  }

  /*! \brief Consumes command-line arguments and holds all test suites.

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

    template<concrete_test... Tests>
      requires (sizeof...(Tests) > 0)
    void add_test_suite(std::string_view name, Tests&&... tests)
    {
      using namespace object;

      check_for_duplicates(name, tests...);

      extract_suite_tree(name, m_Filter, std::forward<Tests>(tests)...);
    }

    template<class... Suites>
      requires (object::is_suite_v<Suites> && ...)
    void add_test_suite(std::string_view name, Suites... s)
    {
      using namespace object;

      extract_suite_tree(m_Filter, suite{std::string{name}, std::move(s)...});
    }

    void execute([[maybe_unused]] timer_resolution r={});

    std::ostream& stream() noexcept { return *m_Stream; }

    const project_paths& proj_paths() const noexcept { return m_ProjPaths; }

    const std::string& copyright() const noexcept { return m_Copyright; }

    const indentation& code_indent() const noexcept { return m_CodeIndent; }

  private:
    enum class output_mode { standard = 0, verbose = 1 };
    enum class instability_mode { none = 0, single_instance, coordinator, sandbox };

    struct prune_info
    {
      prune_mode mode{prune_mode::passive};
      std::string include_cutoff{};
    };

    struct test_to_path
    {
      template<concrete_test Test>
      [[nodiscard]]
      normal_path operator()(const Test& test) const { return test.source_file(); }
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

    struct suite_node
    {
      log_summary summary{};
      std::optional<test_vessel> optTest{};
    };

    using suite_type  = maths::directed_tree<maths::tree_link_direction::forward, maths::null_weight, suite_node>;
    using filter_type = object::granular_filter<normal_path, path_equivalence, test_to_path>;

    std::string      m_Copyright{};
    project_paths    m_ProjPaths;
    indentation      m_CodeIndent{"  "};
    std::ostream*    m_Stream;

    suite_type m_Suites{};
    filter_type m_Filter{path_equivalence{proj_paths().tests().repo()}, test_to_path{}};
    prune_info m_PruneInfo{};

    runner_mode      m_RunnerMode{runner_mode::none};
    output_mode      m_OutputMode{output_mode::standard};
    update_mode      m_UpdateMode{update_mode::none};
    recovery_mode    m_RecoveryMode{recovery_mode::none};
    concurrency_mode m_ConcurrencyMode{concurrency_mode::dynamic};
    instability_mode m_InstabilityMode{instability_mode::none};

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

    [[nodiscard]]
    bool nothing_to_do();

    [[nodiscard]]
    bool in_mode(runner_mode m) const noexcept { return (m_RunnerMode & m) == m; }

    void prune();

    [[nodiscard]]
    prune_outcome do_prune();

    template<class Filter, class Suite>
      requires object::is_suite_v<Suite>
    void extract_suite_tree(Filter&& filter, Suite&& s)
    {
      using namespace object;

      if(!m_Suites.order())
      {
        m_Suites.add_node(suite_type::npos);
      }

      std::vector<std::filesystem::path> materialsPaths{};

      // TO DO: may need generalizing since suites can have arbitrary depth.
      const std::string suiteName{s.name};

      extract_tree(std::forward<Suite>(s),
                   std::forward<Filter>(filter),
                   overloaded{
                     [] <class... Ts> (const suite<Ts...>&s) -> suite_node { return {.summary{log_summary{s.name}}}; },
                     [this, &suiteName, &materialsPaths]<concrete_test T>(T&& test) -> suite_node {
                       test = T{test.name(),
                                suiteName,
                                test.source_file(),
                                proj_paths(),
                                set_materials(test.source_file(), proj_paths(), materialsPaths),
                                make_active_recovery_paths(m_RecoveryMode, proj_paths()),
                                get_output_discriminator(test),
                                get_reduction_discriminator(test)};
                   
                       return {.summary{log_summary{test.name()}}, .optTest{std::move(test)}};
                     }
                   },
                   m_Suites,
                   0);
    }

    template<class Filter, concrete_test... Tests>
      requires (sizeof...(Tests) > 0)
    void extract_suite_tree(std::string_view name, Filter&& filter, Tests&&... tests)
    {
      extract_suite_tree(std::forward<Filter>(filter), object::suite{std::string{name}, std::forward<Tests>(tests)...});
    }

    template<concrete_test... Tests>
      requires (sizeof...(Tests) > 0)
    static void check_for_duplicates(std::string_view name, const Tests&... tests)
    {
      using duplicate_set = std::set<std::pair<std::string_view, std::filesystem::path>>;

      duplicate_set namesAndSources{};

      auto check{
        [&,name](concrete_test auto const& test) {
          if(!namesAndSources.emplace(test.name(), test.source_file()).second)
            throw std::runtime_error{duplication_message(name, test.name(), test.source_file())};
        }
      };

      (check(tests), ...);
    }

    [[nodiscard]]
    static std::string duplication_message(std::string_view suiteName, std::string_view testName, const std::filesystem::path& source);

    [[nodiscard]]
    static active_recovery_files make_active_recovery_paths(recovery_mode mode, const project_paths& projPaths);
 };
}
