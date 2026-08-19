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

    Consider the real numbers. In C++ we can give a name to the set
    by using the type system.

    @code
    struct reals {};
    @endcode

    But what of the elements of `reals`? Here we run into an immediate difficulty. We
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
    space. Let the basis elements be denoted \f$ b_0, \ldots, b_{d-1} \f$. Any vector in the space
    may be written as a linear combination:

    \f[
      v = a_0 b_0 + \ldots + a_{d-1} b_{d-1},
    \f]

    where the \f$ a_i \f$ are valued in the field, F. The set of these values \f$ [a_0, \ldots, a_{d-1}] \f$
    are none other than the coordinates of v with respect to this particular basis.
    The \f$ [a_0, \ldots, a_{d-1}] \f$ are often informally referred to as a vector. However
    strictly speaking this risks an abuse of terminology since it may conflate two distinct concepts:
    an actual vector which is an element of the set which forms the vector space,
    and a representation of this vector via the coordinates with respect to a
    particular basis. This distinction can be further reinforced by pointing out that
    two observers who agree they are talking about the same vector (i.e. set element)
    will nevertheless disagree on the coordinates if they are using different bases -
    as they are entirely entitled to do! To further add to the confusion, the coordinates
    may be referred to as a coordinate vector which is perhaps unfortunate. (To add to
    the fun, in the case of \f$ \bb{R}^d \f$, a tuple of values \f$ (v(0), \ldots, v(d-1)) \f$ can have a
    different interpretation, which we shall gloss over for now but return to in \ref Basis "Basis".)

    Regardless, from the perspective of performing actual calculations, the coordinates
    are key. A crucial point to make is that, when dealing with the coordinates,
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
    Generally we will speak of e.g. the doubles weakly representing the reals.

    Vector spaces are just one of the things treated in the code that follows. There
    are several important generalizations. First, there are affine spaces, which comprise
    a set, A, together with a vector space, V, whose additive group acts freely and
    transitively on A. Intuitively, we can start at any point in A and translate to
    any other point by adding the appropriate vector. In fact, the relationship is
    stronger than this: choosing any point in A and adding any vector in V will give
    a point in A. A nice example of an affine space is Euclidean space. Two observers
    in this space, Alice and Bob, are entirely entitled to define their location as the
    origin. Neither is more right than the other since this space has no distinguished origin.
    Alice and Bob will, in general, disagree about the coordinates of points in the space.
    However, they will agree on the vector which translates from one point to another (though
    if they compare vector coordinates, they may have to contend with using different bases
    on the vector space!).

    An affine space is sometimes described as a vector space which has forgotten its origin.
    Indeed, a vector space is an affine space over itself. This is interesting in terms
    of representing these concepts in C++. Since a vector space is a special case of
    an affine space, this suggests that an affine space concept is more fundamental,
    with the vector space being a refinement. However, a vector space is part of the
    definition of an affine space (a set and a vector space, satisfying certain conditions)
    and so it is this that will be reflected by the concepts defined below: the affine
    space concept depends on the vector_space concept, and not vice-versa.

    It will be useful for our purposes to generalize affine spaces. To start, consider taking a
    convex subset, C, of an affine space. We may translate from any point in C to any
    other by adding the appropriate vector from V. However, there are elements of V
    which, when added to a point in C will take us outside of C and into the broader
    affine space of which it is a part. However, we do not want to define convex spaces
    via an embedding in a bigger space. There are a variety of solutions to this, a
    selection of which is listed, all of which have reasonable representations in C++.

    -# Take the action of V to be a partial action; as such, it is simply not defined
       for elements of V which would take points of C outside of C. While we cannot literally
       restrict the domain of a C++ function such as operator+ in this way, we can furnish
       them with a precondition. The behaviour when called out of contract is undefined; not
       quite in the mathematical sense but at least in a sense which seems to map rather well
       onto our intuition.
    -# Supplement the underlying set, S, with an exceptional state, E such that
       -# The difference of any two points in S is in V
       -# An element of V, when added to S remains either in S or maps to E.
          Note that, since every point in S is mapped by elements of V into E, and there is
          no mapping from E back into S, the action of the additive group of V is not bijective,
          violating one of the axioms of affine spaces.
    -# Define operations which would otherwise be out of bounds to clamp to the boundary. There
       are two subsidiary options:
       -# Projective clamping, whereby the nearest point to the putative point outside of the
          space is selected. (Notions of nearness require additional structure to exist on the space.)
       -# Ray-tracing, whereby the point on the boundary struck by the ray from the starting
          point along the displacement vector is selected.
    -# Periodic remapping
    -# Anti-periodic remapping

    There are now two further generalizations that it will be profitable to explore. The first
    is to consider relaxing a vector space's field
    to a ring. The resulting construction is called a module, which is a generalization
    of a vector space. Our motivation for this is that the integers form a commutative
    ring and not a field, since integers do not, in general, have multiplicative inverses
    valued within the integers. Rather than attempting to deal with modules in full generality,
    we restrict our attention to what may be the most useful, practical cases in the context
    of C++: free modules over commutative rings. Free modules are those which admit a
    basis.

    In line with the above, we also consider affine spaces over free modules and their
    convex generalization where the action of the free module is not bijective.

    The last generalization is to relax the constraint of convexity. This leads us to our
    most primitive abstraction: a partial M-torsor. Partial because the torsor may not
    be complete or may require additional structure to be completed. 'M' to indicate that
    the partial torsor is over a free module.

    The final introductory issue to address is the question of why to bother modelling concepts such
    as vector spaces in the abstract sense if it is their coordinates which are the things
    of use from the perspective of practical computation. The point is that, for example,
    vector spaces and affine spaces admit different operations: whereas elements of a vector
    space can be added, the same is not true of the elements of the set underpinning an
    affine space. By introducing concepts for the abstract algebraic constructs, we can treat
    coordinates on all of these spaces in a common way by using constraints to enable
    or disable specific operations. Thus, the coordinates class template is templated on,
    amongst other things, a partial_m_torsor. To define such a space just requires introducing
    a struct exposing a small amount of data (types and values) known at compile time.
    These data determine whether we intend to model a vector space, a free module over a
    commutative ring, an affine space or whatever. This is sufficient for the coordinates
    implementation to expose the correct set of operations.
 */


#include "sequoia/Core/ContainerUtilities/ArrayUtilities.hpp"
#include "sequoia/Core/Meta/Concepts.hpp"
#include "sequoia/Core/Meta/TypeAlgorithms.hpp"
#include "sequoia/Maths/Algebra/Ratio.hpp"
#include "sequoia/Maths/Arithmetic/SaturatingArithmetic.hpp"
#include "sequoia/PlatformSpecific/Preprocessor.hpp"

#include <algorithm>
#include <cmath>
#include <concepts>
#include <complex>
#include <format>
#include <numbers>
#include <ranges>
#include <span>

namespace sequoia::maths
{
  /** @defgroup MathematicalStructure Structure
      @brief Traits to indicate whether types self-identify as various algebraic structure.

      As outlined in the introductory remarks, there is a crucial distinction between
      using a type to name a set e.g. `struct reals {};` and a representation of
      the elements of the set e.g. `float` or `double`. In the case of the former,
      it is natural to use compile time data to express various abstract properties.
      This set of definitions pertains to how such a type identifies itself; for example,
      perhaps as a field or a commutative ring.

      To model the fact that algebraic properties relate to one another according to a DAG,
      we use virtual inheritance. This allows us to represent diamond hierarchies, which
      naturally occur e.g.

      @verbatim
         commutative_ring
            /         \
      ordered_ring  field
            \         /
          ordered_field
      @endverbatim
   */

  /** @defgroup CommutativeRingTags Subgroup commutative ring tag hierarchy
      @ingroup MathematicalStructure
      @brief Hierarchy for the purpose of self-identification as a commutative ring.

      @{
   */
  struct commutative_ring_tag_t {};

  struct field_tag_t : virtual commutative_ring_tag_t {};

  /** @} */

  /** @defgroup HasStructureType Subgroup Has structure type
      @ingroup MathematicalStructure
      @brief Compile time tools for reflecting on whether a type has a nested type called `structure`, and extracting it.

      @{
   */

  template<class T>
  inline constexpr bool has_structure_type_v{
    requires { typename T::structure; }
  };

  template<class T>
  struct structure_of {};

  template<class T>
  using structure_of_t = structure_of<T>::type;

  template<class T>
    requires has_structure_type_v<T>
  struct structure_of<T>
  {
    using type = T::structure;
  };

  /** @} */

  /** @defgroup HasSetType Subgroup Has set type
      @ingroup MathematicalStructure
      @brief Compile time tools for reflecting on whether a type has a nested type called `set_type`, and extracting it.

      @{
  */

  template<class T>
  inline constexpr bool has_set_type_v{
    requires { typename T::set_type; }
  };

  template<class T>
  struct set_type_of {};

  template<class T>
  using set_type_of_t = set_type_of<T>::type;

  template<class T>
    requires has_set_type_v<T>
  struct set_type_of<T>
  {
    using type = T::set_type;
  };

  /** @} */

  /** @defgroup CommutativeRingIdentification Subgroup Commutative Ring identification
      @ingroup MathematicalStructure
      @brief Captures the conditions under which types consider themselves to be a commutative ring or refinement thereof.

      @{
   */

  template<class T>
  inline constexpr bool identifies_as_commutative_ring_v{
       has_structure_type_v<T>
    && requires {
         requires std::derived_from<typename T::structure, commutative_ring_tag_t>;
       }
  };

  template<class T>
  inline constexpr bool identifies_as_field_v{
        has_structure_type_v<T>
    && requires {
         requires std::derived_from<typename T::structure, field_tag_t>;
       }
  };

  /** @} */

  /** @defgroup CommutativeRing Subgroup Commutative Ring
      @ingroup MathematicalStructure
      @brief Concepts for commutative ring and refinements thereof.

      @{
   */

  template<class T>
  concept commutative_ring = has_set_type_v<T> && identifies_as_commutative_ring_v<T>;

  template<class T>
  concept field = commutative_ring<T> && identifies_as_field_v<T>;

  /** @} */

  /** @defgroup TorsorTags Subgroup partial M-torsor tag hierarchy
      @ingroup MathematicalStructure
      @brief Hierarchy for the purpose of self-identification as a partial M-torsor, or a refinement thereof.

      @{
   */

  struct partial_m_torsor_tag_t {};

  struct convex_space_tag_t : virtual partial_m_torsor_tag_t {};

  struct free_module_tag_t : virtual partial_m_torsor_tag_t {};

  struct affine_space_tag_t : virtual convex_space_tag_t {};

  struct vector_space_tag_t : virtual affine_space_tag_t, virtual free_module_tag_t {};

  /** @} */

  /** @defgroup PartialMTorsorIdentification Partial M-torsor identification
      @ingroup MathematicalStructure
      @brief Captures the conditions under which types consider themselves to be a partial M-torsor or refinement thereof.

      @{
   */

  template<class T>
  inline constexpr bool identifies_as_partial_m_torsor_v{
       has_structure_type_v<T>
    && requires {
         requires std::derived_from<typename T::structure, partial_m_torsor_tag_t>;
       }
  };

  template<class T>
  inline constexpr bool identifies_as_convex_space_v{
       has_structure_type_v<T>
    && requires {
         requires std::derived_from<typename T::structure, convex_space_tag_t>;
       }
  };

  template<class T>
  inline constexpr bool identifies_as_affine_space_v{
       has_structure_type_v<T>
    && requires {
         requires std::derived_from<typename T::structure, affine_space_tag_t>;
       }
  };

  template<class T>
  inline constexpr bool identifies_as_free_module_v{
       has_structure_type_v<T>
    && requires {
         requires std::derived_from<typename T::structure, free_module_tag_t>;
       }
  };

  template<class T>
  inline constexpr bool identifies_as_vector_space_v{
       has_structure_type_v<T>
    && requires {
         requires std::derived_from<typename T::structure, vector_space_tag_t>;
       }
  };

  /** @} */


  /** @defgroup PropertiesOfSpaces Properties of Spaces
      @brief Tools to reflect on whether types expose other types typically associated with various spaces.
   */

  /** @defgroup HasCommutativeRing Subgroup Has Commutative Ring
      @ingroup PropertiesOfSpaces
      @brief Compile time constants reflecting whether a nested type named commutative_ring_type, or a refinement thereof, exists.

      @{
   */

