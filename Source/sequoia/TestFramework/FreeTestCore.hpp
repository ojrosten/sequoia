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

  /*! \brief class from which all tests ultimately derive

      The primary purpose of this class is to reduce code which is templated.
   */

  class test_base
  {
  public:
    explicit test_base(std::string name) : m_Name{std::move(name)} {}

    test_base(std::string name, test_mode mode, std::string_view suiteName, const normal_path& srcFile, project_paths projPaths, individual_materials_paths materials, const std::optional<std::string>& outputDiscriminator, const std::optional<std::string>& summaryDiscriminator)
      : m_Name{std::move(name)}
      , m_ProjectPaths{std::move(projPaths)}
      , m_Materials{std::move(materials)}
      , m_Diagnostics{get_project_paths().project_root(), suiteName, srcFile, mode, outputDiscriminator}
      , m_SummaryFile{srcFile, m_ProjectPaths, summaryDiscriminator}
    {
      std::filesystem::create_directories(m_Diagnostics.false_positive_or_negative_file_path().parent_path());
    }

    test_base(const test_base&)            = delete;
    test_base& operator=(const test_base&) = delete;

    [[nodiscard]]
    const std::string& name() const noexcept
    {
      return m_Name;
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
    const project_paths& get_project_paths() const noexcept
    {
      return m_ProjectPaths;
    }

    [[nodiscard]]
    const individual_diagnostics_paths& diagnostics_file_paths() const noexcept
    {
      return m_Diagnostics;
    }

    [[nodiscard]]
    const test_summary_path& summary_file_path() const noexcept
    {
      return m_SummaryFile;
    }

    [[nodiscard]]
    std::string report(const reporter& rep) const
    {
        return rep.location() ? testing::report_line(rep.message(), m_ProjectPaths.tests().repo(), rep.location().value()) : rep.message();
    }
  protected:
    ~test_base() = default;

    test_base(test_base&&)            noexcept = default;
    test_base& operator=(test_base&&) noexcept = default;

    void write_instability_analysis_output(const normal_path& srcFile, std::optional<std::size_t> index, const failure_output& output) const;
  private:
    std::string m_Name{};
    project_paths m_ProjectPaths{};
    individual_materials_paths m_Materials{};
    individual_diagnostics_paths m_Diagnostics{};
    test_summary_path m_SummaryFile{};
  };

  /*! \brief class template from which all concrete tests should derive.
  
      The design is such that additional checking functionality should be provided
      by the Extender (which will become variadic once variadic friends are adopted).
      Other customization - such as the bespoke summarization of basic_performance_test -
      should be done via inheritance.

      The semantics are such that, of the special member functions, only explicit construction from a
      string_view is publicly available. Destruction and moves are protected; copy
      contruction / assignment are deleted.

      \anchor basic_test_primary
   */

  template<test_mode Mode, class Extender>
  class basic_test : public test_base, public checker<Mode, Extender>
  {
  public:
    using checker_type = checker<Mode, Extender>;
    constexpr static test_mode mode{Mode};

    explicit basic_test(std::string name) : test_base{std::move(name)} {}

    basic_test(std::string name, std::string_view suiteName, const normal_path& srcFile, const project_paths& projPaths, individual_materials_paths materials, active_recovery_files files, const std::optional<std::string>& outputDiscriminator, const std::optional<std::string>& summaryDiscriminator)
      : test_base{std::move(name), Mode, suiteName, srcFile, projPaths, std::move(materials), outputDiscriminator, summaryDiscriminator}
      , checker<Mode, Extender>{std::move(files)}
    {}

    basic_test(const basic_test&)            = delete;
    basic_test& operator=(const basic_test&) = delete;

    using checker_type::reset_results;
  protected:
    using duration = std::chrono::steady_clock::duration;

    ~basic_test() = default;

    basic_test(basic_test&&)            noexcept = default;
    basic_test& operator=(basic_test&&) noexcept = default;

    [[nodiscard]]
    log_summary summarize(duration delta) const
    {
      return checker_type::summary(name(), delta);
    }
  private:
    friend class test_vessel;

    void log_critical_failure(const normal_path& srcFile, std::string_view tag, std::string_view what)
    {
      auto sentry{checker_type::make_sentinel("")};
      sentry.log_critical_failure(exception_message(tag, srcFile, checker_type::exceptions_detected_by_sentinel(), what));
    }

    void write_instability_analysis_output(const normal_path& srcFile, std::optional<std::size_t> index) const
    {
      test_base::write_instability_analysis_output(srcFile, index, checker_type::failure_messages());
    }
  };

  template<class T>
  concept concrete_test =
        requires (T& test){
          { test.run_tests() };
          { test.source_file() } -> std::convertible_to<std::filesystem::path>;
          { test.reset_results() };
        }
    && std::derived_from<T, test_base> && std::movable<T> && std::destructible<T>;

  template<concrete_test T>
  inline constexpr bool has_discriminated_output_v{
    requires(const T& t){
      { t.output_discriminator() } -> std::convertible_to<std::string>;
    }
  };

  template<concrete_test T>
  inline constexpr bool has_discriminated_summary_v{
    requires(const T & t){
      { t.summary_discriminator() } -> std::convertible_to<std::string>;
    }
  };

  /*! \brief Temporary workaround while waiting for variadic friends */
  class trivial_extender
  {
  public:
    trivial_extender() = default;
  protected:
    ~trivial_extender() = default;

    trivial_extender(trivial_extender&&)            noexcept = default;
    trivial_extender& operator=(trivial_extender&&) noexcept = default;
  };

  template<test_mode Mode>
  using basic_free_test = basic_test<Mode, trivial_extender>;

  /*! \anchor free_test_alias */
  using free_test                = basic_free_test<test_mode::standard>;
  using free_false_positive_test = basic_free_test<test_mode::false_positive>;
  using free_false_negative_test = basic_free_test<test_mode::false_negative>;

  template<class T>
  inline constexpr bool has_parallelizable_type{
      requires {
        typename T::parallelizable_type;
        requires std::is_convertible_v<typename T::parallelizable_type, std::true_type> || std::is_convertible_v<typename T::parallelizable_type, std::false_type>;
      }
  };

  /*! \anchor is_parallelizable primary class tempate */
  template<class T>
  struct is_parallelizable;

  template<class T>
  using is_parallelizable_t = typename is_parallelizable<T>::type;

  template<class T>
  inline constexpr bool is_parallelizable_v{is_parallelizable<T>::value};

  template<concrete_test T>
  struct is_parallelizable<T> : std::true_type {};

  template<concrete_test T>
    requires has_parallelizable_type<T>
  struct is_parallelizable<T> : T::parallelizable_type {};

}
