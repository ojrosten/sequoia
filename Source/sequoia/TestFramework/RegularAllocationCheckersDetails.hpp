////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Implementation details for allocation checks of regular types.
*/

#include "sequoia/TestFramework/AllocationCheckersDetails.hpp"
#include "sequoia/TestFramework/RegularCheckersDetails.hpp"

namespace sequoia::testing::impl
{
  /*! \brief Extends allocation_actions for types with copy semantics. */
  template<pseudoregular T>
  struct regular_allocation_actions : allocation_actions<T>
  {
    using allocation_actions<T>::allocation_actions;

    template<test_mode Mode, alloc_getter<T>... Getters>
    static void post_copy_action(test_logger<Mode>& logger, const T& xCopy, const T& yCopy, const dual_allocation_checker<T, Getters>&... checkers)
    {
      check_copy_x_allocation(logger, xCopy, allocation_checker{checkers.info(), checkers.first_count()}...);

      check_copy_y_allocation(logger, yCopy, allocation_checker{checkers.info(), checkers.second_count()}...);
    }

    template<test_mode Mode, alloc_getter<T>... Getters>
    static void post_copy_assign_action(test_logger<Mode>& logger, const T& lhs, const T& rhs, const dual_allocation_checker<T, Getters>&... checkers)
    {
      check_copy_assign_allocation(logger, lhs, rhs, checkers...);
    }

    template<test_mode Mode, invocable<T&> Mutator, alloc_getter<T>... Getters>
    static void post_swap_action(test_logger<Mode>& logger, T& x, const T& y, const T& yClone, Mutator yMutator, const dual_allocation_checker<T, Getters>&... checkers)
    {
      allocation_actions<T>::post_swap_action(logger, x, y, yClone, checkers...);
      check_mutation_after_swap(logger, x, y, yClone, std::move(yMutator), checkers...);
    }
  };

  /*! Provides an extra level of indirection in order that the current number of allocation
       may be acquired before proceeding.
   */
  template<test_mode Mode, class Actions, pseudoregular T, alloc_getter<T>... Getters>
  bool check_copy_assign(test_logger<Mode>& logger, const Actions& actions, T& z, const T& y, const dual_allocation_checker<T, Getters>&... checkers)
  {
    return do_check_copy_assign(logger, actions, z, y, dual_allocation_checker{checkers.info(), z, y}...);
  }

  template<test_mode Mode, pseudoregular T, invocable<T&> Mutator, alloc_getter<T>... Getters>
  void check_para_constructor_allocations(test_logger<Mode>& logger, const T& y, Mutator yMutator, const allocation_info<T, Getters>&... info)
  {
    auto make{
      [&logger, &y](auto&... info){
        T u{y, info.make_allocator()...};
        if(check_equality("Inconsistent para-copy construction", logger, u, y))
        {
          check_para_copy_y_allocation(logger, u, std::tuple_cat(make_allocation_checkers(info)...));
          return std::optional<T>{u};
        }
        return std::optional<T>{};
      }
    };

    if(auto c{make(info...)}; c)
    {
      T v{std::move(*c), info.make_allocator()...};

      if(check_equality("Inconsistent para-move construction", logger, v, y))
      {
        using ctag = container_tag_constant<container_tag::y>;
        check_para_move_allocation(logger, ctag{}, v, std::tuple_cat(make_allocation_checkers(info)...));
        check_mutation_after_move("para-move", logger, v, y, std::move(yMutator), std::tuple_cat(make_allocation_checkers(info, v)...));
      }
    }
  }

  template
  <
    test_mode Mode,
    class Actions,
    pseudoregular T,
    invocable<T&> Mutator,
    alloc_getter<T>... Getters
  >
  bool check_swap(test_logger<Mode>& logger, const Actions& actions, T&& x, T&& y, const T& xClone, const T& yClone, Mutator yMutator, const dual_allocation_checker<T, Getters>&... checkers)
  {
    return do_check_swap(logger, actions, std::move(x), std::move(y), xClone, yClone, std::move(yMutator), dual_allocation_checker{checkers.info(), x, y}...);
  }

  template<test_mode Mode, class Actions, pseudoregular T, invocable<T&> Mutator, alloc_getter<T>... Getters>
    requires (sizeof...(Getters) > 0)
  void check_semantics(std::string_view description, test_logger<Mode>& logger, const Actions& actions, const T& x, const T& y, Mutator yMutator, const allocation_info<T, Getters>&... info)
  {
    sentinel<Mode> sentry{logger, add_type_info<T>(description).append("\n")};

    if(check_semantics(logger, actions, x, y, yMutator, std::tuple_cat(make_dual_allocation_checkers(info, x, y)...)))
    {
      check_para_constructor_allocations(logger, y, yMutator, info...);
    }
  }

  template
  <
    test_mode Mode,
    class Actions,
    pseudoregular T,
    invocable_r<T> xMaker,
    invocable_r<T> yMaker,
    invocable<T&> Mutator,
    alloc_getter<T>... Getters
  >
    requires (sizeof...(Getters) > 0)
  std::pair<T, T> check_semantics(std::string_view description, test_logger<Mode>& logger, const Actions& actions, xMaker xFn, yMaker yFn, Mutator yMutator, const allocation_info<T, Getters>&... info)
  {
    sentinel<Mode> sentry{logger, add_type_info<T>(description).append("\n")};

    auto x{xFn()};
    auto y{yFn()};

    check_initialization_allocations(logger, x, y, info...);
    check_semantics(description, logger, actions, x, y, std::move(yMutator), info...);

    return {std::move(x), std::move(y)};
  }

  /// Unpacks the tuple and feeds to the overload of check_semantics defined in RegularCheckersDetails.hpp
  template
  <
    test_mode Mode,
    class Actions,
    pseudoregular T,
    invocable<T&> Mutator,
    alloc_getter<T>... Getters
  >
  bool check_semantics(test_logger<Mode>& logger, const Actions& actions, const T& x, const T& y, Mutator yMutator, std::tuple<dual_allocation_checker<T, Getters>...> checkers)
  {
    auto fn{
      [&logger, &actions, &x, &y, m{std::move(yMutator)}](auto&&... checkers){
        return check_semantics(logger, actions, x, y, m, std::forward<decltype(checkers)>(checkers)...);
      }
    };

    return std::apply(fn, checkers);
  }
}
