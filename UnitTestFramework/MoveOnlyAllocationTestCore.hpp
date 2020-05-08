////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file MoveOnlyRegularAllocationTestCore.hpp
    \brief Extension of unit testing framework for allocator testing
*/

#include "MoveOnlyTestCore.hpp"
#include "MoveOnlyAllocationCheckers.hpp"
#include "AllocationTestUtilities.hpp"

namespace sequoia::unit_testing
{
  template<class Logger>
  class move_only_allocation_extender
  {
  public:
    explicit move_only_allocation_extender(Logger& logger) : m_Logger{logger} {}

    move_only_allocation_extender(const move_only_allocation_extender&) = delete;    
    move_only_allocation_extender(move_only_allocation_extender&&)      = delete;

    move_only_allocation_extender& operator=(const move_only_allocation_extender&) = delete;  
    move_only_allocation_extender& operator=(move_only_allocation_extender&&)      = delete;

    template<class T, class Mutator, class... Allocators>
    void check_semantics(std::string_view description, T&& x, T&& y, const T& xClone, const T& yClone, Mutator yMutator, move_only_allocation_info<T, Allocators>... info)
    {
      unit_testing::check_semantics(combine_messages("Move-only Semantics", description), m_Logger, std::move(x), std::move(y), xClone, yClone, std::move(yMutator), info...);
    }
  protected:
    ~move_only_allocation_extender() = default;

  private:
    Logger& m_Logger;
  };

  template<class Logger>
  class basic_move_only_allocation_test : public basic_test<checker<Logger, move_only_allocation_extender<Logger>>>
  {
  public:
    using basic_test<checker<Logger, move_only_allocation_extender<Logger>>>::basic_test;
        
    basic_move_only_allocation_test(const basic_move_only_allocation_test&) = delete;

    basic_move_only_allocation_test& operator=(const basic_move_only_allocation_test&) = delete;
    basic_move_only_allocation_test& operator=(basic_move_only_allocation_test&&)      = delete;
  protected:
    basic_move_only_allocation_test(basic_move_only_allocation_test&&) noexcept = default;

    template<class Test>
    static void do_allocation_tests(Test& test)
    {
      test.template test_allocation<false, false>();
      test.template test_allocation<false, true>();
      test.template test_allocation<true, false>();
      test.template test_allocation<true, true>();
    }
  };

  template<test_mode Mode>
  using canonical_move_only_allocation_test = basic_move_only_allocation_test<unit_test_logger<Mode>>;

  using move_only_allocation_test                = canonical_move_only_allocation_test<test_mode::standard>;
  using move_only_allocation_false_negative_test = canonical_move_only_allocation_test<test_mode::false_negative>;
  using move_only_allocation_false_positive_test = canonical_move_only_allocation_test<test_mode::false_positive>;
}
