////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file UnitTestCore.hpp
    \brief Extension for testing of regular semantics.
*/

#include "FreeTestCore.hpp"

namespace sequoia::unit_testing
{
  template<class Logger>
  class regular_extender
  {
  public:
    explicit regular_extender(Logger& logger) : m_Logger{logger} {}

    regular_extender(const regular_extender&)            = delete;
    regular_extender& operator=(const regular_extender&) = delete;
    regular_extender& operator=(regular_extender&&) = delete;

    template<class T>
    void check_semantics(std::string_view description, const T& x, const T& y)
    {
      unit_testing::check_semantics(combine_messages("Regular Semantics", description), m_Logger, x, y);
    }

    template<class T, class Mutator>
    void check_semantics(std::string_view description, const T& x, const T& y, Mutator m)
    {
      unit_testing::check_semantics(combine_messages("Regular Semantics", description), m_Logger, x, y, m);
    }
  protected:
    regular_extender(regular_extender&&) noexcept = default;
    ~regular_extender() = default;
  private:
    Logger& m_Logger;
  };
  
  template<test_mode mode>
  using regular_checker = checker<unit_test_logger<mode>, regular_extender<unit_test_logger<mode>>>;
  
  template<test_mode mode>
  using canonical_regular_test = basic_test<unit_test_logger<mode>, regular_checker<mode>>;

  using regular_test                = canonical_regular_test<test_mode::standard>;
  using false_negative_regular_test = canonical_regular_test<test_mode::false_negative>;
  using false_positive_regular_test = canonical_regular_test<test_mode::false_positive>;  
}
