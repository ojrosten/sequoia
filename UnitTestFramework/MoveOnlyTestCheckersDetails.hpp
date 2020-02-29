////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file MoveOnlyTestCheckersDetails.hpp
    \brief Utilities for performing checks within the unit testing framework.
*/

#include "UnitTestCheckersDetails.hpp"

namespace sequoia::unit_testing::impl
{
  template<class Logger, class Actions, class T, class... Args>
  bool check_regular_semantics(std::string_view description, Logger& logger, const Actions& actions, T&& x, T&& y, const T& xClone, const T& yClone, const Args&... args)
  {
    typename Logger::sentinel s{logger, add_type_info<T>(description)};

    // Preconditions
    if(!check_preconditions(description, logger, actions, x, y, args...))
      return false;

    if(!check(combine_messages(description, "Precondition - for checking regular semantics, x and xClone are assumed to be equal"), logger, x == xClone)) return false;

    if(!check(combine_messages(description, "Precondition - for checking regular semantics, y and yClone are assumed to be equal"), logger, y == yClone)) return false;

    T z{std::move(x)};
    if constexpr(Actions::has_post_move_action)
    {
      actions.post_move_action(description, logger, z, args...);
    }
    check_equality(combine_messages(description, "Move constructor"), logger, z, xClone);

    // x now moved-from so assumption about allocs for x = std::move(y) may be wrong...
    check_move_assign(description, logger, actions, x, std::move(y), yClone, args...);
    check_equality(combine_messages(description, "Move assignment"), logger, x, yClone);

    if constexpr (do_swap<Args...>::value)
    {
      using std::swap;
      swap(z, x);
      check_equality(combine_messages(description, "Swap"), logger, x, xClone);
      check_equality(combine_messages(description, "Swap"), logger, z, yClone);

      y = std::move(z);
    }
    else
    {
      y = std::move(x);
    }

    return check_equality(combine_messages(description, "Post condition: y restored"), logger, y, yClone);
  }
}
