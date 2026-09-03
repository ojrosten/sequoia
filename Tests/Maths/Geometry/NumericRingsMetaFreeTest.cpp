////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2025.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/** \file */

#include "NumericRingsMetaFreeTest.hpp"

#include "sequoia/Maths/Geometry/Spaces.hpp"

#include <complex>

namespace sequoia::testing
{
  using namespace maths;

  namespace
  {
    // Not integral, yet its differences overflow like any integer's: the sharpest
    // case for the trait declining to vouch for a mixture.
    enum class narrow_enum : unsigned char {};

    /** Whether `signed_covering_type` kept its promise for `T`. Where a covering exists
        it must be signed, must cover `T`, and must be no wider than it has to be: a
        signed type is covered by one of its own width - itself - and an unsigned one
        needs a strictly wider signed type, there being no other way to hold its top
        half. Where none exists, that must be because nothing standard is wide enough,
        which can happen only for an unsigned type as wide as `long long`.

        Written with `if constexpr` rather than `&&`: naming
        `signed_covering_type_t<T>` where there is none is ill-formed, and `&&`
        short-circuits at evaluation while still requiring every operand to be a valid
        expression.
     */
    template<class T>
    inline constexpr bool signed_covering_is_sound_v{
      []{
        if constexpr(has_signed_covering_type_v<T>)
        {
          using covering_t = signed_covering_type_t<T>;
          return    std::is_signed_v<covering_t>
                 && covered_by<T, covering_t>
                 && (std::is_signed_v<T> ? (sizeof(covering_t) == sizeof(T))
                                         : (sizeof(covering_t)  > sizeof(T)))
                 // Narrowest: no candidate which covers T is narrower than the one
                 // chosen. Stated by width, so that two candidates of equal width -
                 // long and long long, on the Unix 64-bit model - are both admissible,
                 // which they are, having the same range.
                 && (!covered_by<T, signed char> || (sizeof(covering_t) <= sizeof(signed char)))
                 && (!covered_by<T, short>       || (sizeof(covering_t) <= sizeof(short)))
                 && (!covered_by<T, int>         || (sizeof(covering_t) <= sizeof(int)))
                 && (!covered_by<T, long>        || (sizeof(covering_t) <= sizeof(long)))
                 && (!covered_by<T, long long>   || (sizeof(covering_t) <= sizeof(long long)));
        }
        else
        {
          return (!std::is_signed_v<T>) && (sizeof(T) == sizeof(long long));
        }
      }()
    };

    /** Whether the widening a free module's representation performs is sound for `T`.
        This is the contract displacement_representation asserts, stated once over the
        whole family rather than type by type: either a wider signed type exists, and
        the widened type holds the differences, or none does, and the representation
        keeps `T` - which then cannot hold them, and must not claim to.
     */
    template<class T>
    inline constexpr bool free_module_widening_is_sound_v{
      []{
        using widened_t = free_module_representation_value_type_t<T>;

        if constexpr(has_signed_covering_type_v<std::make_unsigned_t<T>>)
          return differences_covered_by_v<T, widened_t>;
        else
          return std::same_as<widened_t, T> && (!differences_covered_by_v<T, widened_t>);
      }()
    };

    /** The counterexample `multiplication_weakly_commutative` exists for: the two by
        two matrices over the integers, whose multiplication does not commute. Nothing
        about the type says so - it is regular, it adds, subtracts and multiplies, and
        so meets every requirement of `weak_commutative_ring` which can be established
        by inspection. The remaining one must therefore be opted out of by hand.

        The struct holds nothing and the operations are declared but never defined.
        Every question asked of these types is answered at compile time, so no value
        is ever formed, and an implementation would only restate the name.
     */
    struct matrix_2x2
    {
      [[nodiscard]]
      friend bool operator==(const matrix_2x2&, const matrix_2x2&) noexcept = default;

      // No [[nodiscard]] on the three below: the attribute is permitted on a friend
      // definition, as on the defaulted comparison above, but not on a friend
      // declaration.
      matrix_2x2& operator+=(const matrix_2x2&);
      matrix_2x2& operator-=(const matrix_2x2&);
      matrix_2x2& operator*=(const matrix_2x2&);

