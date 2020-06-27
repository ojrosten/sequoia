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

    template<test_mode Mode, class Container, class... Allocators, class... Predictions>
    static void post_copy_action(std::string_view description, sentinel<Mode>& sentry, const Container& xCopy, const Container& yCopy, const dual_allocation_checker<Container, Allocators, Predictions>&... checkers)
    {
      
      check_copy_x_allocation(description, sentry, xCopy, allocation_checker{checkers.info(), checkers.first_count()}...);

      check_copy_y_allocation(description, sentry, yCopy, allocation_checker{checkers.info(), checkers.second_count()}...);
    }

    template<test_mode Mode, class Container, class... Allocators, class... Predictions>
    static void post_copy_assign_action(std::string_view description, sentinel<Mode>& sentry, const Container& lhs, const Container& rhs, const dual_allocation_checker<Container, Allocators, Predictions>&... checkers)
    {
      check_copy_assign_allocation(description, sentry, lhs, rhs, checkers...);
    }

    template<test_mode Mode, class Container, class Mutator, class... Allocators, class... Predictions>
    static void post_swap_action(std::string_view description, sentinel<Mode>& sentry, Container& x, const Container& y, const Container& yClone, Mutator yMutator, const dual_allocation_checker<Container, Allocators, Predictions>&... checkers)
    {
      allocation_actions::post_swap_action(description, sentry, x, y, yClone, checkers...);      
      check_mutation_after_swap(description, sentry, x, y, yClone, std::move(yMutator), checkers...);
    }
  };

  /*! Provides an extra level of indirection in order that the current number of allocation
       may be acquired before proceeding.
   */
  template<test_mode Mode, class Actions, class Container, class... Allocators, class... Predictions>
  bool check_copy_assign(std::string_view description, sentinel<Mode>& sentry, const Actions& actions, Container& z, const Container& y, const dual_allocation_checker<Container, Allocators, Predictions>&... checkers)
  {
    return do_check_copy_assign(description, sentry, actions, z, y, dual_allocation_checker{checkers.info(), z, y}...);   
  }

  template<test_mode Mode, class Container, class Mutator, class... Allocators, class... Predictions>
  void check_para_constructor_allocations(std::string_view description, sentinel<Mode>& sentry, const Container& y, Mutator yMutator, const basic_allocation_info<Container, Allocators, Predictions>&... info)
  {    
    if constexpr(sizeof...(Allocators) > 0)
    {
      auto make{
        [description, &sentry, &y](auto&... info){
          Container u{y, info.make_allocator()...};
          check_equality(sentry.add_details(description, "Inconsistent para-copy construction"), sentry.logger(), u, y);
          check_para_copy_y_allocation(description, sentry, u, std::tuple_cat(make_allocation_checkers(info)...));

          return u;
        }
      };

      Container v{make(info...), info.make_allocator()...};

      check_equality(sentry.add_details(description, "Inconsistent para-move construction"), sentry.logger(), v, y);    
      check_para_move_y_allocation(description, sentry, v, std::tuple_cat(make_allocation_checkers(info)...));
      check_mutation_after_move(description, "allocation assignment", sentry, v, y, std::move(yMutator), std::tuple_cat(make_allocation_checkers(info, v)...));
    }
  }

  template<test_mode Mode, class Actions, class Container, class Mutator, class... Allocators, class... Predictions>
  bool check_swap(std::string_view description, sentinel<Mode>& sentry, const Actions& actions, Container&& x, Container&& y, const Container& xClone, const Container& yClone, Mutator yMutator, const dual_allocation_checker<Container, Allocators, Predictions>&... checkers)
  {
    return do_check_swap(description, sentry, actions, std::move(x), std::move(y), xClone, yClone, std::move(yMutator), dual_allocation_checker{checkers.info(), x, y}...);
  }

  /// Unpacks the tuple and feeds to the overload of check_semantics defined in RegularCheckersDetails.hpp
  template<test_mode Mode, class Actions, class T, class Mutator, class... Allocators, class... Predictions>
  bool check_semantics(std::string_view description, sentinel<Mode>& sentry, const Actions& actions, const T& x, const T& y, Mutator yMutator, std::tuple<dual_allocation_checker<T, Allocators, Predictions>...> checkers)
  {
    auto fn{
      [description,&sentry,&actions,&x,&y,m=std::move(yMutator)](auto&&... checkers){
        return check_semantics(description, sentry, actions, x, y, m, std::forward<decltype(checkers)>(checkers)...);
      }
    };

    return std::apply(fn, checkers);
  }
}
