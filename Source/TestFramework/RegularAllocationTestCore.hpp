////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Extension for checking allocations for types with regular semantics,
    see \ref regular_semantics_definition "here".
*/

#include "RegularTestCore.hpp"
#include "RegularAllocationCheckers.hpp"
#include "AllocationTestUtilities.hpp"

namespace sequoia::testing
{
  /*! \brief class template for plugging into the \ref checker_primary "checker"
      class template to provide allocation checks for regular types.

      \anchor regular_allocation_extender_primary
   */
  template<test_mode Mode>
  class regular_allocation_extender
  {
  public:
    constexpr static test_mode mode{Mode};
    
    explicit regular_allocation_extender(test_logger<Mode>& logger) : m_Logger{logger} {}

    regular_allocation_extender(const regular_allocation_extender&) = delete;    
    regular_allocation_extender(regular_allocation_extender&&)      = delete;

    regular_allocation_extender& operator=(const regular_allocation_extender&) = delete;  
    regular_allocation_extender& operator=(regular_allocation_extender&&)      = delete;

    template<pseudoregular T, invocable<T&> Mutator, alloc_getter<T>... Getters>
      requires (!orderable<T> && (sizeof...(Getters) > 0))
    void check_semantics(std::string_view description, const T& x, const T& y, Mutator m, allocation_info<T, Getters>... info)
    {
      testing::check_semantics(append_lines(description, emphasise("Regular Semantics")), m_Logger, x, y, m, info...);
    }

    template<pseudoregular T, invocable<T&> Mutator, alloc_getter<T>... Getters>
    requires (orderable<T> && (sizeof...(Getters) > 0))
    void check_semantics(std::string_view description, const T& x, const T& y, std::weak_ordering order, Mutator m, allocation_info<T, Getters>... info)
    {
      testing::check_semantics(append_lines(description, emphasise("Ordered Semantics")), m_Logger, x, y, order, m, info...);
    }
  protected:
    ~regular_allocation_extender() = default;

  private:
    test_logger<Mode>& m_Logger;
  };

  template<class Test, test_mode Mode>
  concept reg_alloc_test =
       derived_from<Test, basic_test<checker<Mode, regular_allocation_extender<Mode>>>>
    && !std::is_abstract_v<Test>
    && requires{
         std::declval<Test>().template test_allocation<true, true, true>();
  };

  /*!  \brief Templated on the test_mode, this forms the basis of all allocation tests for regular types.

       This class template provides a mechanism to help with the automatic generation of checks with
       all 8 combinations of the allocation propagation flags. To utilize this, derived classes need
       to define the following function template

       template<bool, bool, bool>
       void test_allocation();

       Within the derived class, a call

       do_allocation_tests(*this);

       will ensure that all checks defined in the test_allocation function template are executed
       for each combination of the allocation propagation flags.
   
       \anchor basic_regular_allocation_test_primary
   */
  template<test_mode Mode>
  class basic_regular_allocation_test : public basic_test<checker<Mode, regular_allocation_extender<Mode>>>
  {
  public:
    using basic_test<checker<Mode, regular_allocation_extender<Mode>>>::basic_test;
        
    basic_regular_allocation_test(const basic_regular_allocation_test&) = delete;

    basic_regular_allocation_test& operator=(const basic_regular_allocation_test&) = delete;
    basic_regular_allocation_test& operator=(basic_regular_allocation_test&&)      = delete;
  protected:
    basic_regular_allocation_test(basic_regular_allocation_test&&) noexcept = default;

    template<reg_alloc_test<Mode> Test>
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

  /*! \anchor regular_allocation_test_alias */
  using regular_allocation_test                = basic_regular_allocation_test<test_mode::standard>;
  using regular_allocation_false_negative_test = basic_regular_allocation_test<test_mode::false_negative>;
  using regular_allocation_false_positive_test = basic_regular_allocation_test<test_mode::false_positive>;
}