      friend matrix_2x2 operator+(const matrix_2x2&, const matrix_2x2&);
      friend matrix_2x2 operator-(const matrix_2x2&, const matrix_2x2&);
      friend matrix_2x2 operator*(const matrix_2x2&, const matrix_2x2&);
    };

    /** The control, and of the same kind: the diagonal two by two matrices form a
        subring whose multiplication does commute, and which therefore has nothing to
        opt out of. Without it, `matrix_2x2` failing `weak_commutative_ring` would be
        equally consistent with the shape having been insufficient all along.
     */
    struct diagonal_matrix_2x2
    {
      [[nodiscard]]
      friend bool operator==(const diagonal_matrix_2x2&, const diagonal_matrix_2x2&) noexcept = default;

      diagonal_matrix_2x2& operator+=(const diagonal_matrix_2x2&);
      diagonal_matrix_2x2& operator-=(const diagonal_matrix_2x2&);
      diagonal_matrix_2x2& operator*=(const diagonal_matrix_2x2&);

      friend diagonal_matrix_2x2 operator+(const diagonal_matrix_2x2&, const diagonal_matrix_2x2&);
      friend diagonal_matrix_2x2 operator-(const diagonal_matrix_2x2&, const diagonal_matrix_2x2&);
      friend diagonal_matrix_2x2 operator*(const diagonal_matrix_2x2&, const diagonal_matrix_2x2&);
    };
  }
}

namespace sequoia::maths
{
  /** That addition forms a group is a statement about behaviour which no inspection
      could make, so both rings must say it for themselves. Without this neither would
      be a `weak_commutative_ring`, and the pair below would separate nothing.
   */
  template<> struct weakly_abelian_group_under_addition<testing::matrix_2x2>          : std::true_type {};
  template<> struct weakly_abelian_group_under_addition<testing::diagonal_matrix_2x2> : std::true_type {};

