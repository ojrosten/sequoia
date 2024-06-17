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
  template<class T, std::size_t D>
  struct my_vec_space
  {
    using value_type = T;
    constexpr static std::size_t cardinality{D};
  };

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
      if constexpr(D == 1)
      {
        check(equality, "Wrapped value", logger, actual.value(), prediction.value());
        if constexpr(std::convertible_to<value_type, bool>)
          check(equality, "Conversion to bool", logger, static_cast<bool>(actual), static_cast<bool>(prediction));
      }

      for(auto i : std::views::iota(0_sz, D))
      {
        check(equality, std::format("Value at index {}", i), logger, actual[i], prediction[i]);
      }
    }

    template<test_mode Mode>
    static void test(equivalence_check_t, test_logger<Mode>& logger, const type& actual, const std::array<value_type, D>& prediction)
    {
      check(equality, "Wrapped values", logger, actual.values(), std::span<const value_type, D>{prediction});
    }
  };
}
