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
#include "sequoia/Core/Meta/Concepts.hpp"

#include "sequoia/TestFramework/Output.hpp"
#include "sequoia/TestFramework/FileSystem.hpp"

#include <memory>

namespace sequoia::testing
{  
  /*! \brief Abstract base class used for type-erasure of the template class basic_test.

      This class allows for convenient, homogeneous treatment of all concrete tests.

      The semantics are such that, of the special member functions, only explicit construction from a
      string_view and (virtual) destruction are publicly available. Moves are protected; copy
      contruction / assignment are deleted.
   */

  class test
  {
  public:
    explicit test(std::string_view name) : m_Name{name} {}

    virtual ~test() = default;

    test(const test&)            = delete;
    test& operator=(const test&) = delete;

    [[nodiscard]]
    log_summary execute();

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
    std::string report_line(const std::filesystem::path& file, int line, std::string_view message);

    void set_filesystem_data(std::filesystem::path testRepo, const std::filesystem::path& outputDir, std::string_view familyName);

    void set_materials(std::filesystem::path workingMaterials, std::filesystem::path predictiveMaterials, std::filesystem::path auxiliaryMaterials);

    void set_recovery_paths(recovery_paths paths);
  protected:
    using duration = std::chrono::steady_clock::duration;

    test(test&&)            noexcept = default;
    test& operator=(test&&) noexcept = default;

    /// Pure virtual method which should be overridden in a concrete test's cpp file in order to provide the correct __FILE__
    [[nodiscard]]
    virtual std::string_view source_file() const noexcept = 0;

    /// The override in a derived test should call the checks performed by the test.
    virtual void run_tests() = 0;
    
    [[nodiscard]]
    virtual log_summary summarize(duration delta) const = 0;

    virtual void log_critical_failure(std::string_view tag, std::string_view what) = 0;

    virtual std::string mode_tag() const = 0;

    virtual void do_set_recovery_paths(recovery_paths paths) = 0;
    
    [[nodiscard]]
    std::filesystem::path output_filename(std::string_view suffix) const;

    const log_summary& write_versioned_output(const log_summary& summary) const;

    static void write(const std::filesystem::path& file, std::string_view text);
  private:
    std::string m_Name{};
    std::filesystem::path
      m_WorkingMaterials{},
      m_PredictiveMaterials{},
      m_AuxiliaryMaterials{},
      m_TestRepo{},
      m_DiagnosticsOutput{},
      m_CaughtExceptionsOutput{};

    [[nodiscard]]
    std::filesystem::path make_output_filepath(const std::filesystem::path& outputDir, std::string_view familyName, std::string_view suffix) const;
  };

  template<class T>
  concept concrete_test = derived_from<T, test> && !std::is_abstract_v<T>;

  [[nodiscard]]
  std::string to_tag(test_mode mode);

  /*! \brief class template from which all concrete tests should derive.

      The class template publicly inherits from test, for the purposes of type-erasure.

      The class template inherits in a protected manner from the template parameter Checker.
      The inheritance is protected in order to keep the public interface of basic_test minimal,
      while allowing convenient internal access to the Checkers various check methods, in particular.

      test::execute is final; however, customisation of the way in which the log_summary is generated
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
  class basic_test : public test, protected Checker
  {
  public:
    constexpr static test_mode mode{Checker::mode};

    using test::test;

    ~basic_test() override = default;
    
    basic_test(const basic_test&)            = delete;
    basic_test& operator=(const basic_test&) = delete;
    
  protected:
    basic_test(basic_test&&)            noexcept = default;
    basic_test& operator=(basic_test&&) noexcept = default;

    /// Any override of this is likely to call this first and potentially append to the log_summary
    [[nodiscard]]
    log_summary summarize(duration delta) const override
    {
      return Checker::summary(name(), delta);
    }

    [[nodiscard]]
    std::string mode_tag() const final
    {
      return to_tag(mode);
    }

    void log_critical_failure(std::string_view tag, std::string_view what) final
    {
      const auto message{
        exception_message(tag, source_filename(), Checker::top_level_message(), what, Checker::exceptions_detected_by_sentinel())
      };

      auto sentry{Checker::make_sentinel("")};
      sentry.log_critical_failure(message);
    }

    void do_set_recovery_paths(recovery_paths paths) final
    {
      Checker::recovery(std::move(paths));
    }
  private:
  };
  
  template<test_mode Mode>
  using basic_free_test = basic_test<checker<Mode>>;

  /*! \anchor free_test_alias */
  using free_test                = basic_free_test<test_mode::standard>;
  using free_false_negative_test = basic_free_test<test_mode::false_negative>;
  using free_false_positive_test = basic_free_test<test_mode::false_positive>;

  /*! \brief Specifies the granularity at which concurrent execution is applied */
  enum class concurrency_mode{
    serial=-1, /// serial execution
    family,    /// families of tests are executed concurrently
    test,      /// tests are executed concurrently, independently of their families
    deep       /// concurrency-aware components of individual tests are executed concurrently
  };

  class test_vessel
  {
  public:
    template<concrete_test T>
    test_vessel(T&& t)
      : m_pTest{std::make_unique<T>(std::forward<T>(t))}
    {}

    test_vessel(const test_vessel&)     = delete;
    test_vessel(test_vessel&&) noexcept = default;

    test_vessel& operator=(const test_vessel&)     = delete;
    test_vessel& operator=(test_vessel&&) noexcept = default;

    [[nodiscard]]
    test* operator->() noexcept { return m_pTest.get(); }

    [[nodiscard]]
    const test* operator->() const noexcept { return m_pTest.get(); }

    [[nodiscard]]
    test& operator*() noexcept { return *m_pTest.get(); }

    [[nodiscard]]
    const test& operator*() const noexcept { return *m_pTest.get(); }
  private:

    std::unique_ptr<test> m_pTest{};
  };
}
