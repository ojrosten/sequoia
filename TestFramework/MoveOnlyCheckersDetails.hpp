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

namespace sequoia::unit_testing::impl
{
  template<test_mode Mode, class Actions, class T, class Mutator, class... Args>
  void check_semantics(std::string_view description, test_logger<Mode>& logger, const Actions& actions, T&& x, T&& y, const T& xClone, const T& yClone, Mutator m, const Args&... args)
  {
    typename test_logger<Mode>::sentinel s{logger, add_type_info<T>(description)};

    // Preconditions
    if(!check_preconditions(description, logger, actions, x, y, args...))
      return;

    if(!check(combine_messages(description, "Precondition - for checking regular semantics, x and xClone are assumed to be equal"), logger, x == xClone)) return;

    if(!check(combine_messages(description, "Precondition - for checking regular semantics, y and yClone are assumed to be equal"), logger, y == yClone)) return;

    T z{check_move_construction(description, logger, actions, std::move(x), xClone, args...)};    

    if constexpr (do_swap<Args...>::value)
    {
      using std::swap;
      swap(z, y);
      check_equality(combine_messages(description, "Swap"), logger, y, xClone);
      check_equality(combine_messages(description, "Swap"), logger, z, yClone);

      check_move_assign(description, logger, actions, y, std::move(z), yClone, m, args...);
    }
    else
    {      
      check_move_assign(description, logger, actions, z, std::move(y), yClone, m, args...);
    }
  }
}
