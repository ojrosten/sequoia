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
  struct value_tester<object::filter_by_names<ItemsKeyType, ItemProjector, Compare>>
  {
    using type                   = object::filter_by_names<ItemsKeyType, ItemProjector, Compare>;
    using equivalent_suites_type = typename type::suites_map_type;
    using equivalent_items_type  = typename type::items_map_type;

    template<test_mode Mode>
    static void test(equivalence_check_t, test_logger<Mode>& logger, const type& data, const equivalent_suites_type& selectedSuites, const equivalent_items_type& selectedItems)
    {
      check(equality, "Selected Suites", logger, data.begin_selected_suites(), data.end_selected_suites(), selectedSuites.begin(), selectedSuites.end());
      check(equality, "Selected Items", logger, data.begin_selected_items(), data.end_selected_items(), selectedItems.begin(), selectedItems.end());
    }
  };
}