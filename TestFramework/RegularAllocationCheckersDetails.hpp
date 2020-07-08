////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Implementation details for allocation checks of regular types.
*/

#include "AllocationCheckersDetails.hpp"
#include "RegularCheckersDetails.hpp"

namespace sequoia::testing
{
  struct allocation_predictions;
}

namespace sequoia::testing::impl
{
  /*! \brief Extends allocation_actions for types with copy semantics. */
  struct regular_allocation_actions : allocation_actions
  {
    constexpr static bool has_post_copy_action{true};
    constexpr static bool has_post_copy_assign_action{true};

    template<test_mode Mode, pseudoregular T, counting_alloc... Allocators, class... Predictions>
    static void post_copy_action(test_logger<Mode>& logger, const sentinel<Mode>& sentry, const T& xCopy, const T& yCopy, const dual_allocation_checker<T, Allocators, Predictions>&... checkers)
    {
      
      check_copy_x_allocation(logger, sentry, xCopy, allocation_checker{checkers.info(), checkers.first_count()}...);

      check_copy_y_allocation(logger, sentry, yCopy, allocation_checker{checkers.info(), checkers.second_count()}...);
    }

    template<test_mode Mode, pseudoregular T, counting_alloc... Allocators, class... Predictions>
    static void post_copy_assign_action(test_logger<Mode>& logger, const sentinel<Mode>& sentry, const T& lhs, const T& rhs, const dual_allocation_checker<T, Allocators, Predictions>&... checkers)
    {
      check_copy_assign_allocation(logger, sentry, lhs, rhs, checkers...);
    }

    template<test_mode Mode, pseudoregular T, invocable<T&> Mutator, counting_alloc... Allocators, class... Predictions>
    static void post_swap_action(test_logger<Mode>& logger, const sentinel<Mode>& sentry, T& x, const T& y, const T& yClone, Mutator yMutator, const dual_allocation_checker<T, Allocators, Predictions>&... checkers)
    {
      allocation_actions::post_swap_action(logger, sentry, x, y, yClone, checkers...);      
      check_mutation_after_swap(logger, sentry, x, y, yClone, std::move(yMutator), checkers...);
    }
  };

  /*! Provides an extra level of indirection in order that the current number of allocation
       may be acquired before proceeding.
   */
  template<test_mode Mode, class Actions, pseudoregular T, counting_alloc... Allocators, class... Predictions>
  bool check_copy_assign(test_logger<Mode>& logger, const sentinel<Mode>& sentry, const Actions& actions, T& z, const T& y, const dual_allocation_checker<T, Allocators, Predictions>&... checkers)
  {
    return do_check_copy_assign(logger, sentry, actions, z, y, dual_allocation_checker{checkers.info(), z, y}...);   
  }

  template<test_mode Mode, pseudoregular T, invocable<T&> Mutator, counting_alloc... Allocators, class... Predictions>
  void check_para_constructor_allocations(test_logger<Mode>& logger, const sentinel<Mode>& sentry, const T& y, Mutator yMutator, const basic_allocation_info<T, Allocators, Predictions>&... info)
  {    
    if constexpr(sizeof...(Allocators) > 0)
    {
      auto make{
        [&logger, &sentry, &y](auto&... info){
          T u{y, info.make_allocator()...};
          if(check_equality(sentry.generate_message("Inconsistent para-copy construction"), logger, u, y))
          {
            check_para_copy_y_allocation(logger, sentry, u, std::tuple_cat(make_allocation_checkers(info)...));
            return std::optional<T>{u};
          }
          return std::optional<T>{};
        }
      };
      
      if(auto c{make(info...)}; c)
      {
        T v{std::move(*c), info.make_allocator()...};

        if(check_equality(sentry.generate_message("Inconsistent para-move construction"), logger, v, y))
        {
          check_para_move_y_allocation(logger, sentry, v, std::tuple_cat(make_allocation_checkers(info)...));
          check_mutation_after_move("para-move", logger, sentry, v, y, std::move(yMutator), std::tuple_cat(make_allocation_checkers(info, v)...));
        }
      }
    }
  }

  template
  <
    test_mode Mode,
    class Actions,
    pseudoregular T,
    invocable<T&> Mutator,
    counting_alloc... Allocators,
    class... Predictions
  >
  bool check_swap(test_logger<Mode>& logger, const sentinel<Mode>& sentry, const Actions& actions, T&& x, T&& y, const T& xClone, const T& yClone, Mutator yMutator, const dual_allocation_checker<T, Allocators, Predictions>&... checkers)
  {
    return do_check_swap(logger, sentry, actions, std::move(x), std::move(y), xClone, yClone, std::move(yMutator), dual_allocation_checker{checkers.info(), x, y}...);
  }

  /// Unpacks the tuple and feeds to the overload of check_semantics defined in RegularCheckersDetails.hpp
  template
  <
    test_mode Mode,
    class Actions,
    pseudoregular T,
    invocable<T&> Mutator,
    counting_alloc... Allocators,
    class... Predictions
  >
  bool check_semantics(test_logger<Mode>& logger, const sentinel<Mode>& sentry, const Actions& actions, const T& x, const T& y, Mutator yMutator, std::tuple<dual_allocation_checker<T, Allocators, Predictions>...> checkers)
  {
    auto fn{
      [&logger, &sentry, &actions, &x, &y, m{std::move(yMutator)}](auto&&... checkers){
        return check_semantics(logger, sentry, actions, x, y, m, std::forward<decltype(checkers)>(checkers)...);
      }
    };

    return std::apply(fn, checkers);
  }
}
