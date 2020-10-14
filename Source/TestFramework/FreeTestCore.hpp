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

namespace sequoia::testing
{  
  /*! \brief Abstract base class used for type-erasure of the template class basic_test.

      This class allows for convenient, homogeneous treatment of all concrete tests.

      The only state it holds is a string representing the name of the test.

      The semantics are such that, of the special member functions, only explicit construction from a
      string_view and (virtual) destruction are publicly available. Move construction is protected;
      all remaining special member functions are deleted to discourage multiple instantiations.
   */
  
  class test
  {
  public:    
    explicit test(std::string_view name)
      : m_Name{name}
    {}

    virtual ~test() = default;
    
    test(const test&)            = delete;  
    test& operator=(const test&) = delete;
    test& operator=(test&&)      = delete;

    [[nodiscard]]
    log_summary execute()
    {
      return do_execute();
    }

    [[nodiscard]]
    std::filesystem::path source_filename() const noexcept
    {
      return {source_file()};
    }

    [[nodiscard]]
    std::string_view name() const noexcept { return m_Name; }

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

    void materials(std::filesystem::path workingMaterials, std::filesystem::path predictiveMaterials)
    {
      m_WorkingMaterials    = std::move(workingMaterials);
      m_PredictiveMaterials = std::move(predictiveMaterials);
    }
  protected:
    test(test&&) noexcept = default;

    /// Pure virtual method, the overrides of which determine the fine details of test execution.
    [[nodiscard]]
    virtual log_summary do_execute() = 0;
    
    /// Pure virtual method which should be overridden in a concrete test's cpp file in order to provide the correct __FILE__
    [[nodiscard]]
    virtual std::string_view source_file() const noexcept = 0;

  private:
    std::string m_Name{};
    std::filesystem::path m_WorkingMaterials{}, m_PredictiveMaterials{};   
  };

  template<class T>
  concept concrete_test = derived_from<T, test> && !std::is_abstract_v<T>;

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
  class basic_test : public test, protected Checker
  {
  public:    
    using test::test;

    ~basic_test() override = default;
    
    basic_test(const basic_test&)            = delete;
    basic_test& operator=(const basic_test&) = delete;
    basic_test& operator=(basic_test&&)      = delete;
    
  protected:
    using time_point = std::chrono::time_point<std::chrono::steady_clock>;
    
    basic_test(basic_test&&) noexcept = default;

    log_summary do_execute() final
    {
      using namespace std::chrono;
      const auto time{steady_clock::now()};
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

      return summarize(time);
    }

    /// The override in a derived test should call the checks performed by the test.
    virtual void run_tests() = 0;

    /// Any override of this is likely to call this first and potentially append to the log_summary
    [[nodiscard]]
    virtual log_summary summarize(const time_point start) const
    {
      using namespace std::chrono;
      return Checker::summary(name(), steady_clock::now() - start);
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
