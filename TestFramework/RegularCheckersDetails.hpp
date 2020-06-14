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
  void do_check_copy_assign(std::string_view description, sentinel<Mode>& sentry, const Actions& actions, T& z, const T& y, const Args&... args)
  {
    z = y;
    check_equality(sentry.add_details(description, "Inconsistent copy assignment (from y)"), sentry.logger(), z, y);

    if constexpr(Actions::has_post_copy_assign_action)
    {
      actions.post_copy_assign_action(description, sentry, z, y, args...);
    }
  }
  
  template<test_mode Mode, class Actions, class T>
  void check_copy_assign(std::string_view description, sentinel<Mode>& sentry, const Actions& actions, T& z, const T& y)
  {
    do_check_copy_assign(description, sentry, actions, z, y);   
  }
  
  template<test_mode Mode, class Actions, class T, class Mutator, std::enable_if_t<std::is_invocable_v<Mutator, T&>, int> = 0>
  void check_swap(std::string_view description, sentinel<Mode>& sentry, const Actions& actions, T&& x, T& y, const T& xClone, const T& yClone, Mutator yMutator)
  {
    do_check_swap(description, sentry, actions, std::forward<T>(x), y, xClone, yClone, std::move(yMutator));
  }

  template<test_mode Mode, class Actions, class T, class Mutator, class... Args>
  bool check_semantics(std::string_view description, sentinel<Mode>& sentry, const Actions& actions, const T& x, const T& y, Mutator yMutator, const Args&... args)
  {    
    // Preconditions
    if(!check_preconditions(description, sentry, actions, x, y, args...))
      return false;
        
    T z{x};
    if constexpr(Actions::has_post_copy_action)
    {
      actions.post_copy_action(description, sentry, z, T{y}, args...);
    }
    check_equality(sentry.add_details(description, "Inconsistent copy constructor (x)"), sentry.logger(), z, x);

    // z = y
    check_copy_assign(description, sentry, actions, z, y, args...);

    check_move_construction(description, sentry, actions, std::move(z), y, args...);

    T w{x};    
    check_move_assign(description, sentry, actions, w, T{y}, y, yMutator, args...);

    if constexpr (do_swap<Args...>::value)
    {
      T v{y};
      check_swap(description, sentry, actions, T{x}, v, x, y, yMutator, args...);
    }

    if constexpr(!std::is_same_v<std::decay_t<Mutator>, null_mutator>)
    {
      T v{y};
      yMutator(v);

      check(sentry.add_details(description, "Either mutation is not doing anything following copy construction or value semantics are broken, with mutation of an object also changing the object from which it was copied"), sentry.logger(), v != y);

      v = y;
      check_equality(sentry.add_details(description, "Inconsistent copy assignment (from mutated y)"), sentry.logger(), v, y);

      yMutator(v);

      check(sentry.add_details(description, "Mutation is not doing anything following copy assignment/ broken value semantics"), sentry.logger(), v != y);
    }

    return true;
  }
}
