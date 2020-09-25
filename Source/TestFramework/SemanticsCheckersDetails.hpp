////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Implementation details for semantics checks that cleanly supports types which do/do not have allocators.

    The general pattern in this file is of paired function templates of the form

    <pre>
    template<test_mode Mode, class Actions, movable_comparable T, class... Args>
    ret_type do_check_foo(std::string_view, test_logger<Mode>& logger, const Actions&, const T&, const T&, const Args&...);

    template<test_mode Mode, class Actions, movable_comparable T>
    ret_type check_foo(std::string_view description, test_logger<Mode>& logger, const sentinel<Mode>& sentry, const Actions& actions, const T& x , const T&y)
    {
      return do_check_foo(description, logger, actions, x, y);
    }
    </pre>

    The idea is that the implementation of check_foo in this file is appropriate for types without allocators.
    As such, check_foo delegates to the instantiation of do_check_foo for which sizeof...(Args) is zero. However,
    do_check_foo is designed such that it can also deal with types which may allocate. As such, do_check_foo takes
    a template argument 'Actions'. Appropriate realizations of this type possess a bunch of constexpr static bools
    which indicate whether or not additional actions should be carried out.

    For example, when checking the semantics of a regular type, two const references to the type must be supplied.
    It is a precondition that these instances are not equal to one another. This is always checked. However, if
    the type allocates, clients may prefer to utilize the allocation-checking extension supplied with sequoia. In
    this case, there is a constexpr flag which indicates that, after checking the precondition, it is further checked
    that <pre>operator==</pre> hasn't unwittingly allocated - as can happen if it accidentally captures by value rather than reference.

    Thus, the general structure is

    <pre>
    some_common_check(...);
    if constexpr (Actions::some_flag)
    {
      do_some_extra_check(...);
    }
    </pre>

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
#include "Advice.hpp"

#include "Concepts.hpp"

#include <compare>
#include <optional>

namespace sequoia::testing::impl
{
  template<class...> struct do_swap;

  template<> struct do_swap<>
  {
    constexpr static bool value{true};
  };

  struct null_mutator
  {
    template<class T> constexpr void operator()(const T&) noexcept {}
  };
  
  template<test_mode Mode, class Actions, movable_comparable T, invocable_r<bool, T> Fn, class... Args>
  bool check_operator_consistency(test_logger<Mode>& logger, std::string_view op, const Actions& actions, const T& x, const T& y, Fn fn, const Args&... args)
  {
    if(!check(std::string{"operator"}.append(op).append(" is inconsistent"), logger, fn(x)))
      return false;

    if constexpr (Actions::has_post_comparison_action)
    {
      if(!actions.post_comparison_action(logger, op, x, y, args...))
        return false;
    }

    return true;
  }

  template<test_mode Mode, class Actions, orderable T, class... Args>
  [[nodiscard]]
  bool check_ordering_consistency(test_logger<Mode>& logger, const Actions& actions, const T& x, const T& y, const Args&... args)
  {
    if(!check_operator_consistency(logger, "<", actions, x, y, [](const T& x) { return !(x < x); }, args...))
        return false;

    if(!check_operator_consistency(logger, "<=", actions, x, y, [](const T& x) { return x <= x; }, args...))
      return false;

    if(!check_operator_consistency(logger, ">", actions, x, y, [](const T& x) { return !(x > x); }, args...))
      return false;

    if(!check_operator_consistency(logger, ">=", actions, x, y, [](const T& x) { return x >= x; }, args...))
      return false;

    if constexpr (three_way_comparable<T>)
    {
      if(!check_operator_consistency(logger, "<=>", actions, x, y, [](const T& x) { return (x <=> x) == 0; }, args...))
        return false;
    }
    
    auto comp{
      [&logger](const T& x, const T& y){
        sentinel sentry{logger, ""};

        check("operator> and operator< are inconsistent", logger, y > x);
        check("operator< and operator<= are inconsistent", logger, x <= y);
        check("operator< and operator>= are inconsistent", logger, y >= x);

        if constexpr (three_way_comparable<T>)
        {
          check("operator< and operator<=> are inconsistent", logger, (x <=> y) < 0);
        }

        return !sentry.failure_detected();
      }
    };

    return x < y ? comp(x,y) : comp(y,x);
  }

  template<test_mode Mode, class Actions, equality_comparable T, class... Args>
  [[nodiscard]]
  bool check_equality_preconditions(test_logger<Mode>& logger, const Actions& actions, const T& x, const T& y, const Args&... args)
  {
    if(!check_operator_consistency(logger, "==", actions, x, y, [](const T& x) { return x == x; }, args...))
      return false;

    if(!check_operator_consistency(logger, "!=", actions, x, y, [](const T& x) { return !(x != x); }, args...))
      return false;

    return check("Precondition - for checking semantics, x and y are assumed to be different", logger, x != y);      
  }

  template<test_mode Mode, class Actions, equality_comparable T, class... Args>
  [[nodiscard]]
  bool check_equality_preconditions(test_logger<Mode>& logger, const Actions& actions, const T& x, const T& y, const T& xClone, const T& yClone, const Args&... args)
  {
    if(!check_equality_preconditions(logger, actions, x, y, args...))
      return false;

    auto mess{
        [](std::string_view var){
          return std::string{"Precondition - for checking move-only semantics, "}
            .append(var).append(" and ").append(var).append("Clone are assumed to be equal");
        }
      };

      return check(mess("x"), logger, x == xClone) && check(mess("y"), logger, y == yClone);
  }

