////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** @file
    @brief Utilities relating to vector sets
 */


#include "sequoia/Core/ContainerUtilities/ArrayUtilities.hpp"
#include "sequoia/Core/Meta/Concepts.hpp"
#include "sequoia/Core/Meta/TypeAlgorithms.hpp"
#include "sequoia/PlatformSpecific/Preprocessor.hpp"

#include <algorithm>
#include <cmath>
#include <concepts>
#include <complex>
#include <format>
#include <ranges>
#include <span>

namespace sequoia::maths
{
  template<class T>
  inline constexpr bool is_addable_v{
    requires(T& t) {
      { t += t } -> std::same_as<T&>;
      { t + t }  -> std::same_as<T>;
    }
  };

  template<class T>
  inline constexpr bool is_subtractable_v{
    requires(T& t) {
      { t -= t } -> std::same_as<T&>;
      { t - t }  -> std::same_as<T>;
    }
  };

  template<class T>
  inline constexpr bool is_multiplicable_v{
    requires(T& t) {
      { t *= t } -> std::same_as<T&>;
      { t * t }  -> std::same_as<T>;
    }
  };

  template<class T>
  inline constexpr bool is_divisible_v{
    requires(T& t) {
      { t /= t } -> std::same_as<T&>;
      { t / t }  -> std::same_as<T>;
    }
  };

  /** @brief Trait for specifying whether a type behaves (appoximately) as an abelian group under addition.

      A fundamental problem of attempting this classification on a computer is the difference
      between a mathematical structure and an approximate representation of that structure.
      ints model the integers, but not exactly since there is a maximum representable value.
      Similarly, floating-point numbers model the reals but only in an approximate sense.
      To signify the fact that  neither integer nor floating-point addition exactly models an
      abelian group, the term 'weak' is used. Not, however, that addition of unsigned integral
      types does precisely model an abelian group and so 'weak' is a minimum requirement.
   */
  template<class T>
  struct weakly_abelian_group_under_addition : std::false_type {};

  template<class T>
  using weakly_abelian_group_under_addition_t = typename weakly_abelian_group_under_addition<T>::type;
  
  template<class T>
  inline constexpr bool weakly_abelian_group_under_addition_v{weakly_abelian_group_under_addition<T>::value};

  template<arithmetic T>
  struct weakly_abelian_group_under_addition<T> : std::true_type {};

  template<std::floating_point T>
  struct weakly_abelian_group_under_addition<std::complex<T>> : std::true_type {};

  /** @brief Trait for specifying whether a type behaves (appoximately) as an abelian group under multiplication.

      The only integral type modelling this (exactly, as it transpires) is bool. It is the only type in C++
      modelling Z/Zn where n is a prime.
   */

  template<class T>
  struct weakly_abelian_group_under_multiplication : std::false_type {};

  template<class T>
  using weakly_abelian_group_under_multiplication_t = typename weakly_abelian_group_under_multiplication<T>::type;

  template<class T>
  inline constexpr bool weakly_abelian_group_under_multiplication_v{weakly_abelian_group_under_multiplication<T>::value};

  template<std::floating_point T>
  struct weakly_abelian_group_under_multiplication<T> : std::true_type {};

  template<std::floating_point T>
  struct weakly_abelian_group_under_multiplication<std::complex<T>> : std::true_type {};

  template<>
  struct weakly_abelian_group_under_multiplication<bool> : std::true_type {};
  
  /** @brief Trait for specifying whether a type exhibits multiplication that (approximately) distributes over addition . */

  template<class T>
  struct multiplication_weakly_distributive_over_addition : std::true_type {};

  template<class T>
  using multiplication_weakly_distributive_over_addition_t = typename multiplication_weakly_distributive_over_addition<T>::type;

  template<class T>
  inline constexpr bool multiplication_weakly_distributive_over_addition_v{multiplication_weakly_distributive_over_addition<T>::value};

  /* @brief concept representing reasonable approximations to a commutative ring */
  
  template<class T>
  concept weak_commutative_ring 
    =    std::regular<T>
      && weakly_abelian_group_under_addition_v<T>
      && multiplication_weakly_distributive_over_addition_v<T>
      && is_addable_v<T>
      && is_subtractable_v<T>
      && is_multiplicable_v<T>;

  /* @brief concept representing reasonable approximations to a field */

  template<class T>
  concept weak_field = weak_commutative_ring<T> && weakly_abelian_group_under_multiplication_v<T> && is_divisible_v<T>;

  /* @brief Reports whether a type, exposes a nested type named commutative_ring_type which satisifes the weak_commutative_ring concept */ 
  template<class T>
  inline constexpr bool has_commutative_ring_type_v{
    requires { 
      typename T::commutative_ring_type;
      requires weak_commutative_ring<typename T::commutative_ring_type>;
    }
  };

  /* @brief Reports whether a type, exposes a nested type named field_type which satisifes the weak_field concept */ 
  template<class T>
  inline constexpr bool has_field_type_v{
    requires { 
      typename T::field_type;
      requires weak_field<typename T::field_type>;
    }
  };

