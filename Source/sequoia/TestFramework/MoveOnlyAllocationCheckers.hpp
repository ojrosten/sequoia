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

#include "sequoia/TestFramework/AllocationCheckers.hpp"
#include "sequoia/TestFramework/MoveOnlyAllocationCheckersDetails.hpp"

namespace sequoia::testing
{
  struct individual_move_only_allocation_predictions
  {
    constexpr individual_move_only_allocation_predictions(para_move_prediction paraMovePrediction,
                                                          mutation_prediction mutationPrediction,
                                                          move_prediction m={})
      : para_move{paraMovePrediction}
      , mutation{mutationPrediction}
      , move{m}
    {}

    para_move_prediction para_move{};
    mutation_prediction mutation{};
    move_prediction move{};
  };

  struct assignment_move_only_allocation_predictions
  {
    constexpr assignment_move_only_allocation_predictions(move_assign_no_prop_prediction moveWithoutPropagation,
                                                          move_assign_prediction pureMove={})
      : move_without_propagation{moveWithoutPropagation}
      , move{pureMove}
    {}

    move_assign_no_prop_prediction move_without_propagation{};
    move_assign_prediction move{};
  };

  template<class T>
  [[nodiscard]]
  constexpr individual_move_only_allocation_predictions shift(const individual_move_only_allocation_predictions& predictions,
                                                              const alloc_prediction_shifter<T>& shifter)
  {
    return {shifter.shift(predictions.para_move, container_tag::y),
            shifter.shift(predictions.mutation),
            shifter.shift(predictions.move)};
  }

  template<class T>
  [[nodiscard]]
  constexpr assignment_move_only_allocation_predictions shift(const assignment_move_only_allocation_predictions& predictions,
                                                             const alloc_prediction_shifter<T>& shifter)
  {
    return {shifter.shift(predictions.move_without_propagation),
            shifter.shift(predictions.move)};
  }

  template<top_level TopLevel>
  class basic_move_only_allocation_predictions : public container_predictions_policy<TopLevel>
  {
  public:
    constexpr basic_move_only_allocation_predictions(para_move_prediction x,
                                                     individual_move_only_allocation_predictions y,
                                                     assignment_move_only_allocation_predictions assignYtoX)

      requires (TopLevel == top_level::yes)
      : m_x{x}
      , m_y{y}
      , m_Assign_y_to_x{assignYtoX}
    {}

    constexpr basic_move_only_allocation_predictions(para_move_prediction x,
                                                     individual_move_only_allocation_predictions y,
                                                     assignment_move_only_allocation_predictions assignYtoX,
                                                     container_counts counts)
      requires (TopLevel == top_level::no)
      : container_predictions_policy<TopLevel>{counts}
      , m_x{x}
      , m_y{y}
      , m_Assign_y_to_x{assignYtoX}
    {}

    [[nodiscard]]
    constexpr mutation_prediction mutation_allocs() const noexcept { return m_y.mutation; }

    [[nodiscard]]
    constexpr para_move_prediction para_move_x_allocs() const noexcept { return m_x; }

    [[nodiscard]]
    constexpr para_move_prediction para_move_y_allocs() const noexcept { return m_y.para_move; }

    [[nodiscard]]
    constexpr move_prediction move_allocs() const noexcept { return m_y.move; }

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
    constexpr para_move_prediction x() const noexcept { return m_x; }

    [[nodiscard]]
    constexpr const individual_move_only_allocation_predictions& y() const noexcept { return m_y; }

    [[nodiscard]]
    constexpr const assignment_move_only_allocation_predictions& assign_y_to_x() const noexcept { return m_Assign_y_to_x; }

    template<class T>
    [[nodiscard]]
    constexpr basic_move_only_allocation_predictions shift(const alloc_prediction_shifter<T>& shifter) const
    {
      auto shifted{*this};

      shifted.m_x             = shifter.shift(m_x, container_tag::x);
      shifted.m_y             = testing::shift(m_y, shifter);
      shifted.m_Assign_y_to_x = testing::shift(m_Assign_y_to_x, shifter);

      return shifted;
    }
  private:
    para_move_prediction m_x{};
    individual_move_only_allocation_predictions m_y;
    assignment_move_only_allocation_predictions m_Assign_y_to_x;
  };

  [[nodiscard]]
  constexpr basic_move_only_allocation_predictions<top_level::yes>
    to_top_level(const basic_move_only_allocation_predictions<top_level::no>& predictions) noexcept
  {
    return {predictions.x(), predictions.y(), predictions.assign_y_to_x()};
  }

  using move_only_allocation_predictions       = basic_move_only_allocation_predictions<top_level::yes>;
  using move_only_inner_allocation_predictions = basic_move_only_allocation_predictions<top_level::no>;

