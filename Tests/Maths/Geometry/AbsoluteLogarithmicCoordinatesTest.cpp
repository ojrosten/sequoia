////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "AbsoluteLogarithmicCoordinatesTest.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path absolute_logarithmic_coordinates_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void absolute_logarithmic_coordinates_test::run_tests()
  {
  }

  //template<class Element, maths::weak_field Field, std::size_t D>
  void absolute_logarithmic_coordinates_test::test_absolute()
  {
    /*using space_t  = my_affine_space<Element, Field, D>;
    using basis_t  = canonical_basis<Element, Field, D>;
    using affine_t = affine_coordinates<space_t, basis_t, alice<space_t>, identity_representation<std::identity>>;
    using delta_t  = affine_t::displacement_coordinates_type;
    using value_t  = Field;
    STATIC_CHECK(!can_multiply<affine_t, value_t>);
    STATIC_CHECK(!can_divide<affine_t, value_t>);
    STATIC_CHECK(!can_divide<affine_t, affine_t>);
    STATIC_CHECK(!can_divide<affine_t, delta_t>);
    STATIC_CHECK(!can_divide<delta_t, affine_t>);
    STATIC_CHECK(!can_divide<delta_t, delta_t>);
    STATIC_CHECK(!can_add<affine_t, affine_t>);
    STATIC_CHECK(can_add<affine_t, delta_t>);
    STATIC_CHECK(can_subtract<affine_t, affine_t>);
    STATIC_CHECK(can_subtract<affine_t, delta_t>);
    STATIC_CHECK(has_unary_plus<affine_t>);
    STATIC_CHECK(!has_unary_minus<affine_t>);
    
    coordinates_operations<affine_t>{*this}.execute();

    using affine2_t = affine_coordinates<space_t, basis_t, bob<space_t>, identity_representation<std::identity>>;
    affine2_t bob_coords{coordinate_transformation<affine_t, affine2_t>{delta_t{Field{-1.0}}}(affine_t{})};

    check(equality, "", bob_coords, affine2_t{Field{-1.0}});*/
  }
}
