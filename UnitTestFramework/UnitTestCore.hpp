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

#include "UnitTestCheckers.hpp"

namespace sequoia::unit_testing
{        
  class test
  {
  public:
    test(std::string_view name) : m_Name{name} {}
    virtual ~test() = default;

    virtual log_summary execute() = 0;

    const std::string& name() const noexcept { return m_Name; }
  protected:
    test(const test&) = default;
    test(test&&)      = default;

    test& operator=(const test&) = default;
    test& operator=(test&&)      = default;
      
  private:
    std::string m_Name;
  };
    
  template<class Logger, class Checker=checker<Logger>>
  class basic_test : public test, protected Checker
  {
  public:      
    basic_test(std::string_view name) : test{name} {}
      
    log_summary execute() override
    {        
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

      return Checker::summary(name());
    }
  protected:
    virtual void run_tests() = 0;

    virtual std::string current_message() const { return std::string{Checker::current_message()}; }

    std::string make_message(std::string_view tag, std::string_view exceptionMessage) const
    {
      auto mess{(std::string{"\tError -- "}.append(tag) += " Exception:") += format(exceptionMessage) += '\n'};
      if(Checker::exceptions_detected_by_sentinel())
      {
        mess += "\tException thrown during last check\n";
      }
      else
      {
        mess += "\tException thrown after check completed\n";
      }

      mess += ("\tLast Recorded Message:\n" + format(current_message()));

      return mess;
    }

    static std::string format(std::string_view s)
    {
      return s.empty() ? std::string{s} : std::string{'\t'}.append(s);
    }
  };

  using unit_test = basic_test<unit_test_logger<test_mode::standard>>;
  using false_positive_test = basic_test<unit_test_logger<test_mode::false_positive>>;
  using false_negative_test = basic_test<unit_test_logger<test_mode::false_negative>>;    
    
  inline std::string report_line(std::string file, const int line, const std::string_view message)
  {
    std::string s{std::move(file) + ", Line " + std::to_string(line)};
    if(!message.empty()) (s += ":\n") += message;

    return s;
  }

  #define LINE(message) report_line(__FILE__, __LINE__, message)    
}
