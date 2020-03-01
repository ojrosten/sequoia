////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file MoveOnlyAllocationCheckersDetails.hpp
    \brief Implementation details for allocation checks within the unit testing framework.
*/

#include "AllocationCheckersDetails.hpp"
#include "MoveOnlyTestCheckersDetails.hpp"

namespace sequoia::unit_testing
{
  struct move_only_allocation_predictions;
}

namespace sequoia::unit_testing::impl
{
  struct move_only_allocation_actions : allocation_actions
  {
    constexpr static bool has_post_equality_action{true};
    constexpr static bool has_post_nequality_action{true};
    constexpr static bool has_post_copy_action{};
    constexpr static bool has_post_copy_assign_action{};
    constexpr static bool has_post_move_action{true};
    constexpr static bool has_post_move_assign_action{true};
    constexpr static bool has_additional_action{};

    template<class Logger, class Container, class... Allocators, class... Predictions>
    static void post_move_action(std::string_view description, Logger& logger, const Container& x, const allocation_checker<Container, Allocators, Predictions>&... checkers)
    {
      check_move_y_allocation(description, logger, x, allocation_checker<Container, Allocators, Predictions>{x, checkers.first_count(), checkers.info()}...);
    }

    template<class Logger, class Container, class... Allocators, class... Predictions>
    static void post_move_assign_action(std::string_view description, Logger& logger, const Container& y, const Container& yClone, const allocation_checker<Container, Allocators, Predictions>&... checkers)
    {
      check_move_assign_allocation(description, logger, y, checkers...);      
    }
  };

  template<class Logger, class Actions, class T, class... Allocators>
  bool check_regular_semantics(std::string_view description, Logger& logger, const Actions& actions, T&& x, T&& y, const T& xClone, const T& yClone, std::tuple<allocation_checker<T, Allocators, move_only_allocation_predictions>...> checkers)
  {
    auto fn{
      [description,&logger,&actions,&x,&y,&xClone,&yClone](auto&&... checkers){
        return impl::check_regular_semantics(description, logger, actions, std::forward<T>(x), std::forward<T>(y), xClone, yClone, std::forward<decltype(checkers)>(checkers)...);
      }
    };

    return unpack_invoke(checkers, fn);
  }
}
