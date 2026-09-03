////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

module;

#include "sequoia/PlatformSpecific/Macros.hpp"
#include "sequoia/TestFramework/Macros.hpp"

export module sequoia.test_framework:MoveOnlyTestCore;

import std;

import :Advice;
import :BinaryRelationships;
import :CoreInfrastructure;
import :FailureInfo;
import :FileEditors;
import :FileSystemUtilities;
import :FreeCheckers;
import :FreeTestCore;
import :IndividualTestPaths;
import :MoveOnlyCheckers;
import :MoveOnlyCheckersDetails;
import :Output;
import :PathCheckers;
import :PointerCheckers;
import :ProductTypeCheckers;
import :ProjectPaths;
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
    \brief Extension for testing classes exhibiting move-only semantics.

    This class is designed to be plugged into the checker class template, in order to extend
    its functionality. See MoveOnlyCheckers.hpp for further information.
*/

export namespace sequoia::testing
{
  [[nodiscard]]
  std::string move_only_message(std::string description);

  /** \brief class template for plugging into the \ref checker_primary "checker"
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
    
    /** Prerequisites:
          x != y
          x equivalent to xEquivalent
          y equivalent to yEquivalent
     */
    template<moveonly T, class U, class V, class Self>
      requires checkable_against_for_semantics<Mode, T, U> && checkable_against_for_semantics<Mode, T, V>
    bool check_semantics(this Self& self,
                         const reporter& description,
                         T&& x,
                         T&& y,
                         const U& xEquivalent,
                         const U& yEquivalent,
                         const V& movedFromPostConstruction,
                         const V& movedFromPostAssignment)
    {
      return testing::check_semantics(
               move_only_message(self.report(description)),
               self.m_Logger,
               std::forward<T>(x),
               std::forward<T>(y),
               xEquivalent,
               yEquivalent,
               optional_ref<const V>{movedFromPostConstruction},
               optional_ref<const V>{movedFromPostAssignment}
             );
    }

    /** Prerequisites:
          x != y
          x equivalent to xEquivalent
          y equivalent to yEquivalent
     */
    template<moveonly T, class U, class Self>
      requires checkable_against_for_semantics<Mode, T, U>
    bool check_semantics(this Self& self, const reporter& description, T&& x, T&& y, const U& xEquivalent, const U& yEquivalent)
    {
      return testing::check_semantics(
               move_only_message(self.report(description)),
               self.m_Logger,
               std::forward<T>(x),
               std::forward<T>(y),
               xEquivalent,
               yEquivalent,
               optional_ref<const U>{},
               optional_ref<const U>{}
             );
    }

    /// Prerequisite: xMaker() != yMaker()
    template
    <
      std::regular_invocable xMaker,      
      moveonly T=std::invoke_result_t<xMaker>,
      regular_invocable_exactly_r<T> yMaker,
      class U,
      class Self
    >
      requires checkable_against_for_semantics<Mode, T, U>
    bool check_semantics(this Self& self,
                         const reporter& description,
                         xMaker xFn,
                         yMaker yFn,
                         const U& movedFromPostConstruction,
                         const U& movedFromPostAssignment)
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

    /// Prerequisite: xMaker() != yMaker()
    template
    <
      std::regular_invocable xMaker,
      moveonly T=std::invoke_result_t<xMaker>,
      regular_invocable_exactly_r<T> yMaker,
      class Self
    >
    bool check_semantics(this Self& self, const reporter& description, xMaker xFn, yMaker yFn)
    {
      return self.check_semantics(description, xFn(), yFn(), xFn(), yFn());
    }

    /** Prerequisites:
          x != y, with values consistent with order
          x equivalent to xEquivalent
          y equivalent to yEquivalent
     */
    template<moveonly T, class U, class V, class Self>
      requires deep_totally_ordered<T> && checkable_against_for_semantics<Mode, T, U> && checkable_against_for_semantics<Mode, T, V>
    bool check_semantics(this Self& self,
                         const reporter& description,
                         T&& x,
                         T&& y,
                         const U& xEquivalent,
                         const U& yEquivalent,
                         const V& movedFromPostConstruction,
                         const V& movedFromPostAssignment,
                         std::weak_ordering order)
    {
      return testing::check_semantics(
               move_only_message(self.report(description)),
               self.m_Logger,
               std::forward<T>(x),
               std::forward<T>(y),
               xEquivalent,
               yEquivalent,
               optional_ref<const V>{movedFromPostConstruction},
               optional_ref<const V>{movedFromPostAssignment},
               order
             );
    }

    /** Prerequisites:
          x != y, with values consistent with order
          x equivalent to xEquivalent
          y equivalent to yEquivalent
     */
    template<moveonly T, class U, class Self>
      requires deep_totally_ordered<T> && checkable_against_for_semantics<Mode, T, U>
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
               std::forward<T>(x),
               std::forward<T>(y),
               xEquivalent,
               yEquivalent,
               optional_ref<const U>{},
               optional_ref<const U>{},
               order
             );
    }

    /** Prerequisites:
          x != y, with values consistent with order
          x equivalent to xEquivalent
          y equivalent to yEquivalent
     */
    template
    <
      std::regular_invocable xMaker,
      moveonly T=std::invoke_result_t<xMaker>,
      regular_invocable_exactly_r<T> yMaker,
      class U,
      class Self
    >
      requires deep_totally_ordered<T> && checkable_against_for_semantics<Mode, T, U>
    bool check_semantics(this Self& self,
                         const reporter& description,
                         xMaker xFn,
                         yMaker yFn,
                         const U& movedFromPostConstruction,
                         const U& movedFromPostAssignment,
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

    /// Prerequisite: xMaker() != yMaker(), with values consistent with order
    template
    <
      class Self,
      std::regular_invocable xMaker,
      moveonly T=std::invoke_result_t<xMaker>,
      regular_invocable_exactly_r<T> yMaker
    >
      requires deep_totally_ordered<T>
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

  /** \anchor move_only_test_alias */
  using move_only_test                = canonical_move_only_test<test_mode::standard>;
  using move_only_false_positive_test = canonical_move_only_test<test_mode::false_positive>;
  using move_only_false_negative_test = canonical_move_only_test<test_mode::false_negative>;
}