  /* @brief Reports whether a type exposes a nested type with the properties of a commutative ring

     The point here is that a field is a special case of a ring. Therefore, anything which defines
     a field is implicitly defining a ring.
   */
  template<class T>
  inline constexpr bool defines_commutative_ring_v{has_commutative_ring_type_v<T> || has_field_type_v<T>};

  /* @brief Reports whether a type exposes a nested type with the properties of a field

     The point here is to capture the case where a type exposes a nested type commutative_ring_type
     but the latter satisfies not just the weak_commutative_ring concept but also the strong
     weak_field concept.
   */
  template<class T>
  inline constexpr bool defines_field_v{
        has_field_type_v<T>
    || requires { 
          typename T::commutative_ring_type;
          requires weak_field<typename T::commutative_ring_type>;
        }
  };

  /** @brief Reports whether a type exposes a nested value, dimension, convertible to a std::size_t */
  template<class T>
  inline constexpr bool has_dimension_v{
    requires { { T::dimension } -> std::convertible_to<std::size_t>; }
  };

  /** @brief Reports whether a type exposes a nested type named set_type */
  template<class T>
  inline constexpr bool has_set_type_v{
    requires { typename T::set_type; }
  };

  /** @defgroup IdentifiesAsSpace Self-identification of Spaces
      @brief Compile time constants to capture whether types self-identify as various spaces

      It is straightforward to say that a type, T, exposes nested types, say set_type
      and field_type. This is suggestive of a vector space but to do able to unequivocally
      identify T as a vector space requires additional information, since we have no way
      of knowing, a priori, whether the vector space axioms are satisfied. Indeed, it may
      not be straightforward to provide the operations for addition and multiplication as
      they apply to the elements of the underlying set: practically speaking, we are usually
      dealing with operations on coordinates with respect to some basis.

      Therefore, the approach taken is to demand that various spaces self identify as such.
      For a free module this would mean exposing a type, is_vector_space, convertible to
      std::true_type. Note, though, that is not sufficient for T to be a free module, as
      there are additional requirements that must be met (see the free_module concept).
      
   */

  /** @ingroup IdentifiesAsSpace
      @brief Compile time constant capturing whether a space self-identifies as convex.
   */
  template<class T>
  inline constexpr bool identifies_as_convex_space_v{
     requires {
      typename T::is_convex_space;
      requires std::convertible_to<typename T::is_convex_space, std::true_type>;
    }
  };

  /** @ingroup IdentifiesAsSpace
      @brief Compile time constant capturing whether a space self-identifies as affine.
   */
  template<class T>
  inline constexpr bool identifies_as_affine_space_v{
     requires {
      typename T::is_affine_space;
      requires std::convertible_to<typename T::is_affine_space, std::true_type>;
    }
  };

  /** @ingroup IdentifiesAsSpace
      @brief Compile time constant capturing whether a space self-identifies as a free module.
   */
  template<class T>
  inline constexpr bool identifies_as_free_module_v{
    requires {
      typename T::is_free_module;
      requires std::convertible_to<typename T::is_free_module, std::true_type>;
    }
  };

  /** @ingroup IdentifiesAsSpace
      @brief Compile time constant capturing whether a space self-identifies as vector space.
   */
  template<class T>
  inline constexpr bool identifies_as_vector_space_v{
    requires {
      typename T::is_vector_space;
      requires std::convertible_to<typename T::is_vector_space, std::true_type>;
    }
  };
  
  /** @brief concept for a free module */
  template<class T>
  concept free_module = has_set_type_v<T> && has_dimension_v<T> && defines_commutative_ring_v<T> && (identifies_as_free_module_v<T> || identifies_as_vector_space_v<T>);

  /** @brief concept for a vector space, which is a special case of a free module */
  template<class T>
  concept vector_space = free_module<T> && defines_field_v<T>;

  /** @defgroup ConvexSpace Convex Spaces
      @brief Concepts and helpers pertaininng to convex spaces

      An affine space comprises a set, A, and a vector space, V,
      such that the additive group of V has a free and transitive
      action on A. Informally, we may add any element of V to
      any element of A, to translate to a different element of A.

      There are several important ways to relax the definition of
      an affine space. First, we can replace the vector space with
      a free module, giving an affine space over a free module.
      For what follows, this is the definition we shall take for
      an affine space; whether it is over a vector space or a more
      general free module can be determined by reflecting on whether
      is exposes a vector space type or free module type.

      Given this working definition of an affine space, another
      condition that we may relax is the free and transitive action
      of the additive group of V on A. The motivation for this is
      spaces which are bounded: there exist combinations of elements
      of V and of A such tha a + v is no longer within the set. From
      the point of view of a C++ implementation, it is not that this
      addition isn't defined, but rather that the result is an exception
      type. In other words, we are taking A to be the union of a
      set of points and an exception type.

      Rather than considering arbitrary relaxations of this form,
      we consider only the case where (excluding any exception type)
      the points of A form a convex space. Therefore for what follows
      we consider a convex space to comprise:
      1. A Set, C', considered the union of a set of points, C, and an exception type
      2. A free module, M
      3. The action of the additive group of M on C' yields another element of C'.
         This is not a bijection, since there are many ways to map onto the single
         exception state.      
   */

