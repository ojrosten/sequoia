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
    using type       = maths::vec<VectorSpace>;
    using value_type = typename VectorSpace::value_type;

    constexpr static std::size_t D{VectorSpace::cardinality};

    template<test_mode Mode>
    static void test(equality_check_t, test_logger<Mode>& logger, const type& actual, const type& prediction)
    {
      check(equality, "Wrapped values", logger, actual.values(), prediction.values());
    }

    template<test_mode Mode>
    static void test(equivalence_check_t, test_logger<Mode>& logger, const type& actual, const std::array<value_type, D>& prediction)
    {
      check(equality, "Wrapped values", logger, actual.values(), std::span<const value_type, D>{prediction});
    }
  };
}
