////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** @file
    @brief Abstractions pertaining to vector spaces, affine spaces and their generalizations.

    Representing abstract algebraic structures in C++ presents an interesting
    challenge. At root, the fundamental abstraction is a set; indeed, a vector
    space is nothing but a set with some additional structure defined. However,
    sets of objects are not straightforward to represent, in general, in C++.

    Consider the real numbers. In C++ we can give a name to the is set
    by using the type system.

    struct R {};

    But what of the elements of R? Here we run into an immediate difficulty. We
    would like to associate them with the values of a type. But to truly do so
    we require a type with an infinite number of values (and uncountably so, in this
    case). Therefore, when seeking representations, particularly of infinite sets,
    we are generally reduced to approximations.

    However, at least in so far as the set underpinning a particular vector space
    goes, it turns out that, for our purposes, we need go no further than naming it.
    As we will explain momentarily, this is because when dealing with vector spaces
    in practice, we are almost always interested in the _coordinates_ of a vector
    with respect to a particular basis and not the abstract entities comprising
    the elements of the underlying set.

    Before discussing coordinate systems, it is helpful to be more precise about
    the fundamental definition of a vector space. The first four axioms pertain
    just to the elements of the underlying set and amount to stating that a vector
    space is an abelian group under addition. Thus, vector addition is associative,
    commutative, has an identity element and admits an inverse.

    However, the remaining properties of a vector space require not just the underlying
    set, V, but also a field, F. Canonical examples of a field include the rationals or
    the reals: sets admitting the standard arithmetic operations and for which every
    element has both an additive and multiplicative inverse. As such, we speak of
    a vector space over a field. A vector space admits a binary operation such that
    any element of V can be multiplied by any element of F, to give another element
    within V (or the same element for the multiplicative identity).

    Every vector space admits at least one basis. For a vector space of dimension d,
    a basis is a set of d elements which are linearly independent and so span the
    space. Let the basis elements be denoted b_0, ..., b_{n-1}. Any vector in the space
    may be written as a linear combination:

    v = a_0 b_0 + ... + a_{n-1} b_{n-1},

    where the a_i are valued in the field, F. The set of these values [a_0, ..., a_{n-1}]
    are none other than the coordinates of v with respect to this particular basis.
    The [a_0, ..., a_{n-1}] are often informally referred to as a vector. However
    stricty speaking this is an abuse of terminology and conflates two distinct concepts:
    an actual vector which is an element of the set which forms the vector space,
    and a representation of this vector via the coordinates with respect to a
    particular basis. This distinction can be further reinforced by pointing out that
    two observers who agree they are talking about the same vector (i.e. set element)
    will nevertheless disagree on the coordinates if they are using different bases -
    as they are entirely entitled to do! To further add to the confusion, the coordinates
    may be referred to as a coordinate vector which is perhaps unfortunate.

    Regardless, from the perspective of performing actual calculations, the coordinates
    are key. An crucial point to make is that, when dealing with the coordinates,
    the underlying elements of the set, V, make no explicit appearance. This is a
    manifestation of the fact that two vector spaces of the same dimension and over
    the same field are isomorphic. This is incredibly helpful since, for many practical
    purposes, we need not represent the underlying set beyond, at most, perhaps giving
    it a name. For example, consider the vector space formed by functions
    which map some set into a field: the question of how to represent the elements
    of this vector space in C++ is completely circumvented.

    However, that is not to say that subtleties of imperfect representations of
    mathematical abstractions are entirely avoided; indeed, quite the contrary! The
    coordinates are valued in a field and so at this stage we must deal with the fact that
    C++ types such as float and double model the real numbers imperfectly. Nevertheless,
    the burden has been shifted from attempting to represent things in C++ that may
    be completely infeasible to things which can be done to reasonable approximation.

    Vector spaces are just one of the things treated in the code that follows. There
    are several important generalizations. First, there are affine spaces, which comprise
    a set, A, together with a vector space, V, whose additive group acts freely and
    transitively on A. Intuitvely, we can start at any point in A and translate to
    any other point by adding the appropriate vector. In fact, the relationship is
    stronger than this: choosing any point in A and adding any vector in V will give
    a point in A. A nice example of an affine space is Euclidean space. Two observers
    in this space, Alice and Bob, are entirely entitled to define their location as the
    origin. Neither is more right than the other since this space has no distinguished origin.
    Alice and Bob will, in general, disagree about the coordinates of points in the space.
    However, they will agree on the vector which translates from one point to another (though
    if they compare vector coordinates, they may have ton contend with using different bases
    on the vector space!).

    An affine space is sometimes described as a vector space which has forgotten its origin.
    Indeed, a vector space is an affine space over itself. This is interesting in terms
    of representing these concepts in C++. Since a vector space is a special case of
    an affine space, this suggests that an affine space concept is more fundamental,
    with the vector space being a refinement. However, a vector space is part of the
    definition of an affine space (a set and a vector space, satisfying certain conditions)
    and so it is this that will be reflected by the concepts defined below: the affine
    space concept depends on the vector_space concent, and not vice-versa.

    It will be useful for our purposes to generalize affine spaces. Consider taking a
    convex subset, C, of an affine space. We may translate from any point in C to any
    other by adding the appropriate vector from V. However, there are elements of V
    which, when added to a point in C will take us outside of C and into the broader
    affine space into which it is a part. However, we do not want define Convex spaces
    via an embedding in a bigger space. There seems to be a solution which fits neatly
    into a C++ implementation. Define a convex space, C, to comprise:
    1. The union, S', of a set of points, S, and an exception state, E
    2. A vector space, V
    such that:
    A. The difference of any two points in S is in V
    B. An element of V, when added to S remains either in S or maps to E
    Note that, since every point in S is mapped by elements of V into E, and there is
    no mapping from E back into S, the action of the additive group of V is not bijective,
    violating one of the axioms of affine spaces.

    The other interesting generialization is to consider relaxing a vector space's field
    to a ring. The resulting construction is called a module, which is a generalization
    of a vector space. Our motivation for this is that the integers form a commutative
    ring and not a field, since integers do not, in general, have multiplicative inverses
    valued within the integers. Rather than attempting to deal with modules in full generality,
    we restrict our attention to what may be the most useful, practical cases in the context
    of C++: free modules over commutative rings. Free modules are those which admit a
    basis.

    In line with the above, we also consider affine spaces over free modules and their
    convex generalization where the action of the free module is not bijective.

    The final issue to address is that question of why to bother modelling concepts such
    as vector spaces in the abstract sense if it is their coordinates which are the things
    of use from the perspective of practical computation. The point is that, for example,
    vector spaces and affine spaces admit different operations: whereas elements of a vector
    space can be added, the same is not true of the elements of the set underpinning an
    affine space. By introducing concepts for the abstract algebraic constructs, we can treat
    coordinates on all of these spaces in a common way by using constraints to enable
    or disable specific operations. Thus, the coordinates class template is templated on,
    amongst other things, a convex_space. To define such a space just requires introducing
    a struct exposing a small amount of data (types and values) known at compile time.
    These data determines whether we intend to model a vector space, a free module over a
    commutative space, an affine space or whatever. This is sufficient for the coordinates
    implementation to expose the correct set of operations.
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
  /** @defgroup ArithmeticProperties Arithmetic Properties
      @brief Tools to reflect on whether types expose the standard arithmetic operations.

      The main subtlety for the topics that will concern us is division. From the perspective
      of abstract algebra,  multiplication is the more primitive operation. If each element of
      a set has a multiplicative inverse then for each element, a, there exists a single element,
      a^{-1}, such that

          a * a^{-1} = 1

      From this, a division operation can be defined according to

          a / b = a  * b^{-1}

      However, there are common types that we will deal with - such as ints and size_ts - for
      which most elements do not have multiplicative inverses valued within the type. A simple
      example is int x = 2. The multiplicative inverse is 1/2, which is not an int.

      Nevertheless, C++'s arithmetic types define division. For signed types this corresponds to a
      so-called Euclidean domain. For the unsigned types the operation is algorithmically the
      same, but I actually don't know the name for the associated mathematical structure.

      Regardless, the purpose of the utilities in the following set is simply naive reflection on
      whether particular operations exist in the C++ language and not the nuanced semantics.
      Therefore, it would be _incorrect_ to conclude that, just because a type is addable,
      subtractable, multiplicable and divisible that it models a field.
   */

  /** @ingroup ArithmeticProperties
      @brief Compile time constant for addability
   */
  template<class T>
  inline constexpr bool is_addable_v{
    requires(T& t) {
      { t += t } -> std::same_as<T&>;
      { t + t }  -> std::convertible_to<T>;
    }
  };

  /** @ingroup ArithmeticProperties
      @brief Compile time constant for subtractability
   */
  template<class T>
  inline constexpr bool is_subtractable_v{
    requires(T& t) {
      { t -= t } -> std::same_as<T&>;
      { t - t }  -> std::convertible_to<T>;
    }
  };

  /** @ingroup ArithmeticProperties
      @brief Compile time constant for multiplicability
   */
  template<class T>
  inline constexpr bool is_multiplicable_v{
    requires(T& t) {
      { t *= t } -> std::same_as<T&>;
      { t * t }  -> std::convertible_to<T>;
    }
  };

  /** @ingroup ArithmeticProperties
      @brief Compile time constant for divisibility
   */
  template<class T>
  inline constexpr bool is_divisible_v{
    requires(T& t) {
      { t /= t } -> std::same_as<T&>;
      { t / t }  -> std::convertible_to<T>;
    }
  };

  /** @defgroup AlgebraicTraits Algebraic Traits
      @brief Traits and concepts for basic elements of abstract algebra.

      A fundamental problem of attempting this classification on a computer is the difference
      between a mathematical structure and an approximate representation of that structure.
      ints model the integers, but not exactly since there is a maximum representable value.
      Similarly, floating-point numbers model the reals but only in an approximate sense.
      To signify the fact that neither integer nor floating-point addition exactly models an
      abelian group, the term 'weak' is used. Note, however, that addition of unsigned integral
      types does precisely model an abelian group and so 'weak' is a minimum requirement.
      
      Entertaingly, the only fundamental type in C++ which exacly models a field is bool.
   */
  
  /** @ingroup AlgebraicTraits
      @brief Trait for specifying whether a type behaves (appoximately) as an abelian group under addition.

      This includes all the arithmetic types, with the unsigned one behaving precisely as an abelian group
      under addition.
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

  /** @ingroup AlgebraicTraits
      @brief Trait for specifying whether a type behaves (appoximately) as an abelian group under multiplication.

      The floating-point types are taken to weakly model an abelian group under multiplication,
      since they attempt to approximate the reals.
      
      The only integral type modelling this (exactly, as it transpires) is bool. It is the only type in C++
      modelling Z/Zn where n is a prime. The other integral types do not model this even in an approximate sense.
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
  
  /** @ingroup AlgebraicTraits
      @brief Trait for specifying whether a type exhibits multiplication that (approximately) distributes over addition.
   */

  template<class T>
  struct multiplication_weakly_distributive_over_addition : std::true_type {};

  template<class T>
  using multiplication_weakly_distributive_over_addition_t = typename multiplication_weakly_distributive_over_addition<T>::type;

  template<class T>
  inline constexpr bool multiplication_weakly_distributive_over_addition_v{multiplication_weakly_distributive_over_addition<T>::value};

  /** @ingroup AlgebraicTraits
      @brief concept representing reasonable approximations to a commutative ring.
   */
  
  template<class T>
  concept weak_commutative_ring 
    =    std::regular<T>
      && weakly_abelian_group_under_addition_v<T>
      && multiplication_weakly_distributive_over_addition_v<T>
      && is_addable_v<T>
      && is_subtractable_v<T>
      && is_multiplicable_v<T>;

  /** @ingroup AlgebraicTraits
      @brief concept representing reasonable approximations to a field.
   */

  template<class T>
  concept weak_field = weak_commutative_ring<T> && weakly_abelian_group_under_multiplication_v<T> && is_divisible_v<T>;

  /** @defgroup PropertiesOfSpaces Properties of Spaces
      @brief Tools to reflect on whether types expose other types typically associated with various spaces.
   */

  /** @ingroup PropertiesOfSpaces
      @brief Compile time constant reflecting whether a type exposes a nested type named commutative_ring_type which satisifes the weak_commutative_ring concept.
   */ 
  template<class T>
  inline constexpr bool has_commutative_ring_type_v{
    requires { 
      typename T::commutative_ring_type;
      requires weak_commutative_ring<typename T::commutative_ring_type>;
    }
  };

  /** @ingroup PropertiesOfSpaces
      @brief Compile time constant reflecting whether a type exposes a nested type named field_type which satisifes the weak_field concept.
   */ 
  template<class T>
  inline constexpr bool has_field_type_v{
    requires { 
      typename T::field_type;
      requires weak_field<typename T::field_type>;
    }
  };

  /** @ingroup PropertiesOfSpaces
      @brief Compile time constant reflecting whether a type exposes a nested type with the properties of a commutative ring.

      The point here is that a field is a special case of a ring. Therefore, anything which defines
      a field is implicitly defining a ring.
    */
  template<class T>
  inline constexpr bool defines_commutative_ring_v{has_commutative_ring_type_v<T> || has_field_type_v<T>};

  /** @ingroup PropertiesOfSpaces
      @brief Reports whether a type exposes a nested type with the properties of a field.

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

  /** @ingroup PropertiesOfSpaces
      @brief Compile time constant reflecting whether a type exposes a nested value, dimension, convertible to a std::size_t
   */
  template<class T>
  inline constexpr bool has_dimension_v{
    requires { { T::dimension } -> std::convertible_to<std::size_t>; }
  };

  /** @ingroup PropertiesOfSpaces
      @brief Compile time constant reflecting whether a type exposes a nested type named set_type.
   */
  template<class T>
  inline constexpr bool has_set_type_v{
    requires { typename T::set_type; }
  };

  /** @defgroup IdentifiesAsSpace Self-identification of Spaces
      @brief Compile time constants to capture whether types self-identify as various spaces.

      It is straightforward to say that a type, T, exposes nested types, say set_type
      and field_type. This is suggestive of a vector space but to do able to unequivocally
      identify T as a vector space requires additional information, since we have no way
      of knowing, a priori, whether the vector space axioms are satisfied. Indeed, it may
      not be straightforward to provide the operations for addition and multiplication as
      they apply to the elements of the underlying set: practically speaking, we are usually
      dealing with operations on coordinates with respect to some basis.

      Therefore, the approach taken is to demand that various spaces self identify as such.
      For a free module this would mean exposing a type, is_free_module, convertible to
      std::true_type. Note, though, that this is not sufficient for T to be a free module,
      as there are additional requirements that must be met (see the free_module concept).      
   */

  /** @ingroup IdentifiesAsSpace
      @brief Compile time constant reflecting whether a space self-identifies as convex.
   */
  template<class T>
  inline constexpr bool identifies_as_convex_space_v{
     requires {
      typename T::is_convex_space;
      requires std::convertible_to<typename T::is_convex_space, std::true_type>;
    }
  };

  /** @ingroup IdentifiesAsSpace
      @brief Compile time constant reflecting whether a space self-identifies as affine.
   */
  template<class T>
  inline constexpr bool identifies_as_affine_space_v{
     requires {
      typename T::is_affine_space;
      requires std::convertible_to<typename T::is_affine_space, std::true_type>;
    }
  };

  /** @ingroup IdentifiesAsSpace
      @brief Compile time constant reflecting whether a space self-identifies as a free module.
   */
  template<class T>
  inline constexpr bool identifies_as_free_module_v{
    requires {
      typename T::is_free_module;
      requires std::convertible_to<typename T::is_free_module, std::true_type>;
    }
  };

  /** @ingroup IdentifiesAsSpace
      @brief Compile time constant reflecting whether a space self-identifies as vector space.
   */
  template<class T>
  inline constexpr bool identifies_as_vector_space_v{
    requires {
      typename T::is_vector_space;
      requires std::convertible_to<typename T::is_vector_space, std::true_type>;
    }
  };
  
  /** @defgroup Spaces Spaces
      @brief Concepts and helpers pertaining to vector spaces, affine spaces and certain generalizations.    
   */

  /** @ingroup Spaces
      @brief concept for a free module, implicitly understood to be over a commutative ring.

      Free modules admit a basis. Our particular interest is in free modules over a commutative
      ring. For want of a better term we slightly abuse free module to stand for
      "free module over a commutative ring".
   */
  template<class T>
  concept free_module = has_set_type_v<T> && has_dimension_v<T> && defines_commutative_ring_v<T> && (identifies_as_free_module_v<T> || identifies_as_vector_space_v<T>);

  /** @ingroup Spaces
      @brief concept for a vector space, which is a special case of a free module
   */
  template<class T>
  concept vector_space = free_module<T> && defines_field_v<T>;

  /** @ingroup PropertiesOfSpaces
      @brief Compile time constant reflecting whether a type exposes a nested
             vector_space_type which satisfies the vector_space concept.
   */
  template<class T>
  inline constexpr bool has_vector_space_type_v{
    requires {
      typename T::vector_space_type;
      requires vector_space<typename T::vector_space_type>;
    }
  };

  /** @ingroup PropertiesOfSpaces
      @brief Compile time constant reflecting whether a type exposes a nested
             free_modul_type which satisfies the vector_space concept.
   */
  template<class T>
  inline constexpr bool has_free_module_type_v{
    requires {
      typename T::free_module_type;
      requires free_module<typename T::free_module_type>;
    }
  };

  /** @ingroup Spaces
      @brief concept for convex spaces

      A convex space may be a free module. Otherwise, it comprises a set and a
      free module (which may be a vector space), and must identify as either a convex
      or affine space.      
   */
  template<class T>
  concept convex_space
    =    free_module<T>
      || (    has_set_type_v<T>
          && (has_vector_space_type_v<T> || has_free_module_type_v<T>)
          && (identifies_as_convex_space_v<T> || identifies_as_affine_space_v<T>));

  /** @ingroup Spaces
      @brief concept for affine spaces

      A vector space is an affine space over itself; beyond that, according to our
      definitions, an affine space is a refinement of a convex space.
   */
  template<class T>
  concept affine_space = vector_space<T> || (convex_space<T> && identifies_as_affine_space_v<T>);
  
  /** @ingroup Spaces
      @brief Helper that universal template parameters will obviate the need for
    */
  template<class T>
  struct is_free_module : std::integral_constant<bool, free_module<T>> {};

  /** @defgroup SpacesUtilities Convex Space Utilities
      @brief Utilites for extracting properties of convex spaces
   */

  template<convex_space Space>
  inline constexpr bool has_distinguished_origin_type_v{
    requires {
      typename Space::distinguished_origin;
      requires (   std::convertible_to<typename Space::distinguished_origin, std::true_type>
                || std::convertible_to<typename Space::distinguished_origin, std::false_type>);
    }
  };

  template<convex_space Space>
  struct has_distinguished_origin : std::false_type
  {};

  template<convex_space Space>
    requires has_distinguished_origin_type_v<Space>
  struct has_distinguished_origin<Space> : Space::distinguished_origin::type
  {};

  template<free_module Space>
  struct has_distinguished_origin<Space> : std::true_type
  {};

  template<affine_space Space>
    requires (!free_module<Space>)
  struct has_distinguished_origin<Space> : std::false_type
  {};

  template<convex_space Space>
  using has_distinguished_origin_t = has_distinguished_origin<Space>::type;

  template<convex_space Space>
  inline constexpr bool has_distinguished_origin_v{has_distinguished_origin<Space>::value};
  
  /** @ingroup PropertiesOfSpace
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
  
  /** @ingroup PropertiesOfSpaces
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

  /** @ingroup PropertiesOfSpaces
      @brief Helper to extract the dimension of the free module associated with a convex space.
   */
  
  template<convex_space ConvexSpace>
  inline constexpr std::size_t dimension_of{free_module_type_of_t<ConvexSpace>::dimension};

  /** @defgroup Basis Basis
      @brief Concepts and helpers for bases of free modules.
   */

  /** @ingroup Basis
      @brief Compile time constant reflecting whether a type self-identifies as a basis.
   */
  template<class T>
  inline constexpr bool identifies_as_basis_v{
     requires {
      typename T::is_basis;
      requires std::convertible_to<typename T::is_basis, std::true_type>;
    }
  };
  
  /** @ingroup Basis
      @brief A basis must identify the free module to which it corresponds.

      This takes into account that a vector space is a special case of a free module.
   */
  template<class B>
  concept basis = identifies_as_basis_v<B> && (has_free_module_type_v<B> || has_vector_space_type_v<B>);

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
      validator that performs no actual checking. However, its privileged status
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
      @brief Trait for validators that behave like the identity.
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
      @brief Trait to determine if a type defines the half line.
   */
  template<class T>
  struct defines_half_line : std::false_type {};

  template<class T>
  using defines_half_line_t = typename defines_half_line<T>::type;

  template<class T>
  inline constexpr bool defines_half_line_v{defines_half_line<T>::value};

  template<>
  struct defines_half_line<half_line_validator> : std::true_type {};

  /** @defgroup DirectProduct Direct Product
      @brief Direct Products are one way in which spaces can be composed to create new spaces.

      At the root of everything considered in this file are sets. One way to compose
      sets is by taking the Cartesian Product. However, the objects we are considering,
      such as free modules, have additional structure. Direct products induce structure
      on the cartesian product. For example, the direct product of two vector spaces over
      the same field may also produce a vector space.

      Note, however, that there are various subtleties. The direct product of two vector
      spaces that are not over the same field does not produce another vector space.
      More generally, the direct product of two modules, M_1 and M_2, respectively over
      rings R_1 and R_2, yields a new module over the direct product of R_1 and R_2.
      If R_1 and R_2 are the same (say R), we may choose to map onto R, meaning that
      it is possible to construct a module via direct product, in this case, over R.
      However, in general this is not allowed.

      For now, we do not handle the general case. Thus, we may only consruct the direct
      product of free modules if they are, roughly speaking, over the same ring. To be
      precise, the free modules' commutative rings must either all satsify the weak_field
      concept or none of them do; on top of which they must share a common type in the C++
      sense.
   */

  template<class... Ts>
  struct direct_product
  {
  };

  template<weak_commutative_ring... Rs>
  inline constexpr bool has_common_ring_v{
    ((weak_field<Rs> && ...) || (!weak_field<Rs> && ...)) && requires { typename std::common_type<Rs...>::type; }
  };

  template<free_module... Ts>
    requires (sizeof...(Ts) >= 1) && has_common_ring_v<commutative_ring_type_of_t<Ts>...>
  struct direct_product<Ts...>
  {
    using set_type              = direct_product<typename Ts::set_type...>;
    using commutative_ring_type = std::common_type_t<commutative_ring_type_of_t<Ts>...>;
    using is_free_module        = std::true_type;
    constexpr static std::size_t dimension{(Ts::dimension + ...)};
  };

  template<affine_space... Ts>
    requires (sizeof...(Ts) >= 1) && (!free_module<Ts> && ...)
  struct direct_product<Ts...>
  {
    using set_type         = direct_product<typename Ts::set_type...>;
    using free_module_type = direct_product<free_module_type_of_t<Ts>...>;
    using is_affine_space  = std::true_type;
  };

  template<convex_space... Ts>
    requires (sizeof...(Ts) >= 1)
          && ((!affine_space<Ts> && ...) || ((free_module<Ts> || ...) && (!free_module<Ts> || ...)))
  struct direct_product<Ts...>
  {
    using set_type         = direct_product<typename Ts::set_type...>;
    using free_module_type = direct_product<free_module_type_of_t<Ts>...>;
    using is_convex_space  = std::true_type;
  };

  template<class T>
  struct is_direct_product : std::false_type {};

  template<class... Ts>
  struct is_direct_product<direct_product<Ts...>> : std::true_type {};

  template<class T>
  using is_direct_product_t = is_direct_product<T>::type;

  template<class T>
  inline constexpr bool is_direct_product_v = is_direct_product<T>::value;

  /** @defgroup DualSpaces Dual Spaces
      @brief Dual vector spaces and various generalizations.

      When considering relationships between vector spaces, linear
      maps play a central role. These are such that
      
        f(x + y) -> f(x) + f(y)

      and are structure-preserving: both vector addition and scalar
      multiplication survive. Therefore, linear maps may be recognized
      as homomorphisms between vector spaces. Note that the space of linear
      mappings may equivalently be called the space of linear functionals.

      Given a vector space, V, over a field F, the space of linear mappings
      from V to F is of particular importance and is known as the dual space
      V*. In this context - as the target of a homomorphism - F is considered
      to be a vector space.
      
      Since dual vector space are just vector spaces, we may handle these
      within the approach introduced above. In particular, we know both
      the field and dimension of the dual space: these are simply those
      of the original vector space. The set underlying the dual vector
      space seems more problematic: how do we represent the set of linear
      functionals? But actually, this is not an issue within our approach
      since all we are required to do is name the strcuture and not attempt
      the far more difficult task of somehow specifying the elements. It
      is therefore sufficient for our purposes to create a class template,
      linear_functionals, the template parameters of which specify the spaces
      between which it maps.

      This construction has an analogue for modules, with the field
      associated with a vector space relaxed to a ring. However, the
      situation is not so simple for the other structures we consider:
      affine and convex spaces. In this case, rather the linear
      functionals which satisfy the above equation, we consider the more
      general convex functionals:

        f(lambda x + (1 - lambda) y) = lambda f(x) + (1 - lambda) f(y),

        with 0 <= lambda <= 1.

      For vector spaces, the dual of the dual is isomorphic to the original
      space. From the perspective of C++, given a type T we shall identify
      the dual of the dual of T as just t itself. However, clients may override this
      behaviour through template specialization if a more precise statement
      of the relationship is required. Indeed, for general modules this does
      not hold and so care must be taken.
   */

  /** @defgroup Sets Sets
      @brief The sets underpinning the various spaces of interest need, for our purposes, just to be named.
   */
  
  namespace sets
  {
    /** @ingroup Sets
        @brief Class template for giving a name to convex functionals.

        It is tempting to constrain the class To to be a convex space. However,
        without additional work, rings and fields do not satisfy the convex_space
        concept as introduced, above.
     */
    template<convex_space From, class To>
    struct convex_functionals
    {
    };

    /** @ingroup Sets
        @brief Class template for giving a name to linear functionals.

        It is tempting to constrain the class To to be a vector space. However,
        without additional work, rings and fields do not satisfy the vector_space
        concept as introduced, above.
     */
    template<vector_space From, class To>
    struct linear_functionals
    {
    };
  }

  /** @ingroup DualSpaces
      @brief Primary class template for defining duals.
   */
  template<class>
  struct dual;

  /** @ingroup DualSpaces
      @brief Specialization for defining duals of convex spaces via convex functionals
   */
  template<convex_space C>
    requires (!affine_space<C>)
  struct dual<C>
  {
    using set_type             = sets::convex_functionals<C, commutative_ring_type_of_t<C>>;
    using free_module_type     = dual<free_module_type_of_t<C>>;
    using is_convex_space      = std::true_type;
    using distinguished_origin = has_distinguished_origin_t<C>;
  };

   /** @ingroup DualSpaces
      @brief Specialization for defining duals of affine spaces via convex functionals
   */
  template<affine_space A>
    requires (!vector_space<A>)
  struct dual<A>
  {
    using set_type         = sets::convex_functionals<A, commutative_ring_type_of_t<A>>;
    using free_module_type = dual<free_module_type_of_t<A>>;
    using is_affine_space  = std::true_type;
  };

  /** @ingroup DualSpaces
      @brief Specialization for defining duals of vector spaces via linear functionals
   */
  template<vector_space V>
  struct dual<V>
  {    
    using field_type      = commutative_ring_type_of_t<V>;
    using set_type        = sets::linear_functionals<V, field_type>;
    using is_vector_space = std::true_type;
    constexpr static auto dimension{V::dimension};
  };

  /** @ingroup DualSpaces
      @brief Helper to detect if a type is defined as a dual of something else
   */
  template<class T>
  struct is_dual : std::false_type {};

  template<class T>
  struct is_dual<dual<T>> : std::true_type {};

  template<class T>
  using is_dual_t = is_dual<T>::type;

  template<class T>
  inline constexpr bool is_dual_v{is_dual<T>::value};

  /** @ingroup DualSpaces
      @brief Helper to generate the dual of a space, taking into account that the dual of the dual may be related to the original space.

      The dual of the dual of a finite dimensional vector space, V, is isomporphic
      to V. We take this double dual to be just V itself. Similarly for the affine
      and convex generalizations, though insisting that the ring is a field.
      Clients can override this behaviour with the appropriate specializations.
   */
  template<class>
  struct dual_of;

  template<class T>
  using dual_of_t = dual_of<T>::type;

  template<class T>
  struct dual_of {
    using type = dual<T>;
  };

  template<class T>
    requires (!convex_space<T>) || (convex_space<T> && weak_field<commutative_ring_type_of_t<T>>)
  struct dual_of<dual<T>> {
    using type = T;
  };

  /** @defgroup Coordinates Coordinates
      @brief Coordinates are the bridge between the abstract mathematics of spaces and practical application.

      When dealing with vectors in practice, almost invariably one is using the coordinates of
      vectors with respect to a particular basis. These are often implicitly conflated with
      the vector itself. However, the latter are simply elements of a vector space and there
      is no sense in which different observers can disagree about properties of this
      fundamental entity. Nevertheless, observers using different bases can absolutely disagree
      on the coordinates, though once they figure out the relationship between their bases
      then it becomes possible to translate from one to the other.

      It is worth noting that, for a vector space, the kernel of the implementation of the
      coordinates depends only the field and the dimension. This reflects the fact that vector
      spaces of the same dimension and over the same field are isomorphic. Similar considerations
      apply to the various related spaces with which we deal.

      A key element of our approach to coordinates is to template on (amongst other things) the
      underlying space. On the one hand this gives a high degree of type safety; on the other
      it means that the various different spaces of interest to us can be handled in a uniform
      manner. For example, affine spaces and vector spaces admit different operations. Knowing
      the characteristics of the underlying space means that we may statically enable or disable
      appropriate functionality. For example, coordinates on a vector space may be multiplied
      by a scalar; not so those on an affine space.
   */

  /** @ingroup Coordinates
      @brief Type to indicate a distinguished origin, relevant for free modules.

      Unlike vector spaces, affine spaces do not have distinguished origin. Therefore, each
      coordinate system for an affine space is with respect to a particular origin. This is
      part of the type system to ensure that different coordinate systems cannot be
      unwittingly mixed. To allow vector spaces to be treated in a similar way to affine spaces,
      a type to represent the fact that their origins are distinguished is supplied.
      
    */

  template<class... Ts>
  struct coordinate_system;
  

  /** @ingroup Coordinates
      @brief Forward declaration for the coordinates class template.
   */

  template<
    convex_space ConvexSpace,
    basis_for<free_module_type_of_t<ConvexSpace>> Basis,
    class... Ts
  >
  class coordinates;

  /** @ingroup Coordinates
      @brief Alias for coordinates of a point in an affine space with respect to a particular origin.

      The basis belongs to the associated vector space, allowing the coordinates type for the affine
      space to be aware of the type of the coordinate representation for displacements
   */
  template<affine_space AffineSpace, basis_for<free_module_type_of_t<AffineSpace>> Basis, class Origin>
  using affine_coordinates = coordinates<AffineSpace, Basis, Origin>;

  /** @ingroup Coordinates
      @brief Alias for coordinates of an element of a vector space with respect to a particular basis.
   */
  template<vector_space VectorSpace, basis_for<free_module_type_of_t<VectorSpace>> Basis>
  using vector_coordinates = coordinates<VectorSpace, Basis>;

  /** @ingroup Coordinates
      @brief Alias for coordinates of an element of a free module with respect to a particular basis.
   */
  template<free_module FreeModule, basis_for<free_module_type_of_t<FreeModule>> Basis>
  using free_module_coordinates = coordinates<FreeModule, Basis>;
  
  /** @ingroup Coordinates
      @brief Class designed for inheritance by concerete coordinate types.

      The type has protected special member functions (including the destructor) and uses
      deducing-this patterns as a type-rich alternative to virtual dispatch.

      From the perspective of the enclosing namespace, maths, there is actually no need
      for a base class. Indeed, there is a single coordinates class template which derives
      from coordinates_base, begging the question as to why a base class is necessary at all.
      The reason is that there are applications in physics which have enough in common
      with maths::coordinates, but are sufficiently distinct, for a base class to be extremely
      useful in terms of reducing what would otherwise be very significant code duplication.

      One of the novelties in the context of physics is the notion of units and quantities
      of different types that can nevertheless by multipled and in some cases (like widths
      and heights) added.
   */

  template<
    convex_space ConvexSpace,
    basis_for<free_module_type_of_t<ConvexSpace>> Basis,
    validator_for<ConvexSpace> Validator,
    class DisplacementCoordinates=free_module_coordinates<free_module_type_of_t<ConvexSpace>, Basis>
  >
  class coordinates_base
  {
  public:
    using space_type                    = ConvexSpace;   
    using basis_type                    = Basis;
    using validator_type                = Validator;
    using set_type                      = ConvexSpace::set_type;
    using free_module_type              = free_module_type_of_t<ConvexSpace>;
    using commutative_ring_type         = commutative_ring_type_of_t<ConvexSpace>;
    using value_type                    = commutative_ring_type;
    using displacement_coordinates_type = DisplacementCoordinates;

    constexpr static bool has_distinguished_origin{has_distinguished_origin_v<ConvexSpace>};
    constexpr static bool has_identity_validator{is_identity_validator_v<Validator>};
    constexpr static bool has_freely_mutable_components{has_identity_validator && has_distinguished_origin};
    constexpr static std::size_t dimension{free_module_type::dimension};
    constexpr static std::size_t D{dimension};

    constexpr coordinates_base() noexcept = default;

    constexpr explicit coordinates_base(std::span<const value_type, D> vals) noexcept(has_identity_validator)
      : m_Values{validate(vals, m_Validator)}
    {}

    template<class... Ts>
      requires (D > 1) && (std::convertible_to<Ts, value_type> && ...)
    constexpr explicit(sizeof...(Ts) == 1) coordinates_base(Ts... ts) noexcept(has_identity_validator)
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

    template<class Self>
    [[nodiscard]]
    constexpr Self operator+(this const Self& self) noexcept
    {
      return self;
    }

    template<class Self>
      requires    std::constructible_from<Self, std::span<const value_type, D>>
               && has_distinguished_origin
               && (!std::is_unsigned_v<value_type>)
    [[nodiscard]]
    constexpr Self operator-(this const Self& self) noexcept(has_identity_validator)
    {
      return Self{utilities::to_array(self.values(), [](value_type t) { return -t; })};
    }

    template<class Derived>
      requires std::is_base_of_v<coordinates_base, Derived>
            && (!std::is_same_v<Derived, displacement_coordinates_type>)
            && std::constructible_from<typename Derived::displacement_coordinates_type, std::span<const value_type, D>>
    [[nodiscard]]
    friend constexpr typename Derived::displacement_coordinates_type operator-(const Derived& lhs, const Derived& rhs) noexcept(has_identity_validator)
    {
      return[&] <std::size_t... Is>(std::index_sequence<Is...>) {
        return typename Derived::displacement_coordinates_type{(lhs.values()[Is] - rhs.values()[Is])...};
      }(std::make_index_sequence<D>{});
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
    constexpr std::span<value_type, D> values() noexcept requires has_freely_mutable_components { return m_Values; }

    [[nodiscard]]
    constexpr const value_type& value() const noexcept requires (D == 1) { return m_Values[0]; }

    [[nodiscard]]
    constexpr value_type& value() noexcept requires (D == 1) && has_freely_mutable_components { return m_Values[0]; }

    /// This is explicit since otherwise, given two vectors a,b, a/b is well-formed due to implicit boolean conversion
    [[nodiscard]]
    constexpr explicit operator bool() const noexcept requires (D == 1) && std::convertible_to<value_type, bool>
    {
      return m_Values[0];
    }

    [[nodiscard]]
    constexpr value_type operator[](std::size_t i) const { return m_Values[i]; }

    [[nodiscard]]
    constexpr value_type& operator[](std::size_t i) requires has_freely_mutable_components { return m_Values[i]; }

    [[nodiscard]]
    constexpr auto begin() const noexcept { return m_Values.begin(); }

    [[nodiscard]]
    constexpr auto end() const noexcept { return m_Values.end(); }

    [[nodiscard]]
    constexpr auto rbegin() const noexcept { return m_Values.rbegin(); }

    [[nodiscard]]
    constexpr auto rend() const noexcept { return m_Values.rend(); }

    [[nodiscard]]
    constexpr auto cbegin() const noexcept { return begin(); }

    [[nodiscard]]
    constexpr auto cend() const noexcept { return end(); }

    [[nodiscard]]
    constexpr auto crbegin() const noexcept { return rbegin(); }

    [[nodiscard]]
    constexpr auto crend() const noexcept { return rend(); }

    [[nodiscard]]
    constexpr auto begin() noexcept requires has_freely_mutable_components { return m_Values.begin(); }

    [[nodiscard]]
    constexpr auto end() noexcept requires has_freely_mutable_components { return m_Values.end(); }

    [[nodiscard]]
    constexpr auto rbegin() noexcept requires has_freely_mutable_components { return m_Values.rbegin(); }

    [[nodiscard]]
    constexpr auto rend() noexcept requires has_freely_mutable_components { return m_Values.rend(); }


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
        std::ranges::for_each(std::views::zip(self.m_Values, rhs), [&f](auto&& z){ f(std::get<0>(z), std::get<1>(z)); });
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

  /** @ingroup Coordinates
      @brief Class template for representing coordinates on vector spaces, affine spaces and various generalizations.
   */
  
  template<convex_space ConvexSpace, basis_for<free_module_type_of_t<ConvexSpace>> Basis, class Origin, validator_for<ConvexSpace> Validator>
  class coordinates<ConvexSpace, Basis, Origin, Validator> final : public coordinates_base<ConvexSpace, Basis, Validator>
  {
  public:
    using origin_type = Origin;

    using coordinates_base<ConvexSpace, Basis, Validator>::coordinates_base;
  };

  template<convex_space ConvexSpace, basis_for<free_module_type_of_t<ConvexSpace>> Basis, validator_for<ConvexSpace> Validator>
    requires has_distinguished_origin_v<ConvexSpace>
  class coordinates<ConvexSpace, Basis, Validator>  final : public coordinates_base<ConvexSpace, Basis, Validator>
  {
  public:
    using coordinates_base<ConvexSpace, Basis, Validator>::coordinates_base;
  };

  template<affine_space AffineSpace, basis_for<free_module_type_of_t<AffineSpace>> Basis, class Origin>
    requires (!free_module<AffineSpace>)
  class coordinates<AffineSpace, Basis, Origin>  final : public coordinates_base<AffineSpace, Basis, std::identity>
  {
  public:
    using origin_type = Origin;
    
    using coordinates_base<AffineSpace, Basis, std::identity>::coordinates_base;
  };

  template<free_module M, basis_for<free_module_type_of_t<M>> Basis>    
  class coordinates<M, Basis>  final : public coordinates_base<M, Basis, std::identity>
  {
  public:
    using coordinates_base<M, Basis, std::identity>::coordinates_base;
  };
  
  namespace sets
  {
    /** @ingroup Sets
        @brief Class template for giving a name to the set of integers and its generalization to other dimensionalities
     */
    template<std::size_t D>
    struct Z
    {
      constexpr static std::size_t dimension{D};
    };

    /** @ingroup Sets
        @brief Class template for giving a name to the set of semi-positive integers and its generalization to other dimensionalities
     */
    template<std::size_t D>
    struct N_0
    {
      constexpr static std::size_t dimension{D};
    };

    /** @ingroup Sets
        @brief Class template for giving a name to the set of real numbers and its generalization to other dimensionalities
     */
    template<std::size_t D>
    struct R
    {
      constexpr static std::size_t dimension{D};
    };


    /** @ingroup Sets
        @brief Class template for giving a name to the set of non-negative real numbers and its generalization to other dimensionalities
     */
    template<std::size_t D>
    struct orthant
    {
      constexpr static std::size_t dimension{D};
    };

    /** @ingroup Sets
        @brief Class template for giving a name to the set of complex numbers and its generalization to other dimensionalities
     */
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

  struct mathematical_arena {};

  template<std::floating_point T, std::size_t D, class Arena=mathematical_arena>
  struct euclidean_vector_space
  {
    using set_type        = sets::R<D>;
    using field_type      = T;
    using is_vector_space = std::true_type;
    using arena_type      = Arena;
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

  template<std::floating_point T, std::size_t D, class Arena=mathematical_arena>
  struct euclidean_affine_space
  {
    using set_type          = sets::R<D>;
    using vector_space_type = euclidean_vector_space<T, D, Arena>;
    using is_affine_space   = std::true_type;
    using arena_type        = Arena;
  };

  template<std::floating_point T, class Arena=mathematical_arena>
  struct euclidean_half_space
  {
    using set_type             = sets::orthant<1>;
    using vector_space_type    = euclidean_vector_space<T, 1>;
    using is_convex_space      = std::true_type;
    using arena_type           = Arena;
    using distinguished_origin = std::true_type;
  };

  template<std::floating_point T, std::size_t D, basis Basis, class Origin, class Arena=mathematical_arena>
  using euclidean_affine_coordinates = affine_coordinates<euclidean_affine_space<T, D, Arena>, Basis, Origin>;

  template<std::floating_point T, std::size_t D, basis Basis, class Arena=mathematical_arena>
  using euclidean_vector_coordinates = vector_coordinates<euclidean_vector_space<T, D, Arena>, Basis>;

  /** @brief Right-handed bases for arbitrary D, build recursively from 1D

      In 1D, x is taken to run from left to right. Therefore, in 2D, y must go up
      and, building on this, in 3D z comes out from the page.
   */
  template<free_module M>
  struct canonical_right_handed_basis
  {
    using is_basis         = std::true_type;
    using free_module_type = M;
  };

  template<free_module M>
  struct dual_of<canonical_right_handed_basis<M>>
  {
    using type = canonical_right_handed_basis<dual_of_t<M>>;
  };

  template<free_module M>
  struct dual_of<canonical_right_handed_basis<dual<M>>>
  {
    using type = canonical_right_handed_basis<M>;
  };   

  template<std::floating_point T, std::size_t D, class Arena=mathematical_arena>
  using vec_coords = euclidean_vector_coordinates<T, D, canonical_right_handed_basis<euclidean_vector_space<T, D, Arena>>, Arena>;
}