  template<class T>
  inline constexpr bool has_commutative_ring_type_v{
    requires { typename T::commutative_ring_type; }
  };

  template<class T>
  inline constexpr bool has_field_type_v{
    requires { typename T::field_type; }
  };

  /** @} */

  /** @defgroup NestedCommutativeRingType Subgroup Nested Commutative Ring Type
      @ingroup PropertiesOfSpaces
      @brief Extracts a nested type named commutative_ring_type, or a refinement thereof, if it exists.

      @{
   */

  template<class T>
  struct nested_commutative_ring_type {};

  template<class T>
  using nested_commutative_ring_type_t = nested_commutative_ring_type<T>::type;

  template<class T>
    requires has_commutative_ring_type_v<T>
  struct nested_commutative_ring_type<T>
  {
    using type = T::commutative_ring_type;
  };

  template<class T>
    requires has_field_type_v<T>
  struct nested_commutative_ring_type<T>
  {
    using type = T::field_type;
  };

  /** @} */

  /** @defgroup DefinesCommutativeRing Subgroup Defines Commutative Ring
      @ingroup PropertiesOfSpaces
      @brief Compile time constants reflecting whether an appropriately named nested type
      exists satisfying the commutative ring concept, or a refinement thereof.

      @{
   */

  template<class T>
  inline constexpr bool defines_commutative_ring_v{
    requires { requires commutative_ring<nested_commutative_ring_type_t<T>>; }
  };

  template<class T>
  inline constexpr bool defines_field_v{
    requires { requires field<nested_commutative_ring_type_t<T>>; }
  };

  /** @} */

  /** @defgroup HasRank Subgroup Has Rank
      @ingroup PropertiesOfSpaces
      @brief Compile time constants reflecting whether a nested value convertible
      to a std::size_t exists, named rank or a refinement thereof.

      Whereas one speaks of the dimension of a vector space, for free modules the
      term rank is generally preferred. As such, we cater for the appearance of
      both terms.

      @{
   */

  template<class T>
  inline constexpr bool has_rank_v{
    requires { { T::rank } -> std::convertible_to<std::size_t>; }
  };

  template<class T>
  inline constexpr bool has_dimension_v{
    requires { { T::dimension } -> std::convertible_to<std::size_t>; }
  };

  /** @} */

  /** @defgroup RankOf Subgroup Rank Of
      @ingroup PropertiesOfSpaces
      @brief Utilities for extracting the rank of a type, be it named rank or a refinement thereof.

      @{
   */

  template<class T>
  struct rank_of {};

  template<class T>
  inline constexpr std::size_t rank_of_v{rank_of<T>::value};

  template<class T>
    requires has_rank_v<T>
  struct rank_of<T>
  {
    constexpr static std::size_t value{T::rank};
  };

  template<class T>
    requires has_dimension_v<T>
  struct rank_of<T>
  {
    constexpr static std::size_t value{T::dimension};
  };

  template<class T>
  inline constexpr bool defines_rank_v{
    requires { { rank_of<T>::value } -> std::convertible_to<std::size_t>; }
  };

  /** @} */

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
  concept free_module = has_set_type_v<T> && defines_rank_v<T> && defines_commutative_ring_v<T> && identifies_as_free_module_v<T>;

  /** @ingroup Spaces
      @brief concept for a vector space, which is a special case of a free module
   */
  template<class T>
  concept vector_space = free_module<T> && defines_field_v<T>;

  /** @defgroup HasFreeModuleType Subgroup Has Free Module Type
      @ingroup PropertiesOfSpaces
      @brief Compile time constants reflecting whether a nested type named
      `free_module_type`, or a refinement thereof, exists.

      @{
   */

  template<class T>
  inline constexpr bool has_free_module_type_v{
    requires { typename T::free_module_type; }
  };

  template<class T>
  inline constexpr bool has_vector_space_type_v{
    requires { typename T::vector_space_type; }
  };

  /** @} */

  /** @defgroup NestedFreeModuleType Subgroup Nested Free Module Type
      @ingroup PropertiesOfSpaces
      @brief Extracts a nested type named free_module_type, or a refinement thereof, if it exists.

      @{
   */

  template<class T>
  struct nested_free_module_type {};

  template<class T>
  using nested_free_module_type_t = nested_free_module_type<T>::type;

  template<class T>
    requires has_free_module_type_v<T>
  struct nested_free_module_type<T>
  {
    using type = T::free_module_type;
  };

  template<class T>
    requires has_vector_space_type_v<T>
  struct nested_free_module_type<T>
  {
    using type = T::vector_space_type;
  };

  /** @} */

  /** @defgroup DefinesFreeModule Subgroup Defines Free Module
      @ingroup PropertiesOfSpaces
      @brief Compile time constants reflecting whether an appropriately named nested type
      exists satisfying the free module concept, or a refinement thereof.

      @{
   */

  template<class T>
  inline constexpr bool defines_free_module_v{
    requires { requires free_module<nested_free_module_type_t<T>>; }
  };

  template<class T>
  inline constexpr bool defines_vector_space_v{
    requires { requires vector_space<nested_free_module_type_t<T>>; }
  };

  /** @} */

  /** @ingroup Spaces
      @brief concept for convex spaces

      A convex space may be a free module. Otherwise, it comprises a set and a
      free module (which may be a vector space), and must identify as a convex
      space.
   */
  template<class T>
  concept convex_space
    =  free_module<T> || (has_set_type_v<T> && identifies_as_convex_space_v<T> && defines_free_module_v<T>);

  /** @ingroup Spaces
      @brief concept for affine spaces

      A vector space is an affine space over itself; beyond that, according to our
      definitions, an affine space is a refinement of a convex space.
   */
  template<class T>
  concept affine_space = vector_space<T> || (convex_space<T> && identifies_as_affine_space_v<T>);

  /** @defgroup FreeModuleTypeOf Subgroup Free module type of
      @ingroup PropertiesOfSpaces
      @brief Extracts the free module type associated with a convex space.

      This takes into account that if the convex space is a free module, then the
      associated free module type is just the space itself.

      @{
   */
  template<class>
  struct free_module_type_of {};

  template<class T>
  using free_module_type_of_t = free_module_type_of<T>::type;

  template<free_module ConvexSpace>
  struct free_module_type_of<ConvexSpace>
  {
    using type = ConvexSpace;
  };

  template<class T>
    requires defines_free_module_v<T>
  struct free_module_type_of<T>
  {
    using type = nested_free_module_type_t<T>;
  };

  /** @} */

  /** @ingroup PropertiesOfSpaces
      @brief Extracts the commutative ring type of the free module associated with a convex space.

      This takes into account that if the free module is a vector space, then the commutative ring is actually a field.

      @{
   */
  template<convex_space ConvexSpace>
  struct commutative_ring_type_of
  {
    using type = nested_commutative_ring_type_t<free_module_type_of_t<ConvexSpace>>;
  };

  template<convex_space ConvexSpace>
  using commutative_ring_type_of_t = commutative_ring_type_of<ConvexSpace>::type;

  /** @} */


  /** @ingroup PropertiesOfSpaces
      @brief Extracts the dimension of the free module associated with a convex space.

      The program is ill-formed if the space defines its own dimension (rank) that
      is inconsistent with the free module's dimension.
   */
  template<convex_space ConvexSpace>
  inline constexpr std::size_t dimension_of_v{
    [](){
      constexpr std::size_t rank{rank_of_v<free_module_type_of_t<ConvexSpace>>};

      if constexpr(defines_rank_v<ConvexSpace>)
      {
        static_assert(rank_of_v<ConvexSpace> == rank);
      }

      return rank;
    }()
  };

  /** @defgroup Basis Basis
      @brief Concepts and helpers for bases of free modules.

      By definition, free modules admit a basis and the latter are an
      essential ingredient in our approach. The introduction describes
      the primary considerations; here we focus on the nuances. A type
      is considered to be a basis if it satisfies several conditions. The
      first two are simple to state:
      -# The type self-identifies as a basis, by appropriately exposing a type;
      -# The type defines a type satisfying the free module concept (this is
         the free module to which the basis applies).

      To understand the remaining conditions requires a detailed understanding of
      various subtleties. Consider first the case of \f$ \bb{R}^d \f$, understood to be a vector space.
      (This latter statement is to avoid the ambiguity whereby, by \f$ \bb{R}^d \f$, we could just
      mean the set, without any additional structure.) Really, \f$ \bb{R}^d \f$ is a shorthand
      for \f$ \bb{R}^S \f$, understood as follows (assuming finite S):
      -# S is an index set, an example of which would be \f$ \{0, \ldots, d-1\} \f$
      -# \f$ \bb{R}^S \f$ is the set of all functions from \f$ S \to \bb{R} \f$

      Though seemingly very abstract, this maps nicely onto C++ intuition:
      -# Take an element of \f$ \bb{R}^S \f$, v - a vector
      -# Take an element of S, i - an index
      -# Consider the function v(i) (from a C++ perspective v[i] may be even more natural):
         this returns the ith coordinate, which in this example is just a real number.

      As a concrete example in d=2, suppose we take v = (0.5, -0.5). Then v(0) = 0.5, v(1) = -0.5.
      But thinking about this more carefully seems to suggest a paradox.
      We claimed in the introduction that the components of a vector must be written with
      respect to a basis. And yet here we haven't introduced a basis, just a function which
      in no way requires a basis for its definition. As is so often the case, much of the
      resolution of the apparent paradox is notational. In this context what we have done is the following:
      -# Taken the mathematical definition of a d-tuple to be a function on \f$ \{0, \ldots, d-1\} \f$.
      -# Taken the notation for a 2-tuple to be (x, y)

      This is a perfectly reasonable thing to do. The problem, suggestive of a paradox,
      is that we may also quite reasonably take (0.5, -0.5) to mean the coordinates of a
      vector with respect to some basis which is implicitly taken to be part of the context
      of the discussion. Henceforth, to be completely clear, we will only ever understand
      (x, y, z) in this latter sense. Should we need to express the other case, we will
      be completely explicit:

      \f[
        v : \{0, 1\} \to \bb{R}, \quad v(0) = 0.5, \quad v(1) = -0.5.
      \f]

      To emphasise: this is completely independent of any basis. Nevertheless, the
      presentation of the space as \f$ \bb{R}^S \f$ canonically determines a distinguished basis. We
      simply run through the elements of the index set, defining the indicators to be
      the vectors whose components are all zeros except at the place of the index, where
      the component is 1.

      \f[
        e_s : \{0, \ldots, d-1\} \to \bb{R}, \quad e_{st} = \delta_{st}
      \f]

      Put differently, \f$ \bb{R}^S \f$ admits a canonical basis.

      With this established, we are now in a position to talk about changes of basis. This
      will be specified by an isomorphism, which in this case is technically an automorphism
      since the mapping is from a space to itself. This will be an element of \f$ GL(S) \f$.

      The final issue to discuss before moving on from \f$ \bb{R}^S \f$ is that of ordered versus
      unordered bases. Thus far we have been tacitly assuming that the index set has the
      structure \f$ \{0, \ldots, d-1\} \f$. This defines an ordering and this can be carried through to
      give an ordered basis \f$ \{e_0, \ldots, e_{d-1}\} \f$. But the index set may not carry an
      intrinsic ordering, for example \f$ \{\mathrm{foo}, \mathrm{bar}, \mathrm{baz}\} \f$. Of course, there is nothing
      to stop us from imposing an order, which is typically what we do in the case
      \f$ \{x, y, z\} \f$. But that is a choice and it would be equally valid (if perverse) to
      identify \f$ e_x \f$ with \f$ e_2 \f$.

      To summarize what we have discovered for \f$ \bb{R}^S \f$: a basis requires specification of

      - A. An index set;
      - B. An automorphism (which could be the identity).

      Noting that all our considerations carry over to free modules over a commutative ring,
      let us move to the more general case: an arbitrary free module, M, over a
      commutative ring, R, of rank d. In this case, since the most we can say about \f$ R^S \f$
      (where, as before, the cardinality of S is d) is that M is isomorphic to it, defining
      a basis requires an additional ingredient:

      - C. An isomorphism from \f$ R^S \f$ to M. This goes by a few different names, with
        basepoint being found in the torsor literature. However, we opt for the term preferred by
        differential geometry, frame.


      We are now in a position to state the requirements on a type that satisfies the basis concept.
      Such a type:
      -# Self-identifies as a basis, by appropriately exposing a type;
      -# Defines a type satisfying the free module concept (this is
         the free module to which the basis applies);
      -# Defines a type `index_set`, which must define d values (say an `index_sequence`,
         or an enumeration);
      -# Defines a type `frame`. This defines the isomorphism from M to \f$ R^S \f$,
         where R is the (commutative) ring associated with the (free) module. If M is
         \f$ R^S \f$, then the concept is only satisfied if the frame is the
         identity_isomorphism.
      -# Defines a type `automorphism`, understood to explicitly or implicitly name an element
         of \f$ GL(S) \f$.
   */

