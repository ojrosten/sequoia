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

#include "sequoia/Core/Object/Suite.hpp"
#include "sequoia/TestFramework/TestFamilySelector.hpp"

#include "sequoia/Core/Logic/Bitmask.hpp"
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
  class test_vessel
  {
  public:
    template<concrete_test Test>
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
    log_summary execute(std::optional<std::size_t> index)
    {
      return m_pTest->execute(index);
    }
  private:
    struct soul
    {
      virtual ~soul() = default;
      virtual const std::string& name() const noexcept = 0;
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
    void add_test_suite(std::string name, Tests&&... tests)
    {
      using namespace object;

      // fix filter: filter_by_names{{etc}, {etc}}
      auto vessels{extract(suite{std::move(name), std::forward<Tests>(tests)...}, [](auto&&...) { return true; }, [](auto&& test) { return test_vessel{std::move(test)}; })};
      m_Tests.insert(m_Tests.end(), std::make_move_iterator(vessels.begin()), std::make_move_iterator(vessels.end()));
    }

    void execute([[maybe_unused]] timer_resolution r={});

    std::ostream& stream() noexcept { return *m_Stream; }

    const project_paths& proj_paths() const noexcept { return m_Selector.proj_paths(); }

    const std::string& copyright() const noexcept { return m_Copyright; }

    const indentation& code_indent() const noexcept { return m_CodeIndent; }

  private:
    enum class output_mode { standard = 0, verbose = 1 };
    enum class instability_mode { none = 0, single_instance, coordinator, sandbox };

    std::string      m_Copyright{};
    family_selector  m_Selector;
    indentation      m_CodeIndent{"  "};
    std::ostream*    m_Stream;

    std::vector<test_vessel> m_Tests{};

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
