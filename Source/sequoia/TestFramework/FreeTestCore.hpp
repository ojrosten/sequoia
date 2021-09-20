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
#include "sequoia/TestFramework/FileSystem.hpp"

#include "sequoia/Core/Meta/Concepts.hpp"
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
    void serialize(const std::filesystem::path& file, std::string_view name, const failure_output& output);
    void versioned_write(const std::filesystem::path& file, std::string_view text);
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
      string_view and (virtual) destruction are publicly available. Moves are protected; copy
      contruction / assignment are deleted.

      \anchor basic_test_primary
   */

  template<class Checker>
    requires requires() { Checker::mode; }
  class basic_test : protected Checker
  {
  public:
    constexpr static test_mode mode{Checker::mode};

    explicit basic_test(std::string name) : m_Name{std::move(name)} {}

    virtual ~basic_test() = default;

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
    std::filesystem::path source_filename() const noexcept
    {
      return std::filesystem::path{source_file()}.lexically_normal();
    }

    [[nodiscard]]
    const std::string& name() const noexcept
    {
      return m_Name;
    }

    [[nodiscard]]
    const std::filesystem::path& working_materials() const noexcept
    {
      return m_WorkingMaterials;
    }

    [[nodiscard]]
    const std::filesystem::path& predictive_materials() const noexcept
    {
      return m_PredictiveMaterials;
    }

    [[nodiscard]]
    const std::filesystem::path& auxiliary_materials() const noexcept
    {
      return m_AuxiliaryMaterials;
    }

    [[nodiscard]]
    const std::filesystem::path& test_repository() const noexcept
    {
      return m_TestRepo;
    }

    [[nodiscard]]
    const std::filesystem::path& diagnostics_output_filename() const noexcept
    {
      return m_DiagnosticsOutput;
    }

    [[nodiscard]]
    const std::filesystem::path& caught_exceptions_output_filename() const noexcept
    {
      return m_CaughtExceptionsOutput;
    }

    [[nodiscard]]
    std::string report_line(const std::filesystem::path& file, int line, std::string_view message)
    {
      return testing::report_line(file, line, message, test_repository());
    }

    void set_filesystem_data(std::filesystem::path testRepo, const std::filesystem::path& outputDir, std::string_view familyName)
    {
      namespace fs = std::filesystem;

      auto makePath{
        [famName{fs::path{replace_all(familyName, " ", "_")}},this](fs::path dir, std::string_view suffix){
            const auto file{
              fs::path{source_file()}.filename()
                                     .replace_extension()
                                     .concat("_")
                                     .concat(to_tag(mode))
                                     .concat(suffix)
                                     .concat(".txt")};
            return (dir /= famName) /= file;
        }
      };

      m_TestRepo                = std::move(testRepo);
      m_DiagnosticsOutput       = makePath(diagnostics_output_path(outputDir),  "Output");
      m_CaughtExceptionsOutput  = makePath(diagnostics_output_path(outputDir),  "Exceptions");
      m_InstabilityAnalysisStub = makePath(temp_test_summaries_path(outputDir), replace_all(name(), " ", "_")).replace_extension();

      fs::create_directories(m_DiagnosticsOutput.parent_path());
    }

    void set_materials(std::filesystem::path workingMaterials, std::filesystem::path predictiveMaterials, std::filesystem::path auxiliaryMaterials)
    {
      m_WorkingMaterials    = std::move(workingMaterials);
      m_PredictiveMaterials = std::move(predictiveMaterials);
      m_AuxiliaryMaterials  = std::move(auxiliaryMaterials);
    }

    void set_recovery_paths(recovery_paths paths)
    {
      Checker::recovery(std::move(paths));
    }
  protected:    
    using duration = std::chrono::steady_clock::duration;

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
    std::filesystem::path
      m_WorkingMaterials{},
      m_PredictiveMaterials{},
      m_AuxiliaryMaterials{},
      m_TestRepo{},
      m_DiagnosticsOutput{},
      m_CaughtExceptionsOutput{},
      m_InstabilityAnalysisStub{};

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
        namespace fs = std::filesystem;

        const auto file{fs::path{m_InstabilityAnalysisStub}.concat("_" + std::to_string(*index)).concat(".txt")};
        impl::serialize(file, fs::path{source_file()}.filename().generic_string(), Checker::failure_messages());
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
  concept concrete_test = !std::is_abstract_v<T> && movable<T> && constructible_from<std::string>
    && requires (T& test){
         { test.execute(std::nullopt) } -> same_as<log_summary>;
         { test.source_filename() }     -> convertible_to<std::filesystem::path>;
         { test.name() }                -> convertible_to<std::string>;
       };

  template<test_mode Mode>
  using basic_free_test = basic_test<checker<Mode>>;

  /*! \anchor free_test_alias */
  using free_test                = basic_free_test<test_mode::standard>;
  using free_false_negative_test = basic_free_test<test_mode::false_negative>;
  using free_false_positive_test = basic_free_test<test_mode::false_positive>;
}
