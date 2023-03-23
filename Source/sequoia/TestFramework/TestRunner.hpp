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

#include "sequoia/TestFramework/TestFamilySelector.hpp"

#include "sequoia/Core/Logic/Bitmask.hpp"
#include "sequoia/Core/Object/Suite.hpp"
#include "sequoia/Maths/Graph/DynamicTree.hpp"
#include "sequoia/PlatformSpecific/Helpers.hpp"
#include "sequoia/TextProcessing/Indent.hpp"

#include <iostream>
#include <chrono>

namespace sequoia::testing
{
  enum class runner_mode { none=0, help=1, test=2, create=4, init=8};
}

namespace sequoia
{
  template<>
  struct as_bitmask<testing::runner_mode> : std::true_type {};
}

namespace sequoia::testing
{
  struct test_to_path
  {
    template<concrete_test Test>
    [[nodiscard]]
    normal_path operator()(const Test& test) const {
      return test.source_filename();
    }
  };

  class path_equivalence
  {
  public:
    explicit path_equivalence(const std::filesystem::path& repo)
      : m_Repo{repo}
    {}

    [[nodiscard]]
    bool operator()(const normal_path& selectedSource, const normal_path& filepath) const;

  private:
    const std::filesystem::path& m_Repo;
  };

  class test_vessel
  {
  public:
    template<class Test>
      requires (!std::is_same_v<Test, test_vessel> && concrete_test<Test>)
    test_vessel(Test&& t)
      : m_pTest{std::make_unique<essence<Test>>(std::forward<Test>(t))}
    {}

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
    normal_path source_filename() const
    {
      return m_pTest->source_filename();
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
    log_summary execute(std::optional<std::size_t> index)
    {
      return m_pTest->execute(index);
    }
  private:
    struct soul
    {
      virtual ~soul() = default;

      virtual const std::string& name() const noexcept = 0;
      virtual normal_path source_filename() const = 0;
      virtual std::filesystem::path working_materials() const = 0;
      virtual std::filesystem::path predictive_materials() const = 0;

      virtual log_summary execute(std::optional<std::size_t> index) = 0;
    };

    template<concrete_test Test>
    struct essence final : soul
    {
      essence(Test&& t) : m_Test{std::forward<Test>(t)}
      {}

      [[nodiscard]]
      const std::string& name() const noexcept final
      {
        return m_Test.name();
      }

      [[nodiscard]]
      log_summary execute(std::optional<std::size_t> index) final
      {
        return m_Test.execute(index);
      }

      [[nodiscard]]
      normal_path source_filename() const final
      {
        return m_Test.source_filename();
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

      Test m_Test;
    };

    std::unique_ptr<soul> m_pTest{};
  };

  [[nodiscard]]
  std::string report_time(const family_summary& s);

  /*! \brief Consumes command-line arguments and holds all test families

      If no arguments are specified, all tests are run with the diagnostic
      files generated; run with --help for information on the various options

   */

  class test_runner
  {
  public:
    test_runner(int argc,
                char** argv,
                std::string copyright,
                std::string codeIndent="  ",
                std::ostream& stream=std::cout);

    test_runner(int argc,
                char** argv,
                std::string copyright,
                const project_paths::initializer& pathsFromProjectRoot,
                std::string codeIndent="  ",
                std::ostream& stream=std::cout);

    test_runner(const test_runner&)     = delete;
    test_runner(test_runner&&) noexcept = default;

    test_runner& operator=(const test_runner&)     = delete;
    test_runner& operator=(test_runner&&) noexcept = default;

    template<concrete_test... Tests>
      requires (sizeof...(Tests) > 0)
    void add_test_family(std::string_view name, Tests... tests)
    {
      m_Selector.add_test_family(name, m_RecoveryMode, std::move(tests)...);
    }

    template<concrete_test... Tests>
    void add_test_suite(std::string_view name, Tests&&... tests)
    {
      using namespace object;
      using namespace maths;

      if(!m_Suites.order())
      {
        m_Suites.add_node(suite_type::npos, log_summary{});
      }

      std::vector<std::filesystem::path> materialsPaths{};

      extract_tree(suite{std::string{name}, std::forward<Tests>(tests)...},
                   m_Filter,
                   overloaded{
                     [] <class... Ts> (const suite<Ts...>&s) -> suite_node { return {.summary{log_summary{s.name}}}; },
                     [this, name, &materialsPaths]<concrete_test T>(T&& test) -> suite_node {
                       init(name, test, materialsPaths);
                       return {.summary{log_summary{test.name()}}, .optTest{std::move(test)}};
                     }
                   },
                   m_Suites,
                   0);
    }

    void execute([[maybe_unused]] timer_resolution r={});

    std::ostream& stream() noexcept { return *m_Stream; }

    const project_paths& proj_paths() const noexcept { return m_Selector.proj_paths(); }

    const std::string& copyright() const noexcept { return m_Copyright; }

    const indentation& code_indent() const noexcept { return m_CodeIndent; }

  private:
    enum class output_mode { standard = 0, verbose = 1 };
    enum class instability_mode { none = 0, single_instance, coordinator, sandbox };

    struct suite_node
    {
      log_summary summary{};
      std::optional<test_vessel> optTest{};
    };

    using suite_type = maths::tree<maths::directed_flavour::directed, maths::tree_link_direction::forward, maths::null_weight, suite_node>;

    std::string      m_Copyright{};
    family_selector  m_Selector;
    indentation      m_CodeIndent{"  "};
    std::ostream*    m_Stream;

    suite_type m_Suites{};
    object::filter_by_names<normal_path, test_to_path, path_equivalence> m_Filter;

    runner_mode      m_RunnerMode{runner_mode::none};
    output_mode      m_OutputMode{output_mode::standard};
    update_mode      m_UpdateMode{update_mode::none};
    recovery_mode    m_RecoveryMode{recovery_mode::none};
    concurrency_mode m_ConcurrencyMode{concurrency_mode::dynamic};
    instability_mode m_InstabilityMode{instability_mode::none};

    std::size_t m_NumReps{1},
                m_RunnerID{};

    void process_args(int argc, char** argv);

    void finalize_concurrency_mode();

    [[nodiscard]]
    family_summary process_family(family_results results);

    [[nodiscard]]
    bool concurrent_execution() const noexcept { return m_ConcurrencyMode != concurrency_mode::serial; }

    void check_argument_consistency();

    [[nodiscard]]
    individual_materials_paths set_materials(const std::filesystem::path& sourceFile, std::vector<std::filesystem::path>& materialsPaths) const;

    template<concrete_test Test>
    void init(std::string_view suiteName, Test& test, std::vector<std::filesystem::path>& materialsPaths) const
    {
      test.set_filesystem_data(proj_paths(), suiteName);
      test.set_recovery_paths(make_active_recovery_paths(m_RecoveryMode, proj_paths()));
      test.set_materials(set_materials(test.source_filename(), materialsPaths));
    }

    void run_tests(std::optional<std::size_t> id);

    [[nodiscard]]
    bool nothing_to_do();

    [[nodiscard]]
    bool mode(runner_mode m) const noexcept
    {
      return (m_RunnerMode & m) == m;
    }

    void prune();
 };
}
