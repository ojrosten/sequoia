////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Implementation details for semantics checks within the unit testing framework.
*/


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
    template<test_mode Mode, class T, class... Allocators>
    static bool check_preconditions(std::string_view description, test_logger<Mode>& logger, const T& x, const T& y)
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
 
  template<test_mode Mode, class Actions, class T, class... Args>
  bool do_check_preconditions(std::string_view description, test_logger<Mode>& logger, const Actions& actions, const T& x, const T& y, const Args&... args)
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

  template<test_mode Mode, class Actions, class T>
  bool check_preconditions(std::string_view description, test_logger<Mode>& logger, const Actions& actions, const T& x, const T& y)
  {
    return do_check_preconditions(description, logger, actions, x, y);
  }


  template<test_mode Mode, class Actions, class T, class Mutator, class... Args>
  void do_check_move_assign(std::string_view description, test_logger<Mode>& logger, const Actions& actions, T& z, T&& y, const T& yClone, Mutator yMutator, const Args&... args)
  {
    z = std::move(y);
    check_equality(combine_messages(description, "Move assignment (from y)"), logger, z, yClone);

    if constexpr(Actions::has_post_move_assign_action)
    {
      actions.post_move_assign_action(description, logger, z, yClone, yMutator, args...);
    }
  }
  
  template<test_mode Mode, class Actions, class T, class Mutator>
  void check_move_assign(std::string_view description, test_logger<Mode>& logger, const Actions& actions, T& z, T&& y, const T& yClone, Mutator m)
  {
    do_check_move_assign(description, logger, actions, z, std::forward<T>(y), yClone, m);
  }

  template<test_mode Mode, class Actions, class T, class... Args>
  void do_check_move_construction(std::string_view description, test_logger<Mode>& logger, const Actions& actions, T&& z, const T& y, const Args&... args)
  {
    const T w{std::move(z)};
    check_equality(combine_messages(description, "Move constructor"), logger, w, y);

    if constexpr(Actions::has_post_move_action)
    {
      actions.post_move_action(description, logger, w, args...);
    }
  }

  template<test_mode Mode, class Actions, class T, class Mutator>
  void check_swap(std::string_view description, test_logger<Mode>& logger, const Actions& actions, T&& x, T& y, const T& xClone, const T& yClone, Mutator yMutator)
  {
    do_check_swap(description, logger, actions, std::forward<T>(x), y, xClone, yClone, std::move(yMutator));
  }

  template<test_mode Mode, class Actions, class T, class Mutator, class... Args>
  void do_check_swap(std::string_view description, test_logger<Mode>& logger, const Actions& actions, T&& x, T& y, const T& xClone, const T& yClone, Mutator yMutator, const Args&... args)
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

  template<test_mode Mode, class Actions, class T>
  void check_move_construction(std::string_view description, test_logger<Mode>& logger, const Actions& actions, T&& z, const T& y)
  {
    do_check_move_construction(description, logger, actions, std::forward<T>(z), y);
  }
}
