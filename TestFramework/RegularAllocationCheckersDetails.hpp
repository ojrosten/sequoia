////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Implementation details for allocation checks within the unit testing framework.
*/

#include "AllocationCheckersDetails.hpp"

namespace sequoia::unit_testing
{
  struct allocation_predictions;
}

namespace sequoia::unit_testing::impl
{
  struct regular_allocation_actions : allocation_actions
  {
    constexpr static bool has_post_copy_action{true};
    constexpr static bool has_post_copy_assign_action{true};

    template<test_mode Mode, class Container, class... Allocators, class... Predictions>
    static void post_copy_action(std::string_view description, test_logger<Mode>& logger, const Container& xCopy, const Container& yCopy, const dual_allocation_checker<Container, Allocators, Predictions>&... checkers)
    {
      
      check_copy_x_allocation(description, logger, xCopy, allocation_checker{checkers.info(), checkers.first_count()}...);

      check_copy_y_allocation(description, logger, yCopy, allocation_checker{checkers.info(), checkers.second_count()}...);
    }

    template<test_mode Mode, class Container, class... Allocators, class... Predictions>
    static void post_copy_assign_action(std::string_view description, test_logger<Mode>& logger, const Container& lhs, const Container& rhs, const dual_allocation_checker<Container, Allocators, Predictions>&... checkers)
    {
      check_copy_assign_allocation(description, logger, lhs, rhs, checkers...);
    }
  };

  template<test_mode Mode, class Actions, class Container, class... Allocators, class... Predictions>
  void check_copy_assign(std::string_view description, test_logger<Mode>& logger, const Actions& actions, Container& z, const Container& y, const dual_allocation_checker<Container, Allocators, Predictions>&... checkers)
  {
    do_check_copy_assign(description, logger, actions, z, y, dual_allocation_checker{checkers.info(), z, y}...);   
  }

  template<test_mode Mode, class Container, class Mutator, class... Allocators, class... Predictions>
  void check_para_constructor_allocations(std::string_view description, test_logger<Mode>& logger, const Container& y, Mutator yMutator, const basic_allocation_info<Container, Allocators, Predictions>&... info)
  {    
    if constexpr(sizeof...(Allocators) > 0)
    {
      auto make{
        [description, &logger, &y](auto&... info){
          Container u{y, info.make_allocator()...};
          check_equality(combine_messages(description, "Copy-like construction"), logger, u, y);
          check_para_copy_y_allocation(description, logger, u, std::tuple_cat(make_allocation_checkers(info)...));

          return u;
        }
      };

      Container v{make(info...), info.make_allocator()...};

      check_equality(combine_messages(description, "Move-like construction"), logger, v, y);    
      check_para_move_y_allocation(description, logger, v, std::tuple_cat(make_allocation_checkers(info)...));
      check_mutation_after_move(description, "allocation assignment", logger, v, y, yMutator, std::tuple_cat(make_allocation_checkers(info, v)...));
    }
  }

  template<test_mode Mode, class Actions, class T, class Mutator, class... Allocators, class... Predictions>
  bool check_semantics(std::string_view description, test_logger<Mode>& logger, const Actions& actions, const T& x, const T& y, Mutator yMutator, std::tuple<dual_allocation_checker<T, Allocators, Predictions>...> checkers)
  {
    auto fn{
      [description,&logger,&actions,&x,&y,m=std::move(yMutator)](auto&&... checkers){
        return impl::check_semantics(description, logger, actions, x, y, m, std::forward<decltype(checkers)>(checkers)...);
      }
    };

    return std::apply(fn, checkers);
  }
}