  struct identity_isomorphism {};

  // TO DO: make the transform a matrix, which can be done by taking the outer product of
  // two vector spaces
  template<
    free_module M,
    class IndexSet=std::index_sequence<M::dimension>,
    class Frame=identity_isomorphism,
    class BasisTransform=identity_isomorphism
  >
  struct general_basis
  {
    using is_basis         = std::true_type;
    using free_module_type = M;
    using index_set        = IndexSet;
    using frame            = Frame;
    using basis_transform  = BasisTransform;
  };

  /** @ingroup Basis
      @brief Compile time constant reflecting whether a nested type named 'is_basis' exist.
   */
  template<class T>
  inline constexpr bool has_is_basis_v{
    requires { typename T::is_basis; }
  };

  /** @ingroup Basis
      @brief Compile time constant reflecting whether a nested type self-identifies as a basis.
   */
  template<class T>
  inline constexpr bool identifies_as_basis_v{
       has_is_basis_v<T>
    && requires { requires std::convertible_to<typename T::is_basis, std::true_type>; }
  };

  /** @ingroup Basis
      @brief Compile time constant reflecting whether a nested type named `isomorphism_type` exists.
   */
  template<class T>
  inline constexpr bool has_isomorphism_type_v{
    requires { typename T::isomorphism_type; }
  };

  /** @ingroup Basis
      @brief Compile time constant reflecting whether a nested type named `admits_canonical_basis` exists.
   */
  template<class T>
  inline constexpr bool has_admits_canonical_basis_v{
    requires{ typename T::admits_canonical_basis; }
  };

  /** @ingroup Basis
      @brief Compile time constant reflecting whether a nested type admits a canonical basis.
   */
  template<free_module M>
  inline constexpr bool admits_canonical_basis_v{
       has_admits_canonical_basis_v<M>
    && requires { requires std::convertible_to<typename M::admits_canonical_basis, std::true_type>; }
  };

  /** @ingroup Basis
      @brief concept for a basis associated with a free module.
   */
  template<class B>
  concept basis = identifies_as_basis_v<B> && defines_free_module_v<B>
               && (admits_canonical_basis_v<free_module_type_of_t<B>> || has_isomorphism_type_v<B>);
  
  /** @ingroup Basis
      @brief A concept to determine if a basis is appropriate for a particular free module.
  */
  template<class B, class M>
  concept basis_for = basis<B> && free_module<M> && std::same_as<free_module_type_of_t<B>, M>;

  template<basis B>
  struct basis_isomorphism_type_of;
  
  template<basis B>
    requires admits_canonical_basis_v<free_module_type_of_t<B>>
  struct basis_isomorphism_type_of<B>
  {
    using type = identity_isomorphism;
  };

  template<basis B>
  using basis_isomorphism_type_of_t = basis_isomorphism_type_of<B>::type;

  template<basis B>
    requires (!admits_canonical_basis_v<free_module_type_of_t<B>>) && has_isomorphism_type_v<B>
  struct basis_isomorphism_type_of<B>
  {
    using type = B::isomorphism_type;
  };

  template<basis Basis1, basis Basis2>
  struct consistent_bases : std::false_type {};

  template<basis Basis1, basis Basis2>
  inline constexpr bool consistent_bases_v{consistent_bases<Basis1, Basis2>::value};

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
      @brief Compile time constant for addability of instances of two types.
   */

  template<class U, class T>
  inline constexpr bool is_addable_to_v{
    requires(const T& t, const U& u) {
      { t + u } -> std::convertible_to<T>; 
    }
  };

  /** @ingroup ArithmeticProperties
      @brief Compile time constant for addability of two instances of the same type.
   */

  template<class T>
  inline constexpr bool is_addable_v{
       is_addable_to_v<T, T>
    && requires(T& t) {
         { t += t } -> std::same_as<T&>;
       }
  };

  /** @ingroup ArithmeticProperties
      @brief Compile time constant for subtractability of instances of two types.
   */

  template<class U, class T>
  inline constexpr bool is_subtractable_from_v{
    requires(const T& t, const U& u) {
      { t - u } -> std::convertible_to<T>; 
    }
  };

  /** @ingroup ArithmeticProperties
      @brief Compile time constant for subtractability of two instances of the same type.
   */
  
  template<class T>
  inline constexpr bool is_subtractable_v{
       is_subtractable_from_v<T, T>
    && requires(T& t) {
         { t -= t } -> std::same_as<T&>;
       }
  };

  /** @ingroup ArithmeticProperties
      @brief Compile time constant for multiplicability.

      Note that there is not a two template paramter analogue of
      is_addable_to_v. The problem is that for any putative
      `is_multiplicable_by_v`, there doesn't seem to be any
      reasonable concept satisfied by the return value. (Multiplying
      an n * m matrix by an m * p matrix is one example of why this
      is problemtic.) We could just require that t * u exists, but
      without being able to say anything about the return value,
      this seems to be too weak a constraint. Besides which, we don't
      need it.
   */

  template<class T>
  inline constexpr bool is_multiplicable_v{
    requires(T& t) {
      { t *= t } -> std::same_as<T&>;
      { t * t }  -> std::convertible_to<T>;
    }
  };

  /** @ingroup ArithmeticProperties
      @brief Compile time constant for divisibility.
   */
  template<class T>
  inline constexpr bool is_divisible_v{
    requires(T& t) {
      { t /= t } -> std::same_as<T&>;
      { t / t }  -> std::convertible_to<T>;
    }
  };

  /** @defgroup AlgebraicTraits Algebraic Traits
      @brief Traits and concepts for types attempting to model algebraic types.

      This section seeks to provides compile time mechanisms to specify (and subsequently query)
      whether arithmetic types, be they built-in (int, float etc) or user-defined, exhibit various properties.
      A fundamental problem of attempting this is the difference
      between a mathematical structure and an approximate representation of that structure.
      This is sharpened in the current context of attempting to do this in code:     
      ints model the integers, but not exactly since there is a maximum representable value.
      Similarly, floating-point numbers model the reals but only in an approximate sense.
      However, it is useful to capture such 'best efforts', for which we use the signifer
      'weak'. For example to signify the fact that neither integer nor floating-point addition exactly models an
      abelian group, trait `weakly_abelian_group_under_addition` is used. Note, however, that addition of unsigned integral
      types does precisely model an abelian group and so 'weak' is a minimum requirement.
      
      Entertaingly, the only fundamental type in C++ which exacly models a field is bool.
   */

  /** @defgroup WeaklyAbelianGroupUnderAddition Subgroup weakly abelian group under addition
      @ingroup AlgebraicTraits
      @brief Trait for specifying whether a type behaves (appoximately) as an abelian group under addition.

      This includes all the arithmetic types, with the unsigned one behaving precisely as an abelian group
      under addition.

      @{
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

  /** @} */ // end of group WeaklyAbelianGroupUnderAddition

  /** @defgroup WeaklyAbelianGroupUnderMultiplication Subgroup weakly abelian group under multiplication
      @ingroup AlgebraicTraits
      @brief Trait for specifying whether a type behaves (appoximately) as an abelian group under multiplication.

      The floating-point types are taken to weakly model an abelian group under multiplication,
      since they attempt to approximate the reals.
      
      The only integral type modelling this (exactly, as it transpires) is bool. It is the only type in C++
      modelling Z/Zn where n is a prime. The other integral types do not model this in a reasonable sense.

      @{
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

  /** @} */ // end of group WeaklyAbelianGroupUnderMultiplication
  
  /** @defgroup MultiplicationWeaklyDistributiveOverAddition Subgroup multiplication weakly distributive over addition
      @ingroup AlgebraicTraits
      @brief Trait for specifying whether a type exhibits multiplication that (approximately) distributes over addition.

      Unlike the previous two traits, this one is taken to be true by default. Therefore, in the
      event of a user-defined type that doesn't satisfy this, they must opt out.

      @{
   */

  template<class T>
  struct multiplication_weakly_distributive_over_addition : std::true_type {};

  template<class T>
  using multiplication_weakly_distributive_over_addition_t = typename multiplication_weakly_distributive_over_addition<T>::type;

  template<class T>
  inline constexpr bool multiplication_weakly_distributive_over_addition_v{multiplication_weakly_distributive_over_addition<T>::value};

  /** @} */ // end of group MultiplicationWeaklyDistributiveOverAddition

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

  /** @defgroup Bounds Bounds

   */

  template<class Bounds>
  inline constexpr bool checks_single_val_against_bounds_v{
    requires (const Bounds& b) {
      { b(std::declval<typename Bounds::value_type>()) } -> std::convertible_to<bool>;
    }
  };

  template<class Bounds, std::size_t D>
  inline constexpr bool checks_array_against_bounds_v{
    requires (const Bounds& b) {
      { b(std::declval<std::array<typename Bounds::value_type, D>>()) } -> std::convertible_to<bool>;
    }
  };

  template<class Bounds>
  concept bounds
    =    has_value_type_v<Bounds>
      && requires (const Bounds& b) {
           { b.lower } -> std::convertible_to<typename Bounds::value_type>;
           { b.upper } -> std::convertible_to<typename Bounds::value_type>;
           // TO DO
         };

  template<auto Bounds>
  concept bounds_value = bounds<decltype(Bounds)> && (Bounds.lower < Bounds.upper);

  template<class Bounds, class ConvexSpace>
  concept bounds_for
    =      bounds<Bounds> && convex_space<ConvexSpace>
        && (   ((dimension_of_v<ConvexSpace> == 1) && checks_single_val_against_bounds_v<Bounds>)
            || ((dimension_of_v<ConvexSpace>  > 1) && checks_array_against_bounds_v<Bounds, dimension_of_v<ConvexSpace>>));

  template<bounds Bounds>
  struct bounds_value_type
  {
    using type = Bounds::value_type;
  };

  template<bounds Bounds>
  using bounds_value_type_t = bounds_value_type<Bounds>::type;

  template<bounds Bounds, std::size_t D>
  struct bounds_value_type<std::array<Bounds, D>> : bounds_value_type<Bounds>
  {
  };

  template<weak_commutative_ring T>
  struct to_bounds_value_type
  {
    using type = T;
  };

  template<weak_commutative_ring T>
  using to_bounds_value_type_t = to_bounds_value_type<T>::type;

  template<std::floating_point T>
  struct to_bounds_value_type<std::complex<T>>
  {
    using type = T;
  };

  template<arithmetic T>
  struct coordinate_bounds;

  template<arithmetic T>
  inline constexpr coordinate_bounds<T> no_bounds{least_lower_bound<T>, greatest_upper_bound<T>};

  template<arithmetic T>
  inline constexpr coordinate_bounds<T> half_line_bounds{T{}, greatest_upper_bound<T>};

  template<arithmetic T>
  inline constexpr coordinate_bounds<T> negative_half_line_bounds{least_lower_bound<T>, T{}};

  template<arithmetic T>
  struct coordinate_bounds
  {
    using value_type = T;

    T lower{}, upper{greatest_upper_bound<T>};

    template<arithmetic U>
      requires initializable_from<T, U>
    [[nodiscard]]
    constexpr bool operator()(U val) const noexcept
    {
      if constexpr(std::floating_point<U>)
      {
        if(std::isnan(val))
          return false;
      }
      
      if(lower > least_lower_bound<T>)
      {
        if(const U uLower{static_cast<U>(lower)}; val < uLower)
          return false;
      }

      if(upper < greatest_upper_bound<T>)
      {
        if(const U uUpper{static_cast<U>(upper)}; val > uUpper)
          return false;
      }

      return true;
    }

    template<arithmetic U, std::size_t D>
      requires initializable_from<T, U>
    [[nodiscard]]
    constexpr bool operator()(const std::array<U, D>& vals) const noexcept
    {
      auto v{std::views::transform(vals, [this](const U val) { return this->operator()(val); })};
      return !std::ranges::contains(v, false);
    }

