////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Extension for checking allocations for types with move-only semantics.

    Move-only semantics are defined \ref move_only_definition "here".
*/

#include "sequoia/TestFramework/MoveOnlyTestCore.hpp"
#include "sequoia/TestFramework/MoveOnlyAllocationCheckers.hpp"
#include "sequoia/TestFramework/AllocationTestUtilities.hpp"

namespace sequoia::testing
{
  /*! \brief class template for plugging into the \ref checker_primary "checker"
      class template to provide allocation checks for move-only types.

      \anchor move_only_allocation_extender_primary
   */
  template<test_mode Mode>
  class move_only_allocation_extender
  {
  public:
    constexpr static test_mode mode{Mode};

    explicit move_only_allocation_extender(test_logger<Mode>& logger) : m_pLogger{&logger} {}

    move_only_allocation_extender(const move_only_allocation_extender&) = delete;
    move_only_allocation_extender(move_only_allocation_extender&&)      = delete;

    move_only_allocation_extender& operator=(const move_only_allocation_extender&) = delete;
    move_only_allocation_extender& operator=(move_only_allocation_extender&&)      = delete;

    template<class Self, moveonly T, std::invocable<T&> Mutator, alloc_getter<T>... Getters>
      requires (!std::totally_ordered<T> && (sizeof...(Getters) > 0))
    void check_semantics(this Self&& self, const report& description, T&& x, T&& y, const T& xClone, const T& yClone, const T& movedFrom, Mutator yMutator, const allocation_info<T, Getters>&... info)
    {
      testing::check_semantics(move_only_message(self.report_line(description)), self.get_logger(), std::move(x), std::move(y), xClone, yClone, opt_moved_from_ref<T>{movedFrom}, std::move(yMutator), info...);
    }

    template<class Self, moveonly T, std::invocable<T&> Mutator, alloc_getter<T>... Getters>
      requires (!std::totally_ordered<T>  && (sizeof...(Getters) > 0))
    void check_semantics(this Self&& self, const report& description, T&& x, T&& y, const T& xClone, const T& yClone, Mutator yMutator, const allocation_info<T, Getters>&... info)
    {
      testing::check_semantics(move_only_message(self.report_line(description)), self.get_logger(), std::move(x), std::move(y), xClone, yClone, opt_moved_from_ref<T>{}, std::move(yMutator), info...);
    }

    template
    <
      class Self,
      moveonly T,
      invocable_r<T> xMaker,
      invocable_r<T> yMaker,
      std::invocable<T&> Mutator,
      alloc_getter<T>... Getters
    >
      requires (!std::totally_ordered<T>  && (sizeof...(Getters) > 0))
    std::pair<T,T> check_semantics(this Self&& self, const report& description, xMaker xFn, yMaker yFn, const T& movedFrom, Mutator yMutator, const allocation_info<T, Getters>&... info)
    {
      return testing::check_semantics(move_only_message(self.report_line(description)), self.get_logger(), std::move(xFn), std::move(yFn), opt_moved_from_ref<T>{movedFrom}, std::move(yMutator), info...);
    }

    template
    <
      class Self,
      moveonly T,
      invocable_r<T> xMaker,
      invocable_r<T> yMaker,
      std::invocable<T&> Mutator,
      alloc_getter<T>... Getters
    >
      requires (!std::totally_ordered<T>  && (sizeof...(Getters) > 0))
    std::pair<T,T> check_semantics(this Self&& self, const report& description, xMaker xFn, yMaker yFn, Mutator yMutator, const allocation_info<T, Getters>&... info)
    {
      return testing::check_semantics(move_only_message(self.report_line(description)), self.get_logger(), std::move(xFn), std::move(yFn), opt_moved_from_ref<T>{}, std::move(yMutator), info...);
    }

    template<class Self, moveonly T, std::invocable<T&> Mutator, alloc_getter<T>... Getters>
      requires (std::totally_ordered<T> && (sizeof...(Getters) > 0))
    void check_semantics(this Self&& self, const report& description, T&& x, T&& y, const T& xClone, const T& yClone, const T& movedFrom, std::weak_ordering order, Mutator yMutator, const allocation_info<T, Getters>&... info)
    {
      testing::check_semantics(move_only_message(self.report_line(description)), self.get_logger(), std::move(x), std::move(y), xClone, yClone, opt_moved_from_ref<T>{movedFrom}, order, std::move(yMutator), info...);
    }

