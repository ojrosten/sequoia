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

  namespace impl
  {
    void versioned_write(const std::filesystem::path& file, const failure_output& output);
    void versioned_write(const std::filesystem::path& file, std::string_view text);

    void serialize(const std::filesystem::path& file, const failure_output& output);
  }

  [[nodiscard]]
  std::string to_tag(test_mode mode);

  /*! \brief class template from which all concrete tests should derive.

      The class template inherits in a protected manner from the template parameter Checker.
      The inheritance is protected in order to keep the public interface of basic_test minimal,
      while allowing convenient internal access to the Checkers various check methods, in particular.

      Customisation of the way in which the log_summary is generated
      is allowed through the virtual function summarize.

      The execute method delegates to the pure virtual function run_tests. In a concrete test case,
      this method will contain, either directly or indirectly, the various checks which are to be
      performed.

      The semantics are such that, of the special member functions, only explicit construction from a
      string_view is publicly available. Destruction and moves are protected; copy
      contruction / assignment are deleted.

      \anchor basic_test_primary
   */

  template<class Checker>
    requires std::is_same_v<decltype(Checker::mode), const test_mode>
  class basic_test : protected Checker
  {
  public:
    constexpr static test_mode mode{Checker::mode};

    explicit basic_test(std::string name) : m_Name{std::move(name)} {}

    basic_test(const basic_test&)            = delete;
    basic_test& operator=(const basic_test&) = delete;

    [[nodiscard]]
    log_summary execute(std::optional<std::size_t> index)
    {
      const timer t{};

      try
      {
        run_tests();
      }
      catch(const std::exception& e)
      {
        log_critical_failure("Unexpected", e.what());
      }
      catch(...)
      {
        log_critical_failure("Unknown", "");
      }

      write_instability_analysis_output(index);

      return write_versioned_output(summarize(t.time_elapsed()));
    }

    [[nodiscard]]
    normal_path source_filename() const
    {
      return {source_file()};
    }

    [[nodiscard]]
    const std::string& name() const noexcept
    {
      return m_Name;
    }

    [[nodiscard]]
    std::filesystem::path project_root() const
    {
      return m_TestRepo.project_root();
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
    std::string report_line(const std::filesystem::path& file, int line, std::string_view message)
    {
      return testing::report_line(file, line, message, m_TestRepo.repo());
    }

    void set_filesystem_data(const project_paths& projPaths, std::string_view familyName)
    {
      m_TestRepo    = projPaths.tests();
      m_Diagnostics = {project_root(), familyName, source_file(), to_tag(mode)};

      std::filesystem::create_directories(m_Diagnostics.diagnostics_file().parent_path());
    }

    void set_materials(individual_materials_paths materials)
    {
      m_Materials = std::move(materials);
    }

    void set_recovery_paths(active_recovery_files files)
    {
      Checker::recovery(std::move(files));
    }
  protected:
    using duration = std::chrono::steady_clock::duration;

    ~basic_test() = default;

    basic_test(basic_test&&)            noexcept = default;
    basic_test& operator=(basic_test&&) noexcept = default;

    /// Any override of this is likely to call this first and potentially append to the log_summary
    [[nodiscard]]
    virtual log_summary summarize(duration delta) const
    {
      return Checker::summary(name(), delta);
    }
  private:
    std::string m_Name{};
    tests_paths m_TestRepo{};
    individual_materials_paths m_Materials{};
    individual_diagnostics_paths m_Diagnostics{};

    void log_critical_failure(std::string_view tag, std::string_view what)
    {
      const auto message{
        exception_message(tag, source_filename(), Checker::exceptions_detected_by_sentinel(), what)
      };

      auto sentry{Checker::make_sentinel("")};
      sentry.log_critical_failure(message);
    }

    void write_instability_analysis_output(std::optional<std::size_t> index) const
    {
      if(index.has_value())
      {
        const auto file{output_paths::instability_analysis_file(project_root(), source_file(), name(), index.value())};
        impl::serialize(file, Checker::failure_messages());
      }
    }

    const log_summary& write_versioned_output(const log_summary& summary) const
    {
      impl::versioned_write(diagnostics_output_filename(), summary.diagnostics_output());
      impl::versioned_write(caught_exceptions_output_filename(), summary.caught_exceptions_output());

      return summary;
    }

    /// Pure virtual method which should be overridden in a concrete test's cpp file in order to provide the correct __FILE__
    [[nodiscard]]
    virtual std::string_view source_file() const noexcept = 0;
    
    /// The override in a derived test should call the checks performed by the test.
    virtual void run_tests() = 0;
  };

  template<class T>
  concept concrete_test =
        requires (T& test){
          { test.execute(std::nullopt) } -> std::same_as<log_summary>;
          { test.source_filename() }     -> std::convertible_to<std::filesystem::path>;
          { test.name() }                -> std::convertible_to<std::string>;
        }
    && std::constructible_from<std::string> && !std::is_abstract_v<T> && std::movable<T>;

  template<test_mode Mode>
  using basic_free_test = basic_test<checker<Mode>>;

  /*! \anchor free_test_alias */
  using free_test                = basic_free_test<test_mode::standard>;
  using free_false_negative_test = basic_free_test<test_mode::false_negative>;
  using free_false_positive_test = basic_free_test<test_mode::false_positive>;
}
