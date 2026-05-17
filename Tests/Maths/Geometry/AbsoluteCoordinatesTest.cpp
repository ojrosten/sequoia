////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "AbsoluteCoordinatesTest.hpp"

namespace sequoia::testing
{
  using namespace maths;

  [[nodiscard]]
  std::filesystem::path absolute_coordinates_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void absolute_coordinates_test::run_tests()
  {
    test_absolute<float, 1>();
    test_absolute<float, 2>();
  }

  template<std::floating_point T, std::size_t D>
  void absolute_coordinates_test::test_absolute()
  {
    using space_t     = euclidean_nonnegative_space<T, D, mathematical_arena>;
    using basis_t     = canonical_right_handed_basis<free_module_type_of_t<space_t>>;
    using coords_t    = coordinates<space_t, basis_t, identity_representation<half_line_bounds<T>>, throwing_validator>;
    using delta_t     = coords_t::displacement_coordinates_type;
    using value_t     = T;
    STATIC_CHECK(can_multiply<coords_t, value_t>);
    STATIC_CHECK(can_divide<coords_t, value_t>);
    STATIC_CHECK(!can_divide<coords_t, coords_t>);
    STATIC_CHECK(!can_divide<coords_t, delta_t>);
    STATIC_CHECK(!can_divide<delta_t, coords_t>);
    STATIC_CHECK(!can_divide<delta_t, delta_t>);
    STATIC_CHECK(can_add<coords_t, coords_t>);
    STATIC_CHECK(can_add<coords_t, delta_t>);
    STATIC_CHECK(can_subtract<coords_t, coords_t>);
    STATIC_CHECK(can_subtract<coords_t, delta_t>);
    STATIC_CHECK(has_unary_plus<coords_t>);
    STATIC_CHECK(!has_unary_minus<coords_t>);
    
    coordinates_operations<coords_t>{*this}.execute();
  }
}
