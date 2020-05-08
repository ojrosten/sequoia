////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file RegularAllocationTestCore.hpp
    \brief Extension of unit testing framework for allocator testing
*/

#include "RegularTestCore.hpp"
#include "RegularAllocationCheckers.hpp"
#include "AllocationTestUtilities.hpp"

namespace sequoia::unit_testing
{
  template<test_mode Mode>
  class allocation_extender
  {
  public:
    constexpr static test_mode mode{Mode};
    
    explicit allocation_extender(unit_test_logger<Mode>& logger) : m_Logger{logger} {}

    allocation_extender(const allocation_extender&) = delete;    
    allocation_extender(allocation_extender&&)      = delete;

    allocation_extender& operator=(const allocation_extender&) = delete;  
    allocation_extender& operator=(allocation_extender&&)      = delete;

    template<class T, class Mutator, class... Allocators>
    void check_semantics(std::string_view description, const T& x, const T& y, Mutator m, allocation_info<T, Allocators>... info)
    {
      unit_testing::check_semantics(combine_messages("Regular Semantics", description), m_Logger, x, y, m, info...);
    }
  protected:
    ~allocation_extender() = default;

  private:
    unit_test_logger<Mode>& m_Logger;
  };

  template<test_mode Mode>
  class basic_regular_allocation_test : public basic_test<checker<Mode, allocation_extender<Mode>>>
  {
  public:
    using basic_test<checker<Mode, allocation_extender<Mode>>>::basic_test;
        
    basic_regular_allocation_test(const basic_regular_allocation_test&) = delete;

    basic_regular_allocation_test& operator=(const basic_regular_allocation_test&) = delete;
    basic_regular_allocation_test& operator=(basic_regular_allocation_test&&)      = delete;
  protected:
    basic_regular_allocation_test(basic_regular_allocation_test&&) noexcept = default;

    template<class Test>
    static void do_allocation_tests(Test& test)
    {
      test.template test_allocation<false, false, false>();
      test.template test_allocation<false, false, true>();
      test.template test_allocation<false, true, false>();
      test.template test_allocation<false, true, true>();
      test.template test_allocation<true, false, false>();
      test.template test_allocation<true, false, true>();
      test.template test_allocation<true, true, false>();
      test.template test_allocation<true, true, true>();
    }
  };

  using regular_allocation_test                = basic_regular_allocation_test<test_mode::standard>;
  using regular_allocation_false_negative_test = basic_regular_allocation_test<test_mode::false_negative>;
  using regular_allocation_false_positive_test = basic_regular_allocation_test<test_mode::false_positive>;
}
