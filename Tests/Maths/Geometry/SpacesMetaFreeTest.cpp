////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2025.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "SpacesMetaFreeTest.hpp"
#include "sequoia/Maths/Geometry/Spaces.hpp"

namespace sequoia::testing
{
  
  using namespace maths;

  namespace
  {
    // Fixtures for the origin and orthant traits.

    struct distinguished_origin_space {
      using set_type             = sets::R<1>;
      using free_module_type     = euclidean_vector_space<1>;
      using structure            = convex_space_tag_t;
      using distinguished_origin = std::true_type;
    };

    struct half_line_space {
      using set_type             = sets::R<1>;
      using free_module_type     = euclidean_vector_space<1>;
      using structure            = convex_space_tag_t;
      using non_negative_orthant = std::true_type;
    };

    struct unremarkable_space {
      using set_type         = sets::R<1>;
      using free_module_type = euclidean_vector_space<1>;
      using structure        = convex_space_tag_t;
    };

    // Fixtures occupying the nodes of the DAG of spaces set out in the
    // introduction to Spaces.hpp. Those over the integers cannot be affine,
    // whatever else they are, since the integers are not a field.

    struct integral_module {
      using set_type              = sets::Z<1>;
      using commutative_ring_type = commutative_rings::integers<1>;
      using structure             = free_module_tag_t;
      constexpr static std::size_t rank{1};
    };

    struct integral_partial_m_torsor {
      using set_type         = sets::Z<1>;
      using free_module_type = integral_module;
      using structure        = partial_m_torsor_tag_t;
    };

    struct integral_m_affine_space {
      using set_type         = sets::Z<1>;
      using free_module_type = integral_module;
      using structure        = m_affine_space_tag_t;
    };

    // Tagged as convex, but over the integers: an ordered ring is not an ordered
    // field, so the tag alone must not be enough.
    struct integral_pseudo_convex_space {
      using set_type         = sets::N_0<1>;
      using free_module_type = integral_module;
      using structure        = convex_space_tag_t;
    };

    // The action is total and the reals are an ordered field, so this is affine
    // and convex without saying so. Only the M-affine tag is available to say it
    // with, there being no affine tag: the promotion to affine is settled by the
    // ring.
    struct real_m_affine_space {
      using set_type         = sets::R<1>;
      using free_module_type = euclidean_vector_space<1>;
      using structure        = m_affine_space_tag_t;
    };

    // Over an ordered field, yet not convex: the action is only partial, so the
    // space is free to comprise, say, two disjoint intervals. This is why
    // convexity must remain a tag and cannot be read off the ring.
    struct real_partial_m_torsor {
      using set_type         = sets::R<1>;
      using free_module_type = euclidean_vector_space<1>;
      using structure        = partial_m_torsor_tag_t;
    };

    struct complex_vector_space {
      using set_type   = sets::C<1>;
      using field_type = commutative_rings::complexes;
      using structure  = vector_space_tag_t;
      constexpr static std::size_t dimension{1};
    };

    struct complex_affine_space {
      using set_type          = sets::C<1>;
      using vector_space_type = complex_vector_space;
      using structure         = m_affine_space_tag_t;
    };

    /*! A partial M-torsor over the complex numbers, and pointed, tensor products
        requiring a distinguished origin of their factors. The ring being a field
        but not an ordered one, this is the cheapest space which is admissible
        everywhere yet convex nowhere - which is what makes it the right probe for
        the non-convex branch of the derived spaces below.
     */
    struct complex_pointed_torsor {
      using set_type             = sets::C<1>;
      using free_module_type     = complex_vector_space;
      using structure            = partial_m_torsor_tag_t;
      using distinguished_origin = std::true_type;
    };
  }
  
  [[nodiscard]]
  std::filesystem::path spaces_meta_free_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void spaces_meta_free_test::run_tests()
  {
    test_arithmetic_traits();
    test_origin_and_orthant_traits();
    test_commutative_rings();
    test_basis_traits();
    test_spaces_dag();
    test_derived_spaces();
  }

