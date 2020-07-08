////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Utilities for performing allocation checks for regular types,
    see \ref regular_semantics_definition "here".
*/

#include "AllocationCheckers.hpp"
#include "RegularAllocationCheckersDetails.hpp"

namespace sequoia::testing
{  
  struct individual_allocation_predictions
  {
    individual_allocation_predictions(copy_prediction copyPrediction, mutation_prediction mutationPrediction)
      : copy{copyPrediction}          
      , mutation{mutationPrediction}
      , para_copy{copyPrediction}
      , para_move{copyPrediction}
    {}

    individual_allocation_predictions(copy_prediction copyPrediction, mutation_prediction mutationPrediction, para_copy_prediction paraCopyPrediction)
      : copy{copyPrediction}          
      , mutation{mutationPrediction}
      , para_copy{paraCopyPrediction}
      , para_move{copyPrediction}
    {}
      
    individual_allocation_predictions(copy_prediction copyPrediction, mutation_prediction mutationPrediction, para_copy_prediction paraCopyPrediction, para_move_prediction paraMovePrediction)
      : copy{copyPrediction}          
      , mutation{mutationPrediction}
      , para_copy{paraCopyPrediction}
      , para_move{paraMovePrediction}
    {}
      
    copy_prediction copy{};
    mutation_prediction mutation{};
    para_copy_prediction para_copy{};
    para_move_prediction para_move{};
  };

  struct assignment_allocation_predictions
  {
    assignment_allocation_predictions(int withPropagation, int withoutPropagation)
      : with_propagation{withPropagation}, without_propagation{withoutPropagation}
    {}
      
    assign_prop_prediction with_propagation{};
    assign_prediction without_propagation{};
  };
 
  struct allocation_predictions
  {
    allocation_predictions(copy_prediction copyX, individual_allocation_predictions yPredictions, assignment_allocation_predictions assignYtoX)
      : copy_x{copyX}, y{yPredictions}, assign_y_to_x{assignYtoX}
    {}

    [[nodiscard]]
    int para_move_allocs() const noexcept
    {
      return y.para_move;
    }

    [[nodiscard]]
    int assign_without_propagation_allocs() const noexcept
    {
      return assign_y_to_x.without_propagation;
    }

    [[nodiscard]]
    int mutation_allocs() const noexcept { return y.mutation; }

    int copy_x{};
    individual_allocation_predictions y;
    assignment_allocation_predictions assign_y_to_x;
  };

  // Done through inheritance rather than a using declaration
  // in order to make use of CTAD. Should be able to revert
  // to using in C++20...

  template<class T, counting_alloc Allocator>
  class allocation_info
    : public basic_allocation_info<T, Allocator, allocation_predictions>
  {
  public:
    using basic_allocation_info<T, Allocator, allocation_predictions>::basic_allocation_info;
  };

  template<class Fn>
  allocation_info(Fn&& allocGetter, allocation_predictions predictions)
    -> allocation_info<std::decay_t<typename function_signature<decltype(&std::decay_t<Fn>::operator())>::arg>, std::decay_t<typename function_signature<decltype(&std::decay_t<Fn>::operator())>::ret>>;

  template<class Fn>
  allocation_info(Fn&& allocGetter, std::initializer_list<allocation_predictions> predictions)
    -> allocation_info<std::decay_t<typename function_signature<decltype(&std::decay_t<Fn>::operator())>::arg>, std::decay_t<typename function_signature<decltype(&std::decay_t<Fn>::operator())>::ret>>;
    
  template<test_mode Mode, pseudoregular T, invocable<T&> Mutator, counting_alloc... Allocators>
  void check_semantics(std::string_view description, test_logger<Mode>& logger, const T& x, const T& y, Mutator yMutator, allocation_info<T, Allocators>... info)
  {
    sentinel<Mode> sentry{logger, add_type_info<T>(description)};
      
    if(impl::check_semantics(logger, sentry, impl::regular_allocation_actions{}, x, y, yMutator, std::tuple_cat(impl::make_dual_allocation_checkers(info, x, y)...)))
    {
      impl::check_para_constructor_allocations(logger, sentry, y, yMutator, info...);
    }
  }
}
