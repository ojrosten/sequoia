////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file MoveOnlTestCore.hpp
    \brief Extension for testing of regular semantics.
*/

#include "FreeTestCore.hpp"
#include "MoveOnlyCheckers.hpp"

namespace sequoia::unit_testing
{
  template<class Logger>
  class move_only_extender
  {
  public:
    explicit move_only_extender(Logger& logger) : m_Logger{logger} {}

    move_only_extender(const move_only_extender&)            = delete;
    move_only_extender& operator=(const move_only_extender&) = delete;
    move_only_extender& operator=(move_only_extender&&) = delete;

    template<class T, class... Allocators>
    void check_semantics(std::string_view description, T&& x, T&& y, const T& xClone, const T& yClone)
    {
      unit_testing::check_semantics(combine_messages("Move-only Semantics", description), m_Logger, std::move(x), std::move(y), xClone, yClone);
    }
  protected:
    move_only_extender(move_only_extender&&) noexcept = default;
    ~move_only_extender() = default;
  private:
    Logger& m_Logger;
  };
  
  template<test_mode mode>
  using move_only_checker = checker<unit_test_logger<mode>, move_only_extender<unit_test_logger<mode>>>;
  
  template<test_mode mode>
  using canonical_move_only_test = basic_test<move_only_checker<mode>>;

  using move_only_test                = canonical_move_only_test<test_mode::standard>;
  using move_only_false_negative_test = canonical_move_only_test<test_mode::false_negative>;
  using move_only_false_positive_test = canonical_move_only_test<test_mode::false_positive>;  
}