  void spaces_meta_free_test::test_arithmetic_traits()
  {
    STATIC_CHECK(is_addable_v<int>);
    STATIC_CHECK(is_subtractable_v<int>);
    STATIC_CHECK(is_multiplicable_v<int>);
    STATIC_CHECK(is_divisible_v<int>);

    STATIC_CHECK(weakly_abelian_group_under_addition_v<int>);
    STATIC_CHECK(weakly_abelian_group_under_addition_v<std::size_t>);
    STATIC_CHECK(weakly_abelian_group_under_addition_v<float>);
    STATIC_CHECK(weakly_abelian_group_under_addition_v<double>);
    STATIC_CHECK(weakly_abelian_group_under_addition_v<std::complex<float>>);
    STATIC_CHECK(weakly_abelian_group_under_addition_v<std::complex<double>>);
    
    STATIC_CHECK(!weakly_abelian_group_under_multiplication_v<int>);
    STATIC_CHECK(!weakly_abelian_group_under_multiplication_v<std::size_t>);
    STATIC_CHECK(weakly_abelian_group_under_multiplication_v<float>);
    STATIC_CHECK(weakly_abelian_group_under_multiplication_v<double>);
    STATIC_CHECK(weakly_abelian_group_under_multiplication_v<std::complex<float>>);
    STATIC_CHECK(weakly_abelian_group_under_multiplication_v<std::complex<double>>);
  }

  void spaces_meta_free_test::test_origin_and_orthant_traits()
  {
    STATIC_CHECK(has_distinguished_origin_v<distinguished_origin_space>);
    STATIC_CHECK(!is_non_negative_orthant_v<distinguished_origin_space>);

    STATIC_CHECK(has_distinguished_origin_v<half_line_space>);
    STATIC_CHECK(is_non_negative_orthant_v<half_line_space>);
    STATIC_CHECK(has_distinguished_origin_v<dual<half_line_space>>);
    STATIC_CHECK(is_non_negative_orthant_v<dual<half_line_space>>);

    STATIC_CHECK(!has_distinguished_origin_v<unremarkable_space>);
    STATIC_CHECK(!is_non_negative_orthant_v<unremarkable_space>);
    STATIC_CHECK(!has_distinguished_origin_v<dual<unremarkable_space>>);
    STATIC_CHECK(!is_non_negative_orthant_v<dual<unremarkable_space>>);
  }

  /*! The commutative-ring diamond of the introduction. Being ordered and being a
      field are independent: the integers have the first, the complexes the
      second, and only the reals have both.
   */
  void spaces_meta_free_test::test_commutative_rings()
  {
    STATIC_CHECK( commutative_ring<commutative_rings::integers<1>>);
    STATIC_CHECK( ordered_ring<commutative_rings::integers<1>>);
    STATIC_CHECK(!field<commutative_rings::integers<1>>);
    STATIC_CHECK(!ordered_field<commutative_rings::integers<1>>);

    STATIC_CHECK( commutative_ring<commutative_rings::complexes>);
    STATIC_CHECK(!ordered_ring<commutative_rings::complexes>);
    STATIC_CHECK( field<commutative_rings::complexes>);
    STATIC_CHECK(!ordered_field<commutative_rings::complexes>);

    STATIC_CHECK( commutative_ring<commutative_rings::reals<1>>);
    STATIC_CHECK( ordered_ring<commutative_rings::reals<1>>);
    STATIC_CHECK( field<commutative_rings::reals<1>>);
    STATIC_CHECK( ordered_field<commutative_rings::reals<1>>);

    // Only R itself is a field: R^2 under componentwise multiplication has zero
    // divisors, and its product order is not total.
    STATIC_CHECK( commutative_ring<commutative_rings::reals<2>>);
    STATIC_CHECK(!ordered_ring<commutative_rings::reals<2>>);
    STATIC_CHECK(!field<commutative_rings::reals<2>>);
    STATIC_CHECK(!ordered_field<commutative_rings::reals<2>>);
  }

