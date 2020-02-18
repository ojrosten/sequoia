////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file FuzzyTestCore.hpp
    \brief Extension of unit testing framework for fuzzy testing.
*/

#include "UnitTestCore.hpp"

namespace sequoia::unit_testing
{
  template<class Logger, class T, class Compare>
  bool check_approx_equality(std::string_view description, Logger& logger, const T& value, const T& prediction, Compare compare)
  {
    return check(description, logger, compare(value, prediction));
  }

  template<class Logger>
  class fuzzy_extender
  {
  public:
    explicit fuzzy_extender(Logger& logger) : m_Logger{logger} {}

    fuzzy_extender(const fuzzy_extender&) = delete;
    fuzzy_extender(fuzzy_extender&&)      = delete;

    fuzzy_extender& operator=(const fuzzy_extender&) = delete;
    fuzzy_extender& operator=(fuzzy_extender&&)      = delete;

    template<class T, class Compare>
    bool check_approx_equality(std::string_view description, const T& prediction, const T& value, Compare compare)
    {
      return unit_testing::check_approx_equality(description, m_Logger, value, prediction, std::move(compare));
    }

  protected:
    ~fuzzy_extender() = default;

  private:
    Logger& m_Logger;
  };

  template<test_mode mode>
  using fuzzy_checker = checker<unit_test_logger<mode>, fuzzy_extender<unit_test_logger<mode>>>;
  
  template<test_mode mode>
  using basic_fuzzy_test = basic_test<unit_test_logger<mode>, fuzzy_checker<mode>>;

  using fuzzy_test                = basic_fuzzy_test<test_mode::standard>;
  using fuzzy_false_negative_test = basic_fuzzy_test<test_mode::false_negative>;
  using fuzzy_false_positive_test = basic_fuzzy_test<test_mode::false_positive>;  
}
