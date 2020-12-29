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

#include "ConcreteTypeCheckers.hpp"
#include "Concepts.hpp"

#include "Output.hpp"
#include "FileSystem.hpp"

namespace sequoia::testing
{  
  /*! \brief Abstract base class used for type-erasure of the template class basic_test.

      This class allows for convenient, homogeneous treatment of all concrete tests.

      The semantics are such that, of the special member functions, only explicit construction from a
      string_view and (virtual) destruction are publicly available. Move construction is protected;
      all remaining special member functions are deleted to discourage multiple instantiations.
   */

  class test
  {
  public:
    explicit test(std::string_view name) : m_Name{name} {}

    virtual ~test() = default;

    test(const test&)            = delete;
    test& operator=(const test&) = delete;
    test& operator=(test&&)      = delete;

    [[nodiscard]]
    log_summary execute();

    [[nodiscard]]
    std::filesystem::path source_filename() const noexcept
    {
      return source_file();
    }

    [[nodiscard]]
    std::string_view name() const noexcept
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
    const std::filesystem::path& test_repository() const noexcept
    {
      return m_TestRepo;
    }

    [[nodiscard]]
    const std::filesystem::path& versioned_output_filename() const noexcept
    {
      return m_VersionedOutput;
    }

    [[nodiscard]]
    std::string report_line(const std::filesystem::path& file, int line, std::string_view message);

    void set_filesystem_data(std::filesystem::path testRepo, const std::filesystem::path& outputDir, std::string_view familyName);

    void set_materials(std::filesystem::path workingMaterials, std::filesystem::path predictiveMaterials);
  protected:
    using duration = std::chrono::steady_clock::duration;

    test(test&&) noexcept = default;

    /// Pure virtual method which should be overridden in a concrete test's cpp file in order to provide the correct __FILE__
    [[nodiscard]]
    virtual std::string_view source_file() const noexcept = 0;

    [[nodiscard]]
    virtual std::filesystem::path make_versioned_output_filename() const = 0;

    virtual void log_critical_failure(std::string_view tag, std::string_view what) = 0;

    /// The override in a derived test should call the checks performed by the test.
    virtual void run_tests() = 0;

    [[nodiscard]]
    virtual log_summary summarize(duration delta) const = 0;

    virtual const log_summary& write_versioned_output(const log_summary& summary) const;

    [[nodiscard]]
    std::filesystem::path make_versioned_output_filepath(const std::filesystem::path& outputDir, std::string_view familyName) const;

    std::filesystem::path output_filename(std::string_view tag) const;
  private:
    std::string m_Name{};
    std::filesystem::path m_WorkingMaterials{}, m_PredictiveMaterials{}, m_TestRepo{}, m_VersionedOutput{};
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
      string_view and (virtual) destruction are publicly available. Move construction is protected;
      all remaining special member functions are deleted to discourage multiple instantiations.

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
    basic_test& operator=(basic_test&&)      = delete;
    
  protected:
    basic_test(basic_test&&) noexcept = default;

    [[nodiscard]]
    std::filesystem::path make_versioned_output_filename() const override
    {
      return test::output_filename(to_tag(mode));
    }

    const log_summary& write_versioned_output(const log_summary& summary) const override
    {
      if constexpr(mode != test_mode::standard)
      {
        return test::write_versioned_output(summary);
      }
      else
      {
        return summary;
      }
    }


    /// Any override of this is likely to call this first and potentially append to the log_summary
    [[nodiscard]]
    log_summary summarize(duration delta) const override
    {
      return Checker::summary(name(), delta);
    }

  private:
    void log_critical_failure(std::string_view tag, std::string_view what)
    {
      const auto message{
        exception_message(tag, source_filename(), Checker::top_level_message(), what, Checker::exceptions_detected_by_sentinel())
      };

      auto sentry{Checker::make_sentinel("")};
      sentry.log_critical_failure(message);
    }
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
}
