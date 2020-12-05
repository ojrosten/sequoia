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
    constexpr move_only_allocation_predictions(assign_prediction assignWithoutPropagation, mutation_prediction yMutation, para_move_prediction paraMove)
      : assign_without_propagation{assignWithoutPropagation}
      , mutation{yMutation}
      , para_move{paraMove}
    {}

    [[nodiscard]]
    constexpr int para_move_allocs() const noexcept { return para_move; }

    [[nodiscard]]
    constexpr int assign_without_propagation_allocs() const noexcept { return assign_without_propagation; }

    [[nodiscard]]
    constexpr int mutation_allocs() const noexcept { return mutation; }
    
    assign_prediction assign_without_propagation{};
    mutation_prediction mutation{};
    para_move_prediction para_move{};
  };

  template<class T>
  constexpr move_only_allocation_predictions shift(const move_only_allocation_predictions& predictions)
  {
    using shifter = prediction_shifter<T>;
    return {shifter::shift(predictions.assign_without_propagation),
            shifter::shift(predictions.mutation),
            shifter::shift(predictions.para_move)};
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
