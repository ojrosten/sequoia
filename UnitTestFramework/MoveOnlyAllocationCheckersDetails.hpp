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
#include "MoveOnlyCheckersDetails.hpp"

#include <optional>

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
    constexpr static bool has_post_swap_action{true};

    template<class Logger, class Container, class... Allocators, class... Predictions>
    static void post_move_action(std::string_view description, Logger& logger, const Container& x, const allocation_checker<Container, Allocators, Predictions>&... checkers)
    {
      check_move_y_allocation(description, logger, x, allocation_checker<Container, Allocators, Predictions>{x, checkers.first_count(), checkers.info()}...);
    }

    template<class Logger, class Container, class Mutator, class... Allocators, class... Predictions>
    static void post_move_assign_action(std::string_view description, Logger& logger, Container& y, const Container& yClone, Mutator m, const allocation_checker<Container, Allocators, Predictions>&... checkers)
    {
      check_move_assign_allocation(description, logger, y, checkers...);
      check_mutation_after_move(description, "assignment", logger, y, yClone, m, allocation_checker<Container, Allocators, Predictions>{y, 0, checkers.info()}...);
    }
  };

  template<class Logger, class Container, class... Allocators, class... Predictions>
  std::optional<Container> check_para_constructor_allocations(std::string_view description, Logger& logger, Container&& y, const Container& yClone, const basic_allocation_info<Container, Allocators, Predictions>&... info)
  {
    Container u{std::move(y), info.make_allocator()...};
    check_para_move_y_allocation(description, logger, u, std::tuple_cat(make_allocation_checkers(info)...));
    const bool success{check_equality(combine_messages(description, "Move constructor using allocator"), logger, u, yClone)};

    return success ? std::optional<Container>{std::move(u)} : std::nullopt;
  }

  template<class Logger, class Actions, class T, class Mutator, class... Allocators>
  void check_semantics(std::string_view description, Logger& logger, const Actions& actions, T&& x, T&& y, const T& xClone, const T& yClone, Mutator m, std::tuple<allocation_checker<T, Allocators, move_only_allocation_predictions>...> checkers)
  {
    auto fn{
      [description,&logger,&actions,&x,&y,&xClone,&yClone,m{std::move(m)}](auto&&... checkers){
        return impl::check_semantics(description, logger, actions, std::forward<T>(x), std::forward<T>(y), xClone, yClone, std::move(m), std::forward<decltype(checkers)>(checkers)...);
      }
    };

    unpack_invoke(checkers, fn);
  }
}
