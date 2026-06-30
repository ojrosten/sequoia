////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "VectorPolarCoordinatesTest.hpp"

namespace sequoia::testing
{
  using namespace maths;
  
  [[nodiscard]]
  std::filesystem::path vector_polar_coordinates_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void vector_polar_coordinates_test::run_tests()
  {
    test_vec<sets::R<2>, float , 2, basic_polar_representation<float>, identity_validator>();
    test_vec<sets::R<2>, double, 2, polar_representation<double>, identity_validator>();

    test_refined<float, identity_validator>();

    // TO DO: test different bounds
  }

  template<class Set, maths::weak_field Field, std::size_t D, class Representation, class Validator>
  void vector_polar_coordinates_test::test_vec()
  {
    using vec_space_t = my_vec_space<Set, Field, D>;
    using vec_t       = vector_coordinates<vec_space_t, canonical_basis<Set, Field, D>, Representation, Validator>;
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

  template<maths::weak_field Field, class Validator>
  void vector_polar_coordinates_test::test_refined()
  {
    using vec_space_t = my_vec_space<sets::R<2>, Field, 2>;
    using vec_t       = vector_coordinates<vec_space_t, canonical_basis<sets::R<2>, Field, 2>, polar_representation<Field>, Validator>;

    STATIC_CHECK(defines_scalar_multiplication_for_v<vec_space_t, polar_representation<Field>>);
    STATIC_CHECK(      defines_scalar_division_for_v<vec_space_t, polar_representation<Field>>);
    
    check(equality, "", vec_t{1, 1} * 2, vec_t{2, 1});
  }
}
