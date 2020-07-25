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

  template<test_mode Mode, class Actions, moveonly T, alloc_getter<T>... Getters, class... Predictions>
  bool check_swap(test_logger<Mode>& logger, const Actions& actions, T&& x, T&& y, const T& xClone, const T& yClone, const dual_allocation_checker<T, Getters, Predictions>&... checkers)
  {
    return do_check_swap(logger, actions, std::move(x), std::move(y), xClone, yClone, dual_allocation_checker{checkers.info(), x, y}...);
  }

  template<test_mode Mode, moveonly T, alloc_getter<T>... Getters, class... Predictions>
  std::optional<T> check_para_constructor_allocations(test_logger<Mode>& logger, T&& y, const T& yClone, const basic_allocation_info<T, Getters, Predictions>&... info)
  {
    if(!check("Precondition - for checking move-only semantics, y and yClone are assumed to be equal", logger, y == yClone)) return{};
    
    T u{std::move(y), info.make_allocator()...};
    check_para_move_y_allocation(logger, u, std::tuple_cat(make_allocation_checkers(info)...));
    if(check_equality("Inonsistent para-move constructor", logger, u, yClone))
    {
      std::optional<T> v{std::move(u)};
      if(check_equality("Inconsistent move construction", logger, *v, yClone))
      {
        return std::move(v);
      }
    }

    return std::nullopt;
  }

  /// Unpacks the tuple and feeds to the overload of check_semantics defined in MoveOnlyCheckersDetails.hpp
  template<test_mode Mode, class Actions, moveonly T, invocable<T&> Mutator, alloc_getter<T>... Getters>
  void check_semantics(test_logger<Mode>& logger, const Actions& actions, T&& x, T&& y, const T& xClone, const T& yClone, Mutator m, std::tuple<dual_allocation_checker<T, Getters, move_only_allocation_predictions>...> checkers)
  {
    auto fn{
      [&logger, &actions, &x, &y, &xClone, &yClone, m{std::move(m)}](auto&&... checkers){
        return check_semantics(logger, actions, std::forward<T>(x), std::forward<T>(y), xClone, yClone, std::move(m), std::forward<decltype(checkers)>(checkers)...);
      }
    };

    std::apply(fn, std::move(checkers));
  }
}