    template<class Self, moveonly T, std::invocable<T&> Mutator, alloc_getter<T>... Getters>
      requires (std::totally_ordered<T>  && (sizeof...(Getters) > 0))
    void check_semantics(this Self&& self, const report& description, T&& x, T&& y, const T& xClone, const T& yClone, std::weak_ordering order, Mutator yMutator, const allocation_info<T, Getters>&... info)
    {
      testing::check_semantics(move_only_message(self.report_line(description)), self.get_logger(), std::move(x), std::move(y), xClone, yClone, opt_moved_from_ref<T>{}, order, std::move(yMutator), info...);
    }

    template
    <
      class Self,
      moveonly T,
      invocable_r<T> xMaker,
      invocable_r<T> yMaker,
      std::invocable<T&> Mutator,
      alloc_getter<T>... Getters
    >
      requires (std::totally_ordered<T> && (sizeof...(Getters) > 0))
    std::pair<T, T> check_semantics(this Self&& self, const report& description, xMaker xFn, yMaker yFn, const T& movedFrom, std::weak_ordering order, Mutator yMutator, allocation_info<T, Getters>... info)
    {
      return testing::check_semantics(move_only_message(self.report_line(description)), self.get_logger(), std::move(xFn), std::move(yFn), opt_moved_from_ref<T>{movedFrom}, order, std::move(yMutator), info...);
    }

    template
    <
      class Self,
      moveonly T,
      invocable_r<T> xMaker,
      invocable_r<T> yMaker,
      std::invocable<T&> Mutator,
      alloc_getter<T>... Getters
    >
      requires (std::totally_ordered<T>  && (sizeof...(Getters) > 0))
    std::pair<T,T> check_semantics(this Self&& self, const report& description, xMaker xFn, yMaker yFn, std::weak_ordering order, Mutator yMutator, allocation_info<T, Getters>... info)
    {
      return testing::check_semantics(move_only_message(self.report_line(description)), self.get_logger(), std::move(xFn), std::move(yFn), opt_moved_from_ref<T>{}, order, std::move(yMutator), info...);
    }
  protected:
    ~move_only_allocation_extender() = default;

    [[nodiscard]]
    test_logger<Mode>& get_logger() noexcept { return *m_pLogger; }

  private:
    test_logger<Mode>* m_pLogger;
  };

  template<class Test, test_mode Mode>
  concept move_only_alloc_test =
       std::derived_from<Test, basic_test<Mode, move_only_allocation_extender<Mode>>>
    && !std::is_abstract_v<Test>
    && requires{
         std::declval<Test>().template test_allocation<true, true>();
  };


  /*!  \brief Templated on the test_mode, this forms the basis of all allocation tests for move-only types.

       This class template provides a mechanism to help with the automatic generation of checks with
       all 4 combinations of the allocation propagation flags relevant to move-only types. To utilize
       this, derived classes need to define the following function template

       template<bool, bool>
       void test_allocation();

       Within the derived class, a call

       do_allocation_tests(*this);

       will ensure that all checks defined in the test_allocation function template are executed
       for each combination of the allocation propagation flags.

       \anchor basic_move_only_allocation_test_primary
   */

  template<test_mode Mode>
  class basic_move_only_allocation_test : public basic_test<Mode, move_only_allocation_extender<Mode>>
  {
  public:
    using basic_test<Mode, move_only_allocation_extender<Mode>>::basic_test;

    basic_move_only_allocation_test(const basic_move_only_allocation_test&) = delete;
    basic_move_only_allocation_test& operator=(const basic_move_only_allocation_test&) = delete;
  protected:
    basic_move_only_allocation_test(basic_move_only_allocation_test&&)           noexcept = default;
    basic_move_only_allocation_test& operator=(basic_move_only_allocation_test&&) noexcept = default;

    template<move_only_alloc_test<Mode> Test>
    static void do_allocation_tests(Test& test)
    {
      test.template test_allocation<false, false>();
      test.template test_allocation<false, true>();
      test.template test_allocation<true, false>();
      test.template test_allocation<true, true>();
    }
  };

   /*! \anchor move_only_allocation_test_alias */
  using move_only_allocation_test                = basic_move_only_allocation_test<test_mode::standard>;
  using move_only_allocation_false_negative_test = basic_move_only_allocation_test<test_mode::false_negative>;
  using move_only_allocation_false_positive_test = basic_move_only_allocation_test<test_mode::false_positive>;
}