    template<arithmetic U>
      requires initializable_from<T, U>
    [[nodiscard]]
    std::string format_input(const U val) const
    {
      return std::format("{}", val);
    }

    template<arithmetic U, std::size_t D>
      requires initializable_from<T, U>
    [[nodiscard]]
    std::string format_input(const std::array<U, D>& vals) const
    {
      return std::format("{} has at least one value", vals);
    }

    [[nodiscard]]
    friend constexpr bool operator==(const coordinate_bounds&, const coordinate_bounds&) noexcept = default;

    template<arithmetic U>
      requires has_saturating_arithmetic_v<T, U>
    [[nodiscard]]
    friend constexpr coordinate_bounds<std::common_type_t<T, U>> operator*(const coordinate_bounds<T>& a, const coordinate_bounds<U>& b)
    {
      using value_t = std::common_type_t<T, U>;
   
      if((a == no_bounds<T>) || (b == no_bounds<U>))
        return no_bounds<value_t>;
   
      const std::array
        products{
          saturating_mul(a.lower, b.lower),
          saturating_mul(a.lower, b.upper),
          saturating_mul(a.upper, b.lower),
          saturating_mul(a.upper, b.upper)
      };
   
      return {std::ranges::min(products), std::ranges::max(products)};
    }
  };
  
  template<std::floating_point T>
  [[nodiscard]]
  constexpr coordinate_bounds<T> reciprocal(const coordinate_bounds<T>& b) noexcept
  {
    if((b == no_bounds<T>) || (b == half_line_bounds<T>) || (b == negative_half_line_bounds<T>))
      return b;
      
    auto invert{
      [b](T val){
        constexpr auto llb{least_lower_bound<T>},
                       gub{greatest_upper_bound<T>};

        if((val == llb) || (val == gub))
          return T{};

        if(val)
          return T(1) / val;

        return b.upper ? gub : llb;
      }
    };

    if((b.lower >= 0) || (b.upper <= 0))
      return {invert(b.upper), invert(b.lower)};

    return no_bounds<T>;
  }
  
  
  template<class T>
  inline constexpr bool has_bounds_v{
    requires {
      T::bounds_v;
      requires bounds_value<T::bounds_v>;
     }
  };
  

  /** @defgroup Representation Representation
      @brief Representations allow coordinates to be represented using a bijective mapping with respect to an underlying basis.

   */

  // TO DO free mod value_type as well
  template<class R, class ConvexSpace>
  inline constexpr bool representation_for_single_value{
        (dimension_of_v<ConvexSpace> == 1)
     && requires(R& r, const typename R::value_type& val) {
          { r.to_underlying(val)   } -> std::convertible_to<decltype(val)>;
          { r.from_underlying(val) } -> std::convertible_to<decltype(val)>;
        }
  };

  template<class R, class ConvexSpace>
  inline constexpr bool representation_for_span{
     requires(R& r, std::span<const typename R::value_type, dimension_of_v<ConvexSpace>> vals) {
       { r.to_underlying(vals)   } -> std::convertible_to<std::array<typename R::value_type, dimension_of_v<ConvexSpace>>>;
       { r.from_underlying(vals) } -> std::convertible_to<std::array<typename R::value_type, dimension_of_v<ConvexSpace>>>;
     }
  };

  template<class T>
  inline constexpr bool has_free_module_representation_v{
    requires {
      typename T::free_module_representation;
    }
  };

  template<class T>
  inline constexpr bool has_coordinates_type_v {
    requires {
      typename T::coordinates_type;
      { std::tuple_size_v<typename T::coordinates_type> } -> std::convertible_to<std::size_t>;
    }
  };

  
  
  template<class Algebra, class Rep>
  struct weakly_representated_by : std::false_type {};

  template<class Algebra, class Rep>
  using weakly_representated_by_t = weakly_representated_by<Algebra, Rep>::type;

  template<class Algebra, class Rep>
  inline constexpr bool weakly_representated_by_v = weakly_representated_by<Algebra, Rep>::value;

  template<class Rep, class Algebra>
  concept weak_representation_for = weakly_representated_by_v<Algebra, Rep>;
  
  // TO DO constrain coordinates_type to hold things satisfying a coords concept?
  template<class R>
  concept representation = std::default_initializable<R>
                        && has_value_type_v<R>
                        && has_free_module_representation_v<R>
                        && (has_coordinates_type_v<R> || has_bounds_v<R>);

  template<class R, class ConvexSpace>
  concept representation_for
    =    convex_space<ConvexSpace>
      && representation<R>
    // TO DO not this, since the set could be anything and the rep applies to the coordintes
    // but maybe something along these lines
    // && weak_representation_for<value_type_of_t<R>, set_type_of_t<ConvexSpace>>
    // TO DO: this seems to massively slow down compilation  && bounds_for<decltype(R::bounds_v), ConvexSpace>
      && (representation_for_single_value<R, ConvexSpace> || representation_for_span<R, ConvexSpace>);

  template<weak_commutative_ring T>
  struct free_module_representation_value_type
  {
    using type = T;
  };

  template<weak_commutative_ring T>
  using free_module_representation_value_type_t = free_module_representation_value_type<T>::type;

  // TO DO: rename unsigned since it could be signed, now
  // Maybe rename
  template<class Unsigned, class Signed>
  concept covered_by =     std::integral<Unsigned>
                        && std::integral<Signed>
    /*&& std::is_unsigned_v<Unsigned>*/
                        && std::is_signed_v<Signed>
                        && (sizeof(Signed) == 2 * sizeof(Unsigned));
  
  template<class T>
  struct signed_covering_type;

  template<class T>
  using signed_covering_type_t = signed_covering_type<T>::type;
  
  template<covered_by<int> T>
  struct signed_covering_type<T>
  {
    using type = int;
  };

  template<covered_by<long> T>
    requires (sizeof(long) > sizeof(int))
  struct signed_covering_type<T>
  {
    using type = long;
  };

  template<covered_by<long long> T>
    requires (sizeof(long long) > sizeof(long))
  struct signed_covering_type<T>
  {
    using type = long long;
  };

  template<weak_commutative_ring T>
    requires std::integral<T> && std::is_unsigned_v<T>
  struct free_module_representation_value_type<T>
  {
    using type = signed_covering_type_t<T>;
  };

  template<representation Representation, class... Ts>
  // TO DO: constrain Ts to a coordinates concept
  inline constexpr bool consistent_representation_v{
    requires {
      typename Representation::coordinates_type;
      requires std::same_as<typename Representation::coordinates_type, std::tuple<Ts...>>;
    }
  };

  template<convex_space ConvexSpace, representation_for<ConvexSpace> R>
  inline constexpr bool defines_scalar_multiplication_for_v{
    requires(const R& r, std::span<const value_type_of_t<R>, dimension_of_v<ConvexSpace>> vals, value_type_of_t<R> s) {
      { r.mul(vals, s) } -> std::convertible_to<std::array<value_type_of_t<R>, dimension_of_v<ConvexSpace>>>;
    }
  };

  template<convex_space ConvexSpace, representation_for<ConvexSpace> R>
  inline constexpr bool defines_scalar_multiplication_for_single_value_v{
       (dimension_of_v<ConvexSpace> == 1)
       && requires(const R& r, value_type_of_t<R> val, value_type_of_t<R> s) {
         { r.mul(val, s) } -> std::convertible_to<value_type_of_t<R>>;
       }
  };

  template<convex_space ConvexSpace, representation_for<ConvexSpace> R>
  inline constexpr bool defines_scalar_division_for_v{
    requires(const R& r, std::span<const value_type_of_t<R>, dimension_of_v<ConvexSpace>> vals, value_type_of_t<R> s) {
      { r.div(vals, s) } -> std::convertible_to<std::array<value_type_of_t<R>, dimension_of_v<ConvexSpace>>>;
    }
  };

  template<convex_space ConvexSpace, representation_for<ConvexSpace> R>
  inline constexpr bool defines_scalar_division_for_single_value_v{
       (dimension_of_v<ConvexSpace> == 1)
    && requires(const R& r, value_type_of_t<R> val, value_type_of_t<R> s) {
         { r.div(val, s) } -> std::convertible_to<value_type_of_t<R>>;
       }
  };

  template<convex_space ConvexSpace, representation_for<ConvexSpace> R>
  inline constexpr bool defines_addition_for_v{
    requires(const R& r,
             std::span<const value_type_of_t<R>, dimension_of_v<ConvexSpace>> lhs,
             std::span<const value_type_of_t<R>, dimension_of_v<ConvexSpace>> rhs) {
      { r.add(lhs, rhs) } -> std::convertible_to<std::array<value_type_of_t<R>, dimension_of_v<ConvexSpace>>>;
    }
  };

  template<convex_space ConvexSpace, representation_for<ConvexSpace> R>
  inline constexpr bool defines_addition_for_single_value_v{
    (dimension_of_v<ConvexSpace> == 1)
    && requires(const R& r, value_type_of_t<R> lhs, value_type_of_t<R> rhs) {
        { r.add(lhs, rhs) } -> std::convertible_to<value_type_of_t<R>>;
      }
  };

  template<convex_space ConvexSpace, representation_for<ConvexSpace> R>
  inline constexpr bool defines_subtraction_for_v{
    requires(const R& r,
             std::span<const value_type_of_t<R>, dimension_of_v<ConvexSpace>> lhs,
             std::span<const value_type_of_t<R>, dimension_of_v<ConvexSpace>> rhs) {
      { r.sub(lhs, rhs) } -> std::convertible_to<std::array<value_type_of_t<R>, dimension_of_v<ConvexSpace>>>;
    }
  };

  template<convex_space ConvexSpace, representation_for<ConvexSpace> R>
  inline constexpr bool defines_subtraction_for_single_value_v{
       (dimension_of_v<ConvexSpace> == 1)
    && requires(const R& r, value_type_of_t<R> lhs, value_type_of_t<R> rhs) {
         { r.sub(lhs, rhs) } -> std::convertible_to<value_type_of_t<R>>;
       }
  };

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
      own, even for vector/affine spaces,  to deal with edge cases such as NaN.
   */

  /** @ingroup Validators
      @brief Validators for spaces of dimension 1 must provide an operator() for validating single values.

      Let the type of the commutative ring associated with a space be space_value_type.
      The validator must expose an operator() that consumes a single value of
      space_value_type, and its return type must be convertible to space_value_type.
   */
  template<class V, convex_space ConvexSpace, representation_for<ConvexSpace> Representation>
  inline constexpr bool validator_for_single_value{
       (dimension_of_v<ConvexSpace> == 1)
       // TO DO: also free module val type? Also define value_type_of
       && requires(V& v, const typename Representation::value_type& val) { { v(Representation::bounds_v, val) } -> std::convertible_to<decltype(val)>; }
  };

  /** @ingroup Validators
      @brief Validators for spaces of dimension d>1 must provide an operator() for an array of d values.

      Let the type of the commutative ring associated with a space be space_value_type.
      Denote a d-dimensional std::array of such values by A. The validator must expose
      an operator() that consumes a single value of type A and its return type must be
      convertible to A.
   */
  template<class V, convex_space ConvexSpace, representation_for<ConvexSpace> Representation>
  inline constexpr bool validator_for_array{
    requires (V& v, const std::array<typename Representation::value_type, dimension_of_v<ConvexSpace>>& values) {
      { v(Representation::bounds_v, values) } -> std::convertible_to<decltype(values)>;
    }
  };

  /** @ingroup Validators
      @brief concept to check if a validator is compatible with a convex space.
   */
  template<class V, class ConvexSpace, class Representation>
  concept validator_for =
       convex_space<ConvexSpace>
    && representation_for<Representation, ConvexSpace>
    && std::default_initializable<V>
    && std::constructible_from<V, V>
    && (    (   has_coordinates_type_v<Representation>) // TO DO 
         || (  !has_coordinates_type_v<Representation>
             && (validator_for_single_value<V, ConvexSpace, Representation> || validator_for_array<V, ConvexSpace, Representation>)));

  /** @ingroup Validators
      @brief Trait for validators that behave like the identity.
   */
  template<class T>
  struct defines_identity_validator : std::false_type {};

  template<class T>
  using defines_identity_validator_t = defines_identity_validator<T>::type;

  template<class T>
  inline constexpr bool defines_identity_validator_v{defines_identity_validator<T>::value};

  
  struct throwing_validator
  {
    template<bounds Bounds, arithmetic T>
    constexpr T operator()(Bounds bnds, T val) const
    {
      return validate(bnds, val);
    }

