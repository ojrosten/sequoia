////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Core functionality for the testing framework.

    This header defines the basic_test class template, from which all concrete tests derive.

    An alias template, basic_free_test, is provided from which all tests of free functions should derive.
*/

#include "sequoia/TestFramework/ConcreteTypeCheckers.hpp"
#include "sequoia/TestFramework/IndividualTestPaths.hpp"

#include "sequoia/Core/Meta/Concepts.hpp"
#include "sequoia/FileSystem/FileSystem.hpp"
#include "sequoia/TextProcessing/Substitutions.hpp"

#include <memory>
#include <chrono>

namespace sequoia::testing
{
  class [[nodiscard]] timer
  {
  public:
    timer();

    [[nodiscard]]
    std::chrono::nanoseconds time_elapsed() const;
  private:
    using time_point = std::chrono::steady_clock::time_point;

    time_point m_Start;
  };

  [[nodiscard]]
  std::string to_tag(test_mode mode);

  /*! \brief class from which all tests ultimately derive

      The primary purpose of this class is to reduce code which is templated.
   */

  class test_base
  {
  public:
    explicit test_base(std::string name) : m_Name{std::move(name)} {}

    test_base(const test_base&)            = delete;
    test_base& operator=(const test_base&) = delete;

    [[nodiscard]]
    const std::string& name() const noexcept
    {
      return m_Name;
    }

    [[nodiscard]]
    const std::filesystem::path& project_root() const
    {
      return m_ProjectPaths.project_root();
    }

    [[nodiscard]]
    std::filesystem::path working_materials() const
    {
      return m_Materials.working();
    }

    [[nodiscard]]
    std::filesystem::path predictive_materials() const
    {
      return m_Materials.prediction();
    }

    [[nodiscard]]
    std::filesystem::path auxiliary_materials() const
    {
      return m_Materials.auxiliary();
    }

    [[nodiscard]]
    const std::filesystem::path& diagnostics_output_filename() const noexcept
    {
      return m_Diagnostics.diagnostics_file();
    }

    [[nodiscard]]
    const std::filesystem::path& caught_exceptions_output_filename() const noexcept
    {
      return m_Diagnostics.caught_exceptions_file();
    }

    [[nodiscard]]
    std::string report_line(const report& rep) const
    {
        return rep.location() ? testing::report_line(rep.message(), m_ProjectPaths.tests().repo(), rep.location().value()) : rep.message();
    }
  protected:
    ~test_base() = default;

    test_base(test_base&&)            noexcept = default;
    test_base& operator=(test_base&&) noexcept = default;

    void initialize(test_mode mode, std::string_view suiteName, const normal_path& srcFile, const project_paths& projPaths, individual_materials_paths materials);

    void write_instability_analysis_output(const normal_path& srcFile, std::optional<std::size_t> index, const failure_output& output) const;

    [[nodiscard]]
    const project_paths& get_project_paths() const noexcept
    {
      return m_ProjectPaths;
    }
  private:
    std::string m_Name{};
    project_paths m_ProjectPaths{};
    individual_materials_paths m_Materials{};
    individual_diagnostics_paths m_Diagnostics{};
  };

  /*! \brief class template from which all concrete tests should derive.

      The semantics are such that, of the special member functions, only explicit construction from a
      string_view is publicly available. Destruction and moves are protected; copy
      contruction / assignment are deleted.

      \anchor basic_test_primary
   */

  template<class Checker>
    requires std::is_same_v<decltype(Checker::mode), const test_mode>
  class basic_test : public test_base, public Checker
  {
  public:
    constexpr static test_mode mode{Checker::mode};

    using test_base::test_base;

    basic_test(const basic_test&)            = delete;
    basic_test& operator=(const basic_test&) = delete;

    void initialize(std::string_view suiteName, const normal_path& srcFile, const project_paths& projPaths, individual_materials_paths materials, active_recovery_files files)
    {
      test_base::initialize(mode, suiteName, srcFile, projPaths, std::move(materials));
      Checker::recovery(std::move(files));
    }

    using Checker::reset_results;
  protected:
    using duration = std::chrono::steady_clock::duration;

    ~basic_test() = default;

    basic_test(basic_test&&)            noexcept = default;
    basic_test& operator=(basic_test&&) noexcept = default;

    [[nodiscard]]
    log_summary summarize(duration delta) const
    {
      return Checker::summary(name(), delta);
    }
  private:
    friend class test_vessel;

    void log_critical_failure(const normal_path& srcFile, std::string_view tag, std::string_view what)
    {
      auto sentry{Checker::make_sentinel("")};
      sentry.log_critical_failure(exception_message(tag, srcFile, Checker::exceptions_detected_by_sentinel(), what));
    }

    void write_instability_analysis_output(const normal_path& srcFile, std::optional<std::size_t> index) const
    {
      test_base::write_instability_analysis_output(srcFile, index, Checker::failure_messages());
    }
  };

  template<class T>
  concept concrete_test =
        requires (T& test){
          { test.run_tests() };
          { test.source_file() } -> std::convertible_to<std::filesystem::path>;
          { test.reset_results() };
        }
    && std::derived_from<T, test_base> && !std::is_abstract_v<T> && std::movable<T>;

  template<test_mode Mode>
  using basic_free_test = basic_test<checker<Mode>>;

  /*! \anchor free_test_alias */
  using free_test                = basic_free_test<test_mode::standard>;
  using free_false_negative_test = basic_free_test<test_mode::false_negative>;
  using free_false_positive_test = basic_free_test<test_mode::false_positive>;
}
