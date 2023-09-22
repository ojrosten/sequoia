////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */


#include "sequoia/TestFramework/RegularTestCore.hpp"
#include "sequoia/Core/Object/Suite.hpp"

namespace sequoia::testing
{
  template<class ItemsKeyType, class ItemProjector, class Compare>
  struct value_tester<object::granular_filter<ItemsKeyType, ItemProjector, Compare>>
  {
    using type                   = object::granular_filter<ItemsKeyType, ItemProjector, Compare>;
    using equivalent_suites_type = std::optional<typename type::suites_map_type>;
    using equivalent_items_type  = std::optional<typename type::items_map_type>;

    template<test_mode Mode>
    static void test(equivalence_check_t, test_logger<Mode>& logger, const type& data, const equivalent_suites_type& selectedSuites, const equivalent_items_type& selectedItems)
    {
      if(data.selected_suites() && selectedSuites)
      {
        check(equality, "Selected Suites", logger, data.selected_suites()->begin(), data.selected_suites()->end(), selectedSuites->begin(), selectedSuites->end());
      }
      else
      {
        check(equality, "Selected Suites", logger, static_cast<bool>(data.selected_suites()), static_cast<bool>(selectedSuites));
      }

      if(data.selected_items() && selectedItems)
      {
        check(equality, "Selected Items", logger, data.selected_items()->begin(), data.selected_items()->end(), selectedItems->begin(), selectedItems->end());
      }
      else
      {
        check(equality, "Selected Items", logger, static_cast<bool>(data.selected_items()), static_cast<bool>(selectedItems));
      }
    }
  };
}