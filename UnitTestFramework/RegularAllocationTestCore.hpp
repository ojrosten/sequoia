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
#include "AllocationCheckers.hpp"
#include "AllocationTestUtilities.hpp"

namespace sequoia::unit_testing
{
  template<class Logger>
  class allocation_extender
  {
  public:
    explicit allocation_extender(Logger& logger) : m_Logger{logger} {}

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
    Logger& m_Logger;
  };
  
  template<class Logger>
  class basic_allocation_test : public basic_test<Logger, checker<Logger, allocation_extender<Logger>>>
  {
  public:
    using basic_test<Logger, checker<Logger, allocation_extender<Logger>>>::basic_test;
        
    basic_allocation_test(const basic_allocation_test&) = delete;

    basic_allocation_test& operator=(const basic_allocation_test&) = delete;
    basic_allocation_test& operator=(basic_allocation_test&&)      = delete;
  protected:
    basic_allocation_test(basic_allocation_test&&) noexcept = default;

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
   
  template<test_mode Mode>
  using canonical_allocation_test = basic_allocation_test<unit_test_logger<Mode>>;

  using allocation_test                = canonical_allocation_test<test_mode::standard>;
  using allocation_false_negative_test = canonical_allocation_test<test_mode::false_negative>;
  using allocation_false_positive_test = canonical_allocation_test<test_mode::false_positive>;
}
