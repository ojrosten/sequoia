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
      , para_copy{convert<construction_allocation_event::para_copy>(copyPrediction)}
      , para_move{convert<construction_allocation_event::para_move>(copyPrediction)}
    {}

    constexpr individual_allocation_predictions(copy_prediction copyPrediction,
                                                mutation_prediction mutationPrediction,
                                                para_copy_prediction paraCopyPrediction)
      : copy{copyPrediction}
      , mutation{mutationPrediction}
      , para_copy{paraCopyPrediction}
      , para_move{convert<construction_allocation_event::para_move>(copyPrediction)}
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
    constexpr assignment_allocation_predictions(assign_no_prop_prediction withoutPropagation, assign_prediction withPropagation)
      : without_propagation{withoutPropagation}
      , with_propagation{withPropagation}
      , move_without_propagation{convert<assignment_allocation_event::move_assign_no_prop>(without_propagation)}
    {}

    constexpr assignment_allocation_predictions(assign_no_prop_prediction withoutPropagation,
                                                assign_prediction withPropagation,
                                                move_assign_no_prop_prediction moveWithoutPropagation,
                                                move_assign_prediction pureMove)
      : without_propagation{withoutPropagation}
      , with_propagation{withPropagation}
      , move_without_propagation{moveWithoutPropagation}
      , move{pureMove}
    {}

    assign_no_prop_prediction without_propagation{};
    assign_prediction with_propagation{};
    move_assign_no_prop_prediction move_without_propagation{};
    move_assign_prediction move{};
  };


  template<class T>
  [[nodiscard]]
  constexpr individual_allocation_predictions shift(const individual_allocation_predictions& predictions,
                                                    const alloc_prediction_shifter<T>& shifter)
  {
    return {shifter.shift(predictions.copy, container_tag::y),
            shifter.shift(predictions.mutation),
            shifter.shift(predictions.para_copy),
            shifter.shift(predictions.para_move, container_tag::y),
            shifter.shift(predictions.move)};
  }

  template<class T>
  [[nodiscard]]
  constexpr assignment_allocation_predictions shift(const assignment_allocation_predictions& predictions,
                                                    const alloc_prediction_shifter<T>& shifter)
  {
    return {shifter.shift(predictions.without_propagation),
            shifter.shift(predictions.with_propagation),
            shifter.shift(predictions.move_without_propagation),
            shifter.shift(predictions.move)};
  }

  template<top_level TopLevel>
  class basic_allocation_predictions : public container_predictions_policy<TopLevel>
  {
  public:
    constexpr basic_allocation_predictions(copy_prediction x,
                                           individual_allocation_predictions y,
                                           assignment_allocation_predictions assignYtoX)
      requires (TopLevel == top_level::yes)
      : m_x{x}
      , m_y{y}
      , m_Assign_y_to_x{assignYtoX}
    {}

    constexpr basic_allocation_predictions(copy_prediction x,
                                           individual_allocation_predictions y,
                                           assignment_allocation_predictions assignYtoX,
                                           container_counts counts)

      requires (TopLevel == top_level::no)
      : container_predictions_policy<TopLevel>{counts}
      , m_x{x}
      , m_y{y}
      , m_Assign_y_to_x{assignYtoX}
    {}

    [[nodiscard]]
    constexpr para_move_prediction para_move_y_allocs() const noexcept { return m_y.para_move; }

    [[nodiscard]]
    constexpr move_assign_no_prop_prediction move_assign_no_prop_allocs() const noexcept
    {
      return m_Assign_y_to_x.move_without_propagation;
    }

    [[nodiscard]]
    constexpr move_assign_prediction move_assign_allocs() const noexcept
    {
      return m_Assign_y_to_x.move;
    }

    [[nodiscard]]
    constexpr mutation_prediction mutation_allocs() const noexcept { return m_y.mutation; }

    [[nodiscard]]
    constexpr move_prediction move_allocs() const noexcept { return m_y.move; }

    [[nodiscard]]
    constexpr copy_prediction x() const noexcept { return m_x; }

    [[nodiscard]]
    constexpr const individual_allocation_predictions& y() const noexcept { return m_y; }

    [[nodiscard]]
    constexpr const assignment_allocation_predictions& assign_y_to_x() const noexcept { return m_Assign_y_to_x; }

    template<class T>
    [[nodiscard]]
    constexpr basic_allocation_predictions shift(const alloc_prediction_shifter<T>& shifter) const
    {
      auto shifted{*this};

      shifted.m_x             = shifter.shift(m_x, container_tag::x);
      shifted.m_y             = testing::shift(m_y, shifter);
      shifted.m_Assign_y_to_x = testing::shift(m_Assign_y_to_x, shifter);

      return shifted;
    }
  private:
    copy_prediction m_x{};
    individual_allocation_predictions m_y;
    assignment_allocation_predictions m_Assign_y_to_x;
  };

  [[nodiscard]]
  constexpr basic_allocation_predictions<top_level::yes>
    to_top_level(const basic_allocation_predictions<top_level::no>& predictions) noexcept
  {
    return {predictions.x(), predictions.y(), predictions.assign_y_to_x()};
  }

  using allocation_predictions       = basic_allocation_predictions<top_level::yes>;
  using inner_allocation_predictions = basic_allocation_predictions<top_level::no>;

  template<class T>
  [[nodiscard]]
  constexpr allocation_predictions shift(const allocation_predictions& predictions)
  {
    const alloc_prediction_shifter<T> shifter{{1_containers, 1_containers, 0_postmutation}, top_level::yes};
    return predictions.shift(shifter);
  }

  template<class T>
  [[nodiscard]]
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

  template<test_mode Mode, pseudoregular T, std::invocable<T&> Mutator, alloc_getter<T>... Getters>
    requires (!std::totally_ordered<T> && (sizeof...(Getters) > 0))
  void check_semantics(std::string description, test_logger<Mode>& logger, const T& x, const T& y, Mutator yMutator, const allocation_info<T, Getters>&... info)
  {
    impl::check_semantics(std::move(description), logger, impl::regular_allocation_actions<T>{}, x, y, std::move(yMutator), info...);
  }

  template<test_mode Mode, pseudoregular T, std::invocable<T&> Mutator, alloc_getter<T>... Getters>
    requires (std::totally_ordered<T> && (sizeof...(Getters) > 0))
  void check_semantics(std::string description, test_logger<Mode>& logger, const T& x, const T& y, std::weak_ordering order, Mutator yMutator, const allocation_info<T, Getters>&... info)
  {
    impl::check_semantics(std::move(description), logger, impl::regular_allocation_actions<T>{order}, x, y, std::move(yMutator), info...);
  }

  template
  <
    test_mode Mode,
    pseudoregular T,
    invocable_r<T> xMaker,
    invocable_r<T> yMaker,
    std::invocable<T&> Mutator,
    alloc_getter<T>... Getters
  >
    requires (!std::totally_ordered<T> && sizeof...(Getters) > 0)
  std::pair<T, T> check_semantics(std::string description, test_logger<Mode>& logger, xMaker xFn, yMaker yFn, Mutator yMutator, const allocation_info<T, Getters>&... info)
  {
    return impl::check_semantics(std::move(description), logger, impl::regular_allocation_actions<T>{}, std::move(xFn), std::move(yFn), std::move(yMutator), info...);
  }

  template
  <
    test_mode Mode,
    pseudoregular T,
    invocable_r<T> xMaker,
    invocable_r<T> yMaker,
    std::invocable<T&> Mutator,
    alloc_getter<T>... Getters
  >
    requires (std::totally_ordered<T> && sizeof...(Getters) > 0)
  std::pair<T, T> check_semantics(std::string description, test_logger<Mode>& logger, xMaker xFn, yMaker yFn, std::weak_ordering order, Mutator yMutator, const allocation_info<T, Getters>&... info)
  {
    return impl::check_semantics(std::move(description), logger, impl::regular_allocation_actions<T>{order}, std::move(xFn), std::move(yFn), std::move(yMutator), info...);
  }
}
