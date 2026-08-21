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
    maths::partial_m_torsor PhysicalValueSpace,
    physics::physical_unit Unit,
    maths::basis_for<maths::free_module_type_of_t<PhysicalValueSpace>> Basis,  
    maths::representation_for<PhysicalValueSpace> Representation,
    class Origin,
    maths::validator_for<PhysicalValueSpace, Representation> Validator
  >
  struct value_tester<physics::physical_value<PhysicalValueSpace, Unit, Basis, Representation, Origin, Validator>>
  {
    using type       = physics::physical_value<PhysicalValueSpace, Unit, Basis, Representation, Origin, Validator>;
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

  /*! Answers whether physics::physical_value can be formed from these arguments.

      Deliberately unconstrained. The whole purpose of the predicate is to be
      asked about arguments which may not fit, so constraining its own parameters
      defeats it: a negative check on arguments failing those constraints is a
      hard error rather than the `false` the caller is asking for. In particular
      `maths::basis_for<maths::free_module_type_of_t<ValueSpace>> Basis` is
      ill-formed for any ValueSpace which is not a space at all.
   */
  template<
    class ValueSpace,
    class Unit,
    class Basis,
    class Representation,
    class Origin,
    class Validator
  >
  inline constexpr bool defines_physical_value_v{
    requires {
      typename physics::physical_value<ValueSpace, Unit, Basis, Representation, Origin, Validator>;
    }
  };
}
