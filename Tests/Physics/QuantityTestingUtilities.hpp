////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/RegularTestCore.hpp"
#include "sequoia/Physics/Quantities.hpp"

namespace sequoia::testing
{
  template<maths::convex_space QuantitySpace, physics::quantity_unit Unit>
  struct value_tester<physics::quantity<QuantitySpace, Unit>>
  {
    using type       = physics::quantity<QuantitySpace, Unit>;
    using value_type = typename physics::quantity<QuantitySpace, Unit>::value_type;

    template<test_mode Mode>
    static void test(equality_check_t, test_logger<Mode>& logger, const type& actual, const type& prediction)
    {
      check(equality, "Wrapped Value", logger, actual.value(), prediction.value());
    }
    
    template<test_mode Mode>
    static void test(equivalence_check_t, test_logger<Mode>& logger, const type& actual, const value_type& prediction)
    {
      check(equality, "Wrapped Value", logger, actual.value(), prediction);
    }
  };
}
