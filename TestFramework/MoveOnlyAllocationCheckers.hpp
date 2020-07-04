////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
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
    move_only_allocation_predictions(assign_prediction assignWithoutPropagation, mutation_prediction yMutation, para_move_prediction paraMove)
      : assign_without_propagation{assignWithoutPropagation}
      , mutation{yMutation}
      , para_move{paraMove}
    {}

    [[nodiscard]]
    int para_move_allocs() const noexcept { return para_move; }

    [[nodiscard]]
    int assign_without_propagation_allocs() const noexcept { return assign_without_propagation; }

    [[nodiscard]]
    int mutation_allocs() const noexcept { return mutation; }
    
    assign_prediction assign_without_propagation{};
    mutation_prediction mutation{};
    para_move_prediction para_move{};
  };

  // Done through inheritance rather than a using declaration
  // in order to make use of CTAD. Should be able to revert
  // to 'using' in C++20...
    
  template<class Container, class Allocator>
  class move_only_allocation_info
    : public basic_allocation_info<Container, Allocator, move_only_allocation_predictions>
  {
  public:
    using basic_allocation_info<Container, Allocator, move_only_allocation_predictions>::basic_allocation_info;
  };

  template<class Fn>
  move_only_allocation_info(Fn&& allocGetter, move_only_allocation_predictions predictions)
    -> move_only_allocation_info<std::decay_t<typename function_signature<decltype(&std::decay_t<Fn>::operator())>::arg>, std::decay_t<typename function_signature<decltype(&std::decay_t<Fn>::operator())>::ret>>;

  template<class Fn>
  move_only_allocation_info(Fn&& allocGetter, std::initializer_list<move_only_allocation_predictions> predictions)
    -> move_only_allocation_info<std::decay_t<typename function_signature<decltype(&std::decay_t<Fn>::operator())>::arg>, std::decay_t<typename function_signature<decltype(&std::decay_t<Fn>::operator())>::ret>>;
  
  template<test_mode Mode, moveonly T, class Mutator, class... Allocators>
  void check_semantics(std::string_view description, test_logger<Mode>& logger, T&& x, T&& y, const T& xClone, const T& yClone, Mutator m, move_only_allocation_info<T, Allocators>... info)
  {
    sentinel<Mode> sentry{logger, add_type_info<T>(description)};

    if(auto opt{impl::check_para_constructor_allocations(logger, sentry, std::forward<T>(y), yClone, info...)})
    {    
      check_semantics(logger, sentry, impl::move_only_allocation_actions{}, std::forward<T>(x), std::move(*opt), xClone, yClone, std::move(m), std::tuple_cat(impl::make_dual_allocation_checkers(info, x, y)...));
    }
  }
}
