////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
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
    constexpr individual_allocation_predictions(copy_prediction copyPrediction, mutation_prediction mutationPrediction)
      : copy{copyPrediction}
      , mutation{mutationPrediction}
      , para_copy{convert<individual_allocation_event::para_copy>(copyPrediction)}
      , para_move{convert<individual_allocation_event::para_move>(copyPrediction)}
    {}

    constexpr individual_allocation_predictions(copy_prediction copyPrediction,
                                                mutation_prediction mutationPrediction,
                                                para_copy_prediction paraCopyPrediction)
      : copy{copyPrediction}
      , mutation{mutationPrediction}
      , para_copy{paraCopyPrediction}
      , para_move{convert<individual_allocation_event::para_move>(copyPrediction)}
    {}
      
    constexpr individual_allocation_predictions(copy_prediction copyPrediction,
                                                mutation_prediction mutationPrediction,
                                                para_copy_prediction paraCopyPrediction,
                                                para_move_prediction paraMovePrediction,
                                                move_prediction m={})
      : copy{copyPrediction}
      , mutation{mutationPrediction}
      , para_copy{paraCopyPrediction}
      , para_move{paraMovePrediction}
      , move{m}
    {}

    copy_prediction copy{};
    mutation_prediction mutation{};
    para_copy_prediction para_copy{};
    para_move_prediction para_move{};
    move_prediction move{};
  };

  struct assignment_allocation_predictions
  {
    constexpr assignment_allocation_predictions(assign_prop_prediction withPropagation, assign_prediction withoutPropagation)
      : with_propagation{withPropagation}
      , without_propagation{withoutPropagation}
      , copy_like_move{convert<assignment_allocation_event::copy_like_move_assign>(without_propagation)}
    {}

    constexpr assignment_allocation_predictions(assign_prop_prediction withPropagation,
                                                assign_prediction withoutPropagation,
                                                copy_like_move_assign_prediction copyLikeMove,
                                                move_assign_prediction pureMove)
      : with_propagation{withPropagation}
      , without_propagation{withoutPropagation}
      , copy_like_move{copyLikeMove}
      , move{pureMove}
    {}

    assign_prop_prediction with_propagation{};
    assign_prediction without_propagation{};
    copy_like_move_assign_prediction copy_like_move{};
    move_assign_prediction move{};
  };
 
  struct allocation_predictions
  {
    constexpr allocation_predictions(copy_prediction copyX,
                                     individual_allocation_predictions yPredictions,
                                     assignment_allocation_predictions assignYtoX,
                                     container_counts containerCounts={})
      : copy_x{copyX}
      , y{yPredictions}
      , assign_y_to_x{assignYtoX}
      , containers{containerCounts}
    {}

    [[nodiscard]]
    constexpr para_move_prediction para_move_allocs() const noexcept { return y.para_move; }

    [[nodiscard]]
    constexpr copy_like_move_assign_prediction copy_like_move_assign_allocs() const noexcept
    {
      return assign_y_to_x.copy_like_move;
    }

    [[nodiscard]]
    constexpr move_assign_prediction move_assign_allocs() const noexcept
    {
      return assign_y_to_x.move;
    }

    [[nodiscard]]
    constexpr mutation_prediction mutation_allocs() const noexcept { return y.mutation; }

    [[nodiscard]]
    constexpr move_prediction move_allocs() const noexcept { return y.move; }

    copy_prediction copy_x{};
    individual_allocation_predictions y;
    assignment_allocation_predictions assign_y_to_x;
    container_counts containers;
  };

  template<class T>
  constexpr individual_allocation_predictions shift(const individual_allocation_predictions& predictions, const container_counts& containers)
  {
    using shifter = alloc_prediction_shifter<T>;
    return {shifter::shift(predictions.copy,      containers.num_y),
            shifter::shift(predictions.mutation,  containers.post_mutation_correction()),
            shifter::shift(predictions.para_copy, containers.num_y),
            shifter::shift(predictions.para_move, containers.num_y),
            shifter::shift(predictions.move,      containers.num_y)};
  }

  template<class T>
  constexpr assignment_allocation_predictions shift(const assignment_allocation_predictions& predictions, const container_counts& containers)
  {
    using shifter = alloc_prediction_shifter<T>;
    return {shifter::shift(predictions.with_propagation,    containers.num_y),
            shifter::shift(predictions.without_propagation, containers.num_y),
            shifter::shift(predictions.copy_like_move,      containers.num_x, containers.num_y),
            shifter::shift(predictions.move, containers.num_y)};
  }

  template<class T>
  constexpr allocation_predictions shift(const allocation_predictions& predictions)
  {
    using shifter = alloc_prediction_shifter<T>;
    const auto& containers{predictions.containers};
    return {shifter::shift(predictions.copy_x, containers.num_x),
            shift<T>(predictions.y, containers),
            shift<T>(predictions.assign_y_to_x, containers)};
  }

  // TO DO: revert to 'using' in C++20; presently this is done through inheritance
  // in order to make use of CTAD.

  template<class T, alloc_getter<T> Getter>
  class allocation_info
    : public basic_allocation_info<T, Getter, allocation_predictions>
  {
  public:
    using basic_allocation_info<T, Getter, allocation_predictions>::basic_allocation_info;
  };

  template
  <
    class Fn,
    class Signature=function_signature<decltype(&std::remove_cvref_t<Fn>::operator())>
  >
  allocation_info(Fn allocGetter, allocation_predictions predictions)
    -> allocation_info<std::remove_cvref_t<typename Signature::arg>, Fn>;

  template
  <
    class Fn,
    class Signature=function_signature<decltype(&std::remove_cvref_t<Fn>::operator())>
  >
  allocation_info(Fn allocGetter, std::initializer_list<allocation_predictions> predictions)
    -> allocation_info<std::remove_cvref_t<typename Signature::arg>, Fn>;
    
  template<test_mode Mode, pseudoregular T, invocable<T&> Mutator, alloc_getter<T>... Getters>
  void check_semantics(std::string_view description, test_logger<Mode>& logger, const T& x, const T& y, Mutator yMutator, const allocation_info<T, Getters>&... info)
  {
    sentinel<Mode> sentry{logger, add_type_info<T>(description).append("\n")};
      
    if(impl::check_semantics(logger, impl::regular_allocation_actions<T>{}, x, y, yMutator, std::tuple_cat(impl::make_dual_allocation_checkers(info, x, y)...)))
    {
      impl::check_para_constructor_allocations(logger, y, yMutator, info...);
    }
  }
}
