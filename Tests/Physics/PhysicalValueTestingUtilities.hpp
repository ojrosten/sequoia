////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/RegularTestCore.hpp"
#include "sequoia/Physics/PhysicalValues.hpp"

namespace sequoia::testing
{
  template<
    maths::convex_space PhysicalValueSpace,
    physics::physical_unit Unit,
    maths::basis_for<maths::free_module_type_of_t<PhysicalValueSpace>> Basis,
    class Origin,
    class Validator
  >
  struct value_tester<physics::physical_value<PhysicalValueSpace, Unit, Basis, Origin, Validator>>
  {
    using type       = physics::physical_value<PhysicalValueSpace, Unit, Basis, Origin, Validator>;
    using value_type = type::value_type;
    constexpr static auto dimension{type::dimension};

    template<test_mode Mode>
    static void test(equality_check_t, test_logger<Mode>& logger, const type& actual, const type& prediction)
    {
      if constexpr(dimension == 1)
      {
        check(equality, "Wrapped Value", logger, actual.value(), prediction.value());
      }
      else
      {
        check(equality, "Wrapped Values", logger, actual.values(), prediction.values());
      }
    }
    
    template<test_mode Mode>
      requires (dimension == 1)
    static void test(equivalence_check_t, test_logger<Mode>& logger, const type& actual, const value_type& prediction)
    {
      check(equality, "Wrapped Value", logger, actual.value(), prediction);
    }

    template<test_mode Mode>
      requires (dimension > 1)
    static void test(equivalence_check_t, test_logger<Mode>& logger, const type& actual, const std::array<value_type, dimension>& prediction)
    {
      check(equivalence, "Wrapped Values", logger, actual.values(), prediction);
    }
  };

  template<
    maths::convex_space ValueSpace,
    class Unit,
    maths::basis_for<maths::free_module_type_of_t<ValueSpace>> Basis,
    class Origin,
    class Validator
  >
  inline constexpr bool defines_physical_value_v{
    requires {
      typename physics::physical_value<ValueSpace, Unit, Basis, Origin, Validator>;
    }
  };
}
