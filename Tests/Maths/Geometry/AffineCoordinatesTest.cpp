////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "AffineCoordinatesTest.hpp"

namespace sequoia::maths
{
  using namespace testing;

  template<
    affine_space A,
    basis_for<free_module_type_of_t<A>> Basis,
    representation_for<A> Representation,
    validator_for<A, Representation> Validator
  >
  struct coordinate_transformation<
    affine_coordinates<A, Basis, Representation, alice, Validator>,
    affine_coordinates<A, Basis,Representation, bob, Validator>
  >
  {
    using disp_type = affine_coordinates<A, Basis, Representation, alice, Validator>::displacement_coordinates_type;

    disp_type displacement{};

    explicit coordinate_transformation(const disp_type& d)
      : displacement{d}
    {}
      
    [[nodiscard]]
    constexpr affine_coordinates<A, Basis, Representation, bob, Validator>
      operator()(const affine_coordinates<A, Basis, Representation, alice, Validator>& c) const noexcept
      {     
        return affine_coordinates<A, Basis, Representation, bob, Validator>{(c + displacement).values()};
      }
  };
}

namespace sequoia::testing
{
  using namespace maths;

  [[nodiscard]]
  std::filesystem::path affine_coordinates_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void affine_coordinates_test::run_tests()
  {
    test_affine<sets::R<1>, commutative_rings::reals<1> , 1, float>();
    test_affine<sets::R<1>, commutative_rings::reals<1> , 1, double>();
    test_affine<sets::C<1>, commutative_rings::complexes, 1, std::complex<float>>();

    // The complex plane regarded as a two-dimensional real affine space, rather
    // than as a one-dimensional complex one: the set is the same, the field is not.
    test_affine<sets::C<1>, commutative_rings::reals<1> , 2, float>();
  }

  template<class Set, class Field, std::size_t D, class Rep>
    requires maths::identifies_as_field_v<Field>
  void affine_coordinates_test::test_affine()
  {
    using space_t  = my_affine_space<Set, Field, D>;
    using basis_t  = general_basis<free_module_type_of_t<space_t>>;
    using rep_t    = canonical_representation<Rep, no_bounds<to_bounds_value_type_t<Rep>>>;
    using affine_t = affine_coordinates<space_t, basis_t, rep_t, alice, identity_validator>;
    using delta_t  = affine_t::displacement_coordinates_type;

    STATIC_CHECK(m_affine_space<space_t>);
    STATIC_CHECK(affine_space<space_t>);
    STATIC_CHECK(!free_module<space_t>);
    STATIC_CHECK(not defines_rank_v<space_t>);
    STATIC_CHECK(dimension_of_v<space_t> == D);

    operator_checks<affine_t, operator_expectations{
        .point_plus_point               = admits::no,
        .point_plus_displacement        = admits::yes,
        .point_minus_point              = admits::yes,
        .point_minus_displacement       = admits::yes,
        .point_unary_plus               = admits::yes,
        .point_unary_minus              = admits::no,
        .point_times_scalar             = admits::no,
        .point_over_scalar              = admits::no,
        .point_over_point               = admits::no,
        .point_over_displacement        = admits::no,
        .displacement_over_point        = admits::no,
        .displacement_times_scalar      = admits::yes,
        .displacement_over_scalar       = admits::yes,
        .displacement_over_displacement = admits::no,
        .displacement_unary_minus       = admits::yes
      }
    >{*this}.execute();

    coordinates_operations<affine_t>{*this}.execute();

    using affine2_t = affine_coordinates<space_t, basis_t, rep_t, bob, identity_validator>;
    affine2_t bob_coords{coordinate_transformation<affine_t, affine2_t>{delta_t{Rep{-1.0}}}(affine_t{})};

    check(equality, "Translation of alice's origin to bob's", bob_coords, affine2_t{Rep{-1.0}});
  }
}
