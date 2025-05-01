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

    using equivalent_type = std::pair<equivalent_suites_type, equivalent_items_type>;

    template<test_mode Mode>
    static void test(equivalence_check_t, test_logger<Mode>& logger, const type& data, const equivalent_type& prediction)
    {
      if(data.selected_suites() && prediction.first)
      {
        check(equality, "Selected Suites", logger, data.selected_suites()->begin(), data.selected_suites()->end(), prediction.first->begin(), prediction.first->end());
      }
      else
      {
        check(equality, "Selected Suites", logger, static_cast<bool>(data.selected_suites()), static_cast<bool>(prediction.first));
      }

      if(data.selected_items() && prediction.second)
      {
        check(equality, "Selected Items", logger, data.selected_items()->begin(), data.selected_items()->end(), prediction.second->begin(), prediction.second->end());
      }
      else
      {
        check(equality, "Selected Items", logger, static_cast<bool>(data.selected_items()), static_cast<bool>(prediction.second));
      }
    }
  };
}
