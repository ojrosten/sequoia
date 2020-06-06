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

#include "SemanticsCheckersDetails.hpp"

namespace sequoia::testing::impl
{
  template<test_mode Mode, class Actions, class T, class Mutator, class... Args>
  void check_semantics(std::string_view description, sentinel<Mode>& sentry, const Actions& actions, T&& x, T&& y, const T& xClone, const T& yClone, Mutator m, const Args&... args)
  {
    // Preconditions
    if(!check_preconditions(description, sentry, actions, x, y, args...))
      return;

    if(!check(sentry.add_details(description, "Precondition - for checking regular semantics, x and xClone are assumed to be equal"), sentry.logger(), x == xClone)) return;

    if(!check(sentry.add_details(description, "Precondition - for checking regular semantics, y and yClone are assumed to be equal"), sentry.logger(), y == yClone)) return;

    T z{check_move_construction(description, sentry, actions, std::move(x), xClone, args...)};    

    if constexpr (do_swap<Args...>::value)
    {
      using std::swap;
      swap(z, y);
      check_equality(sentry.add_details(description, "Inconsistent Swap (y)"), sentry.logger(), y, xClone);
      check_equality(sentry.add_details(description, "Inconsistent Swap (x)"), sentry.logger(), z, yClone);

      check_move_assign(description, sentry, actions, y, std::move(z), yClone, std::move(m), args...);
    }
    else
    {      
      check_move_assign(description, sentry, actions, z, std::move(y), yClone, std::move(m), args...);
    }
  }
}
