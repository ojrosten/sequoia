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
    template<affine_space A>
    struct alice
    {
      using space_type = A;
    };

    template<affine_space A>
    struct bob
    {
      using space_type = A;
    };
  }
}

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
    affine_coordinates<A, Basis, Representation, alice<A>, Validator>,
    affine_coordinates<A, Basis,Representation, bob<A>, Validator>
  >
  {
    using disp_type = affine_coordinates<A, Basis, Representation, alice<A>, Validator>::displacement_coordinates_type;

    disp_type displacement{};

    explicit coordinate_transformation(const disp_type& d)
      : displacement{d}
    {}
      
    [[nodiscard]]
    constexpr affine_coordinates<A, Basis, Representation, bob<A>, Validator>
      operator()(const affine_coordinates<A, Basis, Representation, alice<A>, Validator>& c) const noexcept
      {     
        return affine_coordinates<A, Basis, Representation, bob<A>, Validator>{(c + displacement).values()};
      }
  };
}

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path affine_coordinates_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void affine_coordinates_test::run_tests()
  {
    test_affine<sets::R<1>, sets::R<1>, 1, float>();
    test_affine<sets::R<1>, sets::R<1>, 1, double>();
    test_affine<sets::C<1>, sets::C<1>, 1, std::complex<float>>();
    test_affine<sets::C<1>, sets::R<1>, 2, float>();
  }

  template<class Set, class Field, std::size_t D, class Rep>
    requires maths::identifies_as_field_v<Field>
  void affine_coordinates_test::test_affine()
  {
    using space_t  = my_affine_space<Set, Field, D>;
    using basis_t  = canonical_basis<Set, Field, D>;
    using affine_t = affine_coordinates<space_t, basis_t, canonical_representation<Rep, no_bounds<to_bounds_value_type_t<Rep>>>, alice<space_t>, identity_validator>;
    using delta_t  = affine_t::displacement_coordinates_type;
    using value_t  = Rep;
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

    using affine2_t = affine_coordinates<space_t, basis_t, canonical_representation<Rep, no_bounds<to_bounds_value_type_t<Rep>>>, bob<space_t>, identity_validator>;
    affine2_t bob_coords{coordinate_transformation<affine_t, affine2_t>{delta_t{Rep{-1.0}}}(affine_t{})};

    check(equality, "", bob_coords, affine2_t{Rep{-1.0}});
  }
}