  template<class T>
  constexpr move_only_allocation_predictions shift(const move_only_allocation_predictions& predictions)
  {
    const alloc_prediction_shifter<T> shifter{{1_containers, 1_containers, 0_postmutation}, top_level::yes};
    return predictions.shift(shifter);
  }

  template<class T>
  [[nodiscard]]
  constexpr move_only_inner_allocation_predictions shift(const move_only_inner_allocation_predictions& predictions)
  {
    const alloc_prediction_shifter<T> shifter{predictions.containers(), top_level::no};
    return predictions.shift(shifter);
  }

  template<moveonly T>
  struct type_to_allocation_predictions<T>
  {
    using predictions_type = move_only_allocation_predictions;
    using inner_predictions_type = move_only_inner_allocation_predictions;
  };

  template<test_mode Mode, moveonly T, class U, std::invocable<T&> Mutator, alloc_getter<T>... Getters>
    requires (checkable_against<Mode, T, U> && !std::totally_ordered<T> && (sizeof...(Getters) > 0))
  void check_semantics(std::string description,
                       test_logger<Mode>& logger,
                       T&& x,
                       T&& y,
                       const U& xEquivalent,
                       const U& yEquivalent,
                       optional_ref<const U> movedFromPostConstruction,
                       optional_ref<const U> movedFromPostAssignment,
                       Mutator m,
                       const allocation_info<T, Getters>&... info)
  {
    impl::check_semantics(std::move(description),
                          logger,
                          impl::move_only_allocation_actions<T>{},
                          std::forward<T>(x),
                          std::forward<T>(y),
                          xEquivalent,
                          yEquivalent,
                          movedFromPostConstruction,
                          movedFromPostAssignment,
                          std::move(m),
                          info...);
  }

  template<test_mode Mode, moveonly T, class U, std::invocable<T&> Mutator, alloc_getter<T>... Getters>
    requires (checkable_against<Mode, T, U> && std::totally_ordered<T> && (sizeof...(Getters) > 0))
  void check_semantics(std::string description,
                       test_logger<Mode>& logger,
                       T&& x,
                       T&& y,
                       const U& xEquivalent,
                       const U& yEquivalent,
                       optional_ref<const U> movedFromPostConstruction,
                       optional_ref<const U> movedFromPostAssignment,
                       std::weak_ordering order,
                       Mutator m,
                       const allocation_info<T, Getters>&... info)
  {
    impl::check_semantics(std::move(description),
                          logger,
                          impl::move_only_allocation_actions<T>{order},
                          std::forward<T>(x),
                          std::forward<T>(y),
                          xEquivalent,
                          yEquivalent,
                          movedFromPostConstruction,
                          movedFromPostAssignment,
                          std::move(m),
                          info...);
  }

  template
  <
    test_mode Mode,
    moveonly T,
    regular_invocable_r<T> xMaker,
    regular_invocable_r<T> yMaker,
    std::invocable<T&> Mutator,
    alloc_getter<T>... Getters
  >
    requires (!std::totally_ordered<T> && (sizeof...(Getters) > 0))
  std::pair<T,T> check_semantics(std::string description,
                                 test_logger<Mode>& logger,
                                 xMaker xFn,
                                 yMaker yFn,
                                 optional_ref<const T> movedFromPostConstruction,
                                 optional_ref<const T> movedFromPostAssignment,
                                 Mutator m,
                                 const allocation_info<T, Getters>&... info)
  {
    return impl::check_semantics(std::move(description),
                                 logger,
                                 impl::move_only_allocation_actions<T>{},
                                 std::move(xFn),
                                 std::move(yFn),
                                 movedFromPostConstruction,
                                 movedFromPostAssignment,
                                 std::move(m),
                                 info...);
  }

  template
  <
    test_mode Mode,
    moveonly T,
    regular_invocable_r<T> xMaker,
    regular_invocable_r<T> yMaker,
    std::invocable<T&> Mutator,
    alloc_getter<T>... Getters
  >
    requires (std::totally_ordered<T> && (sizeof...(Getters) > 0))
  std::pair<T,T> check_semantics(std::string description,
                                 test_logger<Mode>& logger,
                                 xMaker xFn,
                                 yMaker yFn,
                                 optional_ref<const T> movedFromPostConstruction,
                                 optional_ref<const T> movedFromPostAssignment,
                                 std::weak_ordering order,
                                 Mutator m,
                                 const allocation_info<T, Getters>&... info)
  {
    return impl::check_semantics(std::move(description),
                                 logger,
                                 impl::move_only_allocation_actions<T>{order},
                                 std::move(xFn),
                                 std::move(yFn),
                                 movedFromPostConstruction,
                                 movedFromPostAssignment,
                                 std::move(m),
                                 info...);
  }
}
