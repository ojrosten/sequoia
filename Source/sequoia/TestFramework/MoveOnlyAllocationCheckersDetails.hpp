////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Implementation details specific to allocation checks for move-only types.
*/

#include "sequoia/TestFramework/AllocationCheckersDetails.hpp"
#include "sequoia/TestFramework/MoveOnlyCheckersDetails.hpp"

#include <optional>

namespace sequoia::testing::impl
{
  template<moveonly T>
  struct move_only_allocation_actions : allocation_actions<T>
  {
    using allocation_actions<T>::allocation_actions;
    
    template<test_mode Mode, alloc_getter<T>... Getters>
    [[nodiscard]]
    bool check_preconditions(test_logger<Mode>& logger, const T& x, const T& y, const T& xClone, const T& yClone, const dual_allocation_checker<T, Getters>&... checkers) const
    {
      return allocation_actions<T>::check_preconditions(logger, *this, x, y, xClone, yClone, dual_allocation_checker{checkers.info(), x, y}...);
    }
  };

  template<test_mode Mode, class Actions, moveonly T, alloc_getter<T>... Getters>
  bool check_swap(test_logger<Mode>& logger, const Actions& actions, T&& x, T&& y, const T& xClone, const T& yClone, const dual_allocation_checker<T, Getters>&... checkers)
  {
    return do_check_swap(logger, actions, std::move(x), std::move(y), xClone, yClone, dual_allocation_checker{checkers.info(), x, y}...);
  }

  template<test_mode Mode, moveonly T, alloc_getter<T>... Getters>
  std::optional<T> check_para_constructor_allocations(test_logger<Mode>& logger, T&& y, const T& yClone, const allocation_info<T, Getters>&... info)
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
  void check_semantics(test_logger<Mode>& logger, const Actions& actions, T&& x, T&& y, const T& xClone, const T& yClone, Mutator m, std::tuple<dual_allocation_checker<T, Getters>...> checkers)
  {
    auto fn{
      [&logger, &actions, &x, &y, &xClone, &yClone, m{std::move(m)}](auto&&... checkers){
        return check_semantics(logger, actions, std::forward<T>(x), std::forward<T>(y), xClone, yClone, std::move(m), std::forward<decltype(checkers)>(checkers)...);
      }
    };

    std::apply(fn, std::move(checkers));
  }
}
