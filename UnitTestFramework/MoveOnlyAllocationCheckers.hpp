////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file MoveOnlyCheckers.hpp
    \brief Utilties for performing allocation checks within the unit testing framework.
*/

#include "UnitTestCheckers.hpp"
#include "AllocationCheckers.hpp"
#include "MoveOnlyAllocationCheckersDetails.hpp"

namespace sequoia::unit_testing
{
  struct move_only_allocation_predictions
  {
    move_only_allocation_predictions(int paraMove, int assignWithoutPropagation)
      : para_move{paraMove}
      , assign_without_propagation{assignWithoutPropagation}
    {}

    [[nodiscard]]
    int para_move_allocs() const noexcept { return para_move; }

    [[nodiscard]]
    int assign_without_propagation_allocs() const noexcept { return assign_without_propagation; }
    
    int para_move{}, assign_without_propagation{};
  };

  // Done through inheritance rather than a using declaration
  // in order to make use of CTAD. Should be able to revert
  // to using in C++20...
    
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
  
  template<class Logger, class T, class... Allocators>
  void check_regular_semantics(std::string_view description, Logger& logger, T&& x, T&& y, const T& xClone, const T& yClone, move_only_allocation_info<T, Allocators>... info)
  {
    typename Logger::sentinel s{logger, add_type_info<T>(description)};

    if(impl::check_regular_semantics(description, logger, impl::move_only_allocation_actions{}, std::forward<T>(x), std::forward<T>(y), xClone, yClone, std::tuple_cat(impl::make_allocation_checkers(info, x, y)...)))
    {
      impl::check_para_constructor_allocations(description, logger, std::forward<T>(y), yClone, info...);
    }
  }
}
