////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/** \file */

#include "sequoia/Maths/Geometry/Spaces.hpp"
#include "sequoia/TestFramework/RegularTestCore.hpp"

namespace sequoia::testing
{
  template<maths::weak_commutative_ring T>
  struct value_tester<maths::coordinate_bounds<T>>
  {
    using bounds_type = maths::coordinate_bounds<T>;
    
    template<test_mode Mode>
    static void test(equality_check_t, test_logger<Mode>& logger, const bounds_type& actual, const bounds_type& prediction)
    {
      check(equality, "Lower", logger, actual.lower, prediction.lower);
      check(equality, "Upper", logger, actual.upper, prediction.upper);
    }
  };
}
