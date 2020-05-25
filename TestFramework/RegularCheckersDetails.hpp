////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Implementation details for checking regular semantics.
 
 */

#include "SemanticsCheckersDetails.hpp"

namespace sequoia::testing::impl
{
  template<test_mode Mode, class Actions, class T, class... Args>
  void do_check_copy_assign(std::string_view description, test_logger<Mode>& logger, const Actions& actions, T& z, const T& y, const Args&... args)
  {
    z = y;
    check_equality(combine_messages(description, "Copy assignment (from y)"), logger, z, y);

    if constexpr(Actions::has_post_copy_assign_action)
    {
      actions.post_copy_assign_action(description, logger, z, y, args...);
    }
  }
  
  template<test_mode Mode, class Actions, class T>
  void check_copy_assign(std::string_view description, test_logger<Mode>& logger, const Actions& actions, T& z, const T& y)
  {
    do_check_copy_assign(description, logger, actions, z, y);   
  }

  template<test_mode Mode, class Actions, class T, class Mutator, class... Args>
  bool check_semantics(std::string_view description, test_logger<Mode>& logger, const Actions& actions, const T& x, const T& y, Mutator yMutator, const Args&... args)
  {        
    typename test_logger<Mode>::sentinel s{logger, add_type_info<T>(description)};
    
    // Preconditions
    if(!check_preconditions(description, logger, actions, x, y, args...))
      return false;
        
    T z{x};
    if constexpr(Actions::has_post_copy_action)
    {
      actions.post_copy_action(description, logger, z, T{y}, args...);
    }
    check_equality(combine_messages(description, "Copy constructor (x)"), logger, z, x);
    check(combine_messages(description, "Equality operator"), logger, z == x);

    // z = y
    check_copy_assign(description, logger, actions, z, y, args...);
    check(combine_messages(description, "Inequality operator"), logger, z != x);

    check_move_construction(description, logger, actions, std::move(z), y, args...);

    T w{x};    
    check_move_assign(description, logger, actions, w, T{y}, y, yMutator, args...);

    if constexpr (do_swap<Args...>::value)
    {
      T v{y};
      check_swap(description, logger, actions, T{x}, v, x, y, yMutator, args...);
    }

    if constexpr(!std::is_same_v<std::decay_t<Mutator>, null_mutator>)
    {
      T v{y};
      yMutator(v);

      check(combine_messages(description, "Mutation is not doing anything following copy constrution/broken value semantics"), logger, v != y);

      v = y;
      check_equality(combine_messages(description, "Copy assignment"), logger, v, y);

      yMutator(v);

      check(combine_messages(description, "Mutation is not doing anything following copy assignment/ broken value semantics"), logger, v != y);
    }

    return true;
  }
}