  void spaces_meta_free_test::test_basis_traits()
  {
    // The basis concept requires index_set to define d values. Pin the count
    // rather than the spelling, since it is the count the contract states and
    // nothing in the codebase reads the type.
    STATIC_CHECK(general_basis<euclidean_vector_space<1>>::index_set::size() == 1);
    STATIC_CHECK(general_basis<euclidean_vector_space<3>>::index_set::size() == 3);
    STATIC_CHECK(general_basis<integral_module>::index_set::size() == 1);

    STATIC_CHECK(basis_for<general_basis<euclidean_vector_space<3>>, euclidean_vector_space<3>>);
    STATIC_CHECK(!basis_for<general_basis<euclidean_vector_space<3>>, euclidean_vector_space<1>>);
  }

  /*! Pins the DAG of spaces. For each node there is a fixture which sits at
      precisely that node, and for each fixture every one of the six concepts
      is asserted, whether it holds or not. The negatives carry most of the
      weight: the hierarchy has previously drifted by a refinement quietly
      acquiring a parent it should not have.
   */
  void spaces_meta_free_test::test_spaces_dag()
  {
    // A partial M-torsor and nothing more: the free module acts only
    // partially, so there is no sense in which any displacement may be applied
    // to any point.
    STATIC_CHECK( partial_m_torsor<integral_partial_m_torsor>);
    STATIC_CHECK(!convex_space<integral_partial_m_torsor>);
    STATIC_CHECK(!m_affine_space<integral_partial_m_torsor>);
    STATIC_CHECK(!affine_space<integral_partial_m_torsor>);
    STATIC_CHECK(!free_module<integral_partial_m_torsor>);
    STATIC_CHECK(!vector_space<integral_partial_m_torsor>);

    // The same node, but over an ordered field. Convexity still fails, and can
    // only fail on the tag: this is the fixture which keeps the tag honest.
    STATIC_CHECK( partial_m_torsor<real_partial_m_torsor>);
    STATIC_CHECK(!convex_space<real_partial_m_torsor>);
    STATIC_CHECK(!m_affine_space<real_partial_m_torsor>);
    STATIC_CHECK(!affine_space<real_partial_m_torsor>);
    STATIC_CHECK(!free_module<real_partial_m_torsor>);
    STATIC_CHECK(!vector_space<real_partial_m_torsor>);

    // Identifying as convex is necessary but not sufficient: interpolation needs
    // an ordered field, and the integers are only an ordered ring.
    STATIC_CHECK( partial_m_torsor<integral_pseudo_convex_space>);
    STATIC_CHECK(!convex_space<integral_pseudo_convex_space>);
    STATIC_CHECK(!m_affine_space<integral_pseudo_convex_space>);
    STATIC_CHECK(!affine_space<integral_pseudo_convex_space>);
    STATIC_CHECK(!free_module<integral_pseudo_convex_space>);
    STATIC_CHECK(!vector_space<integral_pseudo_convex_space>);

    // M-affine but not affine: the action is total, but the integers are not a
    // field. This is the case the unqualified name would have excluded.
    STATIC_CHECK( partial_m_torsor<integral_m_affine_space>);
    STATIC_CHECK(!convex_space<integral_m_affine_space>);
    STATIC_CHECK( m_affine_space<integral_m_affine_space>);
    STATIC_CHECK(!affine_space<integral_m_affine_space>);
    STATIC_CHECK(!free_module<integral_m_affine_space>);
    STATIC_CHECK(!vector_space<integral_m_affine_space>);

    // The same node over the reals, and therefore affine, and therefore convex.
    // Nothing marks it as either; the ring settles both.
    STATIC_CHECK( partial_m_torsor<real_m_affine_space>);
    STATIC_CHECK( convex_space<real_m_affine_space>);
    STATIC_CHECK( m_affine_space<real_m_affine_space>);
    STATIC_CHECK( affine_space<real_m_affine_space>);
    STATIC_CHECK(!free_module<real_m_affine_space>);
    STATIC_CHECK(!vector_space<real_m_affine_space>);

    // A free module is an M-affine space over itself.
    STATIC_CHECK( partial_m_torsor<integral_module>);
    STATIC_CHECK(!convex_space<integral_module>);
    STATIC_CHECK( m_affine_space<integral_module>);
    STATIC_CHECK(!affine_space<integral_module>);
    STATIC_CHECK( free_module<integral_module>);
    STATIC_CHECK(!vector_space<integral_module>);

    // An affine space over the complex numbers. C is a field, so this is
    // affine; C cannot be ordered, so this can never be convex.
    STATIC_CHECK( partial_m_torsor<complex_affine_space>);
    STATIC_CHECK(!convex_space<complex_affine_space>);
    STATIC_CHECK( m_affine_space<complex_affine_space>);
    STATIC_CHECK( affine_space<complex_affine_space>);
    STATIC_CHECK(!free_module<complex_affine_space>);
    STATIC_CHECK(!vector_space<complex_affine_space>);

    // The same holds of the vector space it is modelled on, which is
    // additionally a free module.
    STATIC_CHECK( partial_m_torsor<complex_vector_space>);
    STATIC_CHECK(!convex_space<complex_vector_space>);
    STATIC_CHECK( m_affine_space<complex_vector_space>);
    STATIC_CHECK( affine_space<complex_vector_space>);
    STATIC_CHECK( free_module<complex_vector_space>);
    STATIC_CHECK( vector_space<complex_vector_space>);

    // The spaces defined by Spaces.hpp itself sit where they should.
    STATIC_CHECK( partial_m_torsor<euclidean_vector_space<1>>);
    STATIC_CHECK( convex_space<euclidean_vector_space<1>>);
    STATIC_CHECK( m_affine_space<euclidean_vector_space<1>>);
    STATIC_CHECK( affine_space<euclidean_vector_space<1>>);
    STATIC_CHECK( free_module<euclidean_vector_space<1>>);
    STATIC_CHECK( vector_space<euclidean_vector_space<1>>);

    STATIC_CHECK( partial_m_torsor<euclidean_affine_space<1>>);
    STATIC_CHECK( convex_space<euclidean_affine_space<1>>);
    STATIC_CHECK( m_affine_space<euclidean_affine_space<1>>);
    STATIC_CHECK( affine_space<euclidean_affine_space<1>>);
    STATIC_CHECK(!free_module<euclidean_affine_space<1>>);
    STATIC_CHECK(!vector_space<euclidean_affine_space<1>>);

    STATIC_CHECK( partial_m_torsor<euclidean_nonnegative_space<1>>);
    STATIC_CHECK( convex_space<euclidean_nonnegative_space<1>>);
    STATIC_CHECK(!m_affine_space<euclidean_nonnegative_space<1>>);
    STATIC_CHECK(!affine_space<euclidean_nonnegative_space<1>>);
    STATIC_CHECK(!free_module<euclidean_nonnegative_space<1>>);
    STATIC_CHECK(!vector_space<euclidean_nonnegative_space<1>>);
  }

  /*! Duals and tensor products decide their own structure tag from that of the
      spaces they are built from. Everything else in the suite is built over the
      reals, so without these the convex branch of that decision is the only one
      ever taken.
   */
  void spaces_meta_free_test::test_derived_spaces()
  {
    STATIC_CHECK( convex_space<dual<half_line_space>>);
    STATIC_CHECK( convex_space<dual<unremarkable_space>>);

    STATIC_CHECK( partial_m_torsor<dual<complex_pointed_torsor>>);
    STATIC_CHECK(!convex_space<dual<complex_pointed_torsor>>);

    STATIC_CHECK( convex_space<tensor_product<distinguished_origin_space, distinguished_origin_space>>);

    STATIC_CHECK( partial_m_torsor<tensor_product<complex_pointed_torsor, complex_pointed_torsor>>);
    STATIC_CHECK(!convex_space<tensor_product<complex_pointed_torsor, complex_pointed_torsor>>);
  }
}
