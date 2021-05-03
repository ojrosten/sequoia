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
  template<moveonly T>
  struct move_only_actions : precondition_actions<T>
  {
    using precondition_actions<T>::precondition_actions;

    constexpr static bool has_post_comparison_action{};
    constexpr static bool has_post_move_action{};
    constexpr static bool has_post_move_assign_action{};
    constexpr static bool has_post_swap_action{};

    template<test_mode Mode, class... Args>
    [[nodiscard]]
    bool check_preconditions(test_logger<Mode>& logger, const T& x, const T& y, const T& xClone, const T& yClone, const Args&... args) const
    {
      return precondition_actions<T>::check_preconditions(logger, *this, x, y, xClone, yClone, args...);
    }
  };

  template<test_mode Mode, class Actions, moveonly T, invocable<T&> Mutator, class... Args>
  bool check_semantics(test_logger<Mode>& logger, const Actions& actions, T&& x, T&& y, const T& xClone, const T& yClone, Mutator m, const Args&... args)
  {
    sentinel<Mode> sentry{logger, ""};

    if(!actions.check_preconditions(logger, x, y, xClone, yClone, args...))
      return false;

    auto opt{check_move_construction(logger, actions, std::move(x), xClone, args...)};
    if(!opt) return false;

    if constexpr (do_swap<Args...>::value)
    {
      if(check_swap(logger, actions, std::move(*opt), std::move(y), xClone, yClone, args...))
      {
        check_move_assign(logger, actions, y, std::move(*opt), yClone, std::move(m), args...);
      }
    }
    else
    {
      check_move_assign(logger, actions, *opt, std::move(y), yClone, std::move(m), args...);
    }

    if constexpr (serializable_to<T, std::stringstream> && deserializable_from<T, std::stringstream>)
    {
      check_serialization(logger, actions, std::move(x), yClone, args...);
    }

    return !sentry.failure_detected();
  }
}
