////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "MixedQuantityTest.hpp"

#include "../Maths/Geometry/GeometryTestingUtilities.hpp"

namespace sequoia::testing
{ 
  using namespace physics;

  [[nodiscard]]
  std::filesystem::path mixed_quantity_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void mixed_quantity_test::run_tests()
  {
    test_mixed();
  }

  void mixed_quantity_test::test_mixed()
  {
    using mass_t            = si::mass<float>;
    using d_mass_t          = mass_t::displacement_quantity_type;
    using length_t          = si::length<float>;
    using charge_t          = si::electrical_charge<float>;
    using temperature_t     = si::temperature<float>;
    using unsafe_mass_t     = quantity<mass_space<float, implicit_common_system>, units::kilogram_t, intrinsic_origin, std::identity>;
    using unsafe_len_t      = quantity<length_space<float, implicit_common_system>, units::metre_t, intrinsic_origin, std::identity>;
    
    auto ml = mass_t{1.0, units::kilogram} * length_t{2.0, units::metre},
         lm = length_t{2.0, units::metre} * mass_t{1.0, units::kilogram};
    check(equivalence, "", ml, 2.0f);
    check(equivalence, "", lm, 2.0f);
    check(equality, "", lm / mass_t{2.0, units::kilogram}, length_t{1.0, units::metre});
    check(equality, "", lm / length_t{0.5, units::metre}, mass_t{4.0, units::kilogram});
    check(equality, "", (mass_t{3.0, units::kilogram} / mass_t{1.0, units::kilogram})*mass_t{3.0, units::kilogram}, mass_t{9.0, units::kilogram});
    check(equality, "", (charge_t{-3.0, units::coulomb} / charge_t{1.0, units::coulomb})*mass_t{3.0, units::kilogram}, unsafe_mass_t{-9.0, units::kilogram});
      
    auto mlc = mass_t{1.0, units::kilogram} * length_t{2.0, units::metre} * charge_t{-1.0, units::coulomb},
         clm = charge_t{-1.0, units::coulomb} * length_t{2.0, units::metre} *  mass_t{1.0, units::kilogram};

    check(equivalence, "", mlc, -2.0f);
    check(equivalence, "", clm, -2.0f);
    check(equality, "", mlc / mass_t{1.0, units::kilogram}, length_t{2.0, units::metre} * charge_t{-1.0, units::coulomb});
    check(equality, "", mlc / length_t{1.0, units::metre}, mass_t{2.0, units::kilogram} * charge_t{-1.0, units::coulomb});
    check(equality, "", mlc / charge_t{-1.0, units::coulomb}, unsafe_mass_t{1.0, units::kilogram} * length_t{2.0, units::metre});
      
    auto mlct = mass_t{1.0, units::kilogram} * length_t{2.0, units::metre} * charge_t{-1.0, units::coulomb} * temperature_t{5.0, units::kelvin},
         cltm = charge_t{-1.0, units::coulomb} * length_t{2.0, units::metre} * temperature_t{5.0, units::kelvin} *  mass_t{1.0, units::kilogram};

    check(equivalence, "", mlct, -10.0f);
    check(equivalence, "", cltm, -10.0f);
    check(equality, "", mlct / mass_t{1.0, units::kilogram}, length_t{2.0, units::metre} * charge_t{-1.0, units::coulomb} * temperature_t{5.0, units::kelvin});

    check(equality, "", mass_t{2.0, units::kilogram} * length_t{3.0, units::metre} / d_mass_t{-2.0, units::kilogram}, unsafe_len_t{-3.0, units::metre});
  }
}
