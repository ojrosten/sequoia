////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file UnitTestCore.hpp
    \brief Core functionality for the unit testing framework.
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
  
  class test
  {
  public:
    explicit test(std::string_view name) : m_Name{name} {}

    virtual ~test() = default;

    virtual log_summary execute() = 0;

    [[nodiscard]]
    std::string_view name() const noexcept { return m_Name; }
  protected:
    
    test(const test&)     = default;
    test(test&&) noexcept = default;

    test& operator=(const test&)     = default;
    test& operator=(test&&) noexcept = default;
  private:
    std::string m_Name;
  };
    
  template<class Logger, class Checker=checker<Logger>>
  class basic_test : public test, protected Checker
  {
  public:      
    explicit basic_test(std::string_view name) : test{name} {}
    
    log_summary execute() override
    {
      using namespace std::chrono;
      const auto time{steady_clock::now()};
      try
      {
        run_tests();
      }
      catch(std::exception& e)
      {         
        Checker::log_critical_failure(make_message("Unexpected", e.what()));
      }
      catch(...)
      {
        Checker::log_critical_failure(make_message("Unknown", ""));
      }

      return Checker::summary(name(), steady_clock::now() - time);
    }
  protected:    
    virtual void run_tests() = 0;

    [[nodiscard]]
    virtual std::string current_message() const { return std::string{Checker::current_message()}; }

    [[nodiscard]]
    std::string make_message(std::string_view tag, std::string_view exceptionMessage) const
    {
      return impl::make_message(tag, current_message(), exceptionMessage, Checker::exceptions_detected_by_sentinel());
    }
  };

  using unit_test = basic_test<unit_test_logger<test_mode::standard>>;
  using false_negative_test = basic_test<unit_test_logger<test_mode::false_negative>>;
  using false_positive_test = basic_test<unit_test_logger<test_mode::false_positive>>;

  [[nodiscard]]
  std::string report_line(std::string file, const int line, const std::string_view message);
  
  enum class concurrency_mode{ serial=-1, family, test, deep};

  #define LINE(message) report_line(__FILE__, __LINE__, message)    
}