    template<bounds Bounds, arithmetic T, std::size_t D>
    constexpr const std::array<T, D>& operator()(Bounds bnds, const std::array<T, D>& vals) const
    {      
      return validate(bnds, vals);
    }

    // TO DO: consider moving this elsewhere
    template<bounds Bounds, arithmetic T, std::size_t D>
    constexpr const std::array<T, D>& operator()(const std::array<Bounds, D>& bnds, const std::array<T, D>& vals) const
    {
      for(auto [bnd, val] : std::views::zip(bnds, vals))
        validate(bnd, val);
        
      return vals;
    }
  private:
    template<bounds Bounds, class T>
    constexpr const T& validate(Bounds bnds, const T& t) const
    {
      if(!bnds(t))
        throw std::domain_error{std::format("Input {} outside permitted domain [{}, {}]", bnds.format_input(t), bnds.lower, bnds.upper)};

      return t;
    }
  };

  struct identity_validator
  {
    template<bounds Bounds, weak_commutative_ring T>
    constexpr T operator()(Bounds, T val) const noexcept
    {
      return val;
    }

    template<bounds Bounds, weak_commutative_ring T, std::size_t D>
    constexpr const std::array<T, D>& operator()(Bounds, const std::array<T, D>& vals) const noexcept
    {      
      return vals;
    }

    template<bounds Bounds, weak_commutative_ring T, std::size_t D>
    constexpr const std::array<T, D>& operator()(const std::array<Bounds, D>&, const std::array<T, D>& vals) const
    {        
      return vals;
    }
  };

  template<>
  struct defines_identity_validator<identity_validator> : std::true_type {};

  // Redo this, since we've switched to tensor product
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

   /** @defgroup SpacesUtilities Convex Space Utilities
      @brief Utilites for extracting properties of convex spaces
   */

  template<convex_space C>
  struct is_non_negative_orthant : std::false_type
  {};

  template<convex_space C>
  using is_non_negative_orthant_t = is_non_negative_orthant<C>::type;

  template<convex_space C>
  inline constexpr bool is_non_negative_orthant_v{is_non_negative_orthant<C>::value};

  template<convex_space Space>
  inline constexpr bool identifies_as_non_negative_orthant_v{
    requires {
      typename Space::non_negative_orthant;
      requires std::convertible_to<typename Space::non_negative_orthant, std::true_type>;
    }
  };

  template<convex_space C>
      requires identifies_as_non_negative_orthant_v<C>
  struct is_non_negative_orthant<C> : std::true_type
  {
    static_assert(!affine_space<C>);
  };  

  template<convex_space Space>
  inline constexpr bool has_distinguished_origin_type_v{
    requires {
      typename Space::distinguished_origin;
    }
  };
  
  template<convex_space Space>
  struct has_distinguished_origin : std::false_type
  {};
  
  template<convex_space Space>
  using has_distinguished_origin_t = has_distinguished_origin<Space>::type;

  template<convex_space Space>
  inline constexpr bool has_distinguished_origin_v{has_distinguished_origin<Space>::value};

  template<convex_space Space>
    requires has_distinguished_origin_type_v<Space> && std::convertible_to<typename Space::distinguished_origin, std::true_type>
  struct has_distinguished_origin<Space> : std::true_type
  {
  };

  template<convex_space Space>
    requires (!has_distinguished_origin_type_v<Space>) && is_non_negative_orthant_v<Space>
  struct has_distinguished_origin<Space> : std::true_type
  {
  };

  template<free_module Space>
  struct has_distinguished_origin<Space> : std::true_type
  {};

  template<affine_space Space>
    requires (!free_module<Space>)
  struct has_distinguished_origin<Space> : std::false_type
  {};

  template<class... Ts>
  struct tensor_product
  {
  };

  template<free_module... Ts>
    requires (sizeof...(Ts) >= 1) && are_same_v<commutative_ring_type_of_t<Ts>...>
  struct tensor_product<Ts...>
  {
    using set_type              = tensor_product<typename Ts::set_type...>;
    using commutative_ring_type = std::common_type_t<commutative_ring_type_of_t<Ts>...>;
    using structure             = free_module_tag_t;
    constexpr static std::size_t dimension{(Ts::dimension * ...)};
  };

  template<convex_space... Ts>
    requires (sizeof...(Ts) >= 1)
  && (has_distinguished_origin_v<Ts> && ...)
  // TO DO: distinguished origin
          && ((!affine_space<Ts> && ...) || ((free_module<Ts> || ...) && (!free_module<Ts> || ...)))
  struct tensor_product<Ts...>
  {
    using set_type         = tensor_product<typename Ts::set_type...>;
    using free_module_type = tensor_product<free_module_type_of_t<Ts>...>;
    using structure        = convex_space_tag_t;
  };

  template<class T>
  struct is_tensor_product : std::false_type {};

  template<class... Ts>
  struct is_tensor_product<tensor_product<Ts...>> : std::true_type {};

  template<class T>
  using is_tensor_product_t = is_tensor_product<T>::type;

