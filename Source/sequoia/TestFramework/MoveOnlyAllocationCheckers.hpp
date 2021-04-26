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
    constexpr individual_move_only_allocation_predictions(mutation_prediction mutationPrediction,
                                                          para_move_prediction paraMovePrediction,
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
    constexpr assignment_move_only_allocation_predictions(move_assign_no_prop_prediction copyLikeMove,
                                                          move_assign_prediction pureMove={})
      : move_without_propagation{copyLikeMove}
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
    return {shifter.shift(predictions.mutation),
            shifter.shift(predictions.para_move, container_tag::y),
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
    template<top_level Level=TopLevel>
      requires (Level == top_level::yes)
    constexpr basic_move_only_allocation_predictions(para_move_prediction x,
                                                     individual_move_only_allocation_predictions y,
                                                     assignment_move_only_allocation_predictions assignYtoX)
      : m_x{x}
      , m_y{y}
      , m_Assign_y_to_x{assignYtoX}
    {}

    template<top_level Level=TopLevel>
      requires (Level == top_level::no)
    constexpr basic_move_only_allocation_predictions(para_move_prediction x,
                                                     individual_move_only_allocation_predictions y,
                                                     assignment_move_only_allocation_predictions assignYtoX,
                                                     container_counts counts)
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

  template<test_mode Mode, moveonly T, invocable<T&> Mutator, alloc_getter<T>... Getters>
  void check_semantics(std::string_view description, test_logger<Mode>& logger, T&& x, T&& y, const T& xClone, const T& yClone, Mutator m, const allocation_info<T, Getters>&... info)
  {
    sentinel<Mode> sentry{logger, add_type_info<T>(description).append("\n")};

    if(auto[optx,opty]{impl::check_para_constructor_allocations(logger, std::forward<T>(x), std::forward<T>(y), xClone, yClone, info...)}; (optx != std::nullopt) && (opty != std::nullopt))
    {
      check_semantics(logger, impl::move_only_allocation_actions<T>{}, std::move(*optx), std::move(*opty), xClone, yClone, std::move(m), std::tuple_cat(impl::make_dual_allocation_checkers(info, x, y)...));
    }
  }

  template
  <
    test_mode Mode,
    invocable<> xMaker,
    moveonly T=typename function_signature<xMaker>::ret,
    invocable_r<T> yMaker,
    invocable<T&> Mutator,
    alloc_getter<T>... Getters
  >
  std::pair<T,T> check_semantics(std::string_view description, test_logger<Mode>& logger, xMaker xFn, yMaker yFn, Mutator m, const allocation_info<T, Getters>&... info)
  {
    sentinel<Mode> sentry{logger, add_type_info<T>(description).append("\n")};

    auto x{xFn()};
    auto y{yFn()};

    impl::check_initialization_allocations(logger, x, y, info...);
    check_semantics(description, logger, xFn(), yFn(), x, y, std::move(m), info...);

    return {std::move(x), std::move(y)};
  }
}
