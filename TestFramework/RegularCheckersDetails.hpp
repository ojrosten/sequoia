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
  bool do_check_copy_assign(std::string_view description, sentinel<Mode>& sentry, const Actions& actions, T& z, const T& y, const Args&... args)
  {
    z = y;
    const bool consistent{
       check_equality(sentry.add_details(description, "Inconsistent copy assignment (from y)"), sentry.logger(), z, y)
    };

    if constexpr(Actions::has_post_copy_assign_action)
    {
      if(consistent)
      {
        actions.post_copy_assign_action(description, sentry, z, y, args...);
      }
    }

    return consistent;
  }
  
  template<test_mode Mode, class Actions, class T>
  bool check_copy_assign(std::string_view description, sentinel<Mode>& sentry, const Actions& actions, T& z, const T& y)
  {
    return do_check_copy_assign(description, sentry, actions, z, y);   
  }
  
  template<test_mode Mode, class Actions, class T, class Mutator, std::enable_if_t<std::is_invocable_v<Mutator, T&>, int> = 0>
  bool check_swap(std::string_view description, sentinel<Mode>& sentry, const Actions& actions, T&& x, T&& y, const T& xClone, const T& yClone, Mutator yMutator)
  {
    return do_check_swap(description, sentry, actions, std::move(x), std::move(y), xClone, yClone, std::move(yMutator));
  }

  template<test_mode Mode, class Actions, class T, class Mutator, class... Args>
  bool check_semantics(std::string_view description, sentinel<Mode>& sentry, const Actions& actions, const T& x, const T& y, Mutator yMutator, const Args&... args)
  {    
    // Preconditions
    if(!check_preconditions(description, sentry, actions, x, y, args...))
      return false;
        
    T z{x};
    const bool consistentCopy{
      check_equality(sentry.add_details(description, "Inconsistent copy constructor (x)"),
                     sentry.logger(), z, x)
    };

    if constexpr(Actions::has_post_copy_action)
    {
      if(consistentCopy)
      {
        actions.post_copy_action(description, sentry, z, T{y}, args...);
      }
    }

    const bool consistentCopyAssign{check_copy_assign(description, sentry, actions, z, y, args...)};
    // z == y, if copy assign is consistent, even if copy construction is not

    if(consistentCopyAssign)
    {
      check_move_construction(description, sentry, actions, std::move(z), y, args...);
    }

    if(!consistentCopy)
      return false;

    T w{x};
    check_move_assign(description, sentry, actions, w, T{y}, y, yMutator, args...);

    if constexpr (do_swap<Args...>::value)
    {
      if (consistentCopy)
      {
        check_swap(description, sentry, actions, T{x}, T{y}, x, y, yMutator, args...);
      }
    }

    if constexpr(!std::is_same_v<std::decay_t<Mutator>, null_mutator>)
    {
      if(consistentCopy && consistentCopyAssign)
      {
        T v{y};
        yMutator(v);

        check(sentry.add_details(description, "Either mutation is not doing anything following copy construction or value semantics are broken, with mutation of an object also changing the object from which it was copied"), sentry.logger(), v != y);

        v = y;
        check_equality(sentry.add_details(description, "Inconsistent copy assignment (from mutated y)"), sentry.logger(), v, y);

        yMutator(v);

        check(sentry.add_details(description, "Mutation is not doing anything following copy assignment/ broken value semantics"), sentry.logger(), v != y);
      }
    }

    return !sentry.failure_detected();
  }
}
