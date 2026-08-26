////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2025.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "SpacesMetaFreeTest.hpp"
#include "CommonGeometryTestingUtilities.hpp"

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

    // Space-shaped but for the absent structure, and so a sharper negative than
    // an unrelated type: it looks like a space yet satisfies nothing.
    struct structureless_space {
      using set_type         = sets::R<1>;
      using free_module_type = euclidean_vector_space<1>;
    };

    template<class T>
    inline constexpr bool structure_is_extractable_v{requires { typename structure_of_t<T>; }};

    // A free module naming its rank, and a vector space built upon it which
    // additionally names its dimension. Both members are then visible, which
    // must be allowed.
    struct ranked_module_base {
      constexpr static std::size_t rank{3};
    };

    struct dimensioned_derived_space : ranked_module_base {
      constexpr static std::size_t dimension{3};
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
    test_integral_coverings();
    test_structure_trait();
    test_rank_traits();
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

    STATIC_CHECK( weak_commutative_ring<int>);
    STATIC_CHECK(!weak_field<int>);
    STATIC_CHECK( weak_field<double>);

    // bool is the one fundamental type whose value set carries a field, and the one whose
    // operators decline to supply it: `true + true` is `true`.
    STATIC_CHECK(!weakly_abelian_group_under_addition_v<bool>);
    STATIC_CHECK( weakly_abelian_group_under_multiplication_v<bool>);
    STATIC_CHECK(!weak_commutative_ring<bool>);
    STATIC_CHECK(!weak_field<bool>);
  }

  void spaces_meta_free_test::test_integral_coverings()
  {
    STATIC_CHECK( covered_by<int, int>);
    STATIC_CHECK( covered_by<unsigned char, int>);
    STATIC_CHECK( covered_by<unsigned char, long>);
    STATIC_CHECK(!covered_by<int, unsigned>);
    STATIC_CHECK(!covered_by<unsigned, int>);

    // No standard signed type is wider, so half the values are lost; this is why
    // std::size_t has no signed covering type on the 64-bit platforms.
    STATIC_CHECK(!covered_by<unsigned long long, long long>);

    // Only the integral types participate, whatever the ranges may be.
    STATIC_CHECK(!covered_by<float, double>);
    STATIC_CHECK(!covered_by<int, double>);

    // bool aside: every integral type covers its two values, but it is not a number.
    STATIC_CHECK(!covered_by<bool, int>);

    // Every candidate from short upwards covers unsigned char; the narrowest wins,
    // and for the unsigned types that is always the next width up.
    STATIC_CHECK(std::same_as<signed_covering_type_t<signed char>,    signed char>);
    STATIC_CHECK(std::same_as<signed_covering_type_t<unsigned char>,  short>);
    STATIC_CHECK(std::same_as<signed_covering_type_t<short>,          short>);
    STATIC_CHECK(std::same_as<signed_covering_type_t<unsigned short>, int>);
    STATIC_CHECK(std::same_as<signed_covering_type_t<int>,            int>);

    // char is signed char or short according to whether the implementation gives
    // char a sign, so only the width is portable.
    STATIC_CHECK(sizeof(signed_covering_type_t<char>) <= sizeof(short));

    // Which of long and long long covers unsigned varies with the data model, so
    // only the defining properties are portable.
    STATIC_CHECK(std::is_signed_v<signed_covering_type_t<unsigned>>);
    STATIC_CHECK(covered_by<unsigned, signed_covering_type_t<unsigned>>);
    STATIC_CHECK(sizeof(signed_covering_type_t<unsigned>) > sizeof(unsigned));

    STATIC_CHECK( has_signed_covering_type_v<unsigned char>);
    STATIC_CHECK(!has_signed_covering_type_v<unsigned long long>);

    // Covering a type's values is not covering its differences, save when it is
    // unsigned: int covers its own values reflexively and holds half their spread.
    STATIC_CHECK( differences_covered_by_v<unsigned char, short>);
    STATIC_CHECK( differences_covered_by_v<int, long long>);
    STATIC_CHECK(!differences_covered_by_v<int, int>);
    STATIC_CHECK(!differences_covered_by_v<unsigned, int>);
    STATIC_CHECK( differences_covered_by_v<double, double>);

    STATIC_CHECK(std::same_as<free_module_representation_value_type_t<double>,         double>);
    STATIC_CHECK(std::same_as<free_module_representation_value_type_t<signed char>,    short>);
    STATIC_CHECK(std::same_as<free_module_representation_value_type_t<unsigned char>,  short>);
    STATIC_CHECK(std::same_as<free_module_representation_value_type_t<short>,          int>);
    STATIC_CHECK(std::same_as<free_module_representation_value_type_t<unsigned short>, int>);
    STATIC_CHECK(std::same_as<free_module_representation_value_type_t<int>,
                              signed_covering_type_t<unsigned>>);
    STATIC_CHECK(std::same_as<free_module_representation_value_type_t<unsigned>,
                              signed_covering_type_t<unsigned>>);

    // Nothing is wide enough, so the widening declines and the type keeps itself -
    // right for a module, and caught by displacement_representation for anything else.
    STATIC_CHECK(std::same_as<free_module_representation_value_type_t<long long>, long long>);
  }

  /** Every concept in the header is gated on a nested `structure`, so a type
      which fails to expose one does not fail loudly: it silently satisfies
      nothing. The negative checks therefore carry as much weight as the
      positive ones. Note that the trait detects the nested type and no more -
      it does not, and cannot, check that the tag is the right one.
   */
  void spaces_meta_free_test::test_structure_trait()
  {
    STATIC_CHECK( has_structure_v<unremarkable_space>);
    STATIC_CHECK(!has_structure_v<structureless_space>);
    STATIC_CHECK(!has_structure_v<int>);

    // The underlying set is not the space: it carries a dimension, so it
    // answers other traits in this family, but it declares no structure.
    STATIC_CHECK(!has_structure_v<sets::R<1>>);

    // Giving structure_of's primary template a fallback type would break the
    // correspondence with the trait; this is the check which would notice.
    STATIC_CHECK( structure_is_extractable_v<unremarkable_space>);
    STATIC_CHECK(!structure_is_extractable_v<structureless_space>);

    // What it extracts, node by node.
    STATIC_CHECK(std::is_same_v<structure_of_t<integral_partial_m_torsor>,     partial_m_torsor_tag_t>);
    STATIC_CHECK(std::is_same_v<structure_of_t<integral_m_affine_space>,       m_affine_space_tag_t>);
    STATIC_CHECK(std::is_same_v<structure_of_t<integral_module>,               free_module_tag_t>);
    STATIC_CHECK(std::is_same_v<structure_of_t<complex_vector_space>,          vector_space_tag_t>);
    STATIC_CHECK(std::is_same_v<structure_of_t<euclidean_vector_space<1>>,     vector_space_tag_t>);
    STATIC_CHECK(std::is_same_v<structure_of_t<euclidean_affine_space<1>>,     m_affine_space_tag_t>);
    STATIC_CHECK(std::is_same_v<structure_of_t<euclidean_nonnegative_space<1>>, convex_space_tag_t>);

    // The family is not confined to spaces: the commutative rings declare
    // their structure the same way, and it is this that test_commutative_rings
    // reads indirectly through the concepts.
    STATIC_CHECK(std::is_same_v<structure_of_t<commutative_rings::integers<1>>, ordered_ring_tag_t>);
    STATIC_CHECK(std::is_same_v<structure_of_t<commutative_rings::complexes>,   field_tag_t>);
    STATIC_CHECK(std::is_same_v<structure_of_t<commutative_rings::reals<1>>,    ordered_field_tag_t>);
    STATIC_CHECK(std::is_same_v<structure_of_t<commutative_rings::reals<2>>,    commutative_ring_tag_t>);

    // Derived spaces compute their tag rather than declaring it. The concept
    // checks in test_derived_spaces observe that computation only through its
    // consequences; these pin the result itself.
    STATIC_CHECK(std::is_same_v<structure_of_t<dual<half_line_space>>,        convex_space_tag_t>);
    STATIC_CHECK(std::is_same_v<structure_of_t<dual<complex_pointed_torsor>>, partial_m_torsor_tag_t>);
    STATIC_CHECK(std::is_same_v<structure_of_t<dual<complex_affine_space>>,   m_affine_space_tag_t>);
    STATIC_CHECK(std::is_same_v<structure_of_t<dual<complex_vector_space>>,   vector_space_tag_t>);

    STATIC_CHECK(std::is_same_v<structure_of_t<tensor_product<distinguished_origin_space, distinguished_origin_space>>, convex_space_tag_t>);
    STATIC_CHECK(std::is_same_v<structure_of_t<tensor_product<complex_pointed_torsor, complex_pointed_torsor>>,         partial_m_torsor_tag_t>);
    STATIC_CHECK(std::is_same_v<structure_of_t<tensor_product<euclidean_vector_space<1>, euclidean_vector_space<1>>>,   free_module_tag_t>);
  }

  /** Rank and dimension are one notion under two names, the first preferred for
      free modules and the second for vector spaces. A type may therefore name
      either, and - through inheritance - both. What it may not do is name both
      and disagree.
   */
  void spaces_meta_free_test::test_rank_traits()
  {
    STATIC_CHECK( has_rank_v<integral_module>);
    STATIC_CHECK(!has_dimension_v<integral_module>);
    STATIC_CHECK( rank_of_v<integral_module> == 1);

    STATIC_CHECK(!has_rank_v<complex_vector_space>);
    STATIC_CHECK( has_dimension_v<complex_vector_space>);
    STATIC_CHECK( rank_of_v<complex_vector_space> == 1);
    STATIC_CHECK( rank_of_v<euclidean_vector_space<3>> == 3);

    // The underlying sets name a dimension, so they answer this family despite
    // not being spaces at all - compare test_structure_trait, where the same
    // types answer nothing.
    STATIC_CHECK( rank_of_v<sets::R<3>> == 3);

    // Both names visible, by inheritance, and agreeing. rank takes precedence.
    STATIC_CHECK( has_rank_v<dimensioned_derived_space>);
    STATIC_CHECK( has_dimension_v<dimensioned_derived_space>);
    STATIC_CHECK( rank_of_v<dimensioned_derived_space> == 3);

    // Consistency is trivially satisfied when at most one name is present.
    STATIC_CHECK( rank_and_dimension_consistent_v<dimensioned_derived_space>);
    STATIC_CHECK( rank_and_dimension_consistent_v<integral_module>);
    STATIC_CHECK( rank_and_dimension_consistent_v<complex_vector_space>);
    STATIC_CHECK( rank_and_dimension_consistent_v<int>);

    STATIC_CHECK( defines_rank_v<integral_module>);
    STATIC_CHECK( defines_rank_v<dimensioned_derived_space>);
    STATIC_CHECK(!defines_rank_v<int>);
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
    // A basis defines an index set with one index per basis element. Pin the count
    // rather than the spelling, since it is the count the contract states and
    // nothing in the codebase reads the type.
    STATIC_CHECK(basis<euclidean_vector_space<1>>::index_set_type::size() == 1);
    STATIC_CHECK(basis<euclidean_vector_space<3>>::index_set_type::size() == 3);
    STATIC_CHECK(basis<integral_module, nominated_basis_data<>>::index_set_type::size() == 1);

    // Basis data names both ingredients the introduction identifies: an index set and a frame.
    STATIC_CHECK( has_frame_v<canonical_basis_data<3>>);
    STATIC_CHECK( has_index_set_v<canonical_basis_data<3>>);
    STATIC_CHECK( has_frame_v<alices_basis_data<3>>);
    STATIC_CHECK( has_frame_v<alices_basis_data<3, reflection>>);
    STATIC_CHECK(!has_frame_v<euclidean_vector_space<3>>);
    STATIC_CHECK(!has_index_set_v<euclidean_vector_space<3>>);

    STATIC_CHECK(basis<euclidean_vector_space<3>, alices_basis_data<3>>::index_set_type::size() == 3);
    STATIC_CHECK(std::same_as<basis<euclidean_vector_space<3>, alices_basis_data<3, reflection>>::frame_type,
                              alices_frame<reflection>>);

    // A basis is not basis data. It names frame_type/index_set_type where data names
    // frame/index_set, so the two cannot be passed for one another - which they silently
    // could when both spelled the members the same way.
    STATIC_CHECK(!has_frame_v<basis<euclidean_vector_space<3>>>);
    STATIC_CHECK(!has_index_set_v<basis<euclidean_vector_space<3>>>);
    STATIC_CHECK(!basis_data_for<basis<euclidean_vector_space<3>>, euclidean_vector_space<3>>);

    // Pairing, condition one: a supplied index set must have one index per basis element.
    STATIC_CHECK( has_consistent_index_set_v<mismatched_basis_data, euclidean_vector_space<7>>);
    STATIC_CHECK(!has_consistent_index_set_v<mismatched_basis_data, euclidean_vector_space<3>>);
    // TO DO: false once the enumerators can be counted; pinned so that the day they can,
    // this line fails and says so.
    STATIC_CHECK( has_consistent_index_set_v<enumerated_basis_data, euclidean_vector_space<3>>);
    // Neither sized nor an enumeration, so not an index set: rejected rather than waved through.
    STATIC_CHECK(!has_consistent_index_set_v<unindexable_basis_data, euclidean_vector_space<3>>);

    STATIC_CHECK( basis_data_for<canonical_basis_data<3>, euclidean_vector_space<3>>);
    STATIC_CHECK(!basis_data_for<canonical_basis_data<2>, euclidean_vector_space<3>>);
    STATIC_CHECK( basis_data_for<alices_basis_data<3, reflection>, euclidean_vector_space<3>>);
    STATIC_CHECK(!basis_data_for<mismatched_basis_data, euclidean_vector_space<3>>);
    STATIC_CHECK( basis_data_for<mismatched_basis_data, euclidean_vector_space<7>>);
    STATIC_CHECK(!basis_data_for<euclidean_vector_space<3>, euclidean_vector_space<3>>);

    // Naming the identity as the frame asserts the module *is* R^S. Only a module which
    // declares as much may do so; an abstract free module must nominate a frame, and has
    // no default to fall back on.
    STATIC_CHECK( admits_canonical_basis_v<euclidean_vector_space<3>>);
    STATIC_CHECK(!admits_canonical_basis_v<integral_module>);
    STATIC_CHECK( names_admissible_frame_v<nominated_basis_data<>, integral_module>);
    STATIC_CHECK(!names_admissible_frame_v<canonical_basis_data<1>, integral_module>);
    STATIC_CHECK(!basis_data_for<canonical_basis_data<1>, integral_module>);
    STATIC_CHECK( basis_data_for<nominated_basis_data<>,       integral_module>);

    // The canonical basis is therefore what a basis defaults to, and the default stands
    // or falls with the module's declaration. The falling case cannot be pinned here:
    // naming basis<integral_module> is ill-formed rather than merely false, which is the
    // point - the client is told, at the point of use, which rule they have broken.
    STATIC_CHECK(std::same_as<basis<euclidean_vector_space<3>>::basis_data_type, canonical_basis_data<3>>);

    // Pairing is answerable for arbitrary types, yielding false rather than a hard error.
    STATIC_CHECK(!basis_data_for<canonical_basis_data<3>, int>);
    STATIC_CHECK(!basis_data_for<int, euclidean_vector_space<3>>);

    // Comparability is exactly agreement on the convention. Two bases over Alice's
    // convention are related whatever their automorphisms; Bob's convention is not
    // related to Alice's, which is the static prohibition.
    STATIC_CHECK( consistent_basis_data_v<alices_basis_data<3>, alices_basis_data<3>>);
    STATIC_CHECK( consistent_basis_data_v<alices_basis_data<3>, alices_basis_data<3, reflection>>);
    STATIC_CHECK(!consistent_basis_data_v<alices_basis_data<3>, bobs_basis_data<3>>);
    STATIC_CHECK(!consistent_basis_data_v<alices_basis_data<3, reflection>, bobs_basis_data<3, reflection>>);
    STATIC_CHECK(!consistent_basis_data_v<alices_basis_data<3>, canonical_basis_data<3>>);
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
