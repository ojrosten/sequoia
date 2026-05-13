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
    template<weak_commutative_ring T, auto Bounds=no_bounds<T>> // TO DO: improve bounds std::array should be made to work...
    struct basic_polar_representation
    {
      using value_type = T;
      constexpr static auto bounds_v{Bounds};

      template<auto OtherBounds>
      using rebind_type = basic_polar_representation<T, OtherBounds>;
          
      [[nodiscard]]
      constexpr static std::array<T, 2> to_underlying(std::span<const T, 2> polar)
      {
        return {polar[0] * std::cos(polar[1]), polar[0] * std::sin(polar[1])};
      }

      [[nodiscard]]
      constexpr static std::array<T, 2> from_underlying(std::span<const T, 2> cartesian)
      {
        T theta{(!cartesian[0] && !cartesian[1]) ? T{} : std::atan2(cartesian[1], cartesian[0])};
        if(theta < 0) theta += T{2} * std::numbers::pi_v<T>;
        
        return {std::sqrt(cartesian[0] * cartesian[0] + cartesian[1] * cartesian[1]), theta};
      }
    };

    template<weak_commutative_ring T, auto Bounds=no_bounds<T>>
    struct polar_representation : basic_polar_representation<T, Bounds>
    {
      template<auto OtherBounds>
      using rebind_type = polar_representation<T, OtherBounds>;

      [[nodiscard]]
      static constexpr T compute_angle(T theta, T scale)
      {
        if(!scale)
          return T{};

        constexpr auto pi{std::numbers::pi_v<T>};

        return
            scale > T{} ? theta
          : theta >= pi ? theta - pi
                        : theta + pi;
      }
      
      [[nodiscard]]
      static constexpr std::array<T, 2> mul(std::span<const T, 2> lhs, T scale)
      {
        return {lhs[0] * std::abs(scale), compute_angle(lhs[1], scale)};
      }

      [[nodiscard]]
      static constexpr std::array<T, 2> div(std::span<const T, 2> lhs, T scale)
      {
        return {lhs[0] / std::abs(scale), compute_angle(lhs[1], scale)};
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
    test_vec<sets::R<2>, float , 2, basic_polar_representation<float>>();
    test_vec<sets::R<2>, double, 2, polar_representation<double>>();

    test_refined<float>();
  }

  template<class Set, maths::weak_field Field, std::size_t D, class Representation>
  void vector_polar_coordinates_test::test_vec()
  {
    using vec_space_t = my_vec_space<Set, Field, D>;
    using vec_t       = vector_coordinates<vec_space_t, canonical_basis<Set, Field, D>, Representation>;
    using value_t     = Field;
    using delta_t     = vec_t::displacement_coordinates_type;

    STATIC_CHECK(representation_for_span<Representation, vec_space_t>);
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
    STATIC_CHECK(vec_t::has_freely_mutable_components);

    coordinates_operations<vec_t>{*this}.execute();
       
    check(
      within_tolerance{std::same_as<Field, float> ? value_t(1e-7) : value_t(1e-14)},
      "The resultant angle may be within the tolerance of either 0 or 2pi, depending",
      []() {
        constexpr auto pi{std::numbers::pi_v<Field>};
        constexpr vec_t u{1, 0}, v{1, pi};
        auto w{u-v};      
        w[1] = std::fmod(w[1], 2*pi);
        return w;
      }(),
      delta_t{2, 0}
    );
  }

  template<maths::weak_field Field>
  void vector_polar_coordinates_test::test_refined()
  {
    using vec_space_t = my_vec_space<sets::R<2>, Field, 2>;
    using vec_t       = vector_coordinates<vec_space_t, canonical_basis<sets::R<2>, Field, 2>, polar_representation<Field>>;

    static_assert(defines_scalar_multiplication_for_v<vec_space_t, polar_representation<Field>>);
    
    check(equality, "", vec_t{1, 1} * 2, vec_t{2, 1});
  }
}
