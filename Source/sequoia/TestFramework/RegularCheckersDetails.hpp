////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Implementation details for checking regular semantics.

 */

#include "sequoia/TestFramework/SemanticsCheckersDetails.hpp"

namespace sequoia::testing::impl
{
  template<class Actions, class... Args>
  inline constexpr bool has_post_copy_action
    = requires (std::remove_cvref_t<Actions> actions, std::remove_cvref_t<Args>&... args) {
        actions.post_copy_action(args...);
      };

  template<class Actions, class... Args>
  inline constexpr bool has_post_copy_assign_action
    = requires (std::remove_cvref_t<Actions> actions, std::remove_cvref_t<Args>&... args) {
        actions.post_copy_assign_action(args...);
      };
  
  template<test_mode Mode, class Actions, pseudoregular T, class... Args>
  bool do_check_copy_assign(test_logger<Mode>& logger, [[maybe_unused]] const Actions& actions, T& z, const T& y, const Args&... args)
  {
    z = y;
    if(check(equality, "Inconsistent copy assignment (from y)", logger, z, y))
    {
      if constexpr(has_post_copy_assign_action<Actions, test_logger<Mode>, T, T, Args...>)
      {
        actions.post_copy_assign_action(logger, z, y, args...);
      }

      auto& w{z};
      z = w;
      return check(equality, "Inconsistent self copy assignment", logger, z, y);
    }

    return false;
  }

  template<test_mode Mode, class Actions, pseudoregular T>
  bool check_copy_assign(test_logger<Mode>& logger, const Actions& actions, T& z, const T& y)
  {
    return do_check_copy_assign(logger, actions, z, y);
  }

  template<test_mode Mode, class Actions, pseudoregular T, class U, std::invocable<T&> Mutator, class... Args>
  bool check_semantics(test_logger<Mode>& logger,
                       const Actions& actions,
                       const T& x,
                       const T& y,
                       optional_ref<const U> movedFromPostConstruction,
                       optional_ref<const U> movedFromPostAssignment,                       
                       Mutator yMutator,
                       const Args&... args)
  {
    sentinel<Mode> sentry{logger, ""};

    if(!check_prerequisites(logger, actions, x, y, args...))
      return false;

    T z{x};
    const bool consistentCopy{check(equality, "Inconsistent copy constructor (x)", logger, z, x)};

    if constexpr(has_post_copy_action<Actions, test_logger<Mode>, T, T, Args...>)
    {
      if(consistentCopy)
      {
        actions.post_copy_action(logger, z, T{y}, args...);
      }
    }

    const bool consistentCopyAssign{check_copy_assign(logger, actions, z, y, args...)};
    // z == y, if copy assign is consistent, even if copy construction is not

    if(consistentCopyAssign)
    {
      check_move_construction(logger, actions, std::move(z), y, movedFromPostConstruction, args...);
    }

    if(!consistentCopy)
      return false;

    T w{x};
    check_move_assign(logger, actions, w, T{y}, y, movedFromPostAssignment, yMutator, args...);

    if constexpr (do_swap<Args...>::value)
    {
      check_swap(logger, actions, T{x}, T{y}, x, y, yMutator, args...);
    }

    if constexpr(!std::is_same_v<std::remove_cvref_t<Mutator>, null_mutator>)
    {
      if(consistentCopy && consistentCopyAssign)
      {
        T v{y};
        yMutator(v);

        if(check(
          "Either mutation is not doing anything following copy construction"
          " or value semantics are broken, with mutation of an object also changing"
          " the object from which it was copied", logger, v != y))
        {
          v = y;
          if(check(equality, "Inconsistent copy assignment (from mutated y)", logger, v, y))
          {
            yMutator(v);

            check(
              "Either mutation is not doing anything following copy assignment"
              " or value semantics are broken, with mutation of an object also changing"
              " the object from which it was assigned", logger, v != y);
          }
        }
      }
    }

    if constexpr (serializable_to<T, std::stringstream> && deserializable_from<T, std::stringstream>)
    {
      check_serialization(logger, actions, T{x}, y, args...);
    }

    return !sentry.failure_detected();
  }
}
