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
  template<test_mode Mode, class Actions, moveonly T, class S, std::invocable<T&> Mutator, class... Args>
  bool check_semantics(test_logger<Mode>& logger,
                       const Actions& actions,
                       T&& x,
                       T&& y,
                       const S& xEquivalent,
                       Mutator,
                       const Args&... args)
  {
    sentinel<Mode> sentry{logger, ""};

    if(!check_preconditions(logger, actions, x, y, args...))
      return false;

    auto opt{check_move_construction(logger, actions, std::move(x), xEquivalent, /*movedFromPostConstruction,*/ args...)};
    if(!opt) return false;

    return !sentry.failure_detected();
  }
  
  template<test_mode Mode, class Actions, moveonly T, std::invocable<T&> Mutator, class... Args>
  bool check_semantics(test_logger<Mode>& logger,
                       const Actions& actions,
                       T&& x,
                       T&& y,
                       const T& xClone,
                       const T& yClone,
                       opt_moved_from_ref<T> movedFromPostConstruction,
                       opt_moved_from_ref<T> movedFromPostAssignment,
                       Mutator m,
                       const Args&... args)
  {
    sentinel<Mode> sentry{logger, ""};

    if(!check_preconditions(logger, actions, x, y, xClone, yClone, args...))
      return false;

    auto opt{check_move_construction(logger, actions, std::move(x), xClone, movedFromPostConstruction, args...)};
    if(!opt) return false;

    if constexpr (do_swap<Args...>::value)
    {
      if(check_swap(logger, actions, std::move(*opt), std::move(y), xClone, yClone, args...))
      {
        check_move_assign(logger, actions, y, std::move(*opt), yClone, movedFromPostAssignment, std::move(m), args...);
      }
    }
    else
    {
      check_move_assign(logger, actions, *opt, std::move(y), yClone, movedFromPostAssignment, std::move(m), args...);
    }

    if constexpr (serializable_to<T, std::stringstream> && deserializable_from<T, std::stringstream>)
    {
      check_serialization(logger, actions, std::move(x), yClone, args...);
    }

    return !sentry.failure_detected();
  }
}
