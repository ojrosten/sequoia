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
  {};

  template<test_mode Mode, class Container, class... Allocators, class... Predictions>
  std::optional<Container> check_para_constructor_allocations(std::string_view description, test_logger<Mode>& logger, Container&& y, const Container& yClone, const basic_allocation_info<Container, Allocators, Predictions>&... info)
  {
    Container u{std::move(y), info.make_allocator()...};
    check_para_move_y_allocation(description, logger, u, std::tuple_cat(make_allocation_checkers(info)...));
    const bool success{check_equality(combine_messages(description, "Move constructor using allocator"), logger, u, yClone)};

    return success ? std::optional<Container>{std::move(u)} : std::nullopt;
  }

  /// Unpacks the tuple and feeds to the overload of check_semantics defined in MoveOnlyCheckersDetails.hpp
  template<test_mode Mode, class Actions, class T, class Mutator, class... Allocators>
  void check_semantics(std::string_view description, test_logger<Mode>& logger, const Actions& actions, T&& x, T&& y, const T& xClone, const T& yClone, Mutator m, std::tuple<dual_allocation_checker<T, Allocators, move_only_allocation_predictions>...> checkers)
  {
    auto fn{
      [description,&logger,&actions,&x,&y,&xClone,&yClone,m{std::move(m)}](auto&&... checkers){
        return impl::check_semantics(description, logger, actions, std::forward<T>(x), std::forward<T>(y), xClone, yClone, std::move(m), std::forward<decltype(checkers)>(checkers)...);
      }
    };

    std::apply(fn, checkers);
  }
}
