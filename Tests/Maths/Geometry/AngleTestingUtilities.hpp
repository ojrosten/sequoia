////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/RegularTestCore.hpp"
#include "sequoia/Maths/Geometry/Angle.hpp"

namespace sequoia::testing
{
  template<std::floating_point T, T Period>
  struct value_tester<maths::angle<T, Period>>
  {
    using type = maths::angle<T, Period>;

    template<test_mode Mode>
    static void test(equality_check_t, test_logger<Mode>& logger, const type& actual, const type& prediction)
    {
      check(equality, "Wrapped value", logger, actual.value(), prediction.value());
    }
    
    template<test_mode Mode>
    static void test(equivalence_check_t, test_logger<Mode>& logger, const type& actual, const T& prediction)
    {
      check(equality, "Wrapped value", logger, actual.value(), prediction);
      check(equality, "Principal angle", logger, actual.principal_angle().value(), std::fmod(prediction, Period));
    }
  };
}
