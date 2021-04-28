////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Implementation details specific to allocation checks for move-only types.
*/

#include "sequoia/TestFramework/AllocationCheckersDetails.hpp"
#include "sequoia/TestFramework/MoveOnlyCheckersDetails.hpp"

#include <optional>

namespace sequoia::testing::impl
{
  template<moveonly T>
  struct move_only_allocation_actions : allocation_actions<T>
  {
    using allocation_actions<T>::allocation_actions;

    template<test_mode Mode, alloc_getter<T>... Getters>
    [[nodiscard]]
    bool check_preconditions(test_logger<Mode>& logger, const T& x, const T& y, const T& xClone, const T& yClone, const dual_allocation_checker<T, Getters>&... checkers) const
    {
      return allocation_actions<T>::check_preconditions(logger, *this, x, y, xClone, yClone, dual_allocation_checker{checkers.info(), x, y}...);
    }
  };

  template<test_mode Mode, class Actions, moveonly T, alloc_getter<T>... Getters>
  bool check_swap(test_logger<Mode>& logger, const Actions& actions, T&& x, T&& y, const T& xClone, const T& yClone, const dual_allocation_checker<T, Getters>&... checkers)
  {
    return do_check_swap(logger, actions, std::move(x), std::move(y), xClone, yClone, dual_allocation_checker{checkers.info(), x, y}...);
  }

  template<test_mode Mode, container_tag tag, moveonly T, alloc_getter<T>... Getters>
  std::optional<T>
  check_para_constructor_allocations(test_logger<Mode>& logger,
                                     container_tag_constant<tag>,
                                     T&& z,
                                     const T& zClone,
                                     const allocation_info<T, Getters>&... info)
  {
    const auto tagStr{to_string(container_tag_constant<tag>::value)};

    if(!check(std::string{"Precondition - for checking move-only semantics, "}.append(tagStr).append("and ").append(tagStr).append("Clone are assumed to be equal"),
              logger,
              z == zClone))
      return{};

    T v{std::move(z), info.make_allocator()...};

    using ctag = container_tag_constant<tag>;
    check_para_move_allocation(logger, ctag{}, v, std::tuple_cat(make_allocation_checkers(info)...));
    if(check_equality("Inonsistent para-move constructor", logger, v, zClone))
    {
      std::optional<T> w{std::move(v)};
      if(check_equality("Inconsistent move construction", logger, *w, zClone))
      {
        return std::move(w);
      }
    }

    return std::nullopt;
  }

  template<test_mode Mode, moveonly T, alloc_getter<T>... Getters>
  std::pair<std::optional<T>, std::optional<T>>
  check_para_constructor_allocations(test_logger<Mode>& logger,
                                     T&& x,
                                     T&& y,
                                     const T& xClone,
                                     const T& yClone,
                                     const allocation_info<T, Getters>&... info)
  {
    return {check_para_constructor_allocations(logger, container_tag_constant<container_tag::x>{}, std::forward<T>(x), xClone, info...),
            check_para_constructor_allocations(logger, container_tag_constant<container_tag::y>{}, std::forward<T>(y), yClone, info...)};
  }

  template<test_mode Mode, class Actions, moveonly T, invocable<T&> Mutator, alloc_getter<T>... Getters>
  void check_semantics(std::string_view description, test_logger<Mode>& logger, const Actions& actions, T&& x, T&& y, const T& xClone, const T& yClone, Mutator m, const allocation_info<T, Getters>&... info)
  {
    sentinel<Mode> sentry{logger, add_type_info<T>(description).append("\n")};

    if(auto[optx,opty]{check_para_constructor_allocations(logger, std::forward<T>(x), std::forward<T>(y), xClone, yClone, info...)}; (optx != std::nullopt) && (opty != std::nullopt))
    {
      check_semantics(logger, actions, std::move(*optx), std::move(*opty), xClone, yClone, std::move(m), std::tuple_cat(make_dual_allocation_checkers(info, x, y)...));
    }
  }

  template
  <
    test_mode Mode,
    class Actions,
    invocable<> xMaker,
    moveonly T=std::invoke_result_t<xMaker>,
    invocable_r<T> yMaker,
    invocable<T&> Mutator,
    alloc_getter<T>... Getters
  >
  std::pair<T,T> check_semantics(std::string_view description, test_logger<Mode>& logger, const Actions& actions, xMaker xFn, yMaker yFn, Mutator m, const allocation_info<T, Getters>&... info)
  {
    sentinel<Mode> sentry{logger, add_type_info<T>(description).append("\n")};

    auto x{xFn()};
    auto y{yFn()};

    impl::check_initialization_allocations(logger, x, y, info...);
    check_semantics(description, logger, actions, xFn(), yFn(), x, y, std::move(m), info...);

    return {std::move(x), std::move(y)};
  }

  /// Unpacks the tuple and feeds to the overload of check_semantics defined in MoveOnlyCheckersDetails.hpp
  template<test_mode Mode, class Actions, moveonly T, invocable<T&> Mutator, alloc_getter<T>... Getters>
  void check_semantics(test_logger<Mode>& logger, const Actions& actions, T&& x, T&& y, const T& xClone, const T& yClone, Mutator m, std::tuple<dual_allocation_checker<T, Getters>...> checkers)
  {
    auto fn{
      [&logger, &actions, &x, &y, &xClone, &yClone, m{std::move(m)}](auto&&... checkers){
        return check_semantics(logger, actions, std::forward<T>(x), std::forward<T>(y), xClone, yClone, std::move(m), std::forward<decltype(checkers)>(checkers)...);
      }
    };

    std::apply(fn, std::move(checkers));
  }
}
