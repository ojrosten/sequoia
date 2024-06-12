////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/RegularTestCore.hpp"
#include "sequoia/Maths/Geometry/VectorSpace.hpp"

namespace sequoia::testing
{
  template<class VectorSpace>
  struct value_tester<maths::vec<VectorSpace>>
  {
    using type = maths::vec<VectorSpace>;

    template<test_mode Mode>
    static void test(equality_check_t, test_logger<Mode>& logger, const type& actual, const type& prediction)
    {
      // e.g. check(equality, "Description", logger, actual.some_method(), prediction.some_method());
    }
    
    template<test_mode Mode>
    static void test(equivalence_check_t, test_logger<Mode>& logger, const type& actual, const std::array<typename VectorSpace::value_type, VectorSpace::cardinality>& prediction)
    {
      // e.g. check(equality, "Description", logger, actual.some_method(), prediction);
    }
  };
}
