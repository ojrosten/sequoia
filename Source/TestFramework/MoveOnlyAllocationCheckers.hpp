////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Utilities for performing allocation checks of move-only types.
*/

#include "AllocationCheckers.hpp"
#include "MoveOnlyAllocationCheckersDetails.hpp"

namespace sequoia::testing
{
  struct move_only_allocation_predictions
  {
    constexpr move_only_allocation_predictions(copy_like_move_assign_prediction copyLikeMove,
                                               mutation_prediction yMutation,
                                               para_move_prediction paraMove,
                                               move_prediction m={},
                                               move_assign_prediction moveAssign={},
                                               number_of_containers numContainersX = {},
                                               number_of_containers numContainersY = {})
      : copy_like_move_assign{copyLikeMove}
      , mutation{yMutation}
      , para_move{paraMove}
      , move{m}
      , move_assign{moveAssign}
      , num_containers_x{numContainersX}
      , num_containers_y{numContainersY}
    {}

    constexpr move_only_allocation_predictions(copy_like_move_assign_prediction copyLikeMove,
                                               mutation_prediction yMutation,
                                               para_move_prediction paraMove,
                                               number_of_containers numContainersX,
                                               number_of_containers numContainersY)
      : copy_like_move_assign{ copyLikeMove }
      , mutation{ yMutation }
      , para_move{ paraMove }
      , num_containers_x{ numContainersX }
      , num_containers_y{ numContainersY }
    {}

    [[nodiscard]]
    constexpr int para_move_allocs() const noexcept { return para_move; }

    [[nodiscard]]
    constexpr int copy_like_move_assign_allocs() const noexcept { return copy_like_move_assign; }

    [[nodiscard]]
    constexpr int move_assign_allocs() const noexcept { return move_assign; }

    [[nodiscard]]
    constexpr int mutation_allocs() const noexcept { return mutation; }

    [[nodiscard]]
    constexpr int move_allocs() const noexcept { return move; }
    
    copy_like_move_assign_prediction copy_like_move_assign{};
    mutation_prediction mutation{};
    para_move_prediction para_move{};
    move_prediction move{};
    move_assign_prediction move_assign{};
    number_of_containers num_containers_x{}, num_containers_y{};
  };

  template<class T>
  constexpr move_only_allocation_predictions shift(const move_only_allocation_predictions& predictions)
  {
    using shifter = alloc_prediction_shifter<T>;
    return {shifter::shift(predictions.copy_like_move_assign, predictions.num_containers_x, predictions.num_containers_y),
            shifter::shift(predictions.mutation,    predictions.num_containers_y),
            shifter::shift(predictions.para_move,   predictions.num_containers_y),
            shifter::shift(predictions.move,        predictions.num_containers_y),
            shifter::shift(predictions.move_assign, predictions.num_containers_y)};
  }

  // Done through inheritance rather than a using declaration
  // in order to make use of CTAD. Should be able to revert
  // to 'using' in C++20...
    
  template<class T, alloc_getter<T> Getter>
  class move_only_allocation_info
    : public basic_allocation_info<T, Getter, move_only_allocation_predictions>
  {
  public:
    using basic_allocation_info<T, Getter, move_only_allocation_predictions>::basic_allocation_info;
  };

  template
  <
    class Fn,
    class Signature=function_signature<decltype(&std::remove_cvref_t<Fn>::operator())>
  >
  move_only_allocation_info(Fn allocGetter, move_only_allocation_predictions predictions)
    -> move_only_allocation_info<std::remove_cvref_t<typename Signature::arg>, Fn>;

  template
  <
    class Fn,
    class Signature=function_signature<decltype(&std::remove_cvref_t<Fn>::operator())>
  >
  move_only_allocation_info(Fn allocGetter, std::initializer_list<move_only_allocation_predictions> predictions)
    -> move_only_allocation_info<std::remove_cvref_t<typename Signature::arg>, Fn>;
  
  template<test_mode Mode, moveonly T, invocable<T&> Mutator, alloc_getter<T>... Getters>
  void check_semantics(std::string_view description, test_logger<Mode>& logger, T&& x, T&& y, const T& xClone, const T& yClone, Mutator m, move_only_allocation_info<T, Getters>... info)
  {
    sentinel<Mode> sentry{logger, add_type_info<T>(description).append("\n")};

    if(auto opt{impl::check_para_constructor_allocations(logger, std::forward<T>(y), yClone, info...)})
    {    
      check_semantics(logger, impl::move_only_allocation_actions<T>{}, std::forward<T>(x), std::move(*opt), xClone, yClone, std::move(m), std::tuple_cat(impl::make_dual_allocation_checkers(info, x, y)...));
    }
  }
}
