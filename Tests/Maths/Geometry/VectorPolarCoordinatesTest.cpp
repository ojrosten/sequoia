////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "VectorPolarCoordinatesTest.hpp"

#include <cmath>
#include <numbers>

namespace sequoia::testing
{
  using namespace maths;
  
  namespace
  {
    struct polar_representation
    {
      template<weak_commutative_ring T> 
      [[nodiscard]]
      constexpr static std::array<T, 2> to_underlying(std::span<const T, 2> polar)
      {
        return {polar[0] * std::cos(polar[1]), polar[0] * std::sin(polar[1])};
      }

      template<weak_commutative_ring T> 
      constexpr static std::array<T, 2>& to_underlying(std::array<T, 2>& polar)
      {        
        return polar = to_underlying(std::span<const T, 2>{polar});
      }

      template<weak_commutative_ring T> 
      [[nodiscard]]
      constexpr static std::array<T, 2> from_underlying(std::span<const T, 2> cartesian)
      {
        return {std::sqrt(cartesian[0] * cartesian[0] + cartesian[1] * cartesian[1]), std::atan(cartesian[1] / cartesian[0])};
      }

      template<weak_commutative_ring T> 
      constexpr static std::array<T, 2>& from_underlying(std::array<T, 2>& polar)
      {
        return polar = from_underlying(std::span<const T, 2>{polar});
      }
    };
  }
  
  [[nodiscard]]
  std::filesystem::path vector_polar_coordinates_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void vector_polar_coordinates_test::run_tests()
  {
    test_vec<sets::R<1>, float, 2, polar_representation>();
  }

  template<class Set, maths::weak_field Field, std::size_t D, class Representation>
  void vector_polar_coordinates_test::test_vec()
  {
    using vec_space_t = my_vec_space<Set, Field, D>;
    using vec_t       = vector_coordinates<vec_space_t, canonical_basis<Set, Field, D>, Representation>;
    using value_t     = Field;
    using delta_t     = vec_t::displacement_coordinates_type;

    STATIC_CHECK(vector_space<direct_product<vec_space_t, vec_space_t>>);
    STATIC_CHECK(!vector_space<direct_product<vec_t, vec_t>>);
    STATIC_CHECK(vector_space<direct_product<direct_product<vec_space_t, vec_space_t>, vec_space_t>>);
    STATIC_CHECK(can_multiply<vec_t, value_t>);
    STATIC_CHECK(can_divide<vec_t, value_t>);
    STATIC_CHECK(!can_divide<vec_t, vec_t>);
    STATIC_CHECK(!can_divide<vec_t, delta_t>);
    STATIC_CHECK(!can_divide<delta_t, vec_t>);
    STATIC_CHECK(!can_divide<delta_t, delta_t>);
    STATIC_CHECK(can_add<vec_t, vec_t>);
    STATIC_CHECK(can_add<vec_t, delta_t>);
    STATIC_CHECK(can_subtract<vec_t, vec_t>);
    STATIC_CHECK(can_subtract<vec_t, delta_t>);
    STATIC_CHECK(has_unary_plus<vec_t>);
    STATIC_CHECK(has_unary_minus<vec_t>);

    constexpr auto pi{std::numbers::pi_v<Field>};
    
    vec_t v{1, 0}, w{1, pi};
    check(within_tolerance{value_t(1e-7)}, "", v - w, delta_t{2, 0});
  }
}
