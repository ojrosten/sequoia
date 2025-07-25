////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "AffineCoordinatesTest.hpp"

namespace sequoia::testing
{
  using namespace maths;

  namespace
  {
    struct alice {};
  }

  [[nodiscard]]
  std::filesystem::path affine_coordinates_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void affine_coordinates_test::run_tests()
  {
    test_affine<float, float, 1>();
    test_affine<double, double, 1>();
    test_affine<std::complex<float>, std::complex<float>, 1>();
    test_affine<std::complex<float>, float, 2>();
  }

  template<class Element, maths::weak_field Field, std::size_t D>
  void affine_coordinates_test::test_affine()
  {
    using affine_t = affine_coordinates<my_affine_space<Element, Field, D>, canonical_basis<Element, Field, D>, alice>;
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
  }
}
