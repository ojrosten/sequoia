////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file UnitTestCheckersDetails.hpp
    \brief Implementation details for performing checks within the unit testing framework.
*/

#include "FreeTestCheckersDetails.hpp"

namespace sequoia::unit_testing::impl
{
  template<class...> struct do_swap;

  template<> struct do_swap<>
  {
    constexpr static bool value{true};
  };

  struct null_mutator
  {
    template<class T> constexpr void operator()(T&) noexcept {};
  };

  struct pre_condition_actions
  {
    template<class Logger, class T, class... Allocators>
    static bool check_preconditions(std::string_view description, Logger& logger, const T& x, const T& y)
    {
      return check(combine_messages(description, "Precondition - for checking regular semantics, x and y are assumed to be different"), logger, x != y);
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
 
  template<class Logger, class Actions, class T, class... Args>
  bool do_check_preconditions(std::string_view description, Logger& logger, const Actions& actions, const T& x, const T& y, const Args&... args)
  {
    if(!check(combine_messages(description, "Equality operator is inconsistent"), logger, x == x))
      return false;

    if constexpr (Actions::has_post_equality_action)
    {
      actions.post_equality_action(combine_messages(description, "no allocation for operator=="), logger, x, y, args...);
    }
    
    if(!check(combine_messages(description, "Inequality operator is inconsistent"), logger, !(x != x)))
      return false;

    if constexpr (Actions::has_post_equality_action)
    {
      actions.post_nequality_action(combine_messages(description, "no allocation for operator!="), logger, x, y, args...);
    }

    return actions.check_preconditions(description, logger, x, y);
  }

  template<class Logger, class Actions, class T>
  bool check_preconditions(std::string_view description, Logger& logger, const Actions& actions, const T& x, const T& y)
  {
    return do_check_preconditions(description, logger, actions, x, y);
  }


  template<class Logger, class Actions, class T, class... Args>
  void do_check_copy_assign(std::string_view description, Logger& logger, const Actions& actions, T& z, const T& y, const Args&... args)
  {
    z = y;
    check_equality(combine_messages(description, "Copy assignment (from y)"), logger, z, y);

    if constexpr(Actions::has_post_copy_assign_action)
    {
      actions.post_copy_assign_action(description, logger, z, y, args...);
    }
  }
  
  template<class Logger, class Actions, class T>
  void check_copy_assign(std::string_view description, Logger& logger, const Actions& actions, T& z, const T& y)
  {
    do_check_copy_assign(description, logger, actions, z, y);   
  }

  template<class Logger, class Actions, class T, class Mutator, class... Args>
  void do_check_move_assign(std::string_view description, Logger& logger, const Actions& actions, T& z, T&& y, const T& yClone, Mutator yMutator, const Args&... args)
  {
    z = std::move(y);
    check_equality(combine_messages(description, "Move assignment (from y)"), logger, z, yClone);

    if constexpr(Actions::has_post_move_assign_action)
    {
      actions.post_move_assign_action(description, logger, z, yClone, yMutator, args...);
    }
  }
  
  template<class Logger, class Actions, class T, class Mutator>
  void check_move_assign(std::string_view description, Logger& logger, const Actions& actions, T& z, T&& y, const T& yClone, Mutator m)
  {
    do_check_move_assign(description, logger, actions, z, std::forward<T>(y), yClone, m);
  }

  template<class Logger, class Actions, class T, class... Args>
  void do_check_move_construction(std::string_view description, Logger& logger, const Actions& actions, T&& z, const T& y, const Args&... args)
  {
    const T w{std::move(z)};
    check_equality(combine_messages(description, "Move constructor"), logger, w, y);

    if constexpr(Actions::has_post_move_action)
    {
      actions.post_move_action(description, logger, w, args...);
    }
  }

  template<class Logger, class Actions, class T, class Mutator>
  void check_swap(std::string_view description, Logger& logger, const Actions& actions, T&& x, T& y, const T& xClone, const T& yClone, Mutator yMutator)
  {
    do_check_swap(description, logger, actions, std::forward<T>(x), y, xClone, yClone, std::move(yMutator));
  }

  template<class Logger, class Actions, class T, class Mutator, class... Args>
  void do_check_swap(std::string_view description, Logger& logger, const Actions& actions, T&& x, T& y, const T& xClone, const T& yClone, Mutator yMutator, const Args&... args)
  {
    using std::swap;
    swap(x, y);

    check_equality(combine_messages(description, "Swap"), logger, y, xClone);
    check_equality(combine_messages(description, "Swap"), logger, x, yClone);
    
    if constexpr(Actions::has_post_swap_action)
    {
      actions.post_swap_action(description, logger, x, y, yClone, yMutator, args...);
    }
  }

  template<class Logger, class Actions, class T>
  void check_move_construction(std::string_view description, Logger& logger, const Actions& actions, T&& z, const T& y)
  {
    do_check_move_construction(description, logger, actions, std::forward<T>(z), y);
  }
  
  template<class Logger, class Actions, class T, class Mutator, class... Args>
  bool check_semantics(std::string_view description, Logger& logger, const Actions& actions, const T& x, const T& y, Mutator yMutator, const Args&... args)
  {        
    typename Logger::sentinel s{logger, add_type_info<T>(description)};
    
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

    // move construction
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
