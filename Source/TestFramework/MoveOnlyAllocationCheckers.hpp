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
  public:
    constexpr move_only_allocation_predictions(copy_like_move_assign_prediction copyLikeMove,
                                               mutation_prediction yMutation,
                                               para_move_prediction paraMove,
                                               move_prediction m,
                                               move_assign_prediction moveAssign,
                                               combined_container_data data = top_level_spec)
      : m_CopyLikeMoveAssign{copyLikeMove}
      , m_Mutation{yMutation}
      , m_ParaMove{paraMove}
      , m_Move{m}
      , m_MoveAssign{moveAssign}
      , m_Containers{data}
    {}

    constexpr move_only_allocation_predictions(copy_like_move_assign_prediction copyLikeMove,
                                               mutation_prediction yMutation,
                                               para_move_prediction paraMove,
                                               combined_container_data data = top_level_spec)
      : m_CopyLikeMoveAssign{copyLikeMove}
      , m_Mutation{yMutation}
      , m_ParaMove{paraMove}
      , m_Containers{data}
    {}

    constexpr move_only_allocation_predictions(copy_like_move_assign_prediction copyLikeMove,
                                               mutation_prediction yMutation,
                                               para_move_prediction paraMove,
                                               container_counts counts)
      : m_CopyLikeMoveAssign{copyLikeMove}
      , m_Mutation{yMutation}
      , m_ParaMove{paraMove}
      , m_Containers{counts, top_level::no}
    {}

    [[nodiscard]]
    constexpr para_move_prediction para_move_allocs() const noexcept { return m_ParaMove; }

    [[nodiscard]]
    constexpr copy_like_move_assign_prediction copy_like_move_assign_allocs() const noexcept { return m_CopyLikeMoveAssign; }

    [[nodiscard]]
    constexpr move_assign_prediction move_assign_allocs() const noexcept { return m_MoveAssign; }

    [[nodiscard]]
    constexpr mutation_prediction mutation_allocs() const noexcept { return m_Mutation; }

    [[nodiscard]]
    constexpr move_prediction move_allocs() const noexcept { return m_Move; }

    [[nodiscard]]
    constexpr combined_container_data containers() const noexcept { return  m_Containers; }
  private:
    copy_like_move_assign_prediction m_CopyLikeMoveAssign{};
    mutation_prediction m_Mutation{};
    para_move_prediction m_ParaMove{};
    move_prediction m_Move{};
    move_assign_prediction m_MoveAssign{};
    combined_container_data m_Containers;
  };

  template<class T>
  constexpr move_only_allocation_predictions shift(const move_only_allocation_predictions& predictions)
  {
    using shifter = alloc_prediction_shifter<T>;
    const auto& containers{predictions.containers()};
    return {shifter::shift(predictions.copy_like_move_assign_allocs(), containers.x, containers.y),
            shifter::shift(predictions.mutation_allocs(),    containers.y, containers.y_post_mutation),
            shifter::shift(predictions.para_move_allocs(),   containers.y),
            shifter::shift(predictions.move_allocs(),        containers.y),
            shifter::shift(predictions.move_assign_allocs(), containers.y),
            containers};
  }

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
