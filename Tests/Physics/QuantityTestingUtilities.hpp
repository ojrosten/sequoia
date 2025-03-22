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
  template<maths::convex_space PhysicalValueSpace, physics::physical_unit Unit, class Origin, class Validator>
  struct value_tester<physics::physical_value<PhysicalValueSpace, Unit, Origin, Validator>>
  {
    using type       = physics::physical_value<PhysicalValueSpace, Unit, Origin, Validator>;
    using value_type = type::value_type;

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