  /** @ingroup ConvexSpace
      @brief Compile time constant capturing whether a type exposes a nested
             vector_space_type which satisfies the vector_space concept.
   */
  template<class T>
  inline constexpr bool has_vector_space_type_v{
    requires {
      typename T::vector_space_type;
      requires vector_space<typename T::vector_space_type>;
    }
  };

  /** @ingroup ConvexSpace
      @brief Compile time constant capturing whether a type exposes a nested
             free_modul_type which satisfies the vector_space concept.
   */
  template<class T>
  inline constexpr bool has_free_module_type_v{
    requires {
      typename T::free_module_type;
      requires free_module<typename T::free_module_type>;
    }
  };

  /** @ingroup ConvexSpace
      @brief concept for convex spaces

      A convex space comprises a set and a free module (which may be a vector space),
      and must identify as either a convex or affine space.      
   */
  template<class T>
  concept convex_space
    =    vector_space<T> || free_module<T>
      || (    has_set_type_v<T>
          && (has_vector_space_type_v<T> || has_free_module_type_v<T>)
          && (identifies_as_convex_space_v<T> || identifies_as_affine_space_v<T>));

  /** @ingroup ConvexSpace
      @brief concept for affine spaces

      A vector space is an affine space over itself; beyond that according to our
      definitions an affine space is a refinement of a convex space.
   */
  template<class T>
  concept affine_space = vector_space<T> || (convex_space<T> && identifies_as_affine_space_v<T>);

  /** @defgroup ConvexSpaceUtilities Convex Space Utilities
      @brief Utilites for extracting properties of convex spaces
   */

  /** @ingroup ConvexSpaceUtilities
      @brief Helper that universal template parameters will obviate the need for
    */
  template<class T>
  struct is_free_module : std::integral_constant<bool, free_module<T>> {};
  
  /** @ingroup ConvexSpaceUtilities
      @brief Helper to extract the free module type associated with a convex space.

      This takes into account that a vector space is a special case of a free module.
   */
  template<convex_space ConvexSpace>
  struct free_module_type_of;

  template<convex_space ConvexSpace>
    requires identifies_as_free_module_v<ConvexSpace> || identifies_as_vector_space_v<ConvexSpace>
  struct free_module_type_of<ConvexSpace>
  {
    using type = ConvexSpace;
  };

  template<convex_space ConvexSpace>
    requires (!identifies_as_free_module_v<ConvexSpace> && !identifies_as_vector_space_v<ConvexSpace>) && has_free_module_type_v<ConvexSpace>
  struct free_module_type_of<ConvexSpace>
  {
    using type = ConvexSpace::free_module_type;
  };

  template<convex_space ConvexSpace>
    requires (!identifies_as_free_module_v<ConvexSpace> && !identifies_as_vector_space_v<ConvexSpace>) && has_vector_space_type_v<ConvexSpace>
  struct free_module_type_of<ConvexSpace>
  {
    using type = ConvexSpace::vector_space_type;
  };

  template<convex_space ConvexSpace>
  using free_module_type_of_t = free_module_type_of<ConvexSpace>::type;
  
  /** @ingroup ConvexSpaceUtilities
      @brief Helper to extract the commutative ring type of the free module associated with a convex space.

      This takes into accoutn that if the free module is a vector space, then the commutative ring is actually a field. 
   */
  template<convex_space ConvexSpace>
  struct commutative_ring_type_of
  {
    using type = free_module_type_of_t<ConvexSpace>::commutative_ring_type;
  };

  template<convex_space ConvexSpace>
    requires vector_space<free_module_type_of_t<ConvexSpace>> && has_field_type_v<free_module_type_of_t<ConvexSpace>>
  struct commutative_ring_type_of<ConvexSpace>
  {
    using type = free_module_type_of_t<ConvexSpace>::field_type;
  };

  template<convex_space ConvexSpace>
  using commutative_ring_type_of_t = commutative_ring_type_of<ConvexSpace>::type;

  template<convex_space ConvexSpace>
  using space_value_type = commutative_ring_type_of_t<ConvexSpace>;

  /** @ingroup ConvexSpaceUtilities
      @brief Helper to extract the dimension of the free module associated with a convex space.
   */
  
  template<convex_space ConvexSpace>
  inline constexpr std::size_t dimension_of{free_module_type_of_t<ConvexSpace>::dimension};

  /** @defgroup Basis Basis
      @brief Concepts for the basis of free modules.
   */

  /** @ingroup Basis
      @brief A basis must identify the free module to which it corresponds.

      This takes into account that a vector space is a special case of a free module.
   */
  template<class B>
  concept basis = has_free_module_type_v<B> || has_vector_space_type_v<B>;

