////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file RegularAllocationCheckersDetails.hpp
    \brief Implementation details for allocation checks within the unit testing framework.
*/

#include "AllocationCheckersDetails.hpp"

namespace sequoia::unit_testing
{
  struct allocation_predictions;
}

namespace sequoia::unit_testing::impl
{
  template<class Logger, class Container, class Mutator, class... Allocators, class... Predictions>
  void check_para_constructor_allocations(std::string_view description, Logger& logger, const Container& y, Mutator yMutator, const basic_allocation_info<Container, Allocators, Predictions>&... info)
  {    
    if constexpr(sizeof...(Allocators) > 0)
    {
      auto make{
        [description, &logger, &y](auto&... info){
          Container u{y, info.make_allocator()...};
          check_equality(combine_messages(description, "Copy-like construction"), logger, u, y);
          check_para_copy_y_allocation(description, logger, u, std::tuple_cat(make_allocation_checkers(info)...));

          return u;
        }
      };

      Container v{make(info...), info.make_allocator()...};

      check_equality(combine_messages(description, "Move-like construction"), logger, v, y);    
      check_para_move_y_allocation(description, logger, v, std::tuple_cat(make_allocation_checkers(info)...));
      check_mutation_after_move(description, "allocation assignment", logger, v, y, yMutator, std::tuple_cat(make_allocation_checkers(info, v, 0)...));
    }
  }

  template<class Logger, class Actions, class T, class Mutator, class... Allocators, class... Predictions>
  bool check_semantics(std::string_view description, Logger& logger, const Actions& actions, const T& x, const T& y, Mutator yMutator, std::tuple<allocation_checker<T, Allocators, Predictions>...> checkers)
  {
    auto fn{
      [description,&logger,&actions,&x,&y,m=std::move(yMutator)](auto&&... checkers){
        return impl::check_semantics(description, logger, actions, x, y, m, std::forward<decltype(checkers)>(checkers)...);
      }
    };

    return unpack_invoke(checkers, fn);
  }
}
