////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Implementation details specific to allocation checks for move-only types.
*/

#include "AllocationCheckersDetails.hpp"
#include "MoveOnlyCheckersDetails.hpp"

#include <optional>

namespace sequoia::testing
{
  struct move_only_allocation_predictions;
}

namespace sequoia::testing::impl
{
  struct move_only_allocation_actions : allocation_actions
  {};

  template<test_mode Mode, class Actions, class Container, class... Allocators, class... Predictions>
  bool check_swap(std::string_view description, sentinel<Mode>& sentry, const Actions& actions, Container&& x, Container&& y, const Container& xClone, const Container& yClone, const dual_allocation_checker<Container, Allocators, Predictions>&... checkers)
  {
    return do_check_swap(description, sentry, actions, std::move(x), std::move(y), xClone, yClone, dual_allocation_checker{checkers.info(), x, y}...);
  }

  template<test_mode Mode, class Container, class... Allocators, class... Predictions>
  std::optional<Container> check_para_constructor_allocations(std::string_view description, sentinel<Mode>& sentry, Container&& y, const Container& yClone, const basic_allocation_info<Container, Allocators, Predictions>&... info)
  {
    Container u{std::move(y), info.make_allocator()...};
    check_para_move_y_allocation(description, sentry, u, std::tuple_cat(make_allocation_checkers(info)...));
    const bool success{check_equality(sentry.add_details(description, "Inonsistent para-move constructor"), sentry.logger(), u, yClone)};

    return success ? std::optional<Container>{std::move(u)} : std::nullopt;
  }

  /// Unpacks the tuple and feeds to the overload of check_semantics defined in MoveOnlyCheckersDetails.hpp
  template<test_mode Mode, class Actions, class T, class Mutator, class... Allocators>
  void check_semantics(std::string_view description, sentinel<Mode>& sentry, const Actions& actions, T&& x, T&& y, const T& xClone, const T& yClone, Mutator m, std::tuple<dual_allocation_checker<T, Allocators, move_only_allocation_predictions>...> checkers)
  {
    auto fn{
      [description,&sentry,&actions,&x,&y,&xClone,&yClone,m{std::move(m)}](auto&&... checkers){
        return check_semantics(description, sentry, actions, std::forward<T>(x), std::forward<T>(y), xClone, yClone, std::move(m), std::forward<decltype(checkers)>(checkers)...);
      }
    };

    std::apply(fn, checkers);
  }
}
