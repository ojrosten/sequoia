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

#include "sequoia/TestFramework/AllocationCheckers.hpp"
#include "sequoia/TestFramework/RegularAllocationCheckersDetails.hpp"

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

  
  template<class T>
  constexpr individual_allocation_predictions shift(const individual_allocation_predictions& predictions,
                                                    const alloc_prediction_shifter<T>& shifter)
  {
    return {shifter.shift(predictions.copy, container_tag::y),
            shifter.shift(predictions.mutation),
            shifter.shift(predictions.para_copy),
            shifter.shift(predictions.para_move),
            shifter.shift(predictions.move)};
  }

  template<class T>
  constexpr assignment_allocation_predictions shift(const assignment_allocation_predictions& predictions,
                                                    const alloc_prediction_shifter<T>& shifter)
  {
    return {shifter.shift(predictions.with_propagation),
            shifter.shift(predictions.without_propagation),
            shifter.shift(predictions.copy_like_move),
            shifter.shift(predictions.move)};
  }

  template<top_level TopLevel>
  class basic_allocation_predictions : public container_predictions_extension_policy<TopLevel>
  {
  public:
    template<top_level Level=TopLevel>
      requires (Level == top_level::yes)
    constexpr basic_allocation_predictions(copy_prediction x,
                                           individual_allocation_predictions y,
                                           assignment_allocation_predictions assignYtoX)
      : m_x{x}
      , m_y{y}
      , m_Assign_y_to_x{assignYtoX}
    {}

    template<top_level Level=TopLevel>
      requires (Level == top_level::no)
    constexpr basic_allocation_predictions(copy_prediction x,
                                           individual_allocation_predictions y,
                                           assignment_allocation_predictions assignYtoX,
                                           container_counts counts)
      : container_predictions_extension_policy<TopLevel>{counts}
      , m_x{x}
      , m_y{y}
      , m_Assign_y_to_x{assignYtoX}
    {}

    [[nodiscard]]
    constexpr para_move_prediction para_move_allocs() const noexcept { return m_y.para_move; }

    [[nodiscard]]
    constexpr copy_like_move_assign_prediction copy_like_move_assign_allocs() const noexcept
    {
      return assign_y_to_x().copy_like_move;
    }

    [[nodiscard]]
    constexpr move_assign_prediction move_assign_allocs() const noexcept
    {
      return assign_y_to_x().move;
    }

    [[nodiscard]]
    constexpr mutation_prediction mutation_allocs() const noexcept { return m_y.mutation; }

    [[nodiscard]]
    constexpr move_prediction move_allocs() const noexcept { return m_y.move; }

    [[nodiscard]]
    constexpr assignment_allocation_predictions assign_y_to_x() const noexcept { return m_Assign_y_to_x; }

    [[nodiscard]]
    constexpr copy_prediction x() const noexcept { return m_x; }

    [[nodiscard]]
    constexpr individual_allocation_predictions y() const noexcept { return m_y; }

    template<class T>
    constexpr basic_allocation_predictions shift(const alloc_prediction_shifter<T>& shifter) const
    {
      auto shifted{*this};

      shifted.m_x             = shifter.shift(m_x, container_tag::x);
      shifted.m_y             = testing::shift(m_y, shifter);
      shifted.m_Assign_y_to_x = testing::shift(m_Assign_y_to_x, shifter);

      return shifted;
    }

    constexpr explicit operator basic_allocation_predictions<top_level::yes>() const noexcept
    {
      return {x(), y(), assign_y_to_x()};
    }

  private:
    copy_prediction m_x{};
    individual_allocation_predictions m_y;
    assignment_allocation_predictions m_Assign_y_to_x;
  };

  using allocation_predictions       = basic_allocation_predictions<top_level::yes>;
  using inner_allocation_predictions = basic_allocation_predictions<top_level::no>;

  template<class T>
  constexpr allocation_predictions shift(const allocation_predictions& predictions)
  {
    const alloc_prediction_shifter<T> shifter{{1_containers, 1_containers, 0_postmutation}, top_level::yes};
    return predictions.shift(shifter);
  }

  template<class T>
  constexpr inner_allocation_predictions shift(const inner_allocation_predictions& predictions)
  {
    const alloc_prediction_shifter<T> shifter{predictions.containers(), top_level::no};
    return predictions.shift(shifter);
  }

  template<pseudoregular T>
  struct type_to_allocation_predictions<T>
  {
    using predictions_type = allocation_predictions;
    using inner_predictions_type = inner_allocation_predictions;
  };

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