  /** @ingroup Basis
      @brief A concept to determine if a basis is appropriate for a particular free module.
  */
  template<class B, class M>
  concept basis_for = basis<B> && (    requires { requires std::is_same_v<typename B::free_module_type, M> ; }
                                    || requires { requires std::is_same_v<typename B::vector_space_type, M>; });

  /** @defgroup Validators Validators
      @brief Validators are central to dealing with spaces where the C++ representation could produce values outside the underlying set.

      As an example, consider a half-line. Suppose the C++ representation involves
      floating-point values. Since these can be both positive and negative, runtime
      validation is required to ensure that invalid states of the half-line aren't
      constructed.

      One natural approach is for validators to throw if they encounter a value out
      of range. However, this is by no means necessary. In some situations it may
      be more appropriate to clamp, particularly if the size of a violation is the
      order of magnitude of the expected (floating-point) precision.

      For cases such as affine and vector spaces where validation is unnecessary
      (blithely ignoring the fact that NaN may be a representable floating-point
      value) std::identity holds a privileged position, indicating a transparent
      validator that performs no actual checking. However, it's privileged status
      is determined by a trait, so that careful clients could implement their
      own to deal with edge cases such as NaN.
   */

  /** @ingroup Validators
      @brief Validators for spaces of dimension 1 must provide an operator() for validating single values.

      Let the type of the commutative ring associated with a space be space_value_type.
      The validator must expose an operator() that consumes a single value of
      space_value_type, and its return type must be convertible to space_value_type.
   */
  template<class V, class ConvexSpace>
  inline constexpr bool validator_for_single_value{
       (dimension_of<ConvexSpace> == 1)
    && requires(V& v, const space_value_type<ConvexSpace>& val) { { v(val) } -> std::convertible_to<decltype(val)>; }
  };

  /** @ingroup Validators
      @brief Validators for spaces of dimension d>1 must provide an operator() for an array of d values.

      Let the type of the commutative ring associated with a space be space_value_type.
      Denote a d-dimensional std::array of such values by A. The validator must expose
      an operator() that consumes a single value of type A and its return type must be
      convertible to A.
   */
  template<class V, class ConvexSpace>
  inline constexpr bool validator_for_array{
    requires (V& v, const std::array<space_value_type<ConvexSpace>, dimension_of<ConvexSpace>>& values) {
      { v(values) } -> std::convertible_to<decltype(values)>;
    }
  };

  /** @ingroup Validators
      @brief concept to check if a validator is compatible with a convex space.
   */
  template<class V, class ConvexSpace>
  concept validator_for =
       convex_space<ConvexSpace>
    && std::default_initializable<V>
    && std::constructible_from<V, V>
    && (validator_for_single_value<V, ConvexSpace> || validator_for_array<V, ConvexSpace>);

  /** @ingroup Validators
      @brief Trait for validators that behave like the identity
   */
  template<class T>
  struct is_identity_validator : std::false_type {};

  template<class T>
  using is_identity_validator_t = is_identity_validator<T>::type;

  template<class T>
  inline constexpr bool is_identity_validator_v{is_identity_validator<T>::value};

  template<>
  struct is_identity_validator<std::identity> : std::true_type {};

  /** @ingroup Validators
      @brief A validator the the half line.

      For signed arithmetic types throws for negative values; otherwise behaves like an identity operation.
   */
  struct half_line_validator
  {
    template<arithmetic T>
    [[nodiscard]]
    constexpr T operator()(const T val) const
    {
      if(val < T{}) throw std::domain_error{std::format("Value {} less than zero", val)};

      return val;
    }

    template<arithmetic T>
      requires std::is_unsigned_v<T>
    [[nodiscard]]
    constexpr T operator()(const T val) const
    {
      return val;
    }
  };

  /** @ingroup Validators
      @brief Helper to determine if a type defines the half line.
   */
  template<class T>
  struct defines_half_line : std::false_type {};

  template<class T>
  using defines_half_line_t = typename defines_half_line<T>::type;

  template<class T>
  inline constexpr bool defines_half_line_v{defines_half_line<T>::value};

  template<>
  struct defines_half_line<half_line_validator> : std::true_type {};

  /** @defgroup Coordinates Coordinates
   */

  /** @ingroup Coordinates
      @brief Type to indicate a distinguished origin, relevant for free modules.

      Unlike vector spaces, affine spaces do not have distinguished origin. Therefore, each
      coordinate system for an affine space is with respect to a particular origin. This is
      part of the type system to ensure that different coordinate systems cannot be
      unwittingly mixed. To allow vector spaces to be treated in a similar way to affine spaces,
      a type to represent the fact that their origins are distinguished is supplied.
      
    */
  struct distinguished_origin {};

  template<
    convex_space ConvexSpace,
    basis_for<free_module_type_of_t<ConvexSpace>>  Basis,
    class Origin,
    validator_for<ConvexSpace> Validator,
    class DisplacementCoordinates
  >
  class coordinates_base;

