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

#include "sequoia/TestFramework/TestLogger.hpp"
#include "sequoia/TestFramework/Advice.hpp"
#include "sequoia/TestFramework/FreeCheckers.hpp"

#include "sequoia/Core/Meta/Concepts.hpp"

#include <compare>
#include <format>
#include <optional>

namespace sequoia::testing
{
  enum class comparison_flavour { equality, inequality, less_than, greater_than, leq, geq, threeway };

  template<comparison_flavour C>
  using comparison_constant = std::integral_constant<comparison_flavour, C>;

  using equality_type     = comparison_constant<comparison_flavour::equality>;
  using inequality_type   = comparison_constant<comparison_flavour::inequality>;
  using less_than_type    = comparison_constant<comparison_flavour::less_than>;
  using greater_than_type = comparison_constant<comparison_flavour::greater_than>;
  using leq_type          = comparison_constant<comparison_flavour::leq>;
  using geq_type          = comparison_constant<comparison_flavour::geq>;
  using threeway_type     = comparison_constant<comparison_flavour::threeway>;

  [[nodiscard]]
  std::string to_string(comparison_flavour f);

  template<class T>
  using opt_moved_from_ref = std::optional<std::reference_wrapper<const T>>;
}

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

  template<class T>
  struct auxiliary_data_policy;

  template<std::equality_comparable T>
  struct auxiliary_data_policy<T>
  {
  protected:
    ~auxiliary_data_policy() = default;
  };

  template<std::totally_ordered T>
  struct auxiliary_data_policy<T>
  {
    constexpr explicit auxiliary_data_policy(std::weak_ordering order)
      : m_Order{order}
    {}

    [[nodiscard]]
    std::weak_ordering order() const noexcept
    {
      return m_Order;
    }
  protected:
    ~auxiliary_data_policy() = default;
  private:
    std::weak_ordering m_Order;
  };

  template<class T>
  struct auxiliary_data : auxiliary_data_policy<T>
  {
    using auxiliary_data_policy<T>::auxiliary_data_policy;
  };
  
  template<class Actions, class... Args>
  inline constexpr bool has_post_comparison_action
    = requires (std::remove_cvref_t<Actions> actions, std::remove_cvref_t<Args>&... args) {
        actions.post_comparison_action(args...);
      };

  template<class Actions, class... Args>
  inline constexpr bool has_post_move_action
   = requires (std::remove_cvref_t<Actions> actions, std::remove_cvref_t<Args>&... args) {
        actions.post_move_action(args...);
      };

  template<class Actions, class... Args>
  inline constexpr bool has_post_move_assign_action
    = requires (std::remove_cvref_t<Actions> actions, std::remove_cvref_t<Args>&... args) {
        actions.post_move_assign_action(args...);
      };

  template<class Actions, class... Args>
  inline constexpr bool has_post_swap_action
    = requires (std::remove_cvref_t<Actions> actions, std::remove_cvref_t<Args>&... args) {
        actions.post_swap_action(args...);
      };

  template<class Actions, class... Args>
  inline constexpr bool has_post_serialization_action
    = requires (std::remove_cvref_t<Actions> actions, std::remove_cvref_t<Args>&... args) {
        actions.post_serialization_action(args...);
      };

  //================================ comparisons ================================//

  template<test_mode Mode, class Actions, pseudoregular T, class... Args>
  [[nodiscard]]
  static bool check_preconditions(test_logger<Mode>& logger, const Actions& actions, const T& x, const T& y, const Args&... args)
  {
    return check_equality_preconditions(logger, actions, x, y, args...);
  }
  
  template<test_mode Mode, class Actions, pseudoregular T, class... Args>
    requires std::totally_ordered<T>
  [[nodiscard]]
  static bool check_preconditions(test_logger<Mode>& logger, const Actions& actions, const T& x, const T& y, const Args&... args)
  {
    return check_orderable_preconditions(logger, actions, x, y, args...);
  }

  template<test_mode Mode, class Actions, moveonly T, class U, class... Args>
  [[nodiscard]]
  static bool check_preconditions(test_logger<Mode>& logger,  const Actions& actions, const T& x, const T& y, const U& xClone, const U& yClone, const Args&... args)
  {
    return check_equality_preconditions(logger, actions, x, y, xClone, yClone, args...);
  }

  template<test_mode Mode, class Actions, moveonly T, class U, class... Args>
    requires std::totally_ordered<T>
  [[nodiscard]]
  static bool check_preconditions(test_logger<Mode>& logger, const Actions& actions, const T& x, const T& y, const U& xClone, const U& yClone, const Args&... args)
  {
    return check_orderable_preconditions(logger, actions, x, y, xClone, yClone, args...);
  }

  template<test_mode Mode, class Actions, moveonly T, class... Args>
  [[nodiscard]]
  static bool check_preconditions(test_logger<Mode>& logger,  const Actions& actions, const T& x, const T& y, const Args&... args)
  {
    return check_equality_preconditions(logger, actions, x, y, args...);
  }

  template<test_mode Mode, class Actions, moveonly T, class... Args>
    requires std::totally_ordered<T>
  [[nodiscard]]
  static bool check_preconditions(test_logger<Mode>& logger, const Actions& actions, const T& x, const T& y, const Args&... args)
  {
    return check_orderable_preconditions(logger, actions, x, y, args...);
  }

  //================================ comparisons ================================//

  template<test_mode Mode, comparison_flavour C, class Actions, movable_comparable T, invocable_r<bool, T> Fn, class... Args>
  bool do_check_comparison_consistency(test_logger<Mode>& logger, comparison_constant<C> comparison, [[maybe_unused]] const Actions& actions, const T& x, std::string_view tag, Fn fn, [[maybe_unused]] const Args&... args)
  {
    if(!check(std::string{"operator"}.append(to_string(comparison.value)).append(" is inconsistent ").append(tag), logger, fn(x)))
      return false;

    if constexpr (has_post_comparison_action<Actions, test_logger<Mode>, comparison_constant<C>, T, std::string_view, Args...>)
    {
      if(!actions.post_comparison_action(logger, comparison, x, tag, args...))
        return false;
    }

    return true;
  }

  template<test_mode Mode, comparison_flavour C, class Actions, movable_comparable T, invocable_r<bool, T> Fn>
  bool check_comparison_consistency(test_logger<Mode>& logger, comparison_constant<C> comparison, const Actions& actions, const T& x, const T& y, Fn fn)
  {
    sentinel sentry{logger, ""};

    do_check_comparison_consistency(logger, comparison, actions, x, "(x)", fn);
    do_check_comparison_consistency(logger, comparison, actions, y, "(y)", fn);

    return !sentry.failure_detected();
  }

  template<test_mode Mode, class Actions, std::totally_ordered T, class... Args>
  [[nodiscard]]
  bool check_ordering_operators(test_logger<Mode>& logger, const Actions& actions, const T& x, const T& y, const Args&... args)
  {
    sentinel sentry{logger, ""};

    check_comparison_consistency(logger, less_than_type{}, actions, x, y, [](const T& x) { return !(x < x); }, args...);
    check_comparison_consistency(logger, leq_type{}, actions, x, y, [](const T& x) { return x <= x; }, args...);
    check_comparison_consistency(logger, greater_than_type{}, actions, x, y, [](const T& x) { return !(x > x); }, args...);
    check_comparison_consistency(logger, geq_type{}, actions, x, y, [](const T& x) { return x >= x; }, args...);

    if constexpr (three_way_comparable<T>)
    {
      check_comparison_consistency(logger, threeway_type{}, actions, x, y, [](const T& x) { return (x <=> x) == 0; }, args...);
    }

    return !sentry.failure_detected();
  }

  template<test_mode Mode, class Actions, std::totally_ordered T, class... Args>
  [[nodiscard]]
  bool check_ordering_consistency(test_logger<Mode>& logger, const Actions& actions, const T& x, const T& y, const Args&... args)
  {
    if(!check_ordering_operators(logger, actions, x, y, args...)) return false;

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

  template<test_mode Mode, class Actions, std::totally_ordered T, class U, class... Args>
  [[nodiscard]]
  bool check_ordering_consistency(test_logger<Mode>& logger, const Actions& actions, const T& x, const T& y, const U&, const U&, const Args&... args)
  {
    return check_ordering_consistency(logger, actions, x, y, args...);
  }

  template<test_mode Mode, class Actions, std::equality_comparable T, class... Args>
  [[nodiscard]]
  bool check_equality_preconditions(test_logger<Mode>& logger, const Actions& actions, const T& x, const T& y, const Args&... args)
  {
    const bool eq{check_comparison_consistency(logger, equality_type{}, actions, x, y, [](const T& x) { return x == x; }, args...)};
    const bool neq{check_comparison_consistency(logger, inequality_type{}, actions, x, y, [](const T& x) { return !(x != x); }, args...)};

    return eq && neq && check("Precondition - for checking semantics, x and y are assumed to be different", logger, x != y);
  }

  template<test_mode Mode, class Actions, std::equality_comparable T, class... Args>
  [[nodiscard]]
  bool check_equality_preconditions(test_logger<Mode>& logger, const Actions& actions, const T& x, const T& y, const T& xClone, const T& yClone, const Args&... args)
  {
    if(!check_equality_preconditions(logger, actions, x, y, args...))
      return false;

    auto mess{
      [](std::string_view var) {
        return std::format("Precondition - for checking move-only semantics, {} and {}Clone are assumed to be equal", var, var);
      }
    };

    return check(mess("x"), logger, x == xClone) && check(mess("y"), logger, y == yClone);
  }

  template<test_mode Mode, class Actions, std::equality_comparable T, class U, class... Args>
    requires (!std::is_same_v<T, U>)
  [[nodiscard]]
  bool check_equality_preconditions(test_logger<Mode>& logger, const Actions& actions, const T& x, const T& y, const U&, const U&, const Args&... args)
  {
    if(!check_equality_preconditions(logger, actions, x, y, args...))
      return false;

    // TO DO: reinstate and combine with function above
    /*auto mess{
      [](std::string_view var) {
        return std::format("Precondition - for checking move-only semantics, {} and {}Clone are assumed to be equal", var, var);
      }
    };

    return check(mess("x"), logger, x == xClone) && check(mess("y"), logger, y == yClone);*/
    return true;
  }
  
  template<test_mode Mode, class Actions, std::totally_ordered T, class... Args>
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

          if constexpr(serializable<T>)
          {
            return check(mess(), logger, cond,
                       tutor{[](const T& x, const T& y) {
                               return prediction_message(to_string(x), to_string(y)); } });
          }
          else
          {
            return check(mess(), logger, cond);
          }
        }
      }
    }

    return false;
  }

  //================================  move construction ================================ //

  template<test_mode Mode, class Actions, movable_comparable T, class U, class... Args>
  std::optional<T> do_check_move_construction(test_logger<Mode>& logger,
                                              [[maybe_unused]] const Actions& actions,
                                              T&& z,
                                              const U& y,
                                              opt_moved_from_ref<U> movedFrom,
                                              const Args&... args)
  {
    T w{std::move(z)};
    if(!check(with_best_available, "Inconsistent move construction", logger, w, y))
      return {};

    if(movedFrom.has_value())
    {
      check(with_best_available, "Incorrect moved-from value after move construction", logger, z, movedFrom.value().get());
    }

    if constexpr(has_post_move_action<Actions, test_logger<Mode>, T, Args...>)
    {
      actions.post_move_action(logger, w, args...);
    }

    return w;
  }

  template<test_mode Mode, class Actions, movable_comparable T, class U>
  std::optional<T> check_move_construction(test_logger<Mode>& logger,
                                           const Actions& actions,
                                           T&& z,
                                           const U& y,
                                           opt_moved_from_ref<U> movedFrom)
  {
    return do_check_move_construction(logger, actions, std::forward<T>(z), y, movedFrom);
  }

  //================================ move assign ================================//

  template<test_mode Mode, class Actions, movable_comparable T, class U, std::invocable<T&> Mutator, class... Args>
  void do_check_move_assign(test_logger<Mode>& logger,
                            [[maybe_unused]] const Actions& actions,
                            T& z,
                            T&& y,
                            const U& yClone,
                            opt_moved_from_ref<U> movedFrom,
                            [[maybe_unused]] Mutator&& yMutator,
                            const Args&... args)
  {
    z = std::move(y);
    if(!check(with_best_available, "Inconsistent move assignment (from y)", logger, z, yClone))
       return;

    if(movedFrom.has_value())
    {
      check(with_best_available, "Incorrect moved-from value after move assignment", logger, y, movedFrom.value().get());
    }

    if constexpr(has_post_move_assign_action<Actions, test_logger<Mode>, T, T, Mutator, Args...>)
    {
      actions.post_move_assign_action(logger, z, yClone, std::move(yMutator), args...);
    }
  }

  template<test_mode Mode, class Actions, movable_comparable T, class U, std::invocable<T&> Mutator>
  void check_move_assign(test_logger<Mode>& logger,
                         const Actions& actions,
                         T& z,
                         T&& y,
                         const U& yClone,
                         opt_moved_from_ref<U> movedFrom,
                         Mutator m)
  {
    do_check_move_assign(logger, actions, z, std::forward<T>(y), yClone, movedFrom, std::move(m));
  }

  //================================ swap ================================//

  template<test_mode Mode, class Actions, movable_comparable T, class U, class... Args>
  bool do_check_swap(test_logger<Mode>& logger,
                     [[maybe_unused]] const Actions& actions,
                     T&& x,
                     T&& y,
                     const U& xClone,
                     const U& yClone,
                     [[maybe_unused]] const Args&... args)
  {
    std::ranges::swap(x, y);

    const bool swapy{
      check(with_best_available, "Inconsistent Swap (y)", logger, y, xClone)
    };

    const bool swapx{
      check(with_best_available, "Inconsistent Swap (x)", logger, x, yClone)
    };

    if(swapx && swapy)
    {
      if constexpr(has_post_swap_action<Actions, test_logger<Mode>, T, T, T, Args...>)
      {
        actions.post_swap_action(logger, x, y, yClone, args...);
      }

      std::ranges::swap(y,y);
      return check(with_best_available, "Inconsistent Self Swap", logger, y, xClone);
    }

    return false;
  }

  template<test_mode Mode, class Actions, movable_comparable T, class U>
  bool check_swap(test_logger<Mode>& logger, const Actions& actions, T&& x, T&& y, const U& xClone, const U& yClone)
  {
    return do_check_swap(logger, actions, std::move(x), std::move(y), xClone, yClone);
  }

  template<test_mode Mode, class Actions, pseudoregular T, class U, std::invocable<T&> Mutator>
  bool check_swap(test_logger<Mode>& logger, const Actions& actions, T&& x, T&& y, const U& xClone, const U& yClone, Mutator yMutator)
  {
    return do_check_swap(logger, actions, std::move(x), std::move(y), xClone, yClone, std::move(yMutator));
  }

  //================================  serialization  ================================ //

  template<test_mode Mode, class Actions, movable_comparable T, class... Args>
    requires (serializable_to<T, std::stringstream> && deserializable_from<T, std::stringstream>)
  bool do_check_serialization(test_logger<Mode>& logger,
                              [[maybe_unused]] const Actions& actions,
                              T&& u,
                              const T& y,
                              [[maybe_unused]] const Args&... args)
  {
    std::stringstream s{};
    s << y;

    if constexpr(has_post_serialization_action<Actions, test_logger<Mode>, T, Args...>)
    {
      actions.post_serialization_action(logger, y, args...);
    }

    s >> u;

    return check(with_best_available, "Inconsistent (de)serialization", logger, u, y);
  }

  template<test_mode Mode, class Actions, movable_comparable T>
    requires (serializable_to<T, std::stringstream> && deserializable_from<T, std::stringstream>)
  bool check_serialization(test_logger<Mode>& logger, const Actions& actions, T&& u, const T& y)
  {
    return do_check_serialization(logger, actions, std::forward<T>(u), y);
  }
}
