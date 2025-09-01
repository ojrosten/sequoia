////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "MixedPhysicalValueTest.hpp"

#include "../Maths/Geometry/GeometryTestingUtilities.hpp"

namespace sequoia::testing
{ 
  using namespace physics;
  using namespace si::units;

  [[nodiscard]]
  std::filesystem::path mixed_physical_value_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void mixed_physical_value_test::run_tests()
  {
    test_mixed();
    test_mixed_vector();
    test_mixed_vector();
    test_mixed_kinds();
  }

  void mixed_physical_value_test::test_mixed()
  {
    using mass_t        = si::mass<float>;
    using d_mass_t      = mass_t::displacement_type;
    using length_t      = si::length<float>;
    using current_t     = si::electrical_current<float>;
    using temperature_t = si::temperature<float>;

    using unsafe_mass_t = quantity<  mass_space<float, implicit_common_arena>, kilogram_t, std::identity>;
    using unsafe_len_t  = quantity<length_space<float, implicit_common_arena>, metre_t,    std::identity>;

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

  void mixed_physical_value_test::test_mixed_vector()
  {
    using pos_t  = si::position<float, 2, implicit_common_arena>;
    using temporal_t = si::time<float>;

    check(equivalence,
          "",
          (pos_t{std::array{2.0f, 1.0f}, metre} - pos_t{std::array{1.0f, -1.0f}, metre}) / (temporal_t{4.0, second} - temporal_t{2.0, second}),
          std::array{0.5f, 1.0f});
  }

  void mixed_physical_value_test::test_mixed_kinds()
  {
    using length_t   = si::length<float>;
    using d_len_t    = length_t::displacement_type;
    using width_t    = si::width<float>;
    using d_width_t  = width_t::displacement_type;
    using height_t   = si::height<float>;
    using d_height_t = height_t::displacement_type;

    STATIC_CHECK( addition_combinable<length_t, width_t>);
    STATIC_CHECK(!subtraction_combinable<length_t, width_t>,
                 "If implemented, the result would have to be length_t&; however, subtraction gives delta length_t"
                 "Inconsistency between operator-= and operator- is undesirable");
    STATIC_CHECK( can_add<length_t, width_t>);
    STATIC_CHECK( can_subtract<length_t, width_t>);

    check(equality, "", length_t{1.0, metre} += width_t{1.0, metre},  length_t{2.0, metre});
    check(equality, "", length_t{1.0, metre} += height_t{0.5, metre}, length_t{1.5, metre});
    check(equality, "", width_t{0.5, metre}  + height_t{0.5, metre}, length_t{1.0, metre});
    check(equality, "", height_t{0.5, metre} + width_t{0.5, metre}, length_t{1.0, metre});

    check(equality, "", height_t{0.5, metre} -  width_t{0.5, metre}, d_len_t{0.0, metre});
    check(equality, "", width_t{0.5, metre}  - height_t{0.5, metre}, d_len_t{0.0, metre});

    check(equality, "", static_cast<length_t>(height_t{0.5, metre}), length_t{0.5, metre});
    check(equality, "", static_cast<length_t>(width_t{0.5, metre}),  length_t{0.5, metre});

    using area = decltype(length_t{0.5, metre} * length_t{0.5, metre});
    check(equality, "", static_cast<area>(height_t{0.5, metre} *  width_t{0.5, metre}), length_t{0.5, metre} * length_t{0.5, metre});
    check(equality, "", static_cast<area>(width_t{0.5, metre}  * height_t{0.5, metre}), length_t{0.5, metre} * length_t{0.5, metre});

    using euc_half_line_qty = euclidean_half_line_quantity<float>;
    check(equality, "", static_cast<euc_half_line_qty>(height_t{0.5, metre} /  width_t{0.5, metre}), euc_half_line_qty{1.0, no_unit});
    check(equality, "", static_cast<euc_half_line_qty>( width_t{0.5, metre} / height_t{0.5, metre}), euc_half_line_qty{1.0, no_unit});

    STATIC_CHECK(!std::is_same_v<d_len_t, d_width_t>);
    check(equality, "", static_cast<d_len_t>(d_width_t{0.5, metre}), d_len_t{0.5, metre});

    STATIC_CHECK(!std::is_same_v<d_len_t, d_height_t>);
    check(equality, "", static_cast<d_len_t>(d_height_t{1.5, metre}), d_len_t{1.5, metre});

    using euc_qty = euclidean_1d_vector_quantity<float>;
    STATIC_CHECK(!std::is_same_v<decltype(d_width_t{} / d_height_t{}), euc_qty>);
    check(equality, "", static_cast<euc_qty>(d_width_t{1.5, metre}/d_height_t{0.5, metre}), euc_qty{3.0, no_unit});
  }
}
