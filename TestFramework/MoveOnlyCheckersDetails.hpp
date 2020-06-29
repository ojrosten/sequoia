////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Implementation details for checking move-only semantics.
*/

#include "AllocationCheckersDetails.hpp"

namespace sequoia::testing::impl
{
  template<test_mode Mode, class Actions, class T, class Mutator, class... Args>
  void check_semantics(sentinel<Mode>& sentry, const Actions& actions, T&& x, T&& y, const T& xClone, const T& yClone, Mutator m, const Args&... args)
  {
    // Preconditions
    if(!check_preconditions(sentry, actions, x, y, args...))
      return;

    if(!check(sentry.generate_message("Precondition - for checking move-only semantics, x and xClone are assumed to be equal"), sentry.logger(), x == xClone)) return;

    if(!check(sentry.generate_message("Precondition - for checking move-only semantics, y and yClone are assumed to be equal"), sentry.logger(), y == yClone)) return;

    auto opt{check_move_construction(sentry, actions, std::move(x), xClone, args...)};
    if(!opt) return;
      
    if constexpr (do_swap<Args...>::value)
    {
      if(check_swap(sentry, actions, std::move(*opt), std::move(y), xClone, yClone, args...))
      {
        check_move_assign(sentry, actions, y, std::move(*opt), yClone, std::move(m), args...);
      }
    }
    else
    {      
      check_move_assign(sentry, actions, *opt, std::move(y), yClone, std::move(m), args...);
    }
  }
}