  template<test_mode Mode, class Actions, equality_comparable T, class... Args>
    requires orderable<T>
  [[nodiscard]]
  bool check_orderable_preconditions(test_logger<Mode>& logger, const Actions& actions, const T& x, const T& y, const Args&... args)
  {
    if(check_equality_preconditions(logger, actions, x, y, args...))
    {
      const auto order{actions.order()};
      if(check("Precondition - for checking semantics, order must be weak_ordering::less or weak_ordering::greater",
               logger, order != 0))
      {
        if(check_ordering_consistency(logger, actions, x, y, args...))
        {
          const bool cond{order < 0 ? x < y : x > y};
          auto mess{
            [order](){
              std::string mess{"Precondition - for ordered semantics, it is assumed that "};
              return order == 0 ? mess.append("x < y") : mess.append("y > x");
            }
          };

          return check(mess(), logger, cond,
                       tutor{[](const T& x, const T& y) {
                               return prediction_message(to_string(x), to_string(y)); } });
        }
      }
    }

    return false;
  }

  struct default_actions
  {
    constexpr static bool has_post_comparison_action{};
    constexpr static bool has_post_copy_action{};
    constexpr static bool has_post_copy_assign_action{};
    constexpr static bool has_post_move_action{};
    constexpr static bool has_post_move_assign_action{};
    constexpr static bool has_post_swap_action{};

    template<test_mode Mode, pseudoregular T>
    [[nodiscard]]
    bool check_preconditions(test_logger<Mode>& logger, const T& x, const T& y) const
    {
      return check_equality_preconditions(logger, *this, x, y);
    }

    template<test_mode Mode, moveonly T>
    [[nodiscard]]
    bool check_preconditions(test_logger<Mode>& logger, const T& x, const T& y, const T& xClone, const T& yClone) const
    {
      return check_equality_preconditions(logger, *this, x, y, xClone, yClone);
    }
  };

  struct orderable_actions
  {    
    constexpr static bool has_post_comparison_action{};
    constexpr static bool has_post_copy_action{};
    constexpr static bool has_post_copy_assign_action{};
    constexpr static bool has_post_move_action{};
    constexpr static bool has_post_move_assign_action{};
    constexpr static bool has_post_swap_action{};
    
    explicit orderable_actions(std::weak_ordering order)
      : m_Order{order}
    {}

    template<test_mode Mode, pseudoregular T>
      requires orderable<T>
    [[nodiscard]]
    bool check_preconditions(test_logger<Mode>& logger, const T& x, const T& y) const
    {
      return check_orderable_preconditions(logger, *this, x, y);
    }

    template<test_mode Mode, moveonly T>
      requires orderable<T>
    [[nodiscard]]
    bool check_preconditions(test_logger<Mode>& logger, const T& x, const T& y, const T& xClone, const T& yClone) const
    {
      return check_orderable_preconditions(logger, *this, x, y, xClone, yClone);
    }

    [[nodiscard]]
    std::weak_ordering order() const noexcept
    {
      return m_Order;
    }
  private:
    std::weak_ordering m_Order;
  };

  //================================ move assign ================================//

  template<test_mode Mode, class Actions, movable_comparable T, invocable<T&> Mutator, class... Args>
  void do_check_move_assign(test_logger<Mode>& logger, const Actions& actions, T& z, T&& y, const T& yClone, Mutator&& yMutator, const Args&... args)
  {
    z = std::move(y);
    if(!check_equality("Inconsistent move assignment (from y)", logger, z, yClone))
       return;

    if constexpr(Actions::has_post_move_assign_action)
    {
      actions.post_move_assign_action(logger, z, yClone, std::move(yMutator), args...);
    }
  }
  
  template<test_mode Mode, class Actions, movable_comparable T, invocable<T&> Mutator>
  void check_move_assign(test_logger<Mode>& logger, const Actions& actions, T& z, T&& y, const T& yClone, Mutator m)
  {
    do_check_move_assign(logger, actions, z, std::forward<T>(y), yClone, std::move(m));
  }

  //================================ swap ================================//

  template<test_mode Mode, class Actions, movable_comparable T, class... Args>
  bool do_check_swap(test_logger<Mode>& logger, const Actions& actions, T&& x, T&& y, const T& xClone, const T& yClone, const Args&... args)
  {
    using std::swap;
    swap(x, y);

    const bool swapy{
      check_equality("Inconsistent Swap (y)", logger, y, xClone)
    };
    
    const bool swapx{
      check_equality("Inconsistent Swap (x)", logger, x, yClone)
    };
      
    if constexpr(Actions::has_post_swap_action)
    {
      if(swapx && swapy)
      {
        actions.post_swap_action(logger, x, y, yClone, args...);
      }
    }

    return swapx && swapy;
  }

  template<test_mode Mode, class Actions, movable_comparable T>
  bool check_swap(test_logger<Mode>& logger, const Actions& actions, T&& x, T&& y, const T& xClone, const T& yClone)
  {
    return do_check_swap(logger, actions, std::move(x), std::move(y), xClone, yClone);
  }

  //================================  move construction ================================ //

  template<test_mode Mode, class Actions, movable_comparable T, class... Args>
  std::optional<T> do_check_move_construction(test_logger<Mode>& logger, const Actions& actions, T&& z, const T& y, const Args&... args)
  {
    T w{std::move(z)};
    if(!check_equality("Inconsistent move construction", logger, w, y))
      return {};

    if constexpr(Actions::has_post_move_action)
    {
      actions.post_move_action(logger, w, args...);
    }

    return w;
  }

  template<test_mode Mode, class Actions, movable_comparable T>
  std::optional<T> check_move_construction(test_logger<Mode>& logger, const Actions& actions, T&& z, const T& y)
  {
    return do_check_move_construction(logger, actions, std::forward<T>(z), y);
  }
}
