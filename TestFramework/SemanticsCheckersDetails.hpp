////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Implementation details for semantics checks that cleanly supports types which do/do not have allocators.

    The general pattern in this file is of paired function templates of the form

    template<test_mode Mode, class Actions, class T, class... Args>
    ret_type do_check_foo(std::string_view, sentinel<Mode>&, const Actions&, const T&, const T&, const Args&...);

    template<test_mode Mode, class Actions, class T>
    ret_type check_foo(std::string_view description, sentinel<Mode>& sentry, const Actions& actions, const T& x , const T&y)
    {
      return do_check_foo(description, sentry, actions, x, y);
    }

    The idea is that the implementation of check_foo in this file is appropriate for types without allocators.
    As such, check_foo delegates to the instantiation of do_check_foo for which sizeof...(Args) is zero. However,
    do_check_foo is designed such that it can also deal with types which may allocate. As such, do_check_foo takes
    a template argument 'Actions'. Appropriate realizations of this type possess a bunch of constexpr static bools
    which indicate whether or not additional actions should be carried out.

    For example, when checking the semantics of a regular type, two const references to the type must be supplied.
    It is a precondition that these instances are not equal to one another. This is always checked. However, if
    the type allocates, clients may prefer to utilize the allocation-checking extension supplied with sequoia. In
    this case, there is a constexpr flag which indicates that, after checking the precondition, it is further checked
    that operator== hasn't unwittingly allocated - as can happen if it accidentally captures by value rather than
    reference.

    Thus, the general structure is

    some_common_check(...);
    if constexpr (Actions::some_flag)
    {
      do_some_extra_check(...);
    }

    where the extra check is fed, amongst other things, the parameter pack args...

    Therefore when checking the semantics of an allocating type, the skeleton of the function is the same as for
    non-allocating types. However, in the former case extra checks are folded in. This pattern ensures
    consistency: if the scheme is tweaked for non-allocating types, allocating types will automatically benefit.
    But there is enough flexbility to allow for all the extra allocation checks that allocating types require.

    As such, the various check_foo functions in this header have overloads in AllocationCheckersDetails.hpp which
    supply the extra argument(s) for the additional actions that comprise allocation checking. 

    Note that check_foo / do_check_foo pairs in this file are common to both regular and move-only types.
    However, regular types additionally have copy semantics; the extra pieces necessary for this may be found
    in RegularCheckersDetails.hpp and RegularAllocationCheckerDetails.hpp
*/

#include "TestLogger.hpp"

namespace sequoia::testing::impl
{
  template<class...> struct do_swap;

  template<> struct do_swap<>
  {
    constexpr static bool value{true};
  };

  struct null_mutator
  {
    template<class T> constexpr void operator()(const T&) noexcept {};
  };

  struct pre_condition_actions
  {
    template<test_mode Mode, class T, class... Allocators>
    static bool check_preconditions(std::string_view description, sentinel<Mode>& sentry, const T& x, const T& y)
    {
      return check(sentry.add_details(description, "Precondition - for checking regular semantics, x and y are assumed to be different"), sentry.logger(), x != y);
    }
  };
  
  struct default_actions : pre_condition_actions
  {
    constexpr static bool has_post_equality_action{};
    constexpr static bool has_post_nequality_action{};
    constexpr static bool has_post_copy_action{};
    constexpr static bool has_post_copy_assign_action{};
    constexpr static bool has_post_move_action{};
    constexpr static bool has_post_move_assign_action{};
    constexpr static bool has_post_swap_action{};
  };

  //================================ preconditions ================================//
 
  template<test_mode Mode, class Actions, class T, class... Args>
  bool do_check_preconditions(std::string_view description, sentinel<Mode>& sentry, const Actions& actions, const T& x, const T& y, const Args&... args)
  {
    if(!check(sentry.add_details(description, "Equality operator is inconsistent"), sentry.logger(), x == x))
      return false;

    if constexpr (Actions::has_post_equality_action)
    {
      actions.post_equality_action(description, sentry, x, y, args...);
    }
    
    if(!check(sentry.add_details(description, "Inequality operator is inconsistent"), sentry.logger(), !(x != x)))
      return false;

    if constexpr (Actions::has_post_nequality_action)
    {
      actions.post_nequality_action(description, sentry, x, y, args...);
    }

    return actions.check_preconditions(description, sentry, x, y);
  }

  template<test_mode Mode, class Actions, class T>
  bool check_preconditions(std::string_view description, sentinel<Mode>& sentry, const Actions& actions, const T& x, const T& y)
  {
    return do_check_preconditions(description, sentry, actions, x, y);
  }

  //================================ move assign ================================//

  template<test_mode Mode, class Actions, class T, class Mutator, class... Args>
  void do_check_move_assign(std::string_view description, sentinel<Mode>& sentry, const Actions& actions, T& z, T&& y, const T& yClone, Mutator yMutator, const Args&... args)
  {
    z = std::move(y);
    check_equality(sentry.add_details(description, "Inconsistent move assignment (from y)"), sentry.logger(), z, yClone);

    if constexpr(Actions::has_post_move_assign_action)
    {
      actions.post_move_assign_action(description, sentry, z, yClone, std::move(yMutator), args...);
    }
  }
  
  template<test_mode Mode, class Actions, class T, class Mutator>
  void check_move_assign(std::string_view description, sentinel<Mode>& sentry, const Actions& actions, T& z, T&& y, const T& yClone, Mutator m)
  {
    do_check_move_assign(description, sentry, actions, z, std::forward<T>(y), yClone, std::move(m));
  }

  //================================ swap ================================//

  template<test_mode Mode, class Actions, class T, class... Args>
  void do_check_swap(std::string_view description, sentinel<Mode>& sentry, const Actions& actions, T&& x, T& y, const T& xClone, const T& yClone, const Args&... args)
  {
    using std::swap;
    swap(x, y);

    check_equality(sentry.add_details(description, "Inconsistent Swap (y)"), sentry.logger(), y, xClone);
    check_equality(sentry.add_details(description, "Inconsistent Swap (x)"), sentry.logger(), x, yClone);
    
    if constexpr(Actions::has_post_swap_action)
    {
      actions.post_swap_action(description, sentry, x, y, yClone, args...);
    }
  }

  template<test_mode Mode, class Actions, class T>
  void check_swap(std::string_view description, sentinel<Mode>& sentry, const Actions& actions, T&& x, T& y, const T& xClone, const T& yClone)
  {
    do_check_swap(description, sentry, actions, std::forward<T>(x), y, xClone, yClone);
  }

  //================================  move construction ================================ //

  template<test_mode Mode, class Actions, class T, class... Args>
  T do_check_move_construction(std::string_view description, sentinel<Mode>& sentry, const Actions& actions, T&& z, const T& y, const Args&... args)
  {
    T w{std::move(z)};
    check_equality(sentry.add_details(description, "Inconsistent move construction"), sentry.logger(), w, y);

    if constexpr(Actions::has_post_move_action)
    {
      actions.post_move_action(description, sentry, w, args...);
    }

    return w;
  }

  template<test_mode Mode, class Actions, class T>
  T check_move_construction(std::string_view description, sentinel<Mode>& sentry, const Actions& actions, T&& z, const T& y)
  {
    return do_check_move_construction(description, sentry, actions, std::forward<T>(z), y);
  }
}
