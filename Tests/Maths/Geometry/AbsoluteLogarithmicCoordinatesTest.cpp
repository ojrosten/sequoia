////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "AbsoluteLogarithmicCoordinatesTest.hpp"

#include <cmath>

namespace sequoia::testing
{
  using namespace maths;

  namespace
  {
    struct naive_logarithmic_representation
    {
      using validator_type = std::identity;

      template<weak_commutative_ring T> 
      [[nodiscard]]
      constexpr static std::array<T, 1> to_underlying(std::span<const T, 1> val)
      {
        return {std::exp(val[0])};
      }

      template<weak_commutative_ring T> 
      [[nodiscard]]
      constexpr static std::array<T, 1> from_underlying(std::span<const T, 1> val)
      {
        return {std::log(val[0])};
      }
    };
  }
  
  [[nodiscard]]
  std::filesystem::path absolute_logarithmic_coordinates_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void absolute_logarithmic_coordinates_test::run_tests()
  {
    test_absolute_logarithmic<float , naive_logarithmic_representation>();
    test_absolute_logarithmic<double, naive_logarithmic_representation>();
  }

  template<std::floating_point T, class Representation>
  void absolute_logarithmic_coordinates_test::test_absolute_logarithmic()
  {
    using space_t     = euclidean_nonnegative_space<T, 1, mathematical_arena>;
    using coords_t    = coordinates<space_t, canonical_right_handed_basis<free_module_type_of_t<space_t>>, Representation>;
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
    STATIC_CHECK(!coords_t::has_freely_mutable_components);
    
    coordinates_operations<coords_t>{*this}.execute();
  }
}
