////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

module;

#include "sequoia/PlatformSpecific/Macros.hpp"
#include "sequoia/TestFramework/Macros.hpp"

export module sequoia.test_framework:MoveOnlyCheckersDetails;

import std;

import :Advice;
import :BinaryRelationships;
import :CoreInfrastructure;
import :FailureInfo;
import :FreeCheckers;
import :Output;
import :ProjectPaths;
import :SemanticsCheckersDetails;
import :TestLogger;
import :TestMode;
export import sequoia.core.meta;
export import sequoia.platform_specific;
export import sequoia.text_processing;

/** \file
    \brief Implementation details for checking move-only semantics.
*/

export namespace sequoia::testing::impl
{
  template<test_mode Mode, class Actions, moveonly T, class U, class V, std::invocable<T&> Mutator, class... Args>
    requires checkable_against_for_semantics<Mode, T, U> && checkable_against_for_semantics<Mode, T, V>
  bool check_semantics(test_logger<Mode>& logger,
                       const Actions& actions,
                       T&& x,
                       T&& y,
                       const U& xEquivalent,
                       const U& yEquivalent,
                       optional_ref<const V> movedFromPostConstruction,
                       optional_ref<const V> movedFromPostAssignment,
                       Mutator m,
                       const Args&... args)
  {
    sentinel<Mode> sentry{logger, ""};

    if(!check_prerequisites(logger, actions, x, y, xEquivalent, yEquivalent, args...))
      return false;

    auto opt{check_move_construction(logger, actions, std::move(x), xEquivalent, movedFromPostConstruction, args...)};
    if(!opt) return false;

    if constexpr (do_swap<Args...>::value)
    {
      if(check_swap(logger, actions, std::move(*opt), std::move(y), xEquivalent, yEquivalent, args...))
      {
        check_move_assign(logger, actions, y, std::move(*opt), yEquivalent, movedFromPostAssignment, std::move(m), args...);
      }
    }
    else
    {
      check_move_assign(logger, actions, *opt, std::move(y), yEquivalent, movedFromPostAssignment, std::move(m), args...);
    }

    if constexpr (serializable_to<T, std::stringstream> && deserializable_from<T, std::stringstream>)
    {
      check_serialization(logger, actions, std::move(x), yEquivalent, args...);
    }

    return !sentry.failure_detected();
  }
}
