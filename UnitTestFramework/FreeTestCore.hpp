////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Core functionality for the unit testing framework.

    This header defines the basic_test class template, from which all concrete tests derive.

    An alias template, basic_free_test, is provided from which all tests of free functions should derive.
*/

#include "ConcreteEqualityCheckers.hpp"

namespace sequoia::unit_testing
{
  namespace impl
  {
    [[nodiscard]]
    std::string format(std::string_view s);

    [[nodiscard]]
    std::string make_message(std::string_view tag, std::string_view currentMessage, std::string_view exceptionMessage, const bool exceptionsDetected);
  }

  /*! \brief Abstract base class used for type-erasure of the template class basic_test.

      This class allows for convenient, homogeneous treatment of all concrete tests.

      The only state it holds is a string representing the name of the test.

      The semantics are such that, of the special member functions, only explicit construction from a
      string_view and (virtual) destruction are publically available. Move construction is protected;
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

    /// Pure virtual method, the overrides of which determine the fine details of test execution.
    virtual log_summary execute() = 0;

    /// Pure virtual method which should be overridden in a concrete test's cpp file in order to provide the correct __FILE__
    [[nodiscard]]
    virtual std::string_view source_file_name() const noexcept = 0;

    [[nodiscard]]
    std::string_view name() const noexcept { return m_Name; }
  protected:
    test(test&&) noexcept = default;

  private:
    std::string m_Name;
  };

  /*! \brief class template from which all concrete tests should derive.

      The class template publically inherits from test, for the purposes of type-erasure.

      The class template inherits in a protected manner from the template parameter Checker.
      The inheritance is protected in order to keep the public interface of basic_test minimal,
      while allowing convenient internal access to the Checkers various check methods, in particular.

      test::excute is final; however, customisation of the way in which the log_summary is generated
      is allowed through the virtul function summarize.

      The execute method delegates to the pure virtual function run_tests. In a concrete test case,
      this method will contain, either directly or indirectly, the various checks which are to be
      performed.

      The semantics are such that, of the special member functions, only explicit construction from a
      string_view and (virtual) destruction are publically available. Move construction is protected;
      all remaining special member functions are deleted to discourage multiple instantiations.
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

    log_summary execute() final
    {
      using namespace std::chrono;
      const auto time{steady_clock::now()};
      try
      {
        run_tests();
      }
      catch(const std::exception& e)
      {         
        Checker::log_critical_failure(make_message("Unexpected", e.what()));
      }
      catch(...)
      {
        Checker::log_critical_failure(make_message("Unknown", ""));
      }

      return summarize(time);
    }
  protected:
    using time_point = std::chrono::time_point<std::chrono::steady_clock>;
    
    basic_test(basic_test&&) noexcept = default;

    /// The override in a derived test should call the checks performed by the test.
    virtual void run_tests() = 0;

    /// Any override of this is likely to call this first and potentially append to the log_summary
    [[nodiscard]]
    virtual log_summary summarize(const time_point start)
    {
      using namespace std::chrono;
      return Checker::summary(name(), steady_clock::now() - start);
    }

    [[nodiscard]]
    virtual std::string current_message() const { return std::string{Checker::current_message()}; }

    [[nodiscard]]
    std::string make_message(std::string_view tag, std::string_view exceptionMessage) const
    {
      return impl::make_message(tag, current_message(), exceptionMessage, Checker::exceptions_detected_by_sentinel());
    }
  };
  
  template<test_mode Mode>
  using basic_free_test = basic_test<checker<Mode>>;

  using free_test                = basic_free_test<test_mode::standard>;
  using free_false_negative_test = basic_free_test<test_mode::false_negative>;
  using free_false_positive_test = basic_free_test<test_mode::false_positive>;

  enum class concurrency_mode{ serial=-1, family, test, deep};

  [[nodiscard]]
  std::string report_line(std::string_view file, const int line, const std::string_view message);

  #define LINE(message) report_line(__FILE__, __LINE__, message)    
}
