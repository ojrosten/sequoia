////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2025.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/RegularTestCore.hpp"
#include "sequoia/Core/DataStructures/MemOrderedTuple.hpp"

namespace sequoia::testing
{
  template<class... Ts>
  struct value_tester<mem_ordered_tuple<Ts...>>
  {
    using type = mem_ordered_tuple<Ts...>;

    template<test_mode Mode>
    static void test(equality_check_t, test_logger<Mode>& logger, const type& actual, const type& prediction)
    {
      [&]<std::size_t... Is> (std::index_sequence<Is...>) {
        (check(equality, std::format("Element {}", Is), logger, get<Is>(actual), get<Is>(prediction)), ...);
      }(std::make_index_sequence<sizeof...(Ts)>{});
    }
    
    template<test_mode Mode>
    static void test(equivalence_check_t, test_logger<Mode>& logger, const type& actual, const std::tuple<Ts...>& prediction)
    {
      [&]<std::size_t... Is> (std::index_sequence<Is...>) {
        (check(equality, std::format("Element {}", Is), logger, get<Is>(actual), std::get<Is>(prediction)), ...);
      }(std::make_index_sequence<sizeof...(Ts)>{});
    }
  };
}
