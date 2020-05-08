////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file RegularTestCore.hpp
    \brief Extension for testing classes exhibiting regular semantics.
*/

#include "FreeTestCore.hpp"
#include "RegularCheckers.hpp"

namespace sequoia::unit_testing
{
  template<test_mode Mode>
  class regular_extender
  {
  public:
    constexpr static test_mode mode{Mode};
    
    explicit regular_extender(unit_test_logger<Mode>& logger) : m_Logger{logger} {}

    regular_extender(const regular_extender&)            = delete;
    regular_extender& operator=(const regular_extender&) = delete;
    regular_extender& operator=(regular_extender&&)      = delete;

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
    unit_test_logger<Mode>& m_Logger;
  };
  
  template<test_mode mode>
  using regular_checker = checker<mode, regular_extender<mode>>;
  
  template<test_mode mode>
  using canonical_regular_test = basic_test<regular_checker<mode>>;

  using regular_test                = canonical_regular_test<test_mode::standard>;
  using false_negative_regular_test = canonical_regular_test<test_mode::false_negative>;
  using false_positive_regular_test = canonical_regular_test<test_mode::false_positive>;  
}
