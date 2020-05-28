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
    check_equality(merge(description, "Inconsistent copy assignment (from y)", "\n"), logger, z, y);

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
    check_equality(merge(description, "Inconsistent copy constructor (x)", "\n"), logger, z, x);

    // z = y
    check_copy_assign(description, logger, actions, z, y, args...);

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

      check(merge(description, "Either mutation is not doing anything following copy constrution or value semantics are broken, with mutation of an object also changing the object from which it was copied", "\n"), logger, v != y);

      v = y;
      check_equality(merge(description, "Inconsistent copy assignment (from mutated y)", "\n"), logger, v, y);

      yMutator(v);

      check(merge(description, "Mutation is not doing anything following copy assignment/ broken value semantics", "\n"), logger, v != y);
    }

    return true;
  }
}
