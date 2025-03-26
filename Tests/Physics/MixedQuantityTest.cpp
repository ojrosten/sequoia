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
  using namespace si::units;

  [[nodiscard]]
  std::filesystem::path mixed_quantity_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void mixed_quantity_test::run_tests()
  {
    test_mixed();
    test_mixed_vector();
    test_mixed_kinds();
  }

  void mixed_quantity_test::test_mixed()
  {
    using mass_t        = si::mass<float>;
    using d_mass_t      = mass_t::displacement_type;
    using length_t      = si::length<float>;
    using current_t     = si::electrical_current<float>;
    using temperature_t = si::temperature<float>;

    using unsafe_mass_t = quantity<mass_space<float, implicit_common_arena>, kilogram_t, intrinsic_origin, std::identity>;
    using unsafe_len_t  = quantity<length_space<float, implicit_common_arena>, metre_t, intrinsic_origin, std::identity>;

    auto ml = mass_t{1.0, kilogram} * length_t{2.0, metre},
         lm = length_t{2.0, metre} * mass_t{1.0, kilogram};
    check(equivalence, "", ml, 2.0f);
    check(equivalence, "", lm, 2.0f);
    check(equality, "", lm / mass_t{2.0, kilogram}, length_t{1.0, metre});
    check(equality, "", lm / length_t{0.5, metre}, mass_t{4.0, kilogram});
    check(equality, "", (mass_t{3.0, kilogram} / mass_t{1.0, kilogram})*mass_t{3.0, kilogram}, mass_t{9.0, kilogram});
    check(equality, "", (current_t{-3.0, ampere} / current_t{1.0, ampere})*mass_t{3.0, kilogram}, unsafe_mass_t{-9.0, kilogram});
      
    auto mlc = mass_t{1.0, kilogram} * length_t{2.0, metre} * current_t{-1.0, ampere},
         clm = current_t{-1.0, ampere} * length_t{2.0, metre} *  mass_t{1.0, kilogram};

    check(equivalence, "", mlc, -2.0f);
    check(equivalence, "", clm, -2.0f);
    check(equality, "", mlc / mass_t{1.0, kilogram}, length_t{2.0, metre} * current_t{-1.0, ampere});
    check(equality, "", mlc / length_t{1.0, metre}, mass_t{2.0, kilogram} * current_t{-1.0, ampere});
    check(equality, "", mlc / current_t{-1.0, ampere}, unsafe_mass_t{1.0, kilogram} * length_t{2.0, metre});
      
    auto mlct = mass_t{1.0, kilogram} * length_t{2.0, metre} * current_t{-1.0, ampere} * temperature_t{5.0, kelvin},
         cltm = current_t{-1.0, ampere} * length_t{2.0, metre} * temperature_t{5.0, kelvin} *  mass_t{1.0, kilogram};

    check(equivalence, "", mlct, -10.0f);
    check(equivalence, "", cltm, -10.0f);
    check(equality, "", mlct / mass_t{1.0, kilogram}, length_t{2.0, metre} * current_t{-1.0, ampere} * temperature_t{5.0, kelvin});

    check(equality, "", mass_t{2.0, kilogram} * length_t{3.0, metre} / d_mass_t{-2.0, kilogram}, unsafe_len_t{-3.0, metre});
  }

  void mixed_quantity_test::test_mixed_vector()
  {
    using pos_t  = si::position<2, float>;
    using time_t = si::time<float>;

    check(equivalence,
          "",
          (pos_t{std::array{2.0f, 1.0f}, metre} - pos_t{std::array{1.0f, -1.0f}, metre}) / (time_t{4.0, second} - time_t{2.0, second}),
          std::array{0.5f, 1.0f});
  }

  void mixed_quantity_test::test_mixed_kinds()
  {
    using length_t = si::length<float>;
    using width_t  = si::width<float>;
    using height_t = si::height<float>;

    length_t len{1.0, metre};
    check(equality, "", len += width_t{1.0, metre},  length_t{2.0, metre});
    check(equality, "", len += height_t{0.5, metre}, length_t{2.5, metre});
    
  }
}
