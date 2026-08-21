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
    template<m_affine_space A>
    struct alice
    {
      using space_type = A;
    };

    template<m_affine_space A>
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
    test_affine<sets::R<1>, commutative_rings::reals<1> , 1, float>();
    test_affine<sets::R<1>, commutative_rings::reals<1> , 1, double>();
    test_affine<sets::C<1>, commutative_rings::complexes, 1, std::complex<float>>();
    test_affine<sets::C<1>, commutative_rings::reals<1> , 2, float>();

    test_m_affine<sets::Z<1>, commutative_rings::integers<1>, 1, int>();
    test_m_affine<sets::Z<2>, commutative_rings::integers<2>, 2, int>();
  }

  template<class Set, class Field, std::size_t D, class Rep>
    requires maths::identifies_as_field_v<Field>
  void affine_coordinates_test::test_affine()
  {
    using space_t  = my_affine_space<Set, Field, D>;
    using basis_t  = general_basis<free_module_type_of_t<space_t>>;
    using rep_t    = canonical_representation<Rep, no_bounds<to_bounds_value_type_t<Rep>>>;
    using affine_t = affine_coordinates<space_t, basis_t, rep_t, alice<space_t>, identity_validator>;
    using delta_t  = affine_t::displacement_coordinates_type;
    using value_t  = Rep;
    
    STATIC_CHECK(not defines_rank_v<space_t>);
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

  /*! An M-affine space is an affine space over a free module rather than a vector
      space. The action of the module remains total, so the arithmetic available is
      exactly that of the affine case; what is lost is division, since the ring is
      not a field.
   */
  template<class Set, class Ring, std::size_t D, class Rep>
    requires (!maths::identifies_as_field_v<Ring>)
  void affine_coordinates_test::test_m_affine()
  {
    using space_t    = my_m_affine_space<Set, Ring, D>;
    using module_t   = free_module_type_of_t<space_t>;
    using basis_t    = general_basis<module_t>;
    using rep_t      = canonical_representation<Rep, no_bounds<to_bounds_value_type_t<Rep>>>;
    using m_affine_t = m_affine_coordinates<space_t, basis_t, rep_t, alice<space_t>, identity_validator>;
    using delta_t    = m_affine_t::displacement_coordinates_type;
    using value_t    = Rep;

    STATIC_CHECK(m_affine_space<space_t>);
    STATIC_CHECK(!affine_space<space_t>);
    STATIC_CHECK(!free_module<space_t>);
    STATIC_CHECK(free_module<module_t>);
    STATIC_CHECK(!vector_space<module_t>);
    STATIC_CHECK(basis_for<basis_t, module_t>);
    STATIC_CHECK(not defines_rank_v<space_t>);
    STATIC_CHECK(dimension_of_v<space_t> == D);

    // The affine arithmetic, unchanged by the weakening of the field to a ring.
    STATIC_CHECK(!can_add<m_affine_t, m_affine_t>);
    STATIC_CHECK(can_add<m_affine_t, delta_t>);
    STATIC_CHECK(can_subtract<m_affine_t, m_affine_t>);
    STATIC_CHECK(can_subtract<m_affine_t, delta_t>);
    STATIC_CHECK(has_unary_plus<m_affine_t>);
    STATIC_CHECK(!has_unary_minus<m_affine_t>);
    STATIC_CHECK(!can_multiply<m_affine_t, value_t>);
    STATIC_CHECK(!can_divide<m_affine_t, value_t>);

    // Displacements form the module, so they scale by ring elements but, unlike
    // the vector space case, cannot be divided by them.
    STATIC_CHECK(can_multiply<delta_t, value_t>);
    STATIC_CHECK(!can_divide<delta_t, value_t>);
    STATIC_CHECK(!can_divide<delta_t, delta_t>);
    STATIC_CHECK(has_unary_minus<delta_t>);

    coordinates_operations<m_affine_t>{*this}.execute();
  }
}