  template<
    convex_space ConvexSpace,
    basis_for<free_module_type_of_t<ConvexSpace>> Basis,
    class Origin,
    validator_for<ConvexSpace> Validator
  >
  class coordinates;

  template<affine_space AffineSpace, basis_for<free_module_type_of_t<AffineSpace>> Basis, class Origin>
  using affine_coordinates = coordinates<AffineSpace, Basis, Origin, std::identity>;

  template<vector_space VectorSpace, basis_for<free_module_type_of_t<VectorSpace>> Basis>
  using vector_coordinates = affine_coordinates<VectorSpace, Basis, distinguished_origin>;

  template<free_module FreeModule, basis_for<free_module_type_of_t<FreeModule>> Basis>
  using free_module_coordinates = coordinates<FreeModule, Basis, distinguished_origin, std::identity>;

  //============================== direct_product ==============================//

  template<class... Ts>
  struct direct_product;

  template<class... Ts>
  using direct_product_set_t = direct_product<Ts...>::set_type;

  template<class T, class U>
  struct direct_product<T, U>
  {
  };
  
  template<free_module T, free_module U>
  struct direct_product<T, U>
  {
    using set_type       = direct_product<typename T::set_type, typename U::set_type>;
    using field_type     = std::common_type_t<commutative_ring_type_of_t<T>, commutative_ring_type_of_t<U>>;
    using is_free_module = std::true_type;
    constexpr static std::size_t dimension{T::dimension + U::dimension};
  };

  template<affine_space T, affine_space U>
    requires (!free_module<T> && !free_module<U>)
  struct direct_product<T, U>
  {
    using set_type          = direct_product<typename T::set_type, typename U::set_type>;
    using free_module_type  = direct_product<free_module_type_of_t<T>, free_module_type_of_t<U>>;
    using is_affine_space   = std::true_type;
  };

  template<convex_space T, convex_space U>
    requires (!affine_space<T> && !affine_space<U>)
  struct direct_product<T, U>
  {
    using set_type          = direct_product<typename T::set_type, typename U::set_type>;
    using free_module_type  = direct_product<free_module_type_of_t<T>, free_module_type_of_t<U>>;
    using is_convex_space   = std::true_type;
  };

  // Types assumed to be ordered
  // Could add this as a constraint, at the cost of changing
  // the algorithmic complexity...
  template<free_module... Ts>
  struct direct_product<std::tuple<Ts...>>
  {
    using set_type       = std::tuple<typename Ts::set_type...>;
    using field_type     = std::common_type_t<commutative_ring_type_of_t<Ts>...>;
    using is_free_module = std::true_type;
    constexpr static auto dimension{(Ts::dimension + ...)};
  };
  
  // Types assumed to be ordered wrt type_comparator, but dependent types may not be against the same comparator
  template<convex_space... Ts>
    requires (!free_module<Ts> && ...)
  struct direct_product<std::tuple<Ts...>>
  {
    using set_type         = std::tuple<typename Ts::set_type...>;
    using free_module_type = direct_product<free_module_type_of_t<Ts>...>;
    using is_convex_space  = std::true_type;
    // TO DO: is_convext_space or is_affine_space?!
  };

  template<class>
  struct extract_common_field_type;

  template<class T>
  using extract_common_field_type_t = extract_common_field_type<T>::type;

  template<convex_space... Ts>
  struct extract_common_field_type<std::tuple<Ts...>>
  {
    using type = std::common_type_t<typename Ts::field_type...>;
  };

  template<convex_space... Ts>
    requires (free_module<Ts> || ...) && (!free_module<Ts> || ...)
  struct direct_product<std::tuple<Ts...>>
  {
    using set_type         = std::tuple<typename Ts::set_type...>;
    using field_type       = extract_common_field_type_t<meta::filter_by_trait_t<std::tuple<Ts...>, is_free_module>>;
    using free_module_type = direct_product<free_module_type_of_t<Ts>...>;
  };

  //==============================  dual spaces ============================== //
  template<class>
  struct dual;
  
  template<convex_space C>
    requires (!affine_space<C>)
  struct dual<C>
  {
    using set_type          = C::set_type;
    using vector_space_type = dual<free_module_type_of_t<C>>;
    using is_convex_space = std::true_type;
  };

  template<affine_space C>
    requires (!vector_space<C>) // TO DO : tighten up to insist on field not ring
  struct dual<C>
  {
    using set_type          = C::set_type;
    using vector_space_type = dual<free_module_type_of_t<C>>;
    using is_affine_space   = std::true_type;
  };

  template<vector_space V>
  struct dual<V>
  {
    using set_type          = V::set_type; // TO DO: think about this
    using field_type        = commutative_ring_type_of_t<V>;
    constexpr static auto dimension{V::dimension};
    using is_vector_space = std::true_type;
  };

  template<class C>
  struct is_dual : std::false_type {};

  template<class C>
  struct is_dual<dual<C>> : std::true_type {};

