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
    // Not integral, yet its differences overflow like any integer's: the sharpest
    // case for the trait declining to vouch for a mixture.
    enum class narrow_enum : unsigned char {};

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

    // Names both markers and denies both. Omitting a marker and setting it false are
    // different statements which the traits must merge, and only a fixture making the
    // second can show that they do.
    struct explicitly_ordinary_space {
      using set_type             = sets::R<1>;
      using free_module_type     = euclidean_vector_space<1>;
      using structure            = convex_space_tag_t;
      using distinguished_origin = std::false_type;
      using non_negative_orthant = std::false_type;
    };

    // Distinct from unremarkable_space and reducing to it. Coordinates in a space and
    // in its base are comparable, which is what makes the reduction worth performing.
    struct refinement_of_unremarkable_space {
      using set_type         = sets::R<1>;
      using free_module_type = euclidean_vector_space<1>;
      using structure        = convex_space_tag_t;
      using base_space       = unremarkable_space;
    };

    // Space-shaped but for the absent structure, and so a sharper negative than
    // an unrelated type: it looks like a space yet satisfies nothing.
    struct structureless_space {
      using set_type         = sets::R<1>;
      using free_module_type = euclidean_vector_space<1>;
    };

    template<class T>
    inline constexpr bool structure_is_extractable_v{requires { typename structure_of_t<T>; }};

    template<class T>
    inline constexpr bool set_type_is_extractable_v{requires { typename set_type_of_t<T>; }};

    template<class T>
    inline constexpr bool ring_type_is_extractable_v{requires { typename nested_commutative_ring_type_t<T>; }};

    template<class T>
    inline constexpr bool module_type_is_extractable_v{requires { typename nested_free_module_type_t<T>; }};

    template<class T>
    inline constexpr bool free_module_is_extractable_v{requires { typename free_module_type_of_t<T>; }};

    template<class Bounds, std::size_t D>
    inline constexpr bool array_bounds_value_type_is_extractable_v{
      requires { typename bounds_value_type<std::array<Bounds, D>>::type; }
    };

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

    // The same statement, made about the canonical basis.
    struct canonical_basis_declining_module : integral_module {
      using admits_canonical_basis = std::false_type;
    };

    // Nothing in the codebase declares itself orthonormal, so without this the trait
    // which gates the inner product has no positive case anywhere.
    struct orthonormal_basis_data : canonical_basis_data<3> {
      using orthonormal = std::true_type;
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

    // Fixtures separating detection of a name from satisfaction of the concept it is
    // supposed to name. Each declares the right member, spelled the right way, holding
    // a type which is nothing of the kind: the has_ traits therefore answer true and
    // the defines_ traits false, which is the whole of the difference between them.

    struct spurious_ring_module {
      using set_type              = sets::Z<1>;
      using commutative_ring_type = int;
      using structure             = free_module_tag_t;
      constexpr static std::size_t rank{1};
    };

    struct spurious_module_torsor {
      using set_type         = sets::Z<1>;
      using free_module_type = int;
      using structure        = partial_m_torsor_tag_t;
    };

    // Tagged a field, yet naming no set. Identification is necessary for the ring
    // concepts and not sufficient, and this is the fixture which says so.
    struct setless_field {
      using structure = field_tag_t;
    };

    // A one-dimensional representation supplying only the single-value interface, and a
    // two-dimensional one supplying only the span interface. Between them they separate
    // representation_for's two disjuncts, which canonical_representation - offering both -
    // cannot.

    struct single_value_representation {
      using value_type = double;
      using free_module_representation = single_value_representation;
      constexpr static auto bounds_v{no_bounds<double>};

      [[nodiscard]] constexpr static double to_underlying(double val)   noexcept { return val; }
      [[nodiscard]] constexpr static double from_underlying(double val) noexcept { return val; }

      // The four optional operations, in their single-value form only, so that each
      // defines_..._for_single_value_v is true and its span counterpart false.
      [[nodiscard]] constexpr static double mul(double val, double s) noexcept { return val * s; }
      [[nodiscard]] constexpr static double div(double val, double s) noexcept { return val / s; }
      [[nodiscard]] constexpr static double add(double lhs, double rhs) noexcept { return lhs + rhs; }
      [[nodiscard]] constexpr static double sub(double lhs, double rhs) noexcept { return lhs - rhs; }
    };

    struct span_representation {
      using value_type = double;
      using free_module_representation = span_representation;
      constexpr static auto bounds_v{no_bounds<double>};

      [[nodiscard]]
      constexpr static std::array<double, 2> to_underlying(std::span<const double, 2> vals) noexcept
      {
        return {vals[0], vals[1]};
      }

      [[nodiscard]]
      constexpr static std::array<double, 2> from_underlying(std::span<const double, 2> vals) noexcept
      {
        return {vals[0], vals[1]};
      }
    };

    /* Each of representation's four requirements, dropped one at a time. The concept
       is a conjunction, so only a fixture failing exactly one clause shows that clause
       is live - which is why each retains the bounds it does not need. Retaining them
       means they are read by a trait and never odr-used, hence the attribute.
     */

    struct representation_without_value_type {
      using free_module_representation = representation_without_value_type;
      [[maybe_unused]] constexpr static auto bounds_v{no_bounds<double>};
    };

    struct representation_without_free_module {
      using value_type = double;
      [[maybe_unused]] constexpr static auto bounds_v{no_bounds<double>};
    };

    struct representation_without_bounds_or_coordinates {
      using value_type = double;
      using free_module_representation = representation_without_bounds_or_coordinates;
    };

    struct uninitializable_representation {
      using value_type = double;
      using free_module_representation = uninitializable_representation;
      [[maybe_unused]] constexpr static auto bounds_v{no_bounds<double>};

      uninitializable_representation() = delete;
    };

    // Names coordinates rather than bounds, which is representation's other disjunct.
    struct heterogeneous_representation {
      using value_type = double;
      using free_module_representation = heterogeneous_representation;
      using coordinates_type = std::tuple<double, float>;
    };

    struct homogeneous_tuple_representation {
      using value_type = double;
      using free_module_representation = homogeneous_tuple_representation;
      using coordinates_type = std::tuple<double, double>;
    };

    // A validator which checks nothing but is not the identity: it accepts a single value
    // and no array, so it separates validator_for's two disjuncts and, being unprivileged,
    // shows that defines_identity_validator_v is a declaration rather than an inference.
    struct single_value_validator {
      template<maths::bounds Bounds>
      [[nodiscard]] constexpr double operator()(Bounds, double val) const noexcept { return val; }
    };

    // Callable, but on nothing the framework will ever offer it.
    struct unusable_validator {
      [[nodiscard]] constexpr int operator()(int i) const noexcept { return i; }
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
    test_coverings();
    test_structure_trait();
    test_set_trait();
    test_rank_traits();
    test_origin_and_orthant_traits();
    test_ring_traits();
    test_commutative_rings();
    test_free_module_traits();
    test_basis_traits();
    test_spaces_dag();
    test_derived_spaces();
    test_representation_traits();
    test_validator_traits();
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


    // bool is the one fundamental type whose value set carries a field, and the one whose
    // operators decline to supply it: `true + true` is `true`.
    STATIC_CHECK(!weakly_abelian_group_under_addition_v<bool>);
    STATIC_CHECK( weakly_abelian_group_under_multiplication_v<bool>);
    STATIC_CHECK(!weak_commutative_ring<bool>);

    // std::is_arithmetic_v sees through cv-qualification, whereas an explicit
    // specialisation matches only the unqualified type. bool's treatment is therefore
    // stated against std::remove_cv_t, or const bool would answer the other way.
    STATIC_CHECK(!weakly_abelian_group_under_addition_v<const bool>);
    STATIC_CHECK( weakly_abelian_group_under_multiplication_v<const bool>);
    STATIC_CHECK( weakly_abelian_group_under_addition_v<const int>);
    STATIC_CHECK( weakly_abelian_group_under_addition_v<volatile double>);

    // Each trait is a class template with a matching alias, and the alias is what a
    // client specializes against; nothing else in the suite would notice it going wrong.
    STATIC_CHECK(std::is_same_v<weakly_abelian_group_under_addition_t<int>,                std::true_type>);
    STATIC_CHECK(std::is_same_v<weakly_abelian_group_under_addition_t<bool>,               std::false_type>);
    STATIC_CHECK(std::is_same_v<weakly_abelian_group_under_multiplication_t<double>,       std::true_type>);
    STATIC_CHECK(std::is_same_v<weakly_abelian_group_under_multiplication_t<int>,          std::false_type>);
    STATIC_CHECK(std::is_same_v<multiplication_weakly_distributive_over_addition_t<int>,   std::true_type>);

    STATIC_CHECK( multiplication_weakly_distributive_over_addition_v<int>);
    STATIC_CHECK( multiplication_weakly_distributive_over_addition_v<double>);
    STATIC_CHECK( multiplication_weakly_distributive_over_addition_v<std::complex<double>>);

    // Alone among the three, this one is true by default, so it is answered even for
    // types with no multiplication to distribute. An author who has both operations
    // and no distributivity must opt out; nothing here can detect the omission.
    STATIC_CHECK( multiplication_weakly_distributive_over_addition_v<narrow_enum>);
    STATIC_CHECK( multiplication_weakly_distributive_over_addition_v<commutative_rings::reals<1>>);

    // The two-parameter forms ask whether a T can absorb a U, which is not symmetric:
    // a complex number takes a double and returns a complex, whereas the reverse
    // returns a complex where a double was wanted.
    STATIC_CHECK( is_addable_to_v<double, std::complex<double>>);
    STATIC_CHECK(!is_addable_to_v<std::complex<double>, double>);
    STATIC_CHECK( is_subtractable_from_v<double, std::complex<double>>);
    STATIC_CHECK(!is_subtractable_from_v<std::complex<double>, double>);

    // Both directions hold among the arithmetic types, the narrowing conversion back
    // being implicit; it is the conversion which is asked about, not its wisdom.
    STATIC_CHECK( is_addable_to_v<int, double>);
    STATIC_CHECK( is_addable_to_v<double, int>);

    STATIC_CHECK(!is_addable_to_v<narrow_enum, int>);
    STATIC_CHECK(!is_subtractable_from_v<int, narrow_enum>);
    STATIC_CHECK(!is_addable_v<narrow_enum>);
    STATIC_CHECK(!is_multiplicable_v<narrow_enum>);

    STATIC_CHECK( is_addable_v<std::complex<double>>);
    STATIC_CHECK( is_subtractable_v<std::complex<double>>);
    STATIC_CHECK( is_multiplicable_v<std::complex<double>>);
    STATIC_CHECK( is_divisible_v<std::complex<double>>);

    STATIC_CHECK(!weak_commutative_ring<bool>);
    STATIC_CHECK( weak_commutative_ring<char>);
    STATIC_CHECK( weak_commutative_ring<int>);
    STATIC_CHECK( weak_commutative_ring<unsigned int>);
    STATIC_CHECK( weak_commutative_ring<long>);
    STATIC_CHECK( weak_commutative_ring<float>);
    STATIC_CHECK( weak_commutative_ring<std::complex<float>>);
    STATIC_CHECK( weak_commutative_ring<std::complex<double>>);
    STATIC_CHECK(!weak_field<bool>);
    STATIC_CHECK(!weak_field<int>);
    STATIC_CHECK(!weak_field<unsigned>);
    STATIC_CHECK( weak_field<float>);
    STATIC_CHECK( weak_field<double>);
    STATIC_CHECK( weak_field<std::complex<double>>);

    /* std::regular is the first thing weak_commutative_ring asks for, and it is
       where cv-qualification is dealt with - but only half of it. A const type is
       not assignable and so not regular; a volatile one is both. The concept
       therefore admits volatile double, and every trait constrained on it is
       reachable with a volatile type however it treats cv internally.
     */
    STATIC_CHECK(!std::regular<const int>);
    STATIC_CHECK(!weak_commutative_ring<const int>);
    STATIC_CHECK( std::regular<volatile double>);
    STATIC_CHECK( weak_commutative_ring<volatile double>);
    STATIC_CHECK( weak_commutative_ring<volatile int>);

    // Neither an arithmetic type nor a ring: regular, and with none of the operations.
    STATIC_CHECK(!weak_commutative_ring<narrow_enum>);
    STATIC_CHECK(!weak_commutative_ring<double*>);
    STATIC_CHECK(!weak_commutative_ring<commutative_rings::reals<1>>);
  }

  void spaces_meta_free_test::test_coverings()
  {
    STATIC_CHECK( covered_by<int, int>);
    STATIC_CHECK( covered_by<unsigned char, long>);
    STATIC_CHECK(!covered_by<int, unsigned>);
    STATIC_CHECK(!covered_by<unsigned, int>);

    // No standard signed type is wider, so half the values are lost; this is why
    // std::size_t has no signed covering type on the 64-bit platforms.
    STATIC_CHECK(!covered_by<unsigned long long, long long>);

    // Every numeric ring participates, and within a family the relation is settled
    // by rank, exactly as among the integers.
    STATIC_CHECK( covered_by<float,  double>);
    STATIC_CHECK(!covered_by<double, float>);
    STATIC_CHECK( covered_by<double, double>);
    STATIC_CHECK( covered_by<float,  float>);

    // The families do not mix, and are not meant to: weakly_represented_by gives the
    // integers an integral representation and the reals a floating-point one, so a
    // covering from the other family is refused whether or not the digits suffice.
    // Braced initialization refuses it unaided, an integer literal being a constant
    // expression where std::declval is not.
    STATIC_CHECK(!covered_by<int,         double>);
    STATIC_CHECK(!covered_by<double,      int>);
    STATIC_CHECK(!covered_by<signed char, float>);
    STATIC_CHECK(!covered_by<float,       long long>);

    /* std::complex reduces to its parts, and is the one case which cannot be left to
       braced initialization: std::complex<float> has an *explicit* constructor from
       std::complex<double>, which direct-list-initialization duly considers, so
       without the specialization each would be pronounced capable of holding the
       other. The second assertion is that trap, named rather than described.
     */
    STATIC_CHECK( covered_by<std::complex<float>,  std::complex<double>>);
    STATIC_CHECK( initializable_from<std::complex<float>, std::complex<double>>);
    STATIC_CHECK(!covered_by<std::complex<double>, std::complex<float>>);
    STATIC_CHECK( covered_by<std::complex<double>, std::complex<double>>);

    // The reals sit inside the complexes, and need no specialization to say so:
    // std::complex's constructor from a scalar is not explicit. The integers stay
    // outside, a floating-point part being demanded, which keeps the families apart
    // here too.
    STATIC_CHECK( covered_by<double, std::complex<double>>);
    STATIC_CHECK( covered_by<float,  std::complex<double>>);
    STATIC_CHECK(!covered_by<double, std::complex<float>>);
    STATIC_CHECK(!covered_by<std::complex<double>, double>);
    STATIC_CHECK(!covered_by<int,    std::complex<double>>);
    STATIC_CHECK(!covered_by<std::complex<double>, int>);

    // The domain of the relation. A character type is a weak commutative ring, its
    // addition wrapping like any small integer's, and is refused here all the same:
    // what its values denote is not a number.
    STATIC_CHECK( numeric_ring<int>);
    STATIC_CHECK( numeric_ring<unsigned>);
    STATIC_CHECK( numeric_ring<signed char>);
    STATIC_CHECK( numeric_ring<unsigned char>);
    STATIC_CHECK( numeric_ring<double>);
    STATIC_CHECK( numeric_ring<std::complex<double>>);
    STATIC_CHECK( weak_commutative_ring<char>);
    STATIC_CHECK(!numeric_ring<char>);
    STATIC_CHECK(!numeric_ring<wchar_t>);
    STATIC_CHECK(!numeric_ring<char8_t>);
    STATIC_CHECK(!numeric_ring<char16_t>);
    STATIC_CHECK(!numeric_ring<char32_t>);
    STATIC_CHECK(!numeric_ring<bool>);
    STATIC_CHECK(!numeric_ring<narrow_enum>);
    STATIC_CHECK(!numeric_ring<const int>);

    // The line std::in_range draws: bool and the character types are integral but
    // not integer types. signed char and unsigned char are integer types, which is
    // what keeps std::int8_t - being signed char - admitted.
    STATIC_CHECK(!covered_by<bool,     int>);
    STATIC_CHECK(!covered_by<char,     int>);
    STATIC_CHECK(!covered_by<char8_t,  int>);
    STATIC_CHECK(!covered_by<char16_t, int>);
    STATIC_CHECK(!covered_by<char32_t, long long>);
    STATIC_CHECK(!covered_by<wchar_t,  long long>);
    STATIC_CHECK(!covered_by<int,      char>);
    STATIC_CHECK( covered_by<signed char,   int>);
    STATIC_CHECK( covered_by<unsigned char, int>);

    // Every candidate from short upwards covers unsigned char; the narrowest wins,
    // and for the unsigned types that is always the next width up.
    STATIC_CHECK(std::same_as<signed_covering_type_t<signed char>,    signed char>);
    STATIC_CHECK(std::same_as<signed_covering_type_t<unsigned char>,  short>);
    STATIC_CHECK(std::same_as<signed_covering_type_t<short>,          short>);
    STATIC_CHECK(std::same_as<signed_covering_type_t<unsigned short>, int>);
    STATIC_CHECK(std::same_as<signed_covering_type_t<int>,            int>);

    // char is not an integer type, so nothing covers it, whatever its width.
    STATIC_CHECK(!has_signed_covering_type_v<char>);

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
    STATIC_CHECK(!differences_covered_by_v<short, unsigned char>);
    STATIC_CHECK( differences_covered_by_v<int, long long>);
    STATIC_CHECK(!differences_covered_by_v<long long, int>);
    STATIC_CHECK(!differences_covered_by_v<int, int>);
    STATIC_CHECK(!differences_covered_by_v<unsigned, int>);
    STATIC_CHECK(!differences_covered_by_v<int, unsigned>);
    STATIC_CHECK(!differences_covered_by_v<char, short>);
    STATIC_CHECK( differences_covered_by_v<signed char, short>);

    // A mixture is refused, so the trait does not answer opposite to covered_by on
    // the same enumeration - which it did, before either was constrained.
    STATIC_CHECK(!differences_covered_by_v<double, short>);
    STATIC_CHECK(!differences_covered_by_v<long long, float>);
    STATIC_CHECK(!covered_by<narrow_enum, signed char>);

    /* Away from the integers the two relations coincide: a difference of two values
       is a value, and no widening is on offer which would make it more so. On the
       integers they part company, and that parting is the whole of the difference
       between them - which is why differences_covered_by_v now says only that.
     */
    STATIC_CHECK(differences_covered_by_v<float, double>              == covered_by<float, double>);
    STATIC_CHECK(differences_covered_by_v<double, float>              == covered_by<double, float>);
    STATIC_CHECK(differences_covered_by_v<double, std::complex<double>> == covered_by<double, std::complex<double>>);
    STATIC_CHECK(differences_covered_by_v<std::complex<float>, std::complex<double>>
                   == covered_by<std::complex<float>, std::complex<double>>);
    STATIC_CHECK( covered_by<int, int>);
    STATIC_CHECK(!differences_covered_by_v<int, int>);

    // A type supplying no arithmetic cannot be asked about differences at all: the
    // trait's parameters are constrained, so naming it is a compilation error rather
    // than an answer. That is not something STATIC_CHECK can state, so pin the door.
    STATIC_CHECK(!weak_commutative_ring<narrow_enum>);
    STATIC_CHECK(!weak_commutative_ring<commutative_rings::reals<1>>);

    // std::integral sees through cv-qualification where std::same_as does not, so
    // the exclusions are written against the unqualified type.
    STATIC_CHECK(!integer<const bool>);
    STATIC_CHECK(!integer<const char>);
    STATIC_CHECK( integer<const int>);
    STATIC_CHECK( differences_covered_by_v<double, double>);
    STATIC_CHECK( differences_covered_by_v<float, double>);
    STATIC_CHECK(!differences_covered_by_v<double, float>);

    // Braced initialization settles the floating-point cases too. The families do
    // not mix, and are not meant to: weakly_represented_by gives the integers an
    // integral representation and the reals a floating-point one, so a difference
    // type from the other family is refused whether or not the digits would suffice.
    STATIC_CHECK(!differences_covered_by_v<signed char, float>);
    STATIC_CHECK(!differences_covered_by_v<int, double>);
    STATIC_CHECK(!differences_covered_by_v<int, float>);
    STATIC_CHECK(!differences_covered_by_v<long long, double>);
    STATIC_CHECK(!differences_covered_by_v<float, int>);
    STATIC_CHECK(!differences_covered_by_v<double, int>);

    // Neither integral nor floating point: nothing to count, so only an unwidened
    // covering is admitted.
    STATIC_CHECK( differences_covered_by_v<std::complex<double>, std::complex<double>>);
    STATIC_CHECK( differences_covered_by_v<std::complex<float>,  std::complex<double>>);
    STATIC_CHECK(!differences_covered_by_v<std::complex<double>, std::complex<float>>);
    STATIC_CHECK(!differences_covered_by_v<std::complex<double>, double>);

    // The reals sit inside the complexes, so a complex covering of no lesser rank
    // holds a floating-point type's differences. An integral type's it does not,
    // a signed integral covering being demanded, which keeps the families apart.
    STATIC_CHECK( differences_covered_by_v<double, std::complex<double>>);
    STATIC_CHECK( differences_covered_by_v<float,  std::complex<double>>);
    STATIC_CHECK(!differences_covered_by_v<double, std::complex<float>>);
    STATIC_CHECK(!differences_covered_by_v<int,    std::complex<double>>);

    // Reversals of every pair above. Both relations are antisymmetric on these
    // types - apart from covered_by's reflexivity no pair holds both ways - so all
    // of these are false. It is the reversal which catches a criterion that has
    // quietly lost a hypothesis, so they are enumerated rather than chosen.
    STATIC_CHECK(!covered_by<long, unsigned char>);
    STATIC_CHECK(!covered_by<long long, unsigned long long>);
    STATIC_CHECK(!covered_by<double, float>);
    STATIC_CHECK(!covered_by<double, int>);
    STATIC_CHECK(!covered_by<int, bool>);
    STATIC_CHECK(!covered_by<int, char8_t>);
    STATIC_CHECK(!covered_by<int, char16_t>);
    STATIC_CHECK(!covered_by<long long, char32_t>);
    STATIC_CHECK(!covered_by<long long, wchar_t>);
    STATIC_CHECK(!covered_by<int, signed char>);
    STATIC_CHECK(!covered_by<int, unsigned char>);
    STATIC_CHECK(!covered_by<signed char, narrow_enum>);
    STATIC_CHECK(!covered_by<signed_covering_type_t<unsigned>, unsigned>);

    STATIC_CHECK(!differences_covered_by_v<short, char>);
    STATIC_CHECK(!differences_covered_by_v<short, signed char>);
    STATIC_CHECK(!differences_covered_by_v<short, double>);
    STATIC_CHECK(!differences_covered_by_v<float, long long>);
    STATIC_CHECK(!differences_covered_by_v<float, signed char>);
    STATIC_CHECK(!differences_covered_by_v<double, long long>);
    STATIC_CHECK(!differences_covered_by_v<std::complex<double>, float>);
    STATIC_CHECK(!differences_covered_by_v<std::complex<float>,  double>);
    STATIC_CHECK(!differences_covered_by_v<std::complex<double>, int>);

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

  /** The companion to test_structure_trait, and deliberately its mirror image: a
      space names both its structure and its underlying set, and the two traits
      differ only in which member they read. What separates them is where each
      says nothing - a set names a dimension but no set of its own, whereas the
      commutative rings name a set and no free module.
   */
  void spaces_meta_free_test::test_set_trait()
  {
    STATIC_CHECK( has_set_type_v<unremarkable_space>);
    STATIC_CHECK( has_set_type_v<commutative_rings::reals<1>>);
    STATIC_CHECK(!has_set_type_v<int>);

    // The one fixture which separates this trait from has_structure_v: it names a set
    // and no structure, so it answers here and nowhere in test_structure_trait.
    STATIC_CHECK( has_set_type_v<structureless_space>);
    STATIC_CHECK(!has_structure_v<structureless_space>);

    // A set is not a space, and does not name a set of its own - the point at which
    // the recursion stops.
    STATIC_CHECK(!has_set_type_v<sets::R<1>>);
    STATIC_CHECK(!has_set_type_v<sets::orthant<2>>);

    STATIC_CHECK( set_type_is_extractable_v<unremarkable_space>);
    STATIC_CHECK(!set_type_is_extractable_v<sets::R<1>>);

    STATIC_CHECK(std::is_same_v<set_type_of_t<euclidean_vector_space<3>>,      sets::R<3>>);
    STATIC_CHECK(std::is_same_v<set_type_of_t<euclidean_affine_space<3>>,      sets::R<3>>);
    STATIC_CHECK(std::is_same_v<set_type_of_t<euclidean_nonnegative_space<2>>, sets::orthant<2>>);
    STATIC_CHECK(std::is_same_v<set_type_of_t<integral_module>,                sets::Z<1>>);
    STATIC_CHECK(std::is_same_v<set_type_of_t<integral_pseudo_convex_space>,   sets::N_0<1>>);
    STATIC_CHECK(std::is_same_v<set_type_of_t<complex_vector_space>,           sets::C<1>>);

    // The rings answer too, which is what makes the underlying set common to both
    // families rather than a property of spaces.
    STATIC_CHECK(std::is_same_v<set_type_of_t<commutative_rings::integers<1>>, sets::Z<1>>);
    STATIC_CHECK(std::is_same_v<set_type_of_t<commutative_rings::reals<2>>,    sets::R<2>>);
    STATIC_CHECK(std::is_same_v<set_type_of_t<commutative_rings::complexes>,   sets::C<1>>);
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

    // An orthant has a distinguished origin whether or not it says so: the half line
    // names only the orthant marker, and the second specialization supplies the rest.
    STATIC_CHECK(!has_distinguished_origin_type_v<half_line_space>);
    STATIC_CHECK( identifies_as_non_negative_orthant_v<half_line_space>);
    STATIC_CHECK( has_distinguished_origin_v<half_line_space>);

    STATIC_CHECK( has_distinguished_origin_type_v<distinguished_origin_space>);
    STATIC_CHECK(!identifies_as_non_negative_orthant_v<distinguished_origin_space>);
    STATIC_CHECK(!has_distinguished_origin_type_v<unremarkable_space>);

    // Declaring a marker false is not omitting it. Both traits must read the value,
    // and the first would answer true were it satisfied by the name alone.
    STATIC_CHECK( has_distinguished_origin_type_v<explicitly_ordinary_space>);
    STATIC_CHECK(!has_distinguished_origin_v<explicitly_ordinary_space>);
    STATIC_CHECK(!identifies_as_non_negative_orthant_v<explicitly_ordinary_space>);
    STATIC_CHECK(!is_non_negative_orthant_v<explicitly_ordinary_space>);

    STATIC_CHECK(std::is_same_v<has_distinguished_origin_t<half_line_space>,           std::true_type>);
    STATIC_CHECK(std::is_same_v<has_distinguished_origin_t<explicitly_ordinary_space>, std::false_type>);
    STATIC_CHECK(std::is_same_v<is_non_negative_orthant_t<half_line_space>,            std::true_type>);
  }

  /*! Whether a type *identifies* as a ring, and whether it *names* one, are two
      questions, and neither is the concept. Identification reads the structure tag
      and so inherits the tag hierarchy's derivations for free; naming reads a
      nested type and cannot tell a ring from an int. The concepts are what put
      the two together, and are checked next door in test_commutative_rings.
   */
  void spaces_meta_free_test::test_ring_traits()
  {
    // The four tags, read off each ring in turn. Since ordered_field_tag_t derives
    // from both ordered_ring_tag_t and field_tag_t, the reals answer to all four,
    // and it is the two rings which answer to exactly one refinement apiece that
    // show the derivations are being read rather than the tag matched.
    STATIC_CHECK( identifies_as_commutative_ring_v<commutative_rings::integers<1>>);
    STATIC_CHECK( identifies_as_ordered_ring_v<commutative_rings::integers<1>>);
    STATIC_CHECK(!identifies_as_field_v<commutative_rings::integers<1>>);
    STATIC_CHECK(!identifies_as_ordered_field_v<commutative_rings::integers<1>>);

    STATIC_CHECK( identifies_as_commutative_ring_v<commutative_rings::complexes>);
    STATIC_CHECK(!identifies_as_ordered_ring_v<commutative_rings::complexes>);
    STATIC_CHECK( identifies_as_field_v<commutative_rings::complexes>);
    STATIC_CHECK(!identifies_as_ordered_field_v<commutative_rings::complexes>);

    STATIC_CHECK( identifies_as_commutative_ring_v<commutative_rings::reals<1>>);
    STATIC_CHECK( identifies_as_ordered_ring_v<commutative_rings::reals<1>>);
    STATIC_CHECK( identifies_as_field_v<commutative_rings::reals<1>>);
    STATIC_CHECK( identifies_as_ordered_field_v<commutative_rings::reals<1>>);

    STATIC_CHECK( identifies_as_commutative_ring_v<commutative_rings::reals<2>>);
    STATIC_CHECK(!identifies_as_ordered_ring_v<commutative_rings::reals<2>>);
    STATIC_CHECK(!identifies_as_field_v<commutative_rings::reals<2>>);
    STATIC_CHECK(!identifies_as_ordered_field_v<commutative_rings::reals<2>>);

    // The space tags form a disjoint hierarchy, so a space identifies as no ring at
    // all - and a type with no structure answers nothing, rather than failing loudly.
    STATIC_CHECK(!identifies_as_commutative_ring_v<euclidean_vector_space<1>>);
    STATIC_CHECK(!identifies_as_field_v<euclidean_vector_space<1>>);
    STATIC_CHECK(!identifies_as_commutative_ring_v<int>);
    STATIC_CHECK(!identifies_as_ordered_field_v<int>);

    // Identification is necessary and not sufficient: the concepts additionally
    // demand an underlying set, and this is the fixture which has the tag without it.
    STATIC_CHECK( identifies_as_field_v<setless_field>);
    STATIC_CHECK( identifies_as_commutative_ring_v<setless_field>);
    STATIC_CHECK(!commutative_ring<setless_field>);
    STATIC_CHECK(!field<setless_field>);

    // Naming a ring. A free module names commutative_ring_type and a vector space
    // field_type; nested_commutative_ring_type reads whichever is present, which is
    // what lets everything downstream ask one question instead of two.
    STATIC_CHECK( has_commutative_ring_type_v<integral_module>);
    STATIC_CHECK(!has_field_type_v<integral_module>);
    STATIC_CHECK(!has_commutative_ring_type_v<euclidean_vector_space<1>>);
    STATIC_CHECK( has_field_type_v<euclidean_vector_space<1>>);

    // A space built over a module names neither: it reaches its ring through the
    // module, which is what commutative_ring_type_of does below.
    STATIC_CHECK(!has_commutative_ring_type_v<euclidean_affine_space<1>>);
    STATIC_CHECK(!has_field_type_v<euclidean_affine_space<1>>);
    STATIC_CHECK(!has_commutative_ring_type_v<int>);

    STATIC_CHECK( ring_type_is_extractable_v<integral_module>);
    STATIC_CHECK( ring_type_is_extractable_v<euclidean_vector_space<1>>);
    STATIC_CHECK(!ring_type_is_extractable_v<euclidean_affine_space<1>>);

    STATIC_CHECK(std::is_same_v<nested_commutative_ring_type_t<integral_module>,           commutative_rings::integers<1>>);
    STATIC_CHECK(std::is_same_v<nested_commutative_ring_type_t<euclidean_vector_space<1>>, commutative_rings::reals<1>>);
    STATIC_CHECK(std::is_same_v<nested_commutative_ring_type_t<complex_vector_space>,      commutative_rings::complexes>);

    // The extraction is naive, and is meant to be: it hands back whatever the member
    // names. Only defines_commutative_ring_v asks whether that is a ring.
    STATIC_CHECK(std::is_same_v<nested_commutative_ring_type_t<spurious_ring_module>, int>);
    STATIC_CHECK( has_commutative_ring_type_v<spurious_ring_module>);
    STATIC_CHECK(!defines_commutative_ring_v<spurious_ring_module>);
    STATIC_CHECK(!free_module<spurious_ring_module>);

    STATIC_CHECK( defines_commutative_ring_v<integral_module>);
    STATIC_CHECK(!defines_field_v<integral_module>);
    STATIC_CHECK( defines_commutative_ring_v<euclidean_vector_space<1>>);
    STATIC_CHECK( defines_field_v<euclidean_vector_space<1>>);
    STATIC_CHECK( defines_field_v<complex_vector_space>);

    // Nothing named at all: false rather than ill-formed, since the nested alias is
    // reached through a defined-but-empty primary.
    STATIC_CHECK(!defines_commutative_ring_v<euclidean_affine_space<1>>);
    STATIC_CHECK(!defines_field_v<int>);

    // commutative_ring_type_of reaches through the space to the free module, so every
    // space over a given module answers with that module's ring, whatever else
    // separates them.
    STATIC_CHECK(std::is_same_v<commutative_ring_type_of_t<integral_module>,                commutative_rings::integers<1>>);
    STATIC_CHECK(std::is_same_v<commutative_ring_type_of_t<integral_partial_m_torsor>,      commutative_rings::integers<1>>);
    STATIC_CHECK(std::is_same_v<commutative_ring_type_of_t<integral_m_affine_space>,        commutative_rings::integers<1>>);
    STATIC_CHECK(std::is_same_v<commutative_ring_type_of_t<euclidean_vector_space<1>>,      commutative_rings::reals<1>>);
    STATIC_CHECK(std::is_same_v<commutative_ring_type_of_t<euclidean_affine_space<3>>,      commutative_rings::reals<1>>);
    STATIC_CHECK(std::is_same_v<commutative_ring_type_of_t<euclidean_nonnegative_space<2>>, commutative_rings::reals<1>>);
    STATIC_CHECK(std::is_same_v<commutative_ring_type_of_t<complex_affine_space>,           commutative_rings::complexes>);
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

    // Which representations each ring will accept. The integers demand a signed
    // integer type: char and wchar_t are integral and may well be signed, but what
    // they represent is not a number, and whether they are signed at all is left to
    // the implementation - so a space over one would mean different things on
    // different platforms.
    STATIC_CHECK( weakly_represented_by_v<commutative_rings::integers<1>, int>);
    STATIC_CHECK( weakly_represented_by_v<commutative_rings::integers<1>, signed char>);
    STATIC_CHECK(!weakly_represented_by_v<commutative_rings::integers<1>, unsigned>);
    STATIC_CHECK(!weakly_represented_by_v<commutative_rings::integers<1>, char>);
    STATIC_CHECK(!weakly_represented_by_v<commutative_rings::integers<1>, wchar_t>);
    STATIC_CHECK(!weakly_represented_by_v<commutative_rings::integers<1>, double>);

    STATIC_CHECK( weakly_represented_by_v<commutative_rings::reals<1>, double>);
    STATIC_CHECK(!weakly_represented_by_v<commutative_rings::reals<1>, int>);
    STATIC_CHECK( weakly_represented_by_v<commutative_rings::complexes, std::complex<double>>);
    STATIC_CHECK(!weakly_represented_by_v<commutative_rings::complexes, double>);

    // Only R itself has a representation; nothing represents R^2, whose values are not
    // those of any one arithmetic type.
    STATIC_CHECK(!weakly_represented_by_v<commutative_rings::reals<2>, double>);

    STATIC_CHECK(std::is_same_v<weakly_represented_by_t<commutative_rings::reals<1>, double>, std::true_type>);
    STATIC_CHECK(std::is_same_v<weakly_represented_by_t<commutative_rings::reals<1>, int>,    std::false_type>);

    // The concept names its parameters in the opposite order to the trait, reading as
    // "double is a weak representation for the reals". Reversing it is what shows the
    // order is load-bearing rather than incidental.
    STATIC_CHECK( weak_representation_for<double, commutative_rings::reals<1>>);
    STATIC_CHECK(!weak_representation_for<commutative_rings::reals<1>, double>);
    STATIC_CHECK( weak_representation_for<int, commutative_rings::integers<1>>);
    STATIC_CHECK(!weak_representation_for<unsigned, commutative_rings::integers<1>>);
    STATIC_CHECK( weak_representation_for<std::complex<double>, commutative_rings::complexes>);
  }

  /*! The counterpart of test_ring_traits, one level up: how a space identifies
      itself, how it names its free module, and how the two are read back. The
      concepts built on all this are pinned by test_spaces_dag; what is pinned here
      is the tag hierarchy those concepts consult, which no concept check can
      isolate, since every concept reads several tags at once.
   */
  void spaces_meta_free_test::test_free_module_traits()
  {
    // The tag DAG, node by node. Convexity branches off the root and the module
    // refinements descend through m_affine, so the two are never both true here -
    // whereas convex_space and m_affine_space, the concepts, can be.
    STATIC_CHECK( identifies_as_partial_m_torsor_v<integral_partial_m_torsor>);
    STATIC_CHECK(!identifies_as_convex_space_v<integral_partial_m_torsor>);
    STATIC_CHECK(!identifies_as_m_affine_space_v<integral_partial_m_torsor>);
    STATIC_CHECK(!identifies_as_free_module_v<integral_partial_m_torsor>);
    STATIC_CHECK(!identifies_as_vector_space_v<integral_partial_m_torsor>);

    STATIC_CHECK( identifies_as_partial_m_torsor_v<integral_pseudo_convex_space>);
    STATIC_CHECK( identifies_as_convex_space_v<integral_pseudo_convex_space>);
    STATIC_CHECK(!identifies_as_m_affine_space_v<integral_pseudo_convex_space>);
    STATIC_CHECK(!identifies_as_free_module_v<integral_pseudo_convex_space>);
    STATIC_CHECK(!identifies_as_vector_space_v<integral_pseudo_convex_space>);

    STATIC_CHECK( identifies_as_partial_m_torsor_v<integral_m_affine_space>);
    STATIC_CHECK(!identifies_as_convex_space_v<integral_m_affine_space>);
    STATIC_CHECK( identifies_as_m_affine_space_v<integral_m_affine_space>);
    STATIC_CHECK(!identifies_as_free_module_v<integral_m_affine_space>);
    STATIC_CHECK(!identifies_as_vector_space_v<integral_m_affine_space>);

    STATIC_CHECK( identifies_as_partial_m_torsor_v<integral_module>);
    STATIC_CHECK(!identifies_as_convex_space_v<integral_module>);
    STATIC_CHECK( identifies_as_m_affine_space_v<integral_module>);
    STATIC_CHECK( identifies_as_free_module_v<integral_module>);
    STATIC_CHECK(!identifies_as_vector_space_v<integral_module>);

    STATIC_CHECK( identifies_as_partial_m_torsor_v<complex_vector_space>);
    STATIC_CHECK(!identifies_as_convex_space_v<complex_vector_space>);
    STATIC_CHECK( identifies_as_m_affine_space_v<complex_vector_space>);
    STATIC_CHECK( identifies_as_free_module_v<complex_vector_space>);
    STATIC_CHECK( identifies_as_vector_space_v<complex_vector_space>);

    // A ring identifies as no space, and a type with no structure as nothing at all.
    STATIC_CHECK(!identifies_as_partial_m_torsor_v<commutative_rings::reals<1>>);
    STATIC_CHECK(!identifies_as_vector_space_v<commutative_rings::reals<1>>);
    STATIC_CHECK(!identifies_as_partial_m_torsor_v<int>);
    STATIC_CHECK(!identifies_as_free_module_v<structureless_space>);

    // Naming a free module. As with the ring, the member is spelled one way by a space
    // over a module and another by one over a vector space, and a free module names
    // neither, being its own.
    STATIC_CHECK( has_free_module_type_v<integral_m_affine_space>);
    STATIC_CHECK(!has_vector_space_type_v<integral_m_affine_space>);
    STATIC_CHECK(!has_free_module_type_v<euclidean_affine_space<1>>);
    STATIC_CHECK( has_vector_space_type_v<euclidean_affine_space<1>>);
    STATIC_CHECK(!has_free_module_type_v<euclidean_vector_space<1>>);
    STATIC_CHECK(!has_vector_space_type_v<euclidean_vector_space<1>>);
    STATIC_CHECK(!has_free_module_type_v<integral_module>);

    STATIC_CHECK( module_type_is_extractable_v<integral_partial_m_torsor>);
    STATIC_CHECK(!module_type_is_extractable_v<euclidean_vector_space<1>>);

    STATIC_CHECK(std::is_same_v<nested_free_module_type_t<integral_partial_m_torsor>, integral_module>);
    STATIC_CHECK(std::is_same_v<nested_free_module_type_t<euclidean_affine_space<3>>, euclidean_vector_space<3>>);
    STATIC_CHECK(std::is_same_v<nested_free_module_type_t<complex_affine_space>,      complex_vector_space>);

    // Naive again, and again separated from the concept by defines_free_module_v.
    STATIC_CHECK(std::is_same_v<nested_free_module_type_t<spurious_module_torsor>, int>);
    STATIC_CHECK( has_free_module_type_v<spurious_module_torsor>);
    STATIC_CHECK(!defines_free_module_v<spurious_module_torsor>);
    STATIC_CHECK(!partial_m_torsor<spurious_module_torsor>);

    STATIC_CHECK( defines_free_module_v<integral_m_affine_space>);
    STATIC_CHECK(!defines_vector_space_v<integral_m_affine_space>);
    STATIC_CHECK( defines_free_module_v<euclidean_affine_space<1>>);
    STATIC_CHECK( defines_vector_space_v<euclidean_affine_space<1>>);
    STATIC_CHECK( defines_vector_space_v<complex_affine_space>);

    // A free module defines no module of its own: it *is* one, which is a distinction
    // free_module_type_of makes and defines_free_module_v does not.
    STATIC_CHECK(!defines_free_module_v<euclidean_vector_space<1>>);
    STATIC_CHECK(!defines_free_module_v<int>);

    STATIC_CHECK(std::is_same_v<free_module_type_of_t<euclidean_vector_space<3>>,      euclidean_vector_space<3>>);
    STATIC_CHECK(std::is_same_v<free_module_type_of_t<integral_module>,                integral_module>);
    STATIC_CHECK(std::is_same_v<free_module_type_of_t<euclidean_affine_space<3>>,      euclidean_vector_space<3>>);
    STATIC_CHECK(std::is_same_v<free_module_type_of_t<euclidean_nonnegative_space<2>>, euclidean_vector_space<2>>);
    STATIC_CHECK(std::is_same_v<free_module_type_of_t<integral_partial_m_torsor>,      integral_module>);

    STATIC_CHECK( free_module_is_extractable_v<integral_partial_m_torsor>);
    STATIC_CHECK(!free_module_is_extractable_v<spurious_module_torsor>);
    STATIC_CHECK(!free_module_is_extractable_v<int>);

    // Dimension is likewise read through the free module, so a space need not - and
    // mostly does not - name one of its own.
    STATIC_CHECK(dimension_of_v<euclidean_vector_space<3>>      == 3);
    STATIC_CHECK(dimension_of_v<euclidean_affine_space<3>>      == 3);
    STATIC_CHECK(dimension_of_v<euclidean_nonnegative_space<2>> == 2);
    STATIC_CHECK(dimension_of_v<integral_module>                == 1);
    STATIC_CHECK(dimension_of_v<integral_partial_m_torsor>      == 1);
    STATIC_CHECK(dimension_of_v<complex_affine_space>           == 1);
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

    // The declaration and its value, again separated: omitting the marker and setting
    // it false are different statements which the trait deliberately merges.
    STATIC_CHECK( has_admits_canonical_basis_v<euclidean_vector_space<3>>);
    STATIC_CHECK(!has_admits_canonical_basis_v<integral_module>);
    STATIC_CHECK( has_admits_canonical_basis_v<canonical_basis_declining_module>);
    STATIC_CHECK(!admits_canonical_basis_v<canonical_basis_declining_module>);
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

    /* Orthonormality is declared by the basis data, and gates the inner product,
       the dot product and the norm on euclidean_vector_space. TO DO: nothing in
       Source declares it, so those three hidden friends are unreachable today and
       the four traits built on them are false for every space. The fixture supplies
       the only positive case there is; the negatives below record the gap, and will
       fail the day it is closed.
     */
    STATIC_CHECK( is_orthonormal_basis_v<orthonormal_basis_data>);
    STATIC_CHECK(!is_orthonormal_basis_v<canonical_basis_data<3>>);
    STATIC_CHECK(!is_orthonormal_basis_v<alices_basis_data<3>>);

    STATIC_CHECK(!has_norm_v<euclidean_vector_space<1>>);
    STATIC_CHECK(!has_inner_product_v<euclidean_vector_space<1>>);
    STATIC_CHECK(!normed_vector_space<euclidean_vector_space<3>>);
    STATIC_CHECK(!inner_product_space<euclidean_vector_space<3>>);

    /* A basis's frame terminates the argument packs which physics uses to build
       coordinates from a list of values followed by a frame. The trait lives here
       because it is the basis which supplies the terminator; the packs themselves
       are assembled in PhysicalValues.hpp.
     */
    using basis_t = basis<euclidean_vector_space<2>>;
    STATIC_CHECK( is_units_terminated_pack_v<basis_t, double, double, double, identity_isomorphism>);
    STATIC_CHECK( is_units_terminated_pack_v<basis_t, double, int, double, identity_isomorphism>);
    STATIC_CHECK(!is_units_terminated_pack_v<basis_t, double, double, double>);
    STATIC_CHECK(!is_units_terminated_pack_v<basis_t, double, double, identity_isomorphism, double>);
    STATIC_CHECK(!is_units_terminated_pack_v<basis_t, double, double, double, reflection>);
    STATIC_CHECK(!is_units_terminated_pack_v<basis_t, double, identity_isomorphism>);
    STATIC_CHECK(!is_units_terminated_pack_v<basis_t, double, nominated_frame, identity_isomorphism>);

    // Comparability is exactly agreement on the convention. Two bases over Alice's
    // convention are related whatever their automorphisms; Bob's convention is not
    // related to Alice's, which is the static prohibition.
    STATIC_CHECK( consistent_basis_data_v<alices_basis_data<3>, alices_basis_data<3>>);
    STATIC_CHECK( consistent_basis_data_v<alices_basis_data<3>, alices_basis_data<3, reflection>>);
    STATIC_CHECK(!consistent_basis_data_v<alices_basis_data<3>, bobs_basis_data<3>>);
    STATIC_CHECK(!consistent_basis_data_v<alices_basis_data<3, reflection>, bobs_basis_data<3, reflection>>);
    STATIC_CHECK(!consistent_basis_data_v<alices_basis_data<3>, canonical_basis_data<3>>);
  }

  /*! A representation is a bijection between a space's coordinates and the values
      of some C++ type, and the concept naming one is a conjunction of four
      requirements over an interface with two shapes - single value and span. The
      fixtures below drop one requirement each, and supply one shape each, since a
      conjunction is only shown to be live by a case which fails exactly one term.
   */
  void spaces_meta_free_test::test_representation_traits()
  {
    using rep_t     = canonical_representation<double, no_bounds<double>>;
    using int_rep_t = canonical_representation<int, no_bounds<int>>;

    STATIC_CHECK( has_value_type_v<rep_t>);
    STATIC_CHECK( has_free_module_representation_v<rep_t>);
    STATIC_CHECK( has_bounds_v<rep_t>);
    STATIC_CHECK(!has_coordinates_type_v<rep_t>);
    STATIC_CHECK( representation<rep_t>);

    // One requirement missing apiece.
    STATIC_CHECK(!has_value_type_v<representation_without_value_type>);
    STATIC_CHECK(!representation<representation_without_value_type>);
    STATIC_CHECK(!has_free_module_representation_v<representation_without_free_module>);
    STATIC_CHECK(!representation<representation_without_free_module>);
    STATIC_CHECK(!has_bounds_v<representation_without_bounds_or_coordinates>);
    STATIC_CHECK(!has_coordinates_type_v<representation_without_bounds_or_coordinates>);
    STATIC_CHECK(!representation<representation_without_bounds_or_coordinates>);
    STATIC_CHECK(!std::default_initializable<uninitializable_representation>);
    STATIC_CHECK(!representation<uninitializable_representation>);

    // Bounds and coordinates are alternatives, so naming coordinates and no bounds
    // satisfies the concept just as well.
    STATIC_CHECK(!has_bounds_v<heterogeneous_representation>);
    STATIC_CHECK( has_coordinates_type_v<heterogeneous_representation>);
    STATIC_CHECK( representation<heterogeneous_representation>);

    // The two shapes of the interface. Each fixture supplies one and is therefore a
    // representation for a space of exactly one dimensionality; canonical_representation,
    // taking a span whatever the width, answers only to the second.
    STATIC_CHECK( representation_for_single_value<single_value_representation, euclidean_vector_space<1>>);
    STATIC_CHECK(!representation_for_span<single_value_representation, euclidean_vector_space<1>>);
    STATIC_CHECK( representation_for<single_value_representation, euclidean_vector_space<1>>);
    STATIC_CHECK(!representation_for<single_value_representation, euclidean_vector_space<2>>);

    STATIC_CHECK(!representation_for_single_value<span_representation, euclidean_vector_space<2>>);
    STATIC_CHECK( representation_for_span<span_representation, euclidean_vector_space<2>>);
    STATIC_CHECK( representation_for<span_representation, euclidean_vector_space<2>>);
    STATIC_CHECK(!representation_for<span_representation, euclidean_vector_space<1>>);

    STATIC_CHECK(!representation_for_single_value<rep_t, euclidean_vector_space<1>>);
    STATIC_CHECK( representation_for_span<rep_t, euclidean_vector_space<1>>);
    STATIC_CHECK( representation_for<rep_t, euclidean_vector_space<3>>);

    // Being a representation is not being a representation for a space: the space
    // must be a partial M-torsor, which the underlying set is not.
    STATIC_CHECK(!representation_for<rep_t, sets::R<1>>);
    STATIC_CHECK(!representation_for<rep_t, commutative_rings::reals<1>>);
    STATIC_CHECK(!representation_for<heterogeneous_representation, euclidean_vector_space<2>>);

    // The value type a set of bounds is stated in, and the value type a representation's
    // bounds must be stated in. They differ for the complexes, which are unordered: the
    // bounds are on the modulus, and so are real whatever the representation holds.
    STATIC_CHECK(std::is_same_v<bounds_value_type_t<coordinate_bounds<double>>, double>);
    STATIC_CHECK(std::is_same_v<bounds_value_type_t<coordinate_bounds<float>>,  float>);

    /* TO DO: bounds_value_type has a partial specialization for std::array<Bounds, D>,
       intended for the per-component bounds of a multi-dimensional space. Both the
       primary and the alias are constrained on `bounds`, which an array of bounds is
       not, so nothing can name it. Pinned as unreachable so that the day the
       constraint is relaxed, this line fails and says so.
     */
    STATIC_CHECK(!bounds<std::array<coordinate_bounds<double>, 3>>);
    STATIC_CHECK(!array_bounds_value_type_is_extractable_v<coordinate_bounds<double>, 3>);
    STATIC_CHECK(std::is_same_v<to_bounds_value_type_t<double>,                                double>);
    STATIC_CHECK(std::is_same_v<to_bounds_value_type_t<int>,                                   int>);
    STATIC_CHECK(std::is_same_v<to_bounds_value_type_t<std::complex<double>>,                  double>);
    STATIC_CHECK(std::is_same_v<to_bounds_value_type_t<std::complex<float>>,                   float>);

    STATIC_CHECK( is_canonical_representation_v<rep_t>);
    STATIC_CHECK(std::is_same_v<is_canonical_representation_t<rep_t>,                     std::true_type>);
    STATIC_CHECK(std::is_same_v<is_canonical_representation_t<polar_representation<double>>, std::false_type>);
    STATIC_CHECK( is_canonical_representation_v<int_rep_t>);
    STATIC_CHECK(!is_canonical_representation_v<polar_representation<double>>);
    STATIC_CHECK(!is_canonical_representation_v<single_value_representation>);

    // Heterogeneity is a property of the coordinates a representation names, so a
    // representation naming none is homogeneous by default rather than ill-formed.
    STATIC_CHECK( heterogeneous_coordinates_v<std::tuple<double, float>>);
    STATIC_CHECK(!heterogeneous_coordinates_v<std::tuple<double, double>>);
    STATIC_CHECK(!heterogeneous_coordinates_v<std::tuple<double>>);
    STATIC_CHECK( has_heterogeneous_representation_v<heterogeneous_representation>);
    STATIC_CHECK(!has_heterogeneous_representation_v<homogeneous_tuple_representation>);
    STATIC_CHECK(!has_heterogeneous_representation_v<rep_t>);

    // Consistency is agreement between the representation's coordinates and a
    // supplied pack - in order, which is what the reversal below establishes.
    STATIC_CHECK( consistent_representation_v<heterogeneous_representation, double, float>);
    STATIC_CHECK(!consistent_representation_v<heterogeneous_representation, float, double>);
    STATIC_CHECK(!consistent_representation_v<heterogeneous_representation, double>);
    STATIC_CHECK( consistent_representation_v<homogeneous_tuple_representation, double, double>);
    STATIC_CHECK(!consistent_representation_v<rep_t, double>);

    // A free module is its own free module, so its displacements keep the very
    // representation the points have; anywhere else the free-module representation is
    // taken, and must be wide enough for the differences.
    STATIC_CHECK(std::is_same_v<displacement_representation_t<euclidean_vector_space<1>, rep_t>, rep_t>);
    STATIC_CHECK(std::is_same_v<displacement_representation_t<integral_module, int_rep_t>,       int_rep_t>);

    // For a floating-point value type the widening is a no-op, so the two coincide
    // even where the space is not its own free module. It is the integral case which
    // separates them.
    STATIC_CHECK(std::is_same_v<displacement_representation_t<euclidean_affine_space<1>, rep_t>, rep_t>);
    STATIC_CHECK(std::is_same_v<value_type_of_t<displacement_representation_t<integral_m_affine_space, int_rep_t>>,
                                signed_covering_type_t<unsigned>>);

    // The optional operations. canonical_representation supplies none of them, which
    // is why the coordinates fall back on the value type's own arithmetic.
    STATIC_CHECK(!defines_scalar_multiplication_for_v<euclidean_vector_space<1>, rep_t>);
    STATIC_CHECK(!defines_scalar_division_for_v<euclidean_vector_space<1>, rep_t>);
    STATIC_CHECK(!defines_addition_for_v<euclidean_vector_space<1>, rep_t>);
    STATIC_CHECK(!defines_subtraction_for_v<euclidean_vector_space<1>, rep_t>);
    STATIC_CHECK(!defines_scalar_multiplication_for_single_value_v<euclidean_vector_space<1>, rep_t>);
    STATIC_CHECK(!defines_scalar_division_for_single_value_v<euclidean_vector_space<1>, rep_t>);
    STATIC_CHECK(!defines_addition_for_single_value_v<euclidean_vector_space<1>, rep_t>);
    STATIC_CHECK(!defines_subtraction_for_single_value_v<euclidean_vector_space<1>, rep_t>);

    // The polar pair is exactly this distinction in production code: scaling a polar
    // coordinate is not scaling its components, so polar_representation supplies mul
    // and div where its base does not. Neither supplies addition, which in polar
    // coordinates has no componentwise form at all.
    STATIC_CHECK(!defines_scalar_multiplication_for_v<euclidean_vector_space<2>, basic_polar_representation<double>>);
    STATIC_CHECK(!defines_scalar_division_for_v<euclidean_vector_space<2>, basic_polar_representation<double>>);
    STATIC_CHECK( defines_scalar_multiplication_for_v<euclidean_vector_space<2>, polar_representation<double>>);
    STATIC_CHECK( defines_scalar_division_for_v<euclidean_vector_space<2>, polar_representation<double>>);
    STATIC_CHECK(!defines_addition_for_v<euclidean_vector_space<2>, polar_representation<double>>);
    STATIC_CHECK(!defines_subtraction_for_v<euclidean_vector_space<2>, polar_representation<double>>);

    // And the single-value forms, which no production representation currently
    // supplies, so only a fixture can show they are reachable.
    STATIC_CHECK( defines_scalar_multiplication_for_single_value_v<euclidean_vector_space<1>, single_value_representation>);
    STATIC_CHECK( defines_scalar_division_for_single_value_v<euclidean_vector_space<1>, single_value_representation>);
    STATIC_CHECK( defines_addition_for_single_value_v<euclidean_vector_space<1>, single_value_representation>);
    STATIC_CHECK( defines_subtraction_for_single_value_v<euclidean_vector_space<1>, single_value_representation>);
    STATIC_CHECK(!defines_scalar_multiplication_for_v<euclidean_vector_space<1>, single_value_representation>);
    STATIC_CHECK(!defines_scalar_division_for_v<euclidean_vector_space<1>, single_value_representation>);
    STATIC_CHECK(!defines_addition_for_v<euclidean_vector_space<1>, single_value_representation>);
    STATIC_CHECK(!defines_subtraction_for_v<euclidean_vector_space<1>, single_value_representation>);
  }

  /*! A validator stands between a representation's values and the space's underlying
      set. Which of its two interfaces is required depends on the dimension, and only
      one is needed, so the negative cases here are as much about which door was open
      as about the validator itself.
   */
  void spaces_meta_free_test::test_validator_traits()
  {
    using rep_t         = canonical_representation<double, no_bounds<double>>;
    using complex_rep_t = canonical_representation<std::complex<double>, no_bounds<double>>;
    using space_t       = euclidean_vector_space<1>;

    STATIC_CHECK( validator_for_single_value<identity_validator, space_t, rep_t>);
    STATIC_CHECK( validator_for_array<identity_validator, space_t, rep_t>);
    STATIC_CHECK( validator_for<identity_validator, space_t, rep_t>);

    STATIC_CHECK( validator_for_single_value<throwing_validator, space_t, rep_t>);
    STATIC_CHECK( validator_for_array<throwing_validator, space_t, rep_t>);
    STATIC_CHECK( validator_for<throwing_validator, space_t, rep_t>);

    // Only the single-value door is open, and one open door is enough.
    STATIC_CHECK( validator_for_single_value<single_value_validator, space_t, rep_t>);
    STATIC_CHECK(!validator_for_array<single_value_validator, space_t, rep_t>);
    STATIC_CHECK( validator_for<single_value_validator, space_t, rep_t>);

    // At dimension two the single-value form is unavailable whatever the validator
    // offers, so only the array form can answer.
    STATIC_CHECK(!validator_for_single_value<identity_validator, euclidean_vector_space<2>, rep_t>);
    STATIC_CHECK( validator_for_array<identity_validator, euclidean_vector_space<2>, rep_t>);
    STATIC_CHECK(!validator_for<single_value_validator, euclidean_vector_space<2>, rep_t>);

    STATIC_CHECK(!validator_for_single_value<unusable_validator, space_t, rep_t>);
    STATIC_CHECK(!validator_for_array<unusable_validator, space_t, rep_t>);
    STATIC_CHECK(!validator_for<unusable_validator, space_t, rep_t>);

    // The representation must be one for the space before the validator is consulted
    // at all, so a mismatch there refuses every validator.
    STATIC_CHECK(!validator_for<identity_validator, euclidean_vector_space<2>, single_value_representation>);
    STATIC_CHECK(!validator_for<identity_validator, sets::R<1>, rep_t>);

    /* The two production validators part company over the complex numbers:
       throwing_validator is written against `arithmetic`, which std::complex is not,
       whereas identity_validator is written against `weak_commutative_ring`, which it
       is. A complex space can therefore be represented, but not range-checked - which
       is the honest answer, an ordering being what a range needs and the complexes
       having none.
     */
    STATIC_CHECK( representation_for<complex_rep_t, complex_vector_space>);
    STATIC_CHECK( validator_for<identity_validator, complex_vector_space, complex_rep_t>);
    STATIC_CHECK(!validator_for<throwing_validator, complex_vector_space, complex_rep_t>);

    // Transparency is declared, not inferred: single_value_validator checks nothing
    // either, and says nothing, so it is not privileged.
    STATIC_CHECK( defines_identity_validator_v<identity_validator>);
    STATIC_CHECK(!defines_identity_validator_v<throwing_validator>);
    STATIC_CHECK(!defines_identity_validator_v<single_value_validator>);
    STATIC_CHECK(std::is_same_v<defines_identity_validator_t<identity_validator>, std::true_type>);
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

    // Which of the two a space was built by, and whether it was built at all.
    STATIC_CHECK( is_dual_v<dual<unremarkable_space>>);
    STATIC_CHECK(!is_dual_v<unremarkable_space>);
    STATIC_CHECK(!is_dual_v<int>);
    STATIC_CHECK(std::is_same_v<is_dual_t<dual<unremarkable_space>>, std::true_type>);

    STATIC_CHECK( is_tensor_product_v<tensor_product<complex_pointed_torsor, complex_pointed_torsor>>);
    STATIC_CHECK(!is_tensor_product_v<dual<unremarkable_space>>);
    STATIC_CHECK(!is_tensor_product_v<int>);
    STATIC_CHECK(std::is_same_v<is_tensor_product_t<tensor_product<complex_vector_space, complex_vector_space>>,
                                std::true_type>);

    // Taking the dual twice returns the space, a finite-dimensional space being
    // naturally isomorphic to its double dual - so dual_of is not simply dual.
    STATIC_CHECK(std::is_same_v<dual_of_t<unremarkable_space>,       dual<unremarkable_space>>);
    STATIC_CHECK(std::is_same_v<dual_of_t<dual<unremarkable_space>>, unremarkable_space>);
    STATIC_CHECK(std::is_same_v<dual_of_t<complex_vector_space>,     dual<complex_vector_space>>);

    // A space reduces to its base space, and to itself where it names none. Duality
    // is carried through the reduction rather than discarded by it, which is what
    // keeps a dual comparable with the dual of its base and not with the base.
    STATIC_CHECK( has_base_space_v<refinement_of_unremarkable_space>);
    STATIC_CHECK(!has_base_space_v<unremarkable_space>);
    STATIC_CHECK(std::is_same_v<to_base_space_t<unremarkable_space>,                  unremarkable_space>);
    STATIC_CHECK(std::is_same_v<to_base_space_t<refinement_of_unremarkable_space>,     unremarkable_space>);
    STATIC_CHECK(std::is_same_v<to_base_space_t<dual<unremarkable_space>>,             dual<unremarkable_space>>);
    STATIC_CHECK(std::is_same_v<to_base_space_t<dual<refinement_of_unremarkable_space>>, dual<unremarkable_space>>);

    STATIC_CHECK( have_compatible_base_spaces_v<refinement_of_unremarkable_space, unremarkable_space>);
    STATIC_CHECK( have_compatible_base_spaces_v<unremarkable_space, refinement_of_unremarkable_space>);
    STATIC_CHECK(!have_compatible_base_spaces_v<refinement_of_unremarkable_space, half_line_space>);
    STATIC_CHECK( have_compatible_base_spaces_v<dual<refinement_of_unremarkable_space>, dual<unremarkable_space>>);
    STATIC_CHECK(!have_compatible_base_spaces_v<dual<refinement_of_unremarkable_space>, unremarkable_space>);

    // The arena is likewise inherited by the derived spaces, neither of which names
    // one, so that a client need declare it only on the spaces it writes down.
    STATIC_CHECK( has_arena_type_v<euclidean_vector_space<1>>);
    STATIC_CHECK(!has_arena_type_v<unremarkable_space>);
    STATIC_CHECK(!has_arena_type_v<dual<euclidean_vector_space<1>>>);
    STATIC_CHECK(std::is_same_v<arena_type_of_t<euclidean_vector_space<1>>,       mathematical_arena>);
    STATIC_CHECK(std::is_same_v<arena_type_of_t<dual<euclidean_vector_space<1>>>, mathematical_arena>);
    STATIC_CHECK(std::is_same_v<arena_type_of_t<tensor_product<euclidean_vector_space<1>, euclidean_vector_space<1>>>,
                                mathematical_arena>);

    /* A transformation between coordinates in two spaces is supplied by
       specializing coordinate_transformation, whose primary is empty. Geometry
       specializes it nowhere - the positive cases all live in PhysicalValues.hpp -
       so what is pinned here is that the primary answers false rather than failing.
     */
    STATIC_CHECK(!has_coordinate_transformation_v<euclidean_vector_space<1>, euclidean_vector_space<1>>);
    STATIC_CHECK(!has_coordinate_transformation_v<euclidean_vector_space<1>, euclidean_affine_space<1>>);
    STATIC_CHECK(!has_noexcept_coordinate_transformation_v<euclidean_vector_space<1>, euclidean_vector_space<1>>);

    // The displacements of a space over the integers are real: a difference of
    // integer points need not be an integer point of the space it came from.
    STATIC_CHECK(std::is_same_v<displacement_space_of_t<sets::Z<3>>, sets::R<3>>);
    STATIC_CHECK(std::is_same_v<displacement_space_of_t<commutative_rings::complexes>,
                                commutative_rings::complexes>);
  }
}
