////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

module;

#include "sequoia/PlatformSpecific/Macros.hpp"
#include "sequoia/TestFramework/Macros.hpp"

export module sequoia.test_framework:RegularAllocationTestCore;

import std;

import :Advice;
import :AllocationCheckers;
import :AllocationCheckersCore;
import :AllocationCheckersDetails;
import :AllocationCheckersTraits;
import :AllocationTestUtilities;
import :BinaryRelationships;
import :CoreInfrastructure;
import :FailureInfo;
import :FileEditors;
import :FileSystemUtilities;
import :FreeCheckers;
import :FreeTestCore;
import :IndividualTestPaths;
import :Output;
import :PathCheckers;
import :PointerCheckers;
import :ProductTypeCheckers;
import :ProjectPaths;
import :RegularAllocationCheckers;
import :RegularAllocationCheckersDetails;
import :RegularCheckers;
import :RegularCheckersDetails;
import :RegularTestCore;
import :SemanticsCheckersDetails;
import :StringCheckers;
import :TestLogger;
import :TestMode;
export import sequoia.core.container_utilities;
export import sequoia.core.meta;
export import sequoia.core.object;
export import sequoia.file_system;
export import sequoia.platform_specific;
export import sequoia.streaming;
export import sequoia.text_processing;

/** \file
    \brief Extension for checking allocations for types with regular semantics,
    see \ref regular_semantics_definition "here".
*/

export namespace sequoia::testing
{
  /** \brief class template for plugging into the \ref checker_primary "checker"
      class template to provide allocation checks for regular types.

      \anchor regular_allocation_extender_primary
   */
  template<test_mode Mode>
  class regular_allocation_extender
  {
  public:
    constexpr static test_mode mode{Mode};

    regular_allocation_extender() = default;

    template<class Self, pseudoregular T, std::invocable<T&> Mutator, alloc_getter<T>... Getters>
      requires (!deep_totally_ordered<T> && (sizeof...(Getters) > 0))
    void check_semantics(this Self& self, const reporter& description, const T& x, const T& y, Mutator m, allocation_info<T, Getters>... info)
    {
      testing::check_semantics(append_lines(self.report(description), emphasise("Regular Semantics")), self.m_Logger, x, y, m, info...);
    }

    template
    <
      class Self,
      pseudoregular T,
      invocable_exact_r<T> xMaker,
      invocable_exact_r<T> yMaker,
      std::invocable<T&> Mutator,
      alloc_getter<T>... Getters
    >
      requires (!deep_totally_ordered<T> && (sizeof...(Getters) > 0))
    std::pair<T, T> check_semantics(this Self& self, const reporter& description, xMaker xFn, yMaker yFn, Mutator m, allocation_info<T, Getters>... info)
    {
      return testing::check_semantics(append_lines(self.report(description), emphasise("Regular Semantics")), self.m_Logger, std::move(xFn), std::move(yFn), m, info...);
    }

    template<class Self, pseudoregular T, std::invocable<T&> Mutator, alloc_getter<T>... Getters>
      requires (deep_totally_ordered<T> && (sizeof...(Getters) > 0))
    void check_semantics(this Self& self, const reporter& description, const T& x, const T& y, std::weak_ordering order, Mutator m, allocation_info<T, Getters>... info)
    {
      testing::check_semantics(append_lines(self.report(description), emphasise("Ordered Semantics")), self.m_Logger, x, y, order, m, info...);
    }

    template
    <
      class Self,
      pseudoregular T,
      invocable_exact_r<T> xMaker,
      invocable_exact_r<T> yMaker,
      std::invocable<T&> Mutator,
      alloc_getter<T>... Getters
    >
      requires (deep_totally_ordered<T> && (sizeof...(Getters) > 0))
    std::pair<T, T> check_semantics(this Self& self, const reporter& description, xMaker xFn, yMaker yFn, std::weak_ordering order, Mutator m, allocation_info<T, Getters>... info)
    {
      return testing::check_semantics(append_lines(self.report(description), emphasise("Ordered Semantics")), self.m_Logger, std::move(xFn), std::move(yFn), order, m, info...);
    }
  protected:
    ~regular_allocation_extender() = default;

    regular_allocation_extender(regular_allocation_extender&&)            noexcept = default;
    regular_allocation_extender& operator=(regular_allocation_extender&&) noexcept = default;
  };

  /**  \brief Templated on the test_mode, this forms the basis of all allocation tests for regular types.

       This class template provides a mechanism to help with the automatic generation of checks with
       all 8 combinations of the allocation propagation flags. To utilize this, derived classes need
       to define the following function template

       template<bool, bool, bool>
       void test_allocation();

       Within the derived class, a call

       do_allocation_tests();

       will ensure that all checks defined in the test_allocation function template are executed
       for each combination of the allocation propagation flags.

       \anchor basic_regular_allocation_test_primary
   */
  template<test_mode Mode>
  class basic_regular_allocation_test : public basic_test<Mode, regular_allocation_extender<Mode>>
  {
  public:
    using basic_test<Mode, regular_allocation_extender<Mode>>::basic_test;

  protected:
    ~basic_regular_allocation_test() = default;

    basic_regular_allocation_test(basic_regular_allocation_test&&)            noexcept = default;
    basic_regular_allocation_test& operator=(basic_regular_allocation_test&&) noexcept = default;

    template<class Self>
    void do_allocation_tests(this Self& self)
    {
      self.template test_allocation<false, false, false>();
      self.template test_allocation<false, false, true>();
      self.template test_allocation<false, true, false>();
      self.template test_allocation<false, true, true>();
      self.template test_allocation<true, false, false>();
      self.template test_allocation<true, false, true>();
      self.template test_allocation<true, true, false>();
      self.template test_allocation<true, true, true>();
    }
  };

  /** \anchor regular_allocation_test_alias */
  using regular_allocation_test                = basic_regular_allocation_test<test_mode::standard>;
  using regular_allocation_false_positive_test = basic_regular_allocation_test<test_mode::false_positive>;
  using regular_allocation_false_negative_test = basic_regular_allocation_test<test_mode::false_negative>;
}
