////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Implementation details for checking move-only semantics.
*/

#include "sequoia/TestFramework/SemanticsCheckersDetails.hpp"

namespace sequoia::testing::impl
{
  template<test_mode Mode, class Actions, moveonly T, class U, std::invocable<T&> Mutator, class... Args>
  bool check_semantics(test_logger<Mode>& logger,
                       const Actions& actions,
                       T&& x,
                       T&& y,
                       const U& xEquivalent,
                       const U& yEquivalent,
                       optional_ref<const U> movedFromPostConstruction,
                       optional_ref<const U> movedFromPostAssignment,
                       Mutator m,
                       const Args&... args)
  {
    sentinel<Mode> sentry{logger, ""};

    if(!check_preconditions(logger, actions, x, y, xEquivalent, yEquivalent, args...))
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
