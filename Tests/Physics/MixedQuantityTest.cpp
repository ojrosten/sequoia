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
    test_mixed_vector();
  }

  void mixed_quantity_test::test_mixed()
  {
    using mass_t        = si::mass<float>;
    using d_mass_t      = mass_t::displacement_type;
    using length_t      = si::length<float>;
    using current_t     = si::electrical_current<float>;
    using temperature_t = si::temperature<float>;
    using unsafe_mass_t = quantity<mass_space<float, implicit_common_arena>, units::kilogram_t, intrinsic_origin, std::identity>;
    using unsafe_len_t  = quantity<length_space<float, implicit_common_arena>, units::metre_t, intrinsic_origin, std::identity>;

    auto ml = mass_t{1.0, units::kilogram} * length_t{2.0, units::metre},
         lm = length_t{2.0, units::metre} * mass_t{1.0, units::kilogram};
    check(equivalence, "", ml, 2.0f);
    check(equivalence, "", lm, 2.0f);
    check(equality, "", lm / mass_t{2.0, units::kilogram}, length_t{1.0, units::metre});
    check(equality, "", lm / length_t{0.5, units::metre}, mass_t{4.0, units::kilogram});
    check(equality, "", (mass_t{3.0, units::kilogram} / mass_t{1.0, units::kilogram})*mass_t{3.0, units::kilogram}, mass_t{9.0, units::kilogram});
    check(equality, "", (current_t{-3.0, units::ampere} / current_t{1.0, units::ampere})*mass_t{3.0, units::kilogram}, unsafe_mass_t{-9.0, units::kilogram});
      
    auto mlc = mass_t{1.0, units::kilogram} * length_t{2.0, units::metre} * current_t{-1.0, units::ampere},
         clm = current_t{-1.0, units::ampere} * length_t{2.0, units::metre} *  mass_t{1.0, units::kilogram};

    check(equivalence, "", mlc, -2.0f);
    check(equivalence, "", clm, -2.0f);
    check(equality, "", mlc / mass_t{1.0, units::kilogram}, length_t{2.0, units::metre} * current_t{-1.0, units::ampere});
    check(equality, "", mlc / length_t{1.0, units::metre}, mass_t{2.0, units::kilogram} * current_t{-1.0, units::ampere});
    check(equality, "", mlc / current_t{-1.0, units::ampere}, unsafe_mass_t{1.0, units::kilogram} * length_t{2.0, units::metre});
      
    auto mlct = mass_t{1.0, units::kilogram} * length_t{2.0, units::metre} * current_t{-1.0, units::ampere} * temperature_t{5.0, units::kelvin},
         cltm = current_t{-1.0, units::ampere} * length_t{2.0, units::metre} * temperature_t{5.0, units::kelvin} *  mass_t{1.0, units::kilogram};

    check(equivalence, "", mlct, -10.0f);
    check(equivalence, "", cltm, -10.0f);
    check(equality, "", mlct / mass_t{1.0, units::kilogram}, length_t{2.0, units::metre} * current_t{-1.0, units::ampere} * temperature_t{5.0, units::kelvin});

    check(equality, "", mass_t{2.0, units::kilogram} * length_t{3.0, units::metre} / d_mass_t{-2.0, units::kilogram}, unsafe_len_t{-3.0, units::metre});
  }

  void mixed_quantity_test::test_mixed_vector()
  {
    using pos_t  = si::position<2, float>;
    using time_t = si::time<float>;

    check(equivalence, "", (pos_t{2.0, units::metre} - pos_t{1.0, units::metre}) / (time_t{4.0, units::second} - time_t{2.0, units::second}), 0.5);
  }
}