  template<class C>
  using is_dual_t = is_dual<C>::type;

  template<class C>
  inline constexpr bool is_dual_v{is_dual<C>::value};

  template<class>
  struct dual_of;

  template<class T>
  using dual_of_t = dual_of<T>::type;

  template<class C>
  struct dual_of {
    using type = dual<C>;
  };

  template<class C>
  struct dual_of<dual<C>> {
    using type = C;
  };
}

namespace sequoia::maths
{  
  //============================== coordinates_base definition  ==============================//

  template<
    convex_space ConvexSpace,
    basis_for<free_module_type_of_t<ConvexSpace>> Basis,
    class Origin,
    validator_for<ConvexSpace> Validator,
    class DisplacementCoordinates=free_module_coordinates<free_module_type_of_t<ConvexSpace>, Basis>
  >
  class coordinates_base
  {
  public:
    using convex_space_type             = ConvexSpace;   
    using basis_type                    = Basis;
    using validator_type                = Validator;
    using origin_type                   = Origin;
    using set_type                      = ConvexSpace::set_type;
    using free_module_type              = free_module_type_of_t<ConvexSpace>;
    using commutative_ring_type         = commutative_ring_type_of_t<ConvexSpace>;
    using value_type                    = commutative_ring_type;
    using displacement_coordinates_type = DisplacementCoordinates;

    constexpr static bool has_distinguished_origin{std::is_same_v<Origin, distinguished_origin>};
    constexpr static bool has_identity_validator{is_identity_validator_v<Validator>};
    constexpr static std::size_t dimension{free_module_type::dimension};
    constexpr static std::size_t D{dimension};

    constexpr coordinates_base() noexcept = default;

    constexpr explicit coordinates_base(std::span<const value_type, D> vals) noexcept(has_identity_validator)
      : m_Values{validate(vals, m_Validator)}
    {}

    template<class... Ts>
      requires (D > 1) && (sizeof...(Ts) == D) && (std::convertible_to<Ts, value_type> && ...)
    constexpr coordinates_base(Ts... ts) noexcept(has_identity_validator)
      : m_Values{m_Validator(std::array<value_type, D>{ts...})}
    {}

    template<class T>
      requires (D == 1) && (std::convertible_to<T, value_type>)
    constexpr explicit coordinates_base(T val) noexcept(has_identity_validator)
      : m_Values{m_Validator(val)}
    {}

    template<class Self>
    constexpr Self& operator+=(this Self& self, const displacement_coordinates_type& v) noexcept(has_identity_validator)
    {
      self.apply_to_each_element(v.values(), [](value_type& lhs, value_type rhs){ lhs += rhs; });
      return self;
    }

    template<class Self>
      requires has_distinguished_origin && (!std::is_same_v<coordinates_base, displacement_coordinates_type>)
    constexpr Self& operator+=(this Self& self, const coordinates_base& v) noexcept(has_identity_validator)
    {
      self.apply_to_each_element(v.values(), [](value_type& lhs, value_type rhs){ lhs += rhs; });
      return self;
    }

    template<class Self>
    constexpr Self& operator-=(this Self& self, const displacement_coordinates_type& v) noexcept(has_identity_validator)
    {
      self.apply_to_each_element(v.values(), [](value_type& lhs, value_type rhs){ lhs -= rhs; });
      return self;
    }

    template<class Self>
    constexpr Self& operator*=(this Self& self, value_type u) noexcept(has_identity_validator)
      requires has_distinguished_origin
    {
      self.for_each_element([u](value_type& x) { return x *= u; });
      return self;
    }

    template<class Self>
    constexpr Self& operator/=(this Self& self, value_type u)
      requires vector_space<free_module_type> && has_distinguished_origin
    {
      self.for_each_element([u](value_type& x) { return x /= u; });
      return self;
    }

    template<class Derived>
      requires std::is_base_of_v<coordinates_base, Derived>
    [[nodiscard]]
    friend constexpr Derived operator+(Derived c, const displacement_coordinates_type& v) noexcept(has_identity_validator) { return c += v; }

    template<class Derived>
    requires std::is_base_of_v<coordinates_base, Derived> && (!std::is_same_v<Derived, displacement_coordinates_type>)
    [[nodiscard]]
    friend constexpr Derived operator+(const displacement_coordinates_type& v, Derived c) noexcept(has_identity_validator)
    {
      return c += v;
    }
  
    template<class Derived>
      requires std::is_base_of_v<coordinates_base, Derived> && (!std::is_same_v<Derived, displacement_coordinates_type>) && has_distinguished_origin 
    [[nodiscard]]
    friend constexpr Derived operator+(Derived c, const Derived& v) noexcept(has_identity_validator)
    {
      return c += v;
    }

    template<class Derived>
      requires std::is_base_of_v<coordinates_base, Derived>
    [[nodiscard]]
    friend constexpr Derived operator-(Derived c, const displacement_coordinates_type& v) noexcept(has_identity_validator)
    {
      return c -= v;
    }

