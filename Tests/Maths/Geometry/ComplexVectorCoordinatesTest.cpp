////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/** \file */

#include "ComplexVectorCoordinatesTest.hpp"

import std;

namespace sequoia::testing
{
  using namespace maths;

  [[nodiscard]]
  std::filesystem::path complex_vector_coordinates_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void complex_vector_coordinates_test::run_tests()
  {
    using namespace commutative_rings;
    test_vec<sets::C<1>, complexes, 1, std::complex<float>>();
    test_vec<sets::C<2>, complexes, 2, std::complex<double>>();
    test_vec<sets::C<1>, reals<1> , 2, double>(); // Complex numbers over the reals

    test_complex_vec_1_inner_prod<sets::C<1>, complexes, std::complex<double>>();
  }

  template<class Set, class Field, std::size_t D, class Rep>
    requires maths::identifies_as_field_v<Field>
  void complex_vector_coordinates_test::test_vec()
  {
    using vec_space_t  = my_vec_space<Set, Field, D>;
    using basis_data_t = canonical_basis_data<D>;
    using rep_t        = canonical_representation<Rep, no_bounds<to_bounds_value_type_t<Rep>>>;
    using vec_t        = vector_coordinates<vec_space_t, basis_data_t, rep_t, identity_validator>;
    using value_t      = Rep;
    using delta_t      = vec_t::displacement_coordinates_type;

    STATIC_CHECK(vector_space<tensor_product<vec_space_t, vec_space_t>>);
    STATIC_CHECK(!vector_space<tensor_product<vec_t, vec_t>>);
    STATIC_CHECK(vector_space<tensor_product<tensor_product<vec_space_t, vec_space_t>, vec_space_t>>);
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
  
    coordinates_operations<vec_t>{*this}.execute();
  }

  template<class Set, class Field, class Rep>
      requires maths::identifies_as_field_v<Field>
  void complex_vector_coordinates_test::test_complex_vec_1_inner_prod()
  {
    using space_t      = my_vec_space<Set, Field, 1>;
    using basis_data_t = canonical_basis_data<1>;
    using rep_t        = canonical_representation<Rep, no_bounds<to_bounds_value_type_t<Rep>>>;
    using vec_t        = vector_coordinates<space_t, basis_data_t, rep_t, identity_validator>;
    using value_t      = Rep;

    check(equality, "", inner_product(vec_t{value_t(1, 1)} , vec_t{value_t(1, 1)}), value_t{2});
    check(equality, "", inner_product(vec_t{value_t(1, -1)}, vec_t{value_t(1, 1)}), value_t{0, 2});
  }
}
