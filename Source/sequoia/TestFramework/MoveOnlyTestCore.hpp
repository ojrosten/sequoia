////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Extension for testing classes exhibiting move-only semantics.

    This class is designed to be plugged into the checker class template, in order to extend
    its functionality. See MoveOnlyCheckers.hpp for further information.
*/

#include "sequoia/TestFramework/FreeTestCore.hpp"
#include "sequoia/TestFramework/MoveOnlyCheckers.hpp"

#include "sequoia/Core/Meta/Utilities.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string move_only_message(std::string description);

  /*! \brief class template for plugging into the \ref checker_primary "checker"
      class template to provide allocation checks for move-only types,
      see \ref move_only_definition "here".

      \anchor move_only_extender_primary
   */
  template<test_mode Mode>
  class move_only_extender
  {
  public:
    constexpr static test_mode mode{Mode};

    move_only_extender() = default;
    
    /// Preconditions: x!=y; x==xEquivalent, y==yEquivalent
    template<class Self, moveonly T, class U>
      requires checkable_against<Mode, T, U>
    bool check_semantics(this Self& self,
                         const reporter& description,
                         T&& x,
                         T&& y,
                         const U& xEquivalent,
                         const U& yEquivalent,
                         const U& movedFromPostConstruction,
                         const U& movedFromPostAssignment)
    {
      return testing::check_semantics(
               move_only_message(self.report(description)),
               self.m_Logger,
               std::move(x),
               std::move(y),
               xEquivalent,
               yEquivalent,
               optional_ref<const T>{movedFromPostConstruction},
               optional_ref<const T>{movedFromPostAssignment}
             );
    }

    template<class Self, moveonly T, class U>
      requires checkable_against<Mode, T, U>
    bool check_semantics(this Self& self, const reporter& description, T&& x, T&& y, const U& xEquivalent, const U& yEquivalent)
    {
      return testing::check_semantics(
               move_only_message(self.report(description)),
               self.m_Logger,
               std::move(x),
               std::move(y),
               xEquivalent,
               yEquivalent,
               optional_ref<const U>{},
               optional_ref<const U>{}
             );
    }

    template
    <
      class Self,
      moveonly T,
      regular_invocable_r<T> xMaker,
      regular_invocable_r<T> yMaker
    >
    bool check_semantics(this Self& self,
                         const reporter& description,
                         xMaker xFn,
                         yMaker yFn,
                         const T& movedFromPostConstruction,
                         const T& movedFromPostAssignment)
    {
      return self.check_semantics(
               description,
               xFn(),
               yFn(),
               xFn(),
               yFn(),
               movedFromPostConstruction,
               movedFromPostAssignment);
    }

    template
    <
      class Self,
      std::regular_invocable xMaker,
      moveonly T=std::invoke_result_t<xMaker>,
      regular_invocable_r<T> yMaker
    >
    bool check_semantics(this Self& self, const reporter& description, xMaker xFn, yMaker yFn)
    {
      return self.check_semantics(description, xFn(), yFn(), xFn(), yFn());
    }

    /// Preconditions: x!=y, with values consistent with order; x==xEquivalent, y==yEquivalent
    template<class Self, moveonly T, class U>
      requires std::totally_ordered<T>
    bool check_semantics(this Self& self,
                         const reporter& description,
                         T&& x,
                         T&& y,
                         const U& xEquivalent,
                         const U& yEquivalent,
                         const U& movedFromPostConstruction,
                         const U& movedFromPostAssignment,
                         std::weak_ordering order)
    {
      return testing::check_semantics(
               move_only_message(self.report(description)),
               self.m_Logger,
               std::move(x),
               std::move(y),
               xEquivalent,
               yEquivalent,
               optional_ref<const U>{movedFromPostConstruction},
               optional_ref<const U>{movedFromPostAssignment},
               order
             );
    }

    template<class Self, moveonly T, class U>
      requires checkable_against<Mode, T, U> && std::totally_ordered<T>
    bool check_semantics(this Self& self,
                         const reporter& description,
                         T&& x,
                         T&& y,
                         const U& xEquivalent,
                         const U& yEquivalent,
                         std::weak_ordering order)
    {
      return testing::check_semantics(
               move_only_message(self.report(description)),
               self.m_Logger,
               std::move(x),
               std::move(y),
               xEquivalent,
               yEquivalent,
               optional_ref<const U>{},
               optional_ref<const U>{},
               order
             );
    }

    template
    <
      class Self,
      std::regular_invocable xMaker,
      moveonly T=std::invoke_result_t<xMaker>,
      regular_invocable_r<T> yMaker
    >
      requires std::totally_ordered<T>
    bool check_semantics(this Self& self,
                         const reporter& description,
                         xMaker xFn,
                         yMaker yFn,
                         const T& movedFromPostConstruction,
                         const T& movedFromPostAssignment,
                         std::weak_ordering order)
    {
      return self.check_semantics(
               description,
               xFn(),
               yFn(),
               xFn(),
               yFn(),
               movedFromPostConstruction,
               movedFromPostAssignment,
               order
             );
    }

    template
    <
      class Self,
      std::regular_invocable xMaker,
      moveonly T=std::invoke_result_t<xMaker>,
      regular_invocable_r<T> yMaker
    >
      requires std::totally_ordered<T>
    bool check_semantics(this Self& self, const reporter& description, xMaker xFn, yMaker yFn, std::weak_ordering order)
    {
      return self.check_semantics(description, xFn(), yFn(), xFn(), yFn(), order);
    }

  protected:
    ~move_only_extender() = default;

    move_only_extender(move_only_extender&&)            noexcept = default;
    move_only_extender& operator=(move_only_extender&&) noexcept = default;
  };

  template<test_mode mode>
  using canonical_move_only_test = basic_test<mode, move_only_extender<mode>>;

  /*! \anchor move_only_test_alias */
  using move_only_test                = canonical_move_only_test<test_mode::standard>;
  using move_only_false_positive_test = canonical_move_only_test<test_mode::false_positive>;
  using move_only_false_negative_test = canonical_move_only_test<test_mode::false_negative>;
}