    template<class Derived>
      requires std::is_base_of_v<coordinates_base, Derived> && has_distinguished_origin
    [[nodiscard]]
    friend constexpr Derived operator*(Derived v, value_type u) noexcept(has_identity_validator)
    {
      return v *= u;
    }

    template<class Derived>
      requires std::is_base_of_v<coordinates_base, Derived> && has_distinguished_origin
    [[nodiscard]]
    friend constexpr Derived operator*(value_type u, Derived v) noexcept(has_identity_validator)
    {
      return v * u;
    }

    template<class Derived>
      requires std::is_base_of_v<coordinates_base, Derived> && vector_space<free_module_type> && has_distinguished_origin
    [[nodiscard]]
    friend constexpr Derived operator/(Derived v, value_type u)
    {
      return v /= u;
    }

    [[nodiscard]]
    constexpr const validator_type& validator() const noexcept { return m_Validator; }

    [[nodiscard]]
    constexpr std::span<const value_type, D> values() const noexcept { return m_Values; }

    [[nodiscard]]
    constexpr std::span<value_type, D> values() noexcept requires(has_identity_validator) { return m_Values; }

    [[nodiscard]]
    constexpr const value_type& value() const noexcept requires (D == 1) { return m_Values[0]; }

    [[nodiscard]]
    constexpr value_type& value() noexcept requires (D == 1) && has_identity_validator { return m_Values[0]; }

    // Make this explicit since otherwise, given two vectors a,b, a/b is well-formed due to implicit boolean conversion
    [[nodiscard]]
    constexpr explicit operator bool() const noexcept requires (D == 1) && std::convertible_to<value_type, bool>
    {
      return m_Values[0];
    }

    [[nodiscard]]
    constexpr value_type operator[](std::size_t i) const { return m_Values[i]; }

    [[nodiscard]]
    constexpr value_type& operator[](std::size_t i) requires(has_identity_validator) { return m_Values[i]; }

    [[nodiscard]]
    friend constexpr bool operator==(const coordinates_base& lhs, const coordinates_base& rhs) noexcept { return lhs.m_Values == rhs.m_Values; }

    [[nodiscard]]
    friend constexpr auto operator<=>(const coordinates_base& lhs, const coordinates_base& rhs) noexcept
      requires (D == 1) && std::totally_ordered<value_type>
    {
      return lhs.value() <=> rhs.value();
    }
  protected:
    coordinates_base(const coordinates_base&)     = default;
    coordinates_base(coordinates_base&&) noexcept = default;

    coordinates_base& operator=(const coordinates_base&)     = default;
    coordinates_base& operator=(coordinates_base&&) noexcept = default;

    ~coordinates_base() = default;
    
    template<class Self, class Fn>
      requires std::invocable<Fn, value_type&, value_type>
    constexpr void apply_to_each_element(this Self& self, std::span<const value_type, D> rhs, Fn f)
    {
      if constexpr(has_identity_validator)
      {
        std::ranges::for_each(std::views::zip(self.values(), rhs), [&f](auto&& z){ f(std::get<0>(z), std::get<1>(z)); });
      }
      else
      {
        auto tmp{self.m_Values};
        std::ranges::for_each(std::views::zip(tmp, rhs), [&f](auto&& z){ f(std::get<0>(z), std::get<1>(z)); });
        self.m_Values = validate(tmp, self.m_Validator);
      }
    }

    template<class Self, class Fn>
      requires std::invocable<Fn, value_type&>
    constexpr void for_each_element(this Self& self, Fn f)
    {
      if constexpr(has_identity_validator)
      {
        std::ranges::for_each(self.values(), f);
      }
      else
      {
        auto tmp{self.m_Values};
        std::ranges::for_each(tmp, f);
        self.m_Values = validate(tmp, self.m_Validator);
      }
    }
  private:
    SEQUOIA_NO_UNIQUE_ADDRESS validator_type m_Validator;
    std::array<value_type, D> m_Values{};

    [[nodiscard]]
    static std::array<value_type, D> validate(std::span<const value_type, D> vals, Validator& validator)
    {
      return validate(utilities::to_array(vals), validator);
    }

    [[nodiscard]]
    static std::array<value_type, D> validate(std::array<value_type, D> vals, Validator& validator)
    {
      if constexpr(validator_for_array<Validator, ConvexSpace>)
        return validator(vals);
      else
        return {validator(vals.front())};
    }
  };
  
  template<convex_space ConvexSpace, basis_for<free_module_type_of_t<ConvexSpace>> Basis, class Origin, validator_for<ConvexSpace> Validator>
  class coordinates final : public coordinates_base<ConvexSpace, Basis, Origin, Validator>
  {
  public:
    using base_type = coordinates_base<ConvexSpace, Basis, Origin, Validator>;
    using base_type::base_type;
    using displacement_coordinates_type = base_type::displacement_coordinates_type;
    using value_type                    = base_type::value_type;
    constexpr static bool has_identity_validator{base_type::has_identity_validator};