  /** The single respect in which the two differ. */
  template<> struct multiplication_weakly_commutative<testing::matrix_2x2> : std::false_type {};
}

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path numeric_rings_meta_free_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void numeric_rings_meta_free_test::run_tests()
  {
    test_arithmetic_traits();
    test_commutative_rings();
    test_coverings();
    test_integral_covering_invariants();
  }

  void numeric_rings_meta_free_test::test_arithmetic_traits()
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
    // operators decline to supply it: `true + true` is not even a bool, and forced back
    // into one it is `true`.
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

    // True by default, so it is answered even for types with no multiplication to
    // distribute. An author who has both operations and no distributivity must opt
    // out; nothing here can detect the omission.
    STATIC_CHECK( multiplication_weakly_distributive_over_addition_v<narrow_enum>);
    STATIC_CHECK( multiplication_weakly_distributive_over_addition_v<commutative_rings::reals<1>>);

    // Commutativity of multiplication is the second trait of this shape, and is
    // answered the same way for the same reason.
    STATIC_CHECK( multiplication_weakly_commutative_v<int>);
    STATIC_CHECK( multiplication_weakly_commutative_v<double>);
    STATIC_CHECK( multiplication_weakly_commutative_v<std::complex<double>>);
    STATIC_CHECK( multiplication_weakly_commutative_v<narrow_enum>);
    STATIC_CHECK( multiplication_weakly_commutative_v<commutative_rings::reals<1>>);
    STATIC_CHECK(std::is_same_v<multiplication_weakly_commutative_t<int>,        std::true_type>);
    STATIC_CHECK(std::is_same_v<multiplication_weakly_commutative_t<matrix_2x2>, std::false_type>);

    /* Every other requirement of weak_commutative_ring holds for matrix_2x2, and is
       checked here one at a time, so that the refusal which follows can be laid at
       the door of the commutativity opt-out and nothing else.
     */
    STATIC_CHECK( std::regular<matrix_2x2>);
    STATIC_CHECK( weakly_abelian_group_under_addition_v<matrix_2x2>);
    STATIC_CHECK( multiplication_weakly_distributive_over_addition_v<matrix_2x2>);
    STATIC_CHECK( is_addable_v<matrix_2x2>);
    STATIC_CHECK( is_subtractable_v<matrix_2x2>);
    STATIC_CHECK( is_multiplicable_v<matrix_2x2>);
    STATIC_CHECK(!multiplication_weakly_commutative_v<matrix_2x2>);
    STATIC_CHECK(!weak_commutative_ring<matrix_2x2>);
    STATIC_CHECK(!weak_field<matrix_2x2>);
    STATIC_CHECK(!numeric_ring<matrix_2x2>);

    // The control: identical in shape and in every trait but the one, and admitted.
    STATIC_CHECK( multiplication_weakly_commutative_v<diagonal_matrix_2x2>);
    STATIC_CHECK( weak_commutative_ring<diagonal_matrix_2x2>);

    // numeric_ring's second clause excludes the integral types which are not integers;
    // a user-defined ring passes it by not being integral at all, which is what lets a
    // space be built over one.
    STATIC_CHECK( numeric_ring<diagonal_matrix_2x2>);

    // Not a field either, and for a reason with nothing to do with commutativity:
    // the integers do not divide.
    STATIC_CHECK(!is_divisible_v<diagonal_matrix_2x2>);
    STATIC_CHECK(!weak_field<diagonal_matrix_2x2>);

    // As for bool, an explicit specialization matches only the unqualified type, so
    // const matrix_2x2 reaches the primary and answers true. Harmless, since regularity
    // fails first: a const type is not assignable, so the concept refuses it anyway.
    STATIC_CHECK( multiplication_weakly_commutative_v<const matrix_2x2>);
    STATIC_CHECK(!weak_commutative_ring<const matrix_2x2>);

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

  /** The commutative-ring diamond of the introduction. Being ordered and being a
      field are independent: the integers have the first, the complexes the
      second, and only the reals have both.
   */
  void numeric_rings_meta_free_test::test_commutative_rings()
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

  void numeric_rings_meta_free_test::test_coverings()
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

    /* The facility is for the built-in integer types and is constrained to them,
       because its chain of specializations assumes covering is monotone - whatever
       signed char covers, short covers. A ring converting to signed char and to int
       but not to short would break that and match two specializations at once, so
       the constraint is what keeps an ambiguity from becoming a hard error. These
       therefore assert not merely the answer but that there IS an answer: each of
       these lines compiles, which is the whole point of the constraint.
     */
    STATIC_CHECK(!has_signed_covering_type_v<float>);
    STATIC_CHECK(!has_signed_covering_type_v<double>);
    STATIC_CHECK(!has_signed_covering_type_v<std::complex<double>>);
    STATIC_CHECK(!has_signed_covering_type_v<diagonal_matrix_2x2>);
    STATIC_CHECK(!has_signed_covering_type_v<bool>);
    STATIC_CHECK(!has_signed_covering_type_v<wchar_t>);
    STATIC_CHECK(!has_signed_covering_type_v<char32_t>);

    // Positive control for the block above: the constraint excludes what it should
    // and nothing more, so an integer type in the same position still answers.
    STATIC_CHECK( has_signed_covering_type_v<short>);

    // cv-qualification is not seen through here, unlike by std::integral: a const
    // type is not regular, so it is no numeric_ring and nothing covers it.
    STATIC_CHECK(!has_signed_covering_type_v<const unsigned>);

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

  void numeric_rings_meta_free_test::test_integral_covering_invariants()
  {
    // Reflexive on every integral type, as on every numeric ring: a type represents
    // its own values.
    STATIC_CHECK(covered_by<signed char,        signed char>);
    STATIC_CHECK(covered_by<unsigned char,      unsigned char>);
    STATIC_CHECK(covered_by<short,              short>);
    STATIC_CHECK(covered_by<unsigned short,     unsigned short>);
    STATIC_CHECK(covered_by<int,                int>);
    STATIC_CHECK(covered_by<unsigned,           unsigned>);
    STATIC_CHECK(covered_by<long,               long>);
    STATIC_CHECK(covered_by<unsigned long,      unsigned long>);
    STATIC_CHECK(covered_by<long long,          long long>);
    STATIC_CHECK(covered_by<unsigned long long, unsigned long long>);

    // The standard nests the signed types by rank, so the ascent holds everywhere,
    // adjacent steps and distant ones alike.
    STATIC_CHECK(covered_by<signed char, short>);
    STATIC_CHECK(covered_by<short,       int>);
    STATIC_CHECK(covered_by<int,         long>);
    STATIC_CHECK(covered_by<long,        long long>);
    STATIC_CHECK(covered_by<signed char, long long>);
    STATIC_CHECK(covered_by<short,       long long>);

    // And nests the unsigned types likewise.
    STATIC_CHECK(covered_by<unsigned char,  unsigned short>);
    STATIC_CHECK(covered_by<unsigned short, unsigned>);
    STATIC_CHECK(covered_by<unsigned,       unsigned long>);
    STATIC_CHECK(covered_by<unsigned long,  unsigned long long>);
    STATIC_CHECK(covered_by<unsigned char,  unsigned long long>);

    // No signed type is covered by any unsigned type, however wide: the negative half
    // has nowhere to go. This is the one direction which needs no width argument at
    // all, and so the one which is portable without qualification.
    STATIC_CHECK(!covered_by<signed char, unsigned char>);
    STATIC_CHECK(!covered_by<signed char, unsigned long long>);
    STATIC_CHECK(!covered_by<short,       unsigned short>);
    STATIC_CHECK(!covered_by<int,         unsigned>);
    STATIC_CHECK(!covered_by<long,        unsigned long>);
    STATIC_CHECK(!covered_by<long long,   unsigned long long>);

    // An unsigned type needs a strictly wider signed one. At the narrow end the widths
    // are pinned by the standard's minima, so these hold everywhere...
    STATIC_CHECK( covered_by<unsigned char,  short>);
    STATIC_CHECK( covered_by<unsigned char,  int>);
    STATIC_CHECK( covered_by<unsigned short, int>);
    STATIC_CHECK(!covered_by<unsigned long long, long long>);

    /* ...and at the wide end they do not, which is the whole of the data model's
       effect on this machinery. On the 64-bit Windows model `long` is 32 bits and
       these are true; on the Unix one it is 64 and they are false. Stating them
       against `sizeof` says which, and would catch a covering that had stopped
       consulting the compiler's narrowing rules on either platform - where naming one
       answer would test only the platform it was written on.
     */
    STATIC_CHECK(covered_by<unsigned,      long>      == (sizeof(long)      > sizeof(unsigned)));
    STATIC_CHECK(covered_by<unsigned long, long long> == (sizeof(long long) > sizeof(unsigned long)));
    STATIC_CHECK(covered_by<long,          int>       == (sizeof(long)      == sizeof(int)));
    STATIC_CHECK(covered_by<long long,     long>      == (sizeof(long long) == sizeof(long)));

    // Descending is refused wherever the widths genuinely differ, which at the narrow
    // end they always do.
    STATIC_CHECK(!covered_by<short,          signed char>);
    STATIC_CHECK(!covered_by<int,            short>);
    STATIC_CHECK(!covered_by<long long,      signed char>);
    STATIC_CHECK(!covered_by<unsigned short, unsigned char>);
    STATIC_CHECK(!covered_by<unsigned,       unsigned short>);

    // At the wide end it is refused only where the widths differ, exactly as for the
    // signed types, and for the same reason.
    STATIC_CHECK(covered_by<unsigned long, unsigned>
                   == (sizeof(unsigned long) == sizeof(unsigned)));
    STATIC_CHECK(covered_by<unsigned long long, unsigned long>
                   == (sizeof(unsigned long long) == sizeof(unsigned long)));

    /* The fixed-width aliases. Their value here is not that they are more types: it is
       that their widths are *exact*, where `int` and `long` are only bounded below. So
       they state strictly-wider and strictly-narrower portably, which the standard
       types cannot.

       That `std::int8_t` is `signed char` is the reason the `integer` concept admits
       the two narrow char types while excluding `char` itself: the standard gives
       8-bit arithmetic no other spelling, so excluding them would exclude int8_t.
     */
    STATIC_CHECK(std::is_same_v<std::int8_t,  signed char>);
    STATIC_CHECK(std::is_same_v<std::uint8_t, unsigned char>);
    STATIC_CHECK( numeric_ring<std::int8_t>);
    STATIC_CHECK( numeric_ring<std::uint8_t>);
    STATIC_CHECK(!numeric_ring<char>);

    STATIC_CHECK( covered_by<std::int8_t,   std::int16_t>);
    STATIC_CHECK( covered_by<std::int16_t,  std::int32_t>);
    STATIC_CHECK( covered_by<std::int32_t,  std::int64_t>);
    STATIC_CHECK(!covered_by<std::int16_t,  std::int8_t>);
    STATIC_CHECK(!covered_by<std::int32_t,  std::int16_t>);
    STATIC_CHECK(!covered_by<std::int64_t,  std::int32_t>);
    STATIC_CHECK( covered_by<std::uint8_t,  std::int16_t>);
    STATIC_CHECK( covered_by<std::uint16_t, std::int32_t>);
    STATIC_CHECK( covered_by<std::uint32_t, std::int64_t>);
    STATIC_CHECK(!covered_by<std::uint16_t, std::int16_t>);
    STATIC_CHECK(!covered_by<std::uint32_t, std::int32_t>);
    STATIC_CHECK(!covered_by<std::uint64_t, std::int64_t>);

    // std::size_t has a signed covering precisely when it is not already as wide as
    // anything standard, which on the 64-bit platforms it is.
    STATIC_CHECK( numeric_ring<std::size_t>);
    STATIC_CHECK( numeric_ring<std::ptrdiff_t>);
    STATIC_CHECK( covered_by<std::ptrdiff_t, std::ptrdiff_t>);
    STATIC_CHECK(!covered_by<std::size_t,    std::ptrdiff_t>);
    STATIC_CHECK(has_signed_covering_type_v<std::size_t> == (sizeof(std::size_t) < sizeof(long long)));

    // Every integral type is a weak commutative ring and none is a weak field: integer
    // division exists, but it is not the inverse of multiplication.
    STATIC_CHECK( weak_commutative_ring<signed char>);
    STATIC_CHECK( weak_commutative_ring<unsigned char>);
    STATIC_CHECK( weak_commutative_ring<short>);
    STATIC_CHECK( weak_commutative_ring<unsigned short>);
    STATIC_CHECK( weak_commutative_ring<unsigned long>);
    STATIC_CHECK( weak_commutative_ring<long long>);
    STATIC_CHECK( weak_commutative_ring<unsigned long long>);
    STATIC_CHECK(!weak_field<signed char>);
    STATIC_CHECK(!weak_field<unsigned char>);
    STATIC_CHECK(!weak_field<long long>);
    STATIC_CHECK(!weak_field<unsigned long long>);

    STATIC_CHECK( numeric_ring<short>);
    STATIC_CHECK( numeric_ring<unsigned short>);
    STATIC_CHECK( numeric_ring<long>);
    STATIC_CHECK( numeric_ring<unsigned long>);
    STATIC_CHECK( numeric_ring<long long>);
    STATIC_CHECK( numeric_ring<unsigned long long>);

    /* First invariant: no integer type holds its own differences. For a signed type
       the spread is twice the range; for an unsigned one the differences are signed
       and the type is not. So the two relations disagree on every integral type,
       reflexively - which is the sharpest statement of what separates them, and the
       reason a free module over the integers cannot represent its own displacements
       without widening.
     */
    STATIC_CHECK(!differences_covered_by_v<signed char,        signed char>);
    STATIC_CHECK(!differences_covered_by_v<unsigned char,      unsigned char>);
    STATIC_CHECK(!differences_covered_by_v<short,              short>);
    STATIC_CHECK(!differences_covered_by_v<unsigned short,     unsigned short>);
    STATIC_CHECK(!differences_covered_by_v<int,                int>);
    STATIC_CHECK(!differences_covered_by_v<unsigned,           unsigned>);
    STATIC_CHECK(!differences_covered_by_v<long,               long>);
    STATIC_CHECK(!differences_covered_by_v<unsigned long,      unsigned long>);
    STATIC_CHECK(!differences_covered_by_v<long long,          long long>);
    STATIC_CHECK(!differences_covered_by_v<unsigned long long, unsigned long long>);

    // The floating-point types are the contrast, and the reason the trait cannot
    // simply be defined as irreflexive.
    STATIC_CHECK(differences_covered_by_v<float,  float>);
    STATIC_CHECK(differences_covered_by_v<double, double>);

    // Second invariant: signed_covering_type is signed, covering, and no wider than it
    // must be - or absent, and then only because nothing standard is wide enough.
    STATIC_CHECK(signed_covering_is_sound_v<signed char>);
    STATIC_CHECK(signed_covering_is_sound_v<unsigned char>);
    STATIC_CHECK(signed_covering_is_sound_v<short>);
    STATIC_CHECK(signed_covering_is_sound_v<unsigned short>);
    STATIC_CHECK(signed_covering_is_sound_v<int>);
    STATIC_CHECK(signed_covering_is_sound_v<unsigned>);
    STATIC_CHECK(signed_covering_is_sound_v<long>);
    STATIC_CHECK(signed_covering_is_sound_v<unsigned long>);
    STATIC_CHECK(signed_covering_is_sound_v<long long>);
    STATIC_CHECK(signed_covering_is_sound_v<unsigned long long>);
    STATIC_CHECK(signed_covering_is_sound_v<std::size_t>);
    STATIC_CHECK(signed_covering_is_sound_v<std::ptrdiff_t>);

    // The negative control, without which the predicate above would be indistinguishable
    // from one that is true of everything. char has no covering, and not for the reason
    // the absent branch admits: it is one byte wide, and excluded for what its values
    // denote rather than for want of anything to hold them.
    STATIC_CHECK(!signed_covering_is_sound_v<char>);

    /* Third invariant, and the one with a client: the widening a free module's
       representation performs either yields a type wide enough for the differences, or
       declines and hands back the type it was given. displacement_representation
       asserts exactly this, one type at a time, deep inside the coordinates it is
       building; asserted here it is a property of the machinery.
     */
    STATIC_CHECK(free_module_widening_is_sound_v<signed char>);
    STATIC_CHECK(free_module_widening_is_sound_v<unsigned char>);
    STATIC_CHECK(free_module_widening_is_sound_v<short>);
    STATIC_CHECK(free_module_widening_is_sound_v<unsigned short>);
    STATIC_CHECK(free_module_widening_is_sound_v<int>);
    STATIC_CHECK(free_module_widening_is_sound_v<unsigned>);
    STATIC_CHECK(free_module_widening_is_sound_v<long>);
    STATIC_CHECK(free_module_widening_is_sound_v<unsigned long>);
    STATIC_CHECK(free_module_widening_is_sound_v<long long>);
    STATIC_CHECK(free_module_widening_is_sound_v<unsigned long long>);
    STATIC_CHECK(free_module_widening_is_sound_v<std::int8_t>);
    STATIC_CHECK(free_module_widening_is_sound_v<std::size_t>);

    // The declining case named concretely, since it is the one a client meets as a
    // static_assert. At the widest end there is nothing to widen to, and an unsigned
    // type is handed back unsigned - which no space but a free module may use.
    STATIC_CHECK(std::same_as<free_module_representation_value_type_t<long long>, long long>);
    STATIC_CHECK(std::same_as<free_module_representation_value_type_t<unsigned long long>,
                              unsigned long long>);
    STATIC_CHECK(!has_signed_covering_type_v<unsigned long long>);
  }
}
