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
  template<top_level TopLevel>
  class basic_move_only_allocation_predictions : public container_predictions_extension_policy<TopLevel>
  {
  public:
    template<top_level Level = TopLevel>
      requires (Level == top_level::yes)
    constexpr basic_move_only_allocation_predictions(copy_like_move_assign_prediction copyLikeMove,
                                               mutation_prediction yMutation,
                                               para_move_prediction paraMove,
                                               move_prediction m,
                                               move_assign_prediction moveAssign)
      : m_CopyLikeMoveAssign{copyLikeMove}
      , m_Mutation{yMutation}
      , m_ParaMove{paraMove}
      , m_Move{m}
      , m_MoveAssign{moveAssign}
    {}

    template<top_level Level = TopLevel>
      requires (Level == top_level::yes)
    constexpr basic_move_only_allocation_predictions(copy_like_move_assign_prediction copyLikeMove,
                                               mutation_prediction yMutation,
                                               para_move_prediction paraMove)
      : m_CopyLikeMoveAssign{copyLikeMove}
      , m_Mutation{yMutation}
      , m_ParaMove{paraMove}
    {}

    template<top_level Level = TopLevel>
      requires (Level == top_level::no)
    constexpr basic_move_only_allocation_predictions(copy_like_move_assign_prediction copyLikeMove,
                                               mutation_prediction yMutation,
                                               para_move_prediction paraMove,
                                               move_prediction m,
                                               move_assign_prediction moveAssign,
                                               container_counts counts)
      : container_predictions_extension_policy<TopLevel>{counts}
      , m_CopyLikeMoveAssign{copyLikeMove}
      , m_Mutation{yMutation}
      , m_ParaMove{paraMove}
      , m_Move{m}
      , m_MoveAssign{moveAssign}
    {}

    template<top_level Level = TopLevel>
      requires (Level == top_level::no)
    constexpr basic_move_only_allocation_predictions(copy_like_move_assign_prediction copyLikeMove,
                                                     mutation_prediction yMutation,
                                                     para_move_prediction paraMove,
                                                     container_counts counts)
      : container_predictions_extension_policy<TopLevel>{counts}
      , m_CopyLikeMoveAssign{copyLikeMove}
      , m_Mutation{yMutation}
      , m_ParaMove{paraMove}
    {}

    [[nodiscard]]
    constexpr copy_like_move_assign_prediction copy_like_move_assign_allocs() const noexcept { return m_CopyLikeMoveAssign; }

    [[nodiscard]]
    constexpr mutation_prediction mutation_allocs() const noexcept { return m_Mutation; }

    [[nodiscard]]
    constexpr para_move_prediction para_move_allocs() const noexcept { return m_ParaMove; }

    [[nodiscard]]
    constexpr move_prediction move_allocs() const noexcept { return m_Move; }

    [[nodiscard]]
    constexpr move_assign_prediction move_assign_allocs() const noexcept { return m_MoveAssign; }

    template<class T>
    constexpr basic_move_only_allocation_predictions shift(const alloc_prediction_shifter<T>& shifter) const
    {
      auto shifted{*this};

      shifted.m_CopyLikeMoveAssign = shifter.shift(m_CopyLikeMoveAssign);
      shifted.m_Mutation           = shifter.shift(m_Mutation);
      shifted.m_ParaMove           = shifter.shift(m_ParaMove);
      shifted.m_Move               = shifter.shift(m_Move);
      shifted.m_MoveAssign         = shifter.shift(m_MoveAssign);

      return shifted;
    }

    constexpr explicit operator basic_move_only_allocation_predictions<top_level::yes>() const noexcept
    {
      return {copy_like_move_assign_allocs(), mutation_allocs(), para_move_allocs(), move_allocs(), move_assign_allocs()};
    }
  private:
    copy_like_move_assign_prediction m_CopyLikeMoveAssign{};
    mutation_prediction m_Mutation{};
    para_move_prediction m_ParaMove{};
    move_prediction m_Move{};
    move_assign_prediction m_MoveAssign{};
  };

  using move_only_allocation_predictions       = basic_move_only_allocation_predictions<top_level::yes>;
  using move_only_inner_allocation_predictions = basic_move_only_allocation_predictions<top_level::no>;

  template<class T>
  constexpr move_only_allocation_predictions shift(const move_only_allocation_predictions& predictions)
  {
    const alloc_prediction_shifter<T> shifter{{1_containers, 1_containers, 1_postmutation}, top_level::yes};
    return predictions.shift(shifter);
  }

  template<class T>
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

    if(auto opt{impl::check_para_constructor_allocations(logger, std::forward<T>(y), yClone, info...)})
    {
      check_semantics(logger, impl::move_only_allocation_actions<T>{}, std::forward<T>(x), std::move(*opt), xClone, yClone, std::move(m), std::tuple_cat(impl::make_dual_allocation_checkers(info, x, y)...));
    }
  }
}