    [[nodiscard]]
    friend constexpr displacement_coordinates_type operator-(const coordinates& lhs, const coordinates& rhs) noexcept(has_identity_validator)
    {
      return[&] <std::size_t... Is>(std::index_sequence<Is...>) {
        return displacement_coordinates_type{(lhs.values()[Is] - rhs.values()[Is])...};
      }(std::make_index_sequence<base_type::D>{});
    }

    [[nodiscard]]
    constexpr coordinates operator+() const noexcept(has_identity_validator)
    {
      return coordinates{this->values()};
    }

    [[nodiscard]]
    constexpr coordinates operator-() const noexcept(has_identity_validator)
    {
      return coordinates{utilities::to_array(this->values(), [](value_type t) { return -t; })};
    }
  };

  namespace sets
  {
    template<std::size_t D>
    struct Z
    {
      constexpr static std::size_t dimension{D};
    };

    template<std::size_t D>
    struct Z_0
    {
      constexpr static std::size_t dimension{D};
    };
    
    template<std::size_t D>
    struct R
    {
      constexpr static std::size_t dimension{D};
    };

    template<std::size_t D>
    struct half_space
    {
      constexpr static std::size_t dimension{D};
    };


    template<std::size_t D>
    struct C
    {
      constexpr static std::size_t dimension{D};
    };
  }

  template<class B>
  inline constexpr bool is_orthonormal_basis_v{
    requires {
      typename B::orthonormal;
      requires std::same_as<typename B::orthonormal, std::true_type>;
    }
  };

  template<vector_space V>
  struct arbitary_basis {};

  template<vector_space V>
  inline constexpr bool has_norm_v{
    requires (const vector_coordinates<V, arbitary_basis<V>>& v) {
      { norm(v) } -> std::convertible_to<typename V::field_type>;
    }
  };

  template<vector_space V>
  inline constexpr bool has_inner_product_v{
    requires (const vector_coordinates<V, arbitary_basis<V>>& v) {
      { inner_product(v, v) } -> std::convertible_to<typename V::field_type>;
    }
  };

  template<class V>
  concept normed_vector_space = vector_space<V> && has_norm_v<V>;

  template<class V>
  concept inner_product_space = vector_space<V> && has_inner_product_v<V>;

  template<std::size_t D, std::floating_point T>
  struct euclidean_vector_space
  {
    using set_type          = sets::R<D>;
    using field_type        = T;
    using is_vector_space = std::true_type;
    using vector_space_type = euclidean_vector_space;
    constexpr static std::size_t dimension{D};

    template<basis Basis>
      requires is_orthonormal_basis_v<Basis>
    [[nodiscard]]
    friend constexpr field_type inner_product(const vector_coordinates<euclidean_vector_space, Basis>& v, const vector_coordinates<euclidean_vector_space, Basis>& w)
    {
      return std::ranges::fold_left(std::views::zip(v.values(), w.values()), field_type{}, [](field_type f, const auto& z){ return f + std::get<0>(z) * std::get<1>(z); });
    }

    template<basis Basis>
      requires is_orthonormal_basis_v<Basis>
    [[nodiscard]]
    friend constexpr field_type dot(const vector_coordinates<euclidean_vector_space, Basis>& v, const vector_coordinates<euclidean_vector_space, Basis>& w)
    {
      return inner_product(v, w);
    }

    template<basis Basis>
      requires is_orthonormal_basis_v<Basis>
    [[nodiscard]]
    friend constexpr field_type norm(const vector_coordinates<euclidean_vector_space, Basis>& v)
    {
      if constexpr(D == 1)
      {
        return std::abs(v.value());
      }
      else
      {
        return std::sqrt(inner_product(v, v));
      }
    }
  };

  template<std::size_t D, std::floating_point T>
  struct euclidean_affine_space
  {
    using set_type          = sets::R<D>;
    using vector_space_type = euclidean_vector_space<D, T>;
    using is_affine_space = std::true_type;
    //using affine_space_type = euclidean_affine_space;
  };

  template<std::size_t D, std::floating_point T>
  struct euclidean_half_space
  {
    using set_type          = sets::half_space<D>;
    using vector_space_type = euclidean_vector_space<D, T>;
    using is_convex_space = std::true_type;
    //using convex_space_type = euclidean_half_space;
  };

  template<std::size_t D, std::floating_point T, basis Basis, class Origin>
  using euclidean_affine_coordinates = affine_coordinates<euclidean_affine_space<D, T>, Basis, Origin>;

  template<std::size_t D, std::floating_point T, basis Basis>
  using euclidean_vector_coordinates = vector_coordinates<euclidean_vector_space<D, T>, Basis>;

  template<std::size_t D, std::floating_point T>
  struct standard_basis
  {
    using vector_space_type = euclidean_vector_space<D, T>;
    using orthonormal = std::true_type;
  };

  template<std::size_t D, std::floating_point T>
  using vec_coords = euclidean_vector_coordinates<D, T, standard_basis<D, T>>;
}