  template<class T>
  inline constexpr bool is_tensor_product_v = is_tensor_product<T>::value;

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
    using set_type         = sets::convex_functionals<C, commutative_ring_type_of_t<C>>;
    using free_module_type = dual<free_module_type_of_t<C>>;
    using structure        = convex_space_tag_t;
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
    using structure        = affine_space_tag_t;
  };

  /** @ingroup DualSpaces
      @brief Specialization for defining duals of vector spaces via linear functionals
   */
  template<vector_space V>
  struct dual<V>
  {    
    using field_type = commutative_ring_type_of_t<V>;
    using set_type   = sets::linear_functionals<V, field_type>;
    using structure  = vector_space_tag_t;
    constexpr static auto dimension{V::dimension};
  };

  template<convex_space Space>
  struct has_distinguished_origin<dual<Space>> : has_distinguished_origin<Space>
  {
  };

  template<convex_space C>
  struct is_non_negative_orthant<dual<C>> : is_non_negative_orthant<C>
  {
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
  requires (!convex_space<T>) || (convex_space<T> /* TO DO && weak_field<commutative_ring_type_of_t<T>>*/)
  struct dual_of<dual<T>> {
    using type = T;
  };

  /** @defgroup SpaceConversions Conversions Between Spaces
   */

  template<class T>
  inline constexpr bool has_base_space_v{
    requires { typename T::base_space; }
  };

  template<convex_space T>
  struct to_base_space
  {
    using type = T;
  };

  template<convex_space T>
  using to_base_space_t = to_base_space<T>::type;

  template<convex_space T>
    requires has_base_space_v<T>
  struct to_base_space<T>
  {
    using type = T::base_space;
  };

  template<convex_space T>
  struct to_base_space<dual<T>>
  {
    using type = dual<T>;
  };

  template<convex_space T>
    requires has_base_space_v<T>
  struct to_base_space<dual<T>>
  {
    using type = dual<typename T::base_space>;
  };
  
  template<convex_space T, convex_space U>
  inline constexpr bool have_compatible_base_spaces_v{std::same_as<to_base_space_t<T>, to_base_space_t<U>>};

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
  template<
    affine_space AffineSpace,
    basis_for<free_module_type_of_t<AffineSpace>> Basis,
    representation_for<AffineSpace> Representation,    
    class Origin,
    validator_for<AffineSpace, Representation> Validator
  >
  using affine_coordinates = coordinates<AffineSpace, Basis, Origin, Representation, Validator>;

  /** @ingroup Coordinates
      @brief Alias for coordinates of an element of a vector space with respect to a particular basis.
   */
  template<
    vector_space VectorSpace,
    basis_for<free_module_type_of_t<VectorSpace>> Basis,
    representation_for<VectorSpace> Representation,
    validator_for<VectorSpace, Representation> Validator
  >
  using vector_coordinates = coordinates<VectorSpace, Basis, Representation, Validator>;

  /** @ingroup Coordinates
      @brief Alias for coordinates of an element of a free module with respect to a particular basis.
   */
  template<
    free_module FreeModule,
    basis_for<free_module_type_of_t<FreeModule>> Basis,
    representation_for<FreeModule> Representation,
    validator_for<FreeModule, Representation> Validator
  >
  using free_module_coordinates = coordinates<FreeModule, Basis, Representation, Validator>;
  
  /** @ingroup Coordinates
      @brief Class designed for inheritance by concerete coordinate types.

      The type has protected special member functions (including the destructor) and uses
      deducing-this patterns as a type-rich alternative to virtual dispatch. The purpose
      of this approach is solely code reduction. In the maths namespace the coordintates
      namespace derives from coordinates_base, and it turns out to be convenient for
      the former to have several different specializations.

      Furthermore, there are applications in physics which have enough in common
      with maths::coordinates, but are sufficiently distinct, for a base class to be extremely
      useful in terms of reducing what would otherwise be very significant code duplication.

      One of the novelties in the context of physics is the notion of units and quantities
      of different types that can nevertheless be multipled and in some cases (like widths
      and heights) added.

      Morally, for a space of dimension D, coordinates_base wraps D values of the appropriate
      arithmetic type. However, this wrapping does introduce some subtleties. Most notable,
      the rules for arithmetic promotion are not those of the fundamental types. For example,
      unary plus simply returns a copy, without attempting to promote the return type such
      that it wraps the appropriately promoted arithmetic type.
   */


  namespace impl
  {
    template<basis B, class Rep, class...>
    struct is_units_terminated_pack : std::false_type {};

    template<basis B, class Rep, class... Args, std::size_t... Is>
      requires (sizeof...(Args) == sizeof...(Is) + 1)
            && std::same_as<std::tuple_element_t<sizeof...(Is), std::tuple<Args...>>, basis_isomorphism_type_of_t<B>>
            && (std::convertible_to<std::tuple_element_t<Is, std::tuple<Args...>>, Rep> && ...)
    struct is_units_terminated_pack<B, Rep, std::tuple<Args...>, std::index_sequence<Is...>> : std::true_type
    {
    };
  }

  template<basis B, class Rep, class... Args>
  struct is_units_terminated_pack : std::false_type
  {};
  
  template<basis B, class Rep, class... Args>
    requires (sizeof...(Args) > 1)
  struct is_units_terminated_pack<B, Rep, Args...>
    : impl::is_units_terminated_pack<B, Rep, std::tuple<Args...>, std::make_index_sequence<sizeof...(Args) - 1>>
  {};

  template<basis B, class Rep, class... Args>
  inline constexpr bool is_units_terminated_pack_v{is_units_terminated_pack<B, Rep, Args...>::value};

  template<weak_commutative_ring RingRep, auto Bounds>
    requires bounds<decltype(Bounds)>
  struct canonical_representation
  {
    constexpr static auto bounds_v{Bounds};
    using value_type        = RingRep;
    using bounds_type       = decltype(Bounds);
    using bounds_value_type = bounds_type::value_type;

    using free_module_rep_val_type = free_module_representation_value_type_t<value_type>;

    using free_module_representation
      = canonical_representation<free_module_rep_val_type, no_bounds<to_bounds_value_type_t<free_module_rep_val_type>>>;

    template<class T, std::size_t D>
      requires std::same_as<T, value_type> || std::same_as<T, free_module_rep_val_type>
    [[nodiscard]]
    constexpr static std::array<T, D> to_underlying(std::span<const T, D> in) noexcept
    {
      return utilities::to_array(in);
    }

    template<class T, std::size_t D>
      requires std::same_as<T, value_type> || std::same_as<T, free_module_rep_val_type>
    [[nodiscard]]
    constexpr static std::array<T,  D> from_underlying(std::span<const T, D> in) noexcept
    {
      return utilities::to_array(in);
    }
  };

  template<weak_commutative_ring T, auto Bounds>
    requires bounds_value<Bounds>
  struct dual_of<canonical_representation<T, Bounds>>
  {
    using type = canonical_representation<T, reciprocal(Bounds)>;
  };

  template<class T>
  struct is_canonical_representation : std::false_type
  {};

  template<class T>
  inline constexpr bool is_canonical_representation_v{is_canonical_representation<T>::value};

  template<class T>
  using is_canonical_representation_t = is_canonical_representation<T>::type;

  template<weak_commutative_ring T, auto Bounds>
    requires bounds_value<Bounds>
  struct is_canonical_representation<canonical_representation<T, Bounds>> : std::true_type
  {};
  

  template<std::floating_point T, auto Bounds=no_bounds<T>>
  struct basic_polar_representation
  {
    using value_type = T;
    constexpr static auto bounds_v{Bounds};

    using free_module_representation = basic_polar_representation;
          
    [[nodiscard]]
    constexpr static std::array<T, 2> to_underlying(std::span<const T, 2> polar)
    {
      return {polar[0] * std::cos(polar[1]), polar[0] * std::sin(polar[1])};
    }

    [[nodiscard]]
    constexpr static std::array<T, 2> from_underlying(std::span<const T, 2> cartesian)
    {
      T theta{(!cartesian[0] && !cartesian[1]) ? T{} : std::atan2(cartesian[1], cartesian[0])};
      if(theta < 0) theta += T{2} * std::numbers::pi_v<T>;
        
      return {std::sqrt(cartesian[0] * cartesian[0] + cartesian[1] * cartesian[1]), theta};
    }
  };

  template<std::floating_point T, auto Bounds=no_bounds<T>>
  struct polar_representation : basic_polar_representation<T, Bounds>
  {
    using free_module_representation = polar_representation;

    [[nodiscard]]
    static constexpr T compute_angle(T theta, T scale)
    {
      if(!scale)
        return T{};

      constexpr auto pi{std::numbers::pi_v<T>};

      return
          scale > T{} ? theta
        : theta >= pi ? theta - pi
        : theta + pi;
    }

    // TO DO: unary minus, with corresponding change to coordinates_base
      
    [[nodiscard]]
    static constexpr std::array<T, 2> mul(std::span<const T, 2> lhs, T scale)
    {
      return {lhs[0] * std::abs(scale), compute_angle(lhs[1], scale)};
    }

    [[nodiscard]]
    static constexpr std::array<T, 2> div(std::span<const T, 2> lhs, T scale)
    {
      return {lhs[0] / std::abs(scale), compute_angle(lhs[1], scale)};
    }
  };

  template<class T>
  struct heterogeneous_coordinates : std::false_type {};

  template<class T>
  inline constexpr bool heterogeneous_coordinates_v{heterogeneous_coordinates<T>::value};

  template<class T, class... Us>
  struct heterogeneous_coordinates<std::tuple<T, Us...>>
    : std::bool_constant<((!std::is_same_v<T, Us>) || ...)>
  {};

  template<representation Rep>
  struct has_heterogeneous_representation : std::false_type {};

  template<representation Rep>
  inline constexpr bool has_heterogeneous_representation_v{has_heterogeneous_representation<Rep>::value};

  template<representation Rep>
    requires has_coordinates_type_v<Rep>
  struct has_heterogeneous_representation<Rep>
    : std::bool_constant<heterogeneous_coordinates_v<typename Rep::coordinates_type>>
  {};

  template<
    convex_space ConvexSpace,
    basis_for<free_module_type_of_t<ConvexSpace>> Basis,
    representation_for<ConvexSpace> Representation,
    validator_for<ConvexSpace, Representation> Validator,
    class DisplacementCoordinates=free_module_coordinates<free_module_type_of_t<ConvexSpace>,
                                                          Basis,
                                                          typename Representation::free_module_representation,
                                                          Validator>
  >
  class coordinates_base
  {
  public:
    using space_type                    = ConvexSpace;
    using basis_type                    = Basis;
    using representation_type           = Representation;
    using displacement_coordinates_type = DisplacementCoordinates;
    using set_type                      = ConvexSpace::set_type;
    using free_module_type              = free_module_type_of_t<ConvexSpace>;
    using value_type                    = Representation::value_type;    
    using displacement_value_type       = Representation::free_module_representation::value_type;
    using basis_isomorphism_type        = basis_isomorphism_type_of_t<Basis>;
    using validator_type                = Validator;

    // TO DO: improve conventions
    constexpr static bool has_distinguished_origin{has_distinguished_origin_v<space_type>};
    constexpr static bool has_identity_validator{defines_identity_validator_v<validator_type>};
    constexpr static bool has_freely_mutable_components{free_module<space_type>};
    constexpr static bool has_homogeneous_rep{!has_heterogeneous_representation_v<representation_type>};
    constexpr static bool admits_canonical_basis{admits_canonical_basis_v<free_module_type>};

    constexpr static std::size_t dimension{rank_of_v<free_module_type>};
    constexpr static std::size_t D{dimension};

    constexpr coordinates_base() noexcept = default;

    constexpr explicit coordinates_base(std::span<const value_type, D> vals) noexcept(has_identity_validator)
      requires admits_canonical_basis && has_homogeneous_rep
      : coordinates_base{vals, basis_isomorphism_type{}}
    {}

    constexpr coordinates_base(std::span<const value_type, D> vals, basis_isomorphism_type) noexcept(has_identity_validator)
      requires has_homogeneous_rep
      : m_Values{validate(vals, m_Validator)}
    {}

    template<class... Ts>
      requires admits_canonical_basis && has_homogeneous_rep && (D > 1) && (std::convertible_to<Ts, value_type> && ...)
    constexpr explicit(sizeof...(Ts) == 1) coordinates_base(Ts... ts) noexcept(has_identity_validator)
      : coordinates_base{ts..., basis_isomorphism_type{}}
    {}

    template<class... Ts>
      requires has_homogeneous_rep && (D > 1) && (sizeof...(Ts) > 1) && is_units_terminated_pack_v<basis_type, value_type, Ts...>
    constexpr coordinates_base(Ts... ts) noexcept(has_identity_validator)
      : coordinates_base{std::make_index_sequence<sizeof...(Ts) - 1>{}, std::tuple{ts...}}
    {}

    constexpr explicit coordinates_base(value_type val) noexcept(has_identity_validator)      
      requires admits_canonical_basis && (D == 1)
      : coordinates_base{val, basis_isomorphism_type{}}
    {}

    constexpr coordinates_base(value_type val, basis_isomorphism_type) noexcept(has_identity_validator)
      requires (D == 1)
      : m_Values{m_Validator(representation_type::bounds_v, val)}
    {}

    template<class... Coords>
      requires (sizeof...(Coords) > 1)
               // TO DO: requires that these fulfill a coords concept
            && ((0 + ... + Coords::dimension) == dimension)
            && ((Coords::dimension == 1) && ...) // TO DO: ultimately remove this restriction
            && (consistent_bases_v<basis_type, typename Coords::basis_type> && ...)
            && consistent_representation_v<representation_type, Coords...>
            && (std::same_as<validator_type, typename Coords::validator_type> && ...)
    constexpr coordinates_base(const Coords&... vals) noexcept
      : m_Values{std::array{vals.value()...}} // Note: validation performed upstream
    {}

    template<class Self>
      requires (!std::same_as<Self, coordinates_base>)
    constexpr Self& operator+=(this Self& self, const displacement_coordinates_type& v) noexcept(has_identity_validator)
    {
      return self = (self + v);
    }

    template<class Self, class Other>
      requires is_addable_to_v<Other, Self>
    constexpr Self& operator+=(this Self& self, const Other& v) noexcept(has_identity_validator)
    {
      return self = (self + v);
    }

    template<class Self>
      requires (!std::same_as<Self, coordinates_base>) 
    constexpr Self& operator-=(this Self& self, const displacement_coordinates_type& v) noexcept(has_identity_validator)
    {
       return self = (self - v);
    }

    template<class Self>
      requires (!std::same_as<Self, coordinates_base>) && has_distinguished_origin
    constexpr Self& operator*=(this Self& self, value_type u) noexcept(has_identity_validator)
    {
      return self = (self * u);
    }

    template<class Self>
      requires (!std::same_as<Self, coordinates_base>)  && vector_space<free_module_type>
    // TO DO: remove this: it's a temporary hack while the field / commutative_ring concepts are sorted out
    && (!std::integral<value_type>)
    constexpr Self& operator/=(this Self& self, value_type u)
    {
      return self = (self / u);
    }

    template<class Self>
      requires (!std::same_as<Self, coordinates_base>) 
    [[nodiscard]]
    constexpr Self operator+(this const Self& self) noexcept
    {
      return self;
    }

    template<class Self>
      requires (!std::same_as<Self, coordinates_base>) 
            && has_distinguished_origin
            && (!is_non_negative_orthant_v<space_type>)
            && (!std::is_unsigned_v<value_type>)
    [[nodiscard]]
    constexpr Self operator-(this const Self& self) noexcept(has_identity_validator)
    {
      // TO DO: enable refinement through representation
      return Self{self}.for_each_element([](value_type& t) { t = -t; });
    }

    template<class Derived>
      requires std::derived_from<Derived, coordinates_base>
            && (!std::same_as<Derived, displacement_coordinates_type>)
    [[nodiscard]]
    friend constexpr typename Derived::displacement_coordinates_type operator-(const Derived& lhs, const Derived& rhs)
      noexcept(Derived::displacement_coordinates_type::has_identity_validator)
    {
      using disp_t = Derived::displacement_coordinates_type;

      return
        [&] <std::size_t... Is>(std::index_sequence<Is...>) -> disp_t {
          if constexpr(defines_subtraction_for_v<space_type, representation_type>)
          {
            return {representation_type{}.sub(lhs.values(), rhs.values()), basis_isomorphism_type{}};
          }
          else if constexpr(defines_subtraction_for_single_value_v<space_type, representation_type>)
          {
            return {representation_type{}.sub(lhs.values()[0], rhs.values()[0]), basis_isomorphism_type{}};
          }
          else
          {
            const auto transLHS{Derived::to_underlying(lhs.values())}, transRHS{Derived::to_underlying(rhs.values())};
            if constexpr(std::is_unsigned_v<value_type>)
            {
              static_assert(sizeof(displacement_value_type) >= 2 * sizeof(value_type));
            }
            return {Derived::from_underlying(std::array{(static_cast<displacement_value_type>(transLHS[Is]) - static_cast<displacement_value_type>(transRHS[Is]))...}), basis_isomorphism_type{}};
          }
      }(std::make_index_sequence<D>{});
    }

    template<class Derived>
      requires std::derived_from<Derived, coordinates_base>
    [[nodiscard]]
    friend constexpr Derived operator+(const Derived& c, const displacement_coordinates_type& v) noexcept(has_identity_validator)
    {
      if constexpr(defines_addition_for_v<space_type, representation_type>)
      {
        return {representation_type{}.add(c.values(), v.values()), basis_isomorphism_type{}};
      }
      else if constexpr(defines_addition_for_single_value_v<space_type, representation_type>)
      {
        return {representation_type{}.add(c.values()[0], v.values()[0]), basis_isomorphism_type{}};
      }
      else
      {
        auto adder{
          [&c](value_type& lhs, displacement_value_type rhs) {
            if constexpr((std::is_unsigned_v<value_type> && std::is_signed_v<displacement_value_type>))
            {
              static_assert(2 * sizeof(value_type) == sizeof(displacement_value_type));
              const displacement_value_type rhsToUse{
                [&c, lhs, rhs](){
                  if constexpr(!has_identity_validator)
                  {
                    const auto lhsAsSigned{static_cast<displacement_value_type>(lhs)};
                    const auto bnds{coordinate_bounds<displacement_value_type>{-lhsAsSigned, greatest_upper_bound<displacement_value_type> - lhsAsSigned}};
                    return c.validator()(bnds, rhs);
                  }
                  else
                  {
                    return rhs;
                  }
                }()
              };

              lhs += static_cast<value_type>(rhsToUse);
            }
            else
            {
              lhs += rhs;
            }
          }
        };

        return Derived{c}.apply_to_each_element(v.values(), adder);
      }
    }

    template<class Derived>
      requires std::derived_from<Derived, coordinates_base>  && (!std::same_as<Derived, displacement_coordinates_type>)
    [[nodiscard]]
    friend constexpr Derived operator+(const displacement_coordinates_type& v, const Derived& c) noexcept(has_identity_validator)
    {
      return c + v;
    }
  
    template<class Derived>
      requires std::derived_from<Derived, coordinates_base>
            && (!std::same_as<Derived, displacement_coordinates_type>)
            && has_distinguished_origin
    [[nodiscard]]
    friend constexpr Derived operator+(const Derived& c, const Derived& v) noexcept(has_identity_validator)
    {
      if constexpr(defines_addition_for_v<space_type, representation_type>)
      {
        return {representation_type{}.add(c.values(), v.values()), basis_isomorphism_type{}};
      }
      else if constexpr(defines_addition_for_single_value_v<space_type, representation_type>)
      {
        return {representation_type{}.add(c.values()[0], v.values()[0]), basis_isomorphism_type{}};
      }
      else
      {
        return Derived{c}.apply_to_each_element(v.values(), [](value_type& lhs, value_type rhs){ lhs += rhs; });
      }
    }   

    template<class Derived>
      requires std::derived_from<Derived, coordinates_base>
    [[nodiscard]]
    friend constexpr Derived operator-(const Derived& c, const displacement_coordinates_type& v) noexcept(has_identity_validator)
    {
      if constexpr(defines_subtraction_for_v<space_type, representation_type>)
      {
        return {representation_type{}.sub(c.values(), v.values()), basis_isomorphism_type{}};
      }
      else if constexpr(defines_subtraction_for_single_value_v<space_type, representation_type>)
      {
        return {representation_type{}.sub(c.values()[0], v.values()[0]), basis_isomorphism_type{}};
      }
      else
      {
        auto subtractor{
          [&c](value_type& lhs, displacement_value_type rhs) {
            if constexpr((std::is_unsigned_v<value_type> && std::is_signed_v<displacement_value_type>))
            {
              static_assert(2 * sizeof(value_type) == sizeof(displacement_value_type));
              const displacement_value_type rhsToUse{
                [&c, lhs, rhs](){
                  if constexpr(!has_identity_validator)
                  {
                    const auto lhsAsSigned{static_cast<displacement_value_type>(lhs)};
                    const auto bnds{coordinate_bounds<displacement_value_type>{least_lower_bound<displacement_value_type> + lhsAsSigned, lhsAsSigned}};
                    return c.validator()(bnds, rhs);
                  }
                  else
                  {
                    return rhs;
                  }
                }()
              };

              lhs -= static_cast<value_type>(rhsToUse);
            }
            else
            {
              lhs -= rhs;
            }
          }
        };
        
        return Derived{c}.apply_to_each_element(v.values(), subtractor);
      }
    }

    template<class Derived>
      requires std::derived_from<Derived, coordinates_base> && has_distinguished_origin
    [[nodiscard]]
    friend constexpr Derived operator*(const Derived& v, value_type u) noexcept(has_identity_validator)
    {
      if constexpr(defines_scalar_multiplication_for_v<space_type, representation_type>)
      {
        if constexpr(has_homogeneous_rep)
          return {representation_type{}.mul(v.values(), u), basis_isomorphism_type{}};
        else
          return
            make_from_separate_coords(v,
                                      [u](std::span<const value_type, D> vals){
                                        return representation_type{}.mul(vals, u);
                                      });
      }
      else if constexpr(defines_scalar_multiplication_for_single_value_v<space_type, representation_type>)
      {
        return {representation_type{}.mul(v.values()[0], u), basis_isomorphism_type{}};
      }
      else
      {
        return Derived{v}.for_each_element([u](value_type& x) { return x *= u; });
      }
    }

    template<class Derived>
      requires std::derived_from<Derived, coordinates_base> && has_distinguished_origin
    [[nodiscard]]
    friend constexpr Derived operator*(value_type u, const Derived& v) noexcept(has_identity_validator)
    {
      return v * u;
    }

    template<class Derived>
      requires std::derived_from<Derived, coordinates_base> && vector_space<free_module_type> && has_distinguished_origin
    // TO DO: remove this: it's a temporary hack while the field / commutative_ring concepts are sorted out
    && (!std::integral<value_type>)
    [[nodiscard]]
    friend constexpr Derived operator/(const Derived& v, value_type u)
    {
      if constexpr(defines_scalar_division_for_v<space_type, representation_type>)
      {
        if constexpr(has_homogeneous_rep)
          return {representation_type{}.div(v.values(), u), basis_isomorphism_type{}};
        else
          return
            make_from_separate_coords(v,
                                      [u](std::span<const value_type, D> vals){
                                        return representation_type{}.div(vals, u);
                                      });
      }
      else if constexpr(defines_scalar_division_for_single_value_v<space_type, representation_type>)
      {
        return {representation_type{}.div(v.values()[0], u), basis_isomorphism_type{}};
      }
      else
      {
        return Derived{v}.for_each_element([u](value_type& x) { return x /= u; });
      }
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
    // TO DO: consider restricting to spaces with a privileged origin
    [[nodiscard]]
    constexpr explicit operator bool() const noexcept requires (D == 1) && std::convertible_to<value_type, bool>
    {
      return m_Values[0];
    }

    [[nodiscard]]
    constexpr value_type operator[](std::size_t i) const { return m_Values[i]; }

    [[nodiscard]]
    constexpr value_type& operator[](std::size_t i) requires has_freely_mutable_components { return m_Values[i]; }

    // TO DO: reconsider these (and the above, related, functions) for physical values
    // (more generally, when the basis isomorphism type is non-trivial). The const
    // overloads could use a custom iterator that dereferences to a 'quantity'. But
    // what of the mutable ones?
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
    constexpr coordinates_base(const coordinates_base&)     = default;
    constexpr coordinates_base(coordinates_base&&) noexcept = default;

    constexpr coordinates_base& operator=(const coordinates_base&)     = default;
    constexpr coordinates_base& operator=(coordinates_base&&) noexcept = default;

    ~coordinates_base() = default;
    
    template<class Self, class Fn>
      requires std::invocable<Fn, value_type&, value_type>
    constexpr Self&& apply_to_each_element(this Self&& self, std::span<const displacement_value_type, D> rhs, Fn f)
    {
      if constexpr(has_identity_validator)
      {
        std::ranges::for_each(
          std::views::zip(to_underlying(self.m_Values), to_underlying(rhs)),
          [&f](auto&& z){ f(std::get<0>(z), std::get<1>(z)); }
        );

        from_underlying(self.m_Values); 
      }
      else
      {
        auto tmp{to_underlying(self.m_Values)};
        std::ranges::for_each(std::views::zip(tmp,
          to_underlying(rhs)),
          [&f](auto&& z){ f(std::get<0>(z), std::get<1>(z)); });
        if constexpr(has_coordinates_type_v<representation_type>)
        {
          self.m_Values = from_underlying(tmp);
        }
        else
        {
          self.m_Values = validate(from_underlying(tmp), self.m_Validator);
        }
      }

      return std::forward<Self>(self);
    }

    template<class Self, class Fn>
      requires std::invocable<Fn, value_type&>
    constexpr Self&& for_each_element(this Self&& self, Fn f)
    {
      if constexpr(has_identity_validator)
      {
        std::ranges::for_each(to_underlying(self.m_Values), f);
        self.m_Values = from_underlying(self.m_Values);
      }
      else
      {
        auto tmp{to_underlying(self.m_Values)};
        std::ranges::for_each(tmp, f);
        // TO DO: ideally untangle the logic. This is all rather implicit
        // The assumption is that if the representation defines coordinate_type,
        // then from_underlying goes via said coordinates, which perform their
        // own validation. But this is opaque & perhaps brittle. At the very least,
        // constraints need to be checked to ensure consistency, perhaps with
        // a static_assert or two below, for good measure...
        if constexpr(has_coordinates_type_v<representation_type>)
        {
          self.m_Values = from_underlying(tmp);
        }
        else
        {
          self.m_Values = validate(from_underlying(tmp), self.m_Validator);
        }
      }

      return self;
    }
  private:
    SEQUOIA_NO_UNIQUE_ADDRESS validator_type m_Validator;
    std::array<value_type, D> m_Values{};

    template<std::size_t... Is, class... Args> 
    constexpr coordinates_base(std::index_sequence<Is...>, const std::tuple<Args...>& args)
      : m_Values{m_Validator(representation_type::bounds_v, std::array<value_type, D>{static_cast<value_type>(std::get<Is>(args))...})}
    {}
    
    [[nodiscard]]
    constexpr static std::array<value_type, D> validate(std::span<const value_type, D> vals, validator_type& validator)
    {
      return validate(utilities::to_array(vals), validator);
    }

    [[nodiscard]]
    constexpr static std::array<value_type, D> validate(std::array<value_type, D> vals, validator_type& validator)
    {
      constexpr static auto bounds_v{representation_type::bounds_v};
      if constexpr(validator_for_array<validator_type, space_type, representation_type>)
        return validator(bounds_v, vals);
      else
      {
        static_assert(validator_for_single_value<validator_type, space_type, representation_type>);
        static_assert(D == 1);
        return {validator(bounds_v, vals.front())};
      }
    }

    template<class Coord, class T>
    [[nodiscard]]
    constexpr static Coord make_coord(T val) noexcept(has_identity_validator) {
      using individual_unit_t = Coord::units_type;
      return Coord{val, individual_unit_t{}};
    }

    template<std::derived_from<coordinates_base> Derived, class Fn>
    [[nodiscard]]
    constexpr static Derived make_from_separate_coords(const Derived& v, Fn fn) noexcept(has_identity_validator) {
      return
        [&]<std::size_t... Is>(std::index_sequence<Is...>) -> Derived {
          using separate_coords_t = representation_type::coordinates_type;
          return {make_coord<std::tuple_element_t<Is, separate_coords_t>>(fn(v.values())[Is])...};
        }(std::make_index_sequence<D>{});
    }

    template<class T>
    [[nodiscard]]
    constexpr static std::array<T, D> to_underlying(std::span<const T, D> vals)
    {
      if constexpr(representation_for_span<representation_type, space_type>)
      {
        return representation_type{}.to_underlying(vals);
      }
      else
      {
        static_assert(D == 1);
        return std::array{representation_type{}.to_underlying(vals[0])};
      }
    }

    template<class T>
    constexpr static std::array<T, D>& to_underlying(std::array<T, D>& vals)
    {
      if constexpr(representation_for_span<representation_type, space_type>)
      {
        return vals = representation_type{}.to_underlying(std::span<const T, D>{vals});
      }
      else
      {
        static_assert(D == 1);
        return vals = std::array{representation_type{}.to_underlying(vals[0])};
      }
    }

    template<class T>
    constexpr static std::array<T, D>&& to_underlying(std::array<T, D>&& vals)
    {
      if constexpr(representation_for_span<representation_type, space_type>)
      {
        return std::move(vals = representation_type{}.to_underlying(std::span<const T, D>{vals}));
      }
      else
      {
        static_assert(D == 1);
        return std::move(vals = std::array{representation_type{}.to_underlying(vals[0])});
      }
    }

    template<class T>
    constexpr static std::array<T, D>& from_underlying(std::array<T, D>& vals)
    {
      if constexpr(representation_for_span<representation_type, space_type>)
      {
        return vals = representation_type{}.from_underlying(std::span<const T, D>{vals});
      }
      else
      {
        static_assert(D == 1);
        return vals = std::array{representation_type{}.from_underlying(vals[0])};
      }
    }

    template<class T>
    constexpr static std::array<T, D>&& from_underlying(std::array<T, D>&& vals)
    {
      if constexpr(representation_for_span<representation_type, space_type>)
      {
        return std::move(vals = representation_type{}.from_underlying(std::span<const T, D>{vals}));
      }
      else
      {
        static_assert(D == 1);
        return std::move(vals = std::array{representation_type{}.from_underlying(vals[0])});
      }
    }
  };

  /** @ingroup Coordinates
      @brief Class template for representing coordinates on vector spaces, affine spaces and various generalizations.
   */
  
  template<
    convex_space ConvexSpace,
    basis_for<free_module_type_of_t<ConvexSpace>> Basis,
    class Origin,
    representation_for<ConvexSpace> Representation,
    validator_for<ConvexSpace, Representation> Validator
  >
  class coordinates<ConvexSpace, Basis, Origin, Representation, Validator> final
    : public coordinates_base<ConvexSpace, Basis, Representation, Validator>
  {
  public:
    using origin_type = Origin;

    using coordinates_base<ConvexSpace, Basis, Representation, Validator>::coordinates_base;
  };

  template<
    convex_space ConvexSpace,
    basis_for<free_module_type_of_t<ConvexSpace>> Basis,
    representation_for<ConvexSpace> Representation,
    validator_for<ConvexSpace, Representation> Validator
  >
    requires has_distinguished_origin_v<ConvexSpace> && (!free_module<ConvexSpace>)
  class coordinates<ConvexSpace, Basis, Representation, Validator> final
    : public coordinates_base<ConvexSpace, Basis, Representation, Validator>
  {
  public:
    using coordinates_base<ConvexSpace, Basis, Representation, Validator>::coordinates_base;
  };

  template<
    affine_space AffineSpace,
    basis_for<free_module_type_of_t<AffineSpace>> Basis,
    class Origin,    
    representation_for<AffineSpace> Representation,
    validator_for<AffineSpace, Representation> Validator
  >
    requires (!free_module<AffineSpace>)
  class coordinates<AffineSpace, Basis, Origin, Representation, Validator> final
    : public coordinates_base<AffineSpace, Basis, Representation, Validator>
  {
  public:
    using origin_type = Origin;
    
    using coordinates_base<AffineSpace, Basis, Representation, Validator>::coordinates_base;
  };

  template<
    free_module M,
    basis_for<free_module_type_of_t<M>> Basis,
    representation_for<M> Representation,
    validator_for<M, Representation> Validator
  >    
  class coordinates<M, Basis, Representation, Validator> final
    : public coordinates_base<M, Basis, Representation, Validator>
  {
  public:
    using coordinates_base<M, Basis, Representation, Validator>::coordinates_base;
  };

  template<class From, class To>
  struct coordinate_transformation
  {
  };

  template<class From, class To>
  inline constexpr bool has_coordinate_transformation_v{
    requires (const From& f){
      { std::declval<coordinate_transformation<From, To>>()(f) };// TO DO  -> std::convertible_to<To>;
    }
  };

  template<class From, class To>
  inline constexpr bool has_noexcept_coordinate_transformation_v{
       has_coordinate_transformation_v<From, To>
    && requires (const From& f){
         requires noexcept(std::declval<coordinate_transformation<From, To>>()(f));
    }
  };

  namespace sets
  {
    /** @ingroup Sets
        @brief Class template for giving a name to the set of integers and its generalization to other dimensionalities
     */
    template<std::size_t N>
    struct Z
    {
      // TO DO: rename --> rank
      constexpr static std::size_t dimension{N};
    };

    /** @ingroup Sets
        @brief Class template for giving a name to the set of semi-positive integers and its generalization to other dimensionalities
     */
    template<std::size_t N>
    struct N_0
    {
      constexpr static std::size_t dimension{N};
    };

    /** @ingroup Sets
        @brief Class template for giving a name to the set of real numbers and its generalization to other dimensionalities
     */
    template<std::size_t N>
    struct R
    {
      constexpr static std::size_t dimension{N};
    };

    /** @ingroup Sets
        @brief Class template for giving a name to the set of non-negative real numbers and its generalization to other dimensionalities
     */
    template<std::size_t N>
    struct orthant
    {
      constexpr static std::size_t dimension{N};
    };

    /** @ingroup Sets
        @brief Class template for giving a name to the set of complex numbers and its generalization to other dimensionalities
     */
    template<std::size_t N>
    struct C
    {
      constexpr static std::size_t dimension{N};
    };

    enum boundedness { negative_infty, negative_finite, zero, positive_finite, positve_infy };

    template<boundedness Lower, boundedness Upper>
    struct real_line_segment
    {
      constexpr static std::size_t dimension{1};
      boundedness lower_boundedness{Lower},
                  upper_boundedness{Upper};
    };

    template<boundedness Lower, boundedness Upper>
    struct integral_line_segment
    {
      constexpr static std::size_t dimension{1};
      boundedness lower_boundedness{Lower},
                  upper_boundedness{Upper};
    };
  }

  namespace commutative_rings
  {
    template<std::size_t N>
      requires (0 < N) && (N <= 2)
    struct reals
    {
      using set_type  = sets::R<N>;
      using structure = field_tag_t;
    };

    template<std::size_t N>
    struct integers
    {
      using set_type  = sets::Z<N>;
      using structure = commutative_ring_tag_t;
    };

    struct complexes
    {
      using set_type  = sets::C<1>;
      using structure = field_tag_t;
    };
  }

  template<std::integral Rep>
    requires std::is_signed_v<Rep>
  struct weakly_representated_by<commutative_rings::integers<1>, Rep> : std::true_type {};

  // Allow signed as well as unsigned
  //template<std::integral Rep>
  //struct weakly_representated_by<sets::N_0<1>, Rep> : std::true_type {};

  template<std::floating_point Rep>
  struct weakly_representated_by<commutative_rings::reals<1>, Rep> : std::true_type {};

  template<std::floating_point F>
  struct weakly_representated_by<commutative_rings::complexes, std::complex<F>> : std::true_type {};

  //template<std::floating_point Rep>
  //struct weakly_representated_by<sets::orthant<1>, Rep> : std::true_type {};

  /*template<sets::boundedness Lower, sets::boundedness Upper, std::floating_point Rep>
  struct weakly_representated_by<sets::real_line_segment<Lower, Upper>, Rep>
    : std::true_type
  {};

  template<sets::boundedness Lower, sets::boundedness Upper, std::integral Rep>
  requires    (     ((Lower == sets::boundedness::negative_infty) && (Lower == sets::boundedness::negative_finite))  && std::is_signed_v<Rep>  )
           || (not (((Lower == sets::boundedness::negative_infty) && (Lower == sets::boundedness::negative_finite))) && std::is_unsigned_v<Rep>)
  struct weakly_representated_by<sets::integral_line_segment<Lower, Upper>, Rep>
    : std::true_type
  {};
  */

  template<class T>
  struct displacement_space_of;

  template<class T>
  using displacement_space_of_t = displacement_space_of<T>::type;

  template<class T>
    requires identifies_as_field_v<T>
  struct displacement_space_of<T>
  {
    using type = T;
  };

  template<std::size_t N>
  struct displacement_space_of<sets::Z<N>>
  {
    using type = sets::R<N>;
  };

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
  struct arbitrary_representation {};

  template<vector_space V>
  inline constexpr bool has_norm_v{
    requires (const vector_coordinates<V, arbitary_basis<V>, arbitrary_representation<V>, identity_validator>& v) {
      { norm(v) } -> std::convertible_to<typename V::field_type>;
    }
  };

  template<vector_space V>
  inline constexpr bool has_inner_product_v{
    requires (const vector_coordinates<V, arbitary_basis<V>, arbitrary_representation<V>, identity_validator>& v) {
      { inner_product(v, v) } -> std::convertible_to<typename V::field_type>;
    }
  };

  template<class V>
  concept normed_vector_space = vector_space<V> && has_norm_v<V>;

  template<class V>
  concept inner_product_space = vector_space<V> && has_inner_product_v<V>;

  // TO DO: reconsider whether this is necessary
  struct mathematical_arena {};

  template<std::size_t D, class Arena=mathematical_arena>
  struct euclidean_vector_space
  {
    using set_type               = sets::R<D>;
    using field_type             = commutative_rings::reals<1>;
    using structure              = vector_space_tag_t;
    using arena_type             = Arena;
    using admits_canonical_basis = std::true_type;
    constexpr static std::size_t dimension{D};

    template<basis Basis, representation_for<euclidean_vector_space> Representation, validator_for<euclidean_vector_space, Representation> Validator>
      requires is_orthonormal_basis_v<Basis>
    [[nodiscard]]
    friend constexpr field_type inner_product(
      const vector_coordinates<euclidean_vector_space, Basis, Representation, Validator>& v,
      const vector_coordinates<euclidean_vector_space, Basis, Representation, Validator>& w
    )
    {
      return
        std::ranges::fold_left(
          std::views::zip(v.values(), w.values()), // TO DO: transform_view using repr. or be smarter... e.g. for polar
          field_type{},
          [](field_type f, const auto& z){ return f + std::get<0>(z) * std::get<1>(z); }
        );
    }

    template<basis Basis, representation_for<euclidean_vector_space> Representation, validator_for<euclidean_vector_space, Representation> Validator>
      requires is_orthonormal_basis_v<Basis>
    [[nodiscard]]
    friend constexpr field_type dot(
      const vector_coordinates<euclidean_vector_space, Basis, Representation, Validator>& v,
      const vector_coordinates<euclidean_vector_space, Basis, Representation, Validator>& w
    )
    {
      return inner_product(v, w);
    }

    template<basis Basis, representation_for<euclidean_vector_space> Representation, validator_for<euclidean_vector_space, Representation> Validator>
      requires is_orthonormal_basis_v<Basis>
    [[nodiscard]]
    friend constexpr field_type norm(const vector_coordinates<euclidean_vector_space, Basis, Representation, Validator>& v)
    {
      // TO DO: transform_view using repr. or be smarter...
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

  template<std::size_t D, class Arena=mathematical_arena>
  struct euclidean_affine_space
  {
    using set_type          = sets::R<D>;
    using vector_space_type = euclidean_vector_space<D, Arena>;
    using structure         = affine_space_tag_t;
    using arena_type        = Arena;
  };

  template<std::size_t D, class Arena=mathematical_arena>
  struct euclidean_nonnegative_space
  {
    using set_type             = sets::orthant<D>;
    using vector_space_type    = euclidean_vector_space<D, Arena>;
    using structure            = convex_space_tag_t;
    using arena_type           = Arena;
    using distinguished_origin = std::true_type;
    using non_negative_orthant = std::true_type;
  };

  template<class Arena=mathematical_arena>
  using euclidean_half_line = euclidean_nonnegative_space<1, Arena>;

  template<class T>
  inline constexpr bool has_arena_type_v{
    requires { typename T::arena_type;}
  };

  template<class T>
  struct arena_type_of;

  template<class T>
  using arena_type_of_t = arena_type_of<T>::type;

  template<class T>
    requires has_arena_type_v<T>
  struct arena_type_of<T>
  {
    using type = T::arena_type;
  };

  template<convex_space T>
    requires (!has_arena_type_v<dual<T>>)
  struct arena_type_of<dual<T>>
  {
    using type = arena_type_of_t<T>;
  };

  template<convex_space... Ts>
    requires (!has_arena_type_v<tensor_product<Ts...>>)
  struct arena_type_of<tensor_product<Ts...>>
  {
    using type = std::common_type_t<arena_type_of_t<Ts>...>;
  };
  
  template<
    std::size_t D,
    basis Basis,
    representation Representation,
    class Origin,
    class Validator,
    class Arena=mathematical_arena
  >
  using euclidean_affine_coordinates = affine_coordinates<euclidean_affine_space<D, Arena>, Basis, Representation, Origin, Validator>;

  template<
    std::size_t D,
    basis Basis,
    representation Representation,
    class Validator,
    class Arena=mathematical_arena
  >
    requires (dimension_of_v<free_module_type_of_t<Basis>> == D)
  using euclidean_vector_coordinates = vector_coordinates<euclidean_vector_space<D, Arena>, Basis, Representation, Validator>;

  template<
    std::size_t D,
    basis Basis,
    representation Representation,
    class Validator,
    class Arena=mathematical_arena
  >
    requires (dimension_of_v<free_module_type_of_t<Basis>> == D)
  using euclidean_nonnegative_coordinates = coordinates<euclidean_nonnegative_space<D, Arena>, Basis, Representation, Validator>;

  template<free_module M>
  struct dual_of<general_basis<M>>
  {
    using type = general_basis<dual_of_t<M>>;
  };

  template<free_module M>
  struct dual_of<general_basis<dual<M>>>
  {
    using type = general_basis<M>;
  };

  template<class T>
  struct dilatation;

  template<auto Num, auto Den>
  struct dilatation<ratio<Num, Den>>
  {
    using ratio_type = ratio<Num, Den>;
  };

  template<std::intmax_t Num, std::intmax_t Den>
  struct dilatation<std::ratio<Num, Den>>
  {
    using ratio_type = std::ratio<Num, Den>;
  };
  
  template<class...>
  struct orthogonal_similarity_transformation;

  template<class Ratio> // TO DO: reflections and rotations
  struct orthogonal_similarity_transformation<dilatation<Ratio>>
  {
    using dilatation_type = dilatation<Ratio>;
  };

  template<class...>
  struct orthogonal_basis;

  template<free_module M, class Ratio, basis_for<M> ReferenceBasis>
  struct orthogonal_basis<M, orthogonal_similarity_transformation<dilatation<Ratio>>, ReferenceBasis>
  {
    using reference_basis_type = ReferenceBasis;
    using is_basis             = std::true_type;
    using free_module_type     = M;
  };

  template<std::floating_point T, std::size_t D, class Validator=identity_validator, class Arena=mathematical_arena>
  using vec_coords
    = euclidean_vector_coordinates<D, general_basis<euclidean_vector_space<D, Arena>>, canonical_representation<T, no_bounds<T>>, Validator, Arena>;
}
