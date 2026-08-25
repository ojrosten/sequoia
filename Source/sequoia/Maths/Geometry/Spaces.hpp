////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** @file
    @brief Abstractions pertaining to vector spaces, affine spaces and their
           generalizations.

    Representing abstract algebraic structures in C++ presents an interesting
    challenge. At root, the fundamental abstraction is a set; indeed, a vector
    space is nothing but a set with some additional structure defined. However,
    sets of objects are not straightforward to represent, in general, in C++.

    Consider the real numbers. In C++ we can give a name to the set by using
    the type system.

    @code
    struct reals {};
    @endcode

    But what of the elements of `reals`? Here we run into an immediate
    difficulty. We would like to associate them with the values of a type. But
    to truly do so we require a type with an infinite number of values (and
    uncountably so, in this case). Therefore, when seeking representations,
    particularly of infinite sets, we are generally reduced to approximations.

    However, at least in so far as the set underpinning a particular vector
    space goes, it turns out that, for our purposes, we need go no further than
    naming it. As we will explain momentarily, this is because when dealing
    with vector spaces in practice, we are almost always interested in the
    _coordinates_ of a vector with respect to a particular basis and not the
    abstract entities comprising the elements of the underlying set.

    Before discussing coordinate systems, it is helpful to be more precise
    about the fundamental definition of a vector space. The axioms governing
    addition pertain just to the elements of the underlying set and amount to
    stating that a vector space is an abelian group under addition. Thus,
    vector addition is associative and commutative, there is an identity
    element and every element admits an inverse.

    However, the remaining properties of a vector space require not just the
    underlying set, V, but also a field, F. As such, we speak of a vector space
    over a field. Canonical examples of a field include the rationals and the
    reals. Concretely, a field is a set admitting addition and multiplication
    such that:
    -# Both operations are associative and commutative, with identity elements
       0 and 1 respectively;

    -# Multiplication distributes over addition, i.e. \f$ a(b+c) = ab + ac \f$,
       where a, b and c are all elements of the field;

    -# Every element has an additive inverse;

    -# Every non-zero element has a multiplicative inverse.

    A vector space is equipped with a scalar multiplication, by which any
    element of V may be multiplied by any element of F to give an element of V.
    Writing a and b for elements of F, and \f$ \vc{u} \f$ and \f$ \vc{v} \f$
    for elements of V, scalar multiplication is subject to four further axioms:
    -# It distributes over addition in the vector space, i.e.
       \f$ a(\vc{u} + \vc{v}) = a\vc{u} + a\vc{v} \f$;

    -# It distributes over addition in the field, i.e.
       \f$ (a + b)\vc{v} = a\vc{v} + b\vc{v} \f$;

    -# It is compatible with multiplication in the field, i.e.
       \f$ a(b\vc{v}) = (ab)\vc{v} \f$;

    -# The multiplicative identity of the field leaves vectors unchanged, i.e.
       \f$ 1\vc{v} = \vc{v} \f$.

    It is interesting to observe why the final axiom is necessary: without it,
    setting \f$ a\vc{v} = \vc{0} \f$ for every a and \f$ \vc{v} \f$ would
    satisfy the first three.

    A basis of a vector space is a set of vectors which is linearly independent
    and spans the space. Spanning means that any vector can be built from the
    set using scalar multiplication and addition; linear independence means
    that no member of the set can be built from the others. There are theorems
    that:
    -# Every vector space admits at least one basis;

    -# All bases of a given space have the same size; this common value is the
       dimension of the vector space, d;

    -# In a space of dimension d, any d linearly independent vectors form a
       basis.

    Choosing an ordering of a finite basis, let its elements be denoted
    \f$ \vc{b}_0, \ldots, \vc{b}_{d-1} \f$. Any vector in the space may be
    written as a linear combination:

    \f[
      \vc{v} = a_0 \vc{b}_0 + \ldots + a_{d-1} \vc{b}_{d-1},
    \f]

    where the \f$ a_i \f$ are valued in the field, F. The tuple of these
    values, \f$ [a_0, \ldots, a_{d-1}] \f$, is none other than the coordinates
    of \f$ \vc{v} \f$ with respect to this particular basis. The
    \f$ [a_0, \ldots, a_{d-1}] \f$ are often informally referred to as a
    vector. However strictly speaking this risks an abuse of terminology since
    it may conflate two distinct concepts: an actual vector which is an element
    of the set which forms the vector space, and a representation of this
    vector via the coordinates with respect to a particular basis. This
    distinction can be further reinforced by pointing out that two observers
    who agree they are talking about the same vector (i.e. set element) will
    nevertheless disagree on the coordinates if they are using different bases
    - as they are entirely entitled to do! To further add to the confusion, the
    coordinates may be referred to as a coordinate vector which is perhaps
    unfortunate. (To add to the fun, in the case of \f$ \bb{R}^d \f$, a tuple
    of values \f$ (v(0), \ldots, v(d-1)) \f$ can have a different
    interpretation, which we shall gloss over for now but return to in
    \ref Basis "Basis".)

    Regardless, from the perspective of performing actual calculations, the
    coordinates are key. A crucial point to make is that, when dealing with the
    coordinates, the underlying elements of the set, V, make no explicit
    appearance. This is a manifestation of the fact that a choice of basis
    supplies an isomorphism from V onto \f$ F^d \f$, so that calculations may
    be performed entirely on the coordinates. Put another way, two vector
    spaces over the same field and of the same dimension are isomorphic. These
    considerations are incredibly helpful since, for many practical purposes,
    we need not represent the underlying set beyond, at most, perhaps giving it
    a name. For example, consider the vector space formed by functions which
    map some finite set into a field: the question of how to represent the
    elements of this vector space in C++ is completely circumvented.

    However, that is not to say that subtleties of imperfect representations of
    mathematical abstractions are entirely avoided; indeed, quite the contrary!
    The coordinates are valued in a field and so at this stage we must deal
    with the fact that C++ types such as float and double model the real
    numbers imperfectly. Nevertheless, the burden has been shifted from
    attempting to represent things in C++ that may be completely infeasible to
    things which can be done to reasonable approximation. Generally we will
    speak of e.g. the doubles \ref AlgebraicTraits "weakly" representing the
    reals.

    Vector spaces are just one of the things treated in the code that follows.
    There are several important generalizations. First, there are affine
    spaces, which comprise a set, A, together with a vector space, V, whose
    additive group acts freely and transitively on A. Intuitively, we can start
    at any point in A and translate to any other point by adding the
    appropriate vector. \anchor FreeTransitiveAction There are three separate
    things being asserted:
    -# That V acts at all: adding any vector in V to any point of A gives a
       point of A;

    -# That it acts transitively: for any two points of A there is at least one
       vector which translates from the first to the second;

    -# That it acts freely: there is at most one such vector or, equivalently,
       the only vector which translates a point of A to itself is the zero
       vector.

    The last two together say that the vector translating between any two
    points is unique.

    A nice example of an affine space is Euclidean space. Two observers in this
    space, Alice and Bob, are entirely entitled to define their location as the
    origin. Neither is more right than the other since this space has no
    distinguished origin. Alice and Bob will, in general, disagree about the
    coordinates of points in the space. However, they will agree on the vector
    which translates from one point to another (though if they compare vector
    coordinates, they may have to contend with using different bases on the
    vector space!).

    An affine space is sometimes described as a vector space which has
    forgotten its origin. Indeed, a vector space is an affine space over
    itself. This is interesting in terms of representing these concepts in C++.
    Since a vector space is a special case of an affine space, this suggests
    that an affine space concept is more fundamental, with the vector space
    being a refinement. However, a vector space is part of the definition of an
    affine space (a set and a vector space, satisfying certain conditions) and
    so it is this that will be reflected by the concepts defined below: the
    affine space concept depends on the vector_space concept, and not
    vice-versa.

    It will be useful for our purposes to generalize affine spaces. To start,
    consider taking a convex subset, C, of an affine space. We may translate
    from any point in C to any other by adding the appropriate vector from V.
    However, there are elements of V which, when added to a point in C, will
    take us outside of C and into the broader affine space of which it is a
    part. Yet we do not want to define such a space via an embedding in a
    bigger one; we would like to characterize it in its own right.

    The way forward is to relax the requirement that the action of V be defined
    for every combination of point and vector. Of the three properties relating
    to a \ref FreeTransitiveAction "free and transitive action", the last two
    survive: for any two points of C there is still a unique vector which
    translates from the first to the second. What is lost is the first, that
    any point and any vector may be combined to give a point. Retaining
    uniqueness but not totality leaves us with:
    -# A set, A, together with a vector space, V;

    -# The difference of any two points of A, which is an element of V;

    -# The sum of a point, p, of A and a vector, \f$ \vc{v} \f$, which is
       defined precisely when \f$ \vc{v} \f$ is the difference of some point of
       A and p.

    The last of these is what is meant by saying that V acts partially on A.
    Here partial carries its usual mathematical sense of "not necessarily
    defined everywhere", much as a partial order need not order every pair of
    elements. A total action is therefore a partial action, and affine spaces
    are recovered as precisely those cases for which the sum is defined for
    every point and every vector.

    The mathematical structure we have arrived at is a partial torsor over a
    vector space. There are two important points to bear in mind:
    -# The construction does not require embedding in a bigger space: there is
       no need to consider the set, A, as a subset of anything else.

    -# While convexity is a useful property of some spaces, and provided a hook
       into this discussion, it is not a requirement placed on our partial
       torsor: our construction is more general and flexible than that.

    The notion of a partial action is a standard one, and comes with a useful
    theorem: every partial action of a group on a set can be considered to
    arise from restricting a total action to a subset. While we have
    deliberately avoided explicitly embedding the torsor into an affine space
    as part of our construction, this option is retrospectively available, for
    free, providing useful intuition.

    There is a second generalization that it will be profitable to explore,
    namely relaxing a vector space's field to a ring. The resulting
    construction is called a module, which is a generalization of a vector
    space. Our motivation for this is that the integers form a commutative ring
    and not a field, since integers do not, in general, have multiplicative
    inverses valued within the integers. Rather than attempting to deal with
    modules in full generality, we restrict our attention to what may be the
    most useful, practical cases in the context of C++: free modules over
    commutative rings. Free modules are those which admit a basis.

    Combining the two generalizations brings us to the most primitive
    abstraction with which the code deals: the partial M-torsor. A torsor is a
    set upon which a group acts
    \ref FreeTransitiveAction "freely and transitively"; allowing partiality
    gives the structure described above. The 'M' indicates that the group in
    question is the additive group of a free module, M, rather than of a vector
    space. In the same spirit, it is useful to delineate affine spaces over a
    field from those over a free module; we refer to the latter as M-affine
    spaces. This is a mild extension of the usual terminology, in which an
    affine space is over a vector space.

    Convexity remains a useful refinement, and appears as one in the code: a
    convex space is a partial M-torsor for which, whenever two points belong
    to the space, so do the points lying between them. Intuitively, this
    corresponds to being able to linearly interpolate (lerp): given two points
    p and q
    and a parameter t in the range [0, 1], we may construct intermediate
    points, r, using the recipe
    \f[
        r = (1 - t)p + t q.
    \f]
    However, any notion that r is between p and q requires the ring to
    which t belongs
    -# To be ordered;

    -# To admit values between zero and one other than these two. Indeed, if
       t is an element of the integers, the above formula degenerates to only
       ever producing p or q.

    While there are useful generalizations of convexity that apply when the
    second condition is not satisfied, we do not pursue them. However, there
    is nothing to prohibit appropriate concepts being added in the future.
    Therefore, we define convex spaces to be refinements of a partial
    M-torsor for which the commutative ring is an ordered field. (Note: this
    rules out the case of the dyadic rationals which satisfy both
    conditions above; if a use case for these ever arises the framework can
    be enhanced.)

    The relationships between the structures introduced above form a DAG, in
    which refinements appear below that which they refine:
    @verbatim
                partial M-torsor
               /                \
     (M-convex space)       M-affine space
            |                 /        \
       convex space   affine space   free module
                             \          /
                             vector space
    @endverbatim
    The bracketed entry marks where an M-convex space would sit, were the
    generalizations of convexity alluded to above pursued. Note also that
    affine spaces are not, in general, convex spaces: the latter require an
    ordered field, whereas there is nothing untoward about an affine space
    over the complex numbers.

    The diagram orders the structures by refinement alone, and the order in
    which they must be defined runs the other way: a partial M-torsor is a
    set together with a free module acting on it, so free modules are prior
    to everything above them here, just as vector spaces are prior to affine
    spaces. There is no circularity: free modules and vector spaces appear as
    nodes at all only because a free module is an M-affine space over itself.

    Nor are the edges all of a kind. Most impose a condition: that the action
    is total, or that the ring is a field or an ordered field. The two edges
    descending into a free module and into a vector space are not of this
    sort: every M-affine space can be turned into a free module by nominating
    any one of its points as the origin and transporting the structure there.
    What separates a free module from an M-affine space is therefore not a
    condition but a choice. The distinction manifests if the diagram is read
    upwards. A vector space regarded as a free module is the very same object,
    with a requirement merely no longer in force; regarded as an affine space,
    it has given up its origin and with it the ability to add one point to
    another. That is the sense in which an affine space is a vector space
    which has forgotten its origin.

    We now turn to the important question of how to deal with translations
    which would take us outside the underlying space. Up to this point of our
    rather abstract analysis, we have simply declared that such operations do
    not exist. But this is not a luxury we have in the world of C++; moreover,
    there may be physical or mathematical reasons to introduce additional
    behaviour. For example, consider the convex space comprising a
    one-dimensional interval and the associated free module of translations.
    Perhaps we represent the interval by all the `double`s in the range [0.0,
    1.0]. What happens if we start at the point 0.5 and use `operator+` to
    perform the translation 0.5 + 1.0? We must decide, since there is no way
    for us to stop clients from writing such an expression. Broadly speaking,
    there are four approaches:
    -# Treat this as undefined behaviour;

    -# Treat the behaviour as exceptional;

    -# Adjoin an absorbing state to the space;

    -# Recognize that there is additional physical or mathematical structure
       which remaps the answer back into the space.

    Let us expand on these in turn.

    @par Undefined behaviour
    In C++ we cannot literally restrict the domain of a function such as
    operator+, but we can furnish it with a precondition. The behaviour when
    called out of contract is undefined; not quite in the mathematical sense,
    but in a sense which maps rather well onto our intuition. Since C++26, this
    precondition may be expressible in code.

    @par Exceptional behaviour
    Two things are in play here - the mathematics and its C++ representation -
    and it is worth being clear about which is which. A partial function is the
    same thing as a total function whose codomain has been enlarged by a single
    extra element, \f$ \bot \f$, signalling that there is no result. To widen
    the codomain in this sense is not to change the mathematics but to restate
    it: the space is untouched, and \f$ \bot \f$ is emphatically not a point of
    it, so nothing can be translated from it. Gratifyingly, C++ can transcribe
    the mathematics exactly, by having `operator+` return `std::expected`;
    specifying it to throw expresses the same thing through a different
    mechanism. Note, though, that `operator+=` cannot return anything of the
    sort, since it must write its result back into a point: either it throws,
    or the extra element is admitted into the space itself, which brings us to
    the next option.

    @par Absorbing states
    We can supplement the set underpinning the space in question with an
    absorbing state. In C++, an example would be to use NaN. While adding a
    displacement to a point is now always mathematically defined, the group law
    is broken whenever this state is involved. Consider our previous example
    where 0.5 + 1.0 goes outside the space. As such, suppose we return NaN.
    This is what absorbing means: subtracting 1.0 still leaves us with NaN and
    does not take us back to 0.5. Consequently, translating by 1.0 and then by
    -1.0 no longer agrees with translating by their sum, 0.0, which does
    nothing at all; and it is precisely such agreement that an action of a
    group demands. What makes this a genuine change to the space, rather than a
    way of reporting an error, is that the absorbing state is a value like any
    other: it may be stored, passed on and translated from.

    @par Remapping
    Here there are many options, of which we single out three particularly
    natural ones, analogous to the wrapping modes which graphics hardware
    offers for texture coordinates.

    -# Clamp to the boundary. Again every sum is defined and again the group
       law fails: in the interval [0.0, 1.0], translating 0.5 by 1.0 and then
       by -1.0 arrives at 0.0 rather than at 0.5. Note that this option alone
       requires the embedding in a larger space which we have avoided when not
       strictly necessary, since the point outside the set must be formed
       before it can be clamped back. A variant selects the first point at
       which the ray from the starting point strikes the boundary.

    -# Remap periodically, identifying points which differ by an element of
       some lattice, L. This changes the nature of the space. The action of M
       remains total and transitive but no longer free, since every element of
       L translates every point to itself. What we have is a torsor over the
       quotient M/L.

    -# Remap anti-periodically, so that the coordinate reverses direction each
       time it would leave the set. This also changes the space, but more
       drastically: the group which acts now contains reflections as well as
       translations, and the translations of M do not survive the quotient.
       Taking the unit interval again, translating 0.9 by 0.2 twice gives 0.9,
       whereas translating once by 0.4 gives 0.7. The interval is best regarded
       as a fundamental domain for this larger group rather than as a set on
       which M acts.

    Of the six possibilities canvassed, only the first two - which between them
    leave the mathematics as the partial torsor - keep M acting on the set by
    translations. The absorbing state and clamping retain totality at the
    expense of the group law, while the two periodic remappings retain the
    group law by changing the space and, with it, the group which acts. This is
    why it is the partial torsor, rather than any of the alternatives, which
    generalizes an affine space.

    The final introductory issue to address is the question of why to bother
    modelling concepts such as vector spaces in the abstract sense if it is
    their coordinates which are the things of use from the perspective of
    practical computation. The point is that, for example, vector spaces and
    affine spaces admit different operations: whereas elements of a vector
    space can be added, the same is not true of the elements of the set
    underpinning an affine space. By introducing concepts for the abstract
    algebraic constructs, we can treat coordinates on all of these spaces in a
    common way by using constraints to enable or disable specific operations.
    Thus, the coordinates class template is templated on, amongst other things,
    a partial_m_torsor. To define such a space just requires introducing a
    struct exposing a small amount of data (types and values) known at compile
    time. These data determine whether we intend to model a vector space, a
    free module over a commutative ring, an affine space or whatever. This is
    sufficient for the coordinates implementation to expose the correct set of
    operations, giving a high degree of both type safety and expressivity.
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
#include <type_traits>

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
      @brief Hierarchy for the purpose of self-identification as a commutative ring,
      or a refinement thereof.

      This realizes the diamond drawn in \ref MathematicalStructure "Structure":
      being ordered and being a field are independent refinements of a commutative
      ring, and an ordered field is both.

      @{
   */
  struct commutative_ring_tag_t {};

  struct ordered_ring_tag_t : virtual commutative_ring_tag_t {};

  struct field_tag_t : virtual commutative_ring_tag_t {};

  struct ordered_field_tag_t : virtual ordered_ring_tag_t, virtual field_tag_t {};

  /** @} */

  /** @defgroup HasStructure Subgroup Has structure
      @ingroup MathematicalStructure
      @brief Compile time tools for reflecting on whether a type has a nested type called `structure`, and extracting it.

      @{
   */

  template<class T>
  inline constexpr bool has_structure_v{
    requires { typename T::structure; }
  };

  template<class T>
  struct structure_of {};

  template<class T>
  using structure_of_t = structure_of<T>::type;

  template<class T>
    requires has_structure_v<T>
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
       has_structure_v<T>
    && requires {
         requires std::derived_from<typename T::structure, commutative_ring_tag_t>;
       }
  };

  template<class T>
  inline constexpr bool identifies_as_ordered_ring_v{
        has_structure_v<T>
    && requires {
         requires std::derived_from<typename T::structure, ordered_ring_tag_t>;
       }
  };

  template<class T>
  inline constexpr bool identifies_as_field_v{
        has_structure_v<T>
    && requires {
         requires std::derived_from<typename T::structure, field_tag_t>;
       }
  };

  template<class T>
  inline constexpr bool identifies_as_ordered_field_v{
        has_structure_v<T>
    && requires {
         requires std::derived_from<typename T::structure, ordered_field_tag_t>;
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

  /** @brief A commutative ring equipped with a total order compatible with its
             operations.

      Orderability cannot be deduced from the ring's other properties and so, like
      the structures themselves, is a matter of self-identification. The integers
      are an ordered ring; the complex numbers admit no order compatible with their
      arithmetic and so are a field which is not an ordered field.
   */
  template<class T>
  concept ordered_ring = commutative_ring<T> && identifies_as_ordered_ring_v<T>;

  template<class T>
  concept field = commutative_ring<T> && identifies_as_field_v<T>;

  template<class T>
  concept ordered_field = field<T> && ordered_ring<T> && identifies_as_ordered_field_v<T>;

  /** @} */

  /** @defgroup TorsorTags Subgroup partial M-torsor tag hierarchy
      @ingroup MathematicalStructure
      @brief Hierarchy for the purpose of self-identification as a partial M-torsor, or a refinement thereof.

      @{
   */

  struct partial_m_torsor_tag_t {};

  struct convex_space_tag_t : virtual partial_m_torsor_tag_t {};

  struct m_affine_space_tag_t : virtual partial_m_torsor_tag_t {};

  struct free_module_tag_t : virtual m_affine_space_tag_t {};

  struct vector_space_tag_t : virtual free_module_tag_t {};

  /** @} */

  /** @defgroup PartialMTorsorIdentification Partial M-torsor identification
      @ingroup MathematicalStructure
      @brief Captures the conditions under which types consider themselves to be a partial M-torsor or refinement thereof.

      @{
   */

  template<class T>
  inline constexpr bool identifies_as_partial_m_torsor_v{
       has_structure_v<T>
    && requires {
         requires std::derived_from<typename T::structure, partial_m_torsor_tag_t>;
       }
  };

  template<class T>
  inline constexpr bool identifies_as_convex_space_v{
       has_structure_v<T>
    && requires {
         requires std::derived_from<typename T::structure, convex_space_tag_t>;
       }
  };

  template<class T>
  inline constexpr bool identifies_as_m_affine_space_v{
       has_structure_v<T>
    && requires {
         requires std::derived_from<typename T::structure, m_affine_space_tag_t>;
       }
  };

  template<class T>
  inline constexpr bool identifies_as_free_module_v{
       has_structure_v<T>
    && requires {
         requires std::derived_from<typename T::structure, free_module_tag_t>;
       }
  };

  template<class T>
  inline constexpr bool identifies_as_vector_space_v{
       has_structure_v<T>
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

  /** @brief Whether a type naming both rank and dimension agrees on their value. */
  template<class T>
  inline constexpr bool rank_and_dimension_consistent_v{
    []() {
      if constexpr(has_rank_v<T> && has_dimension_v<T>) return T::rank == T::dimension;
      else                                              return true;
    }()
  };

  template<class T>
  struct rank_of {};

  template<class T>
  inline constexpr std::size_t rank_of_v{rank_of<T>::value};

  template<class T>
    requires has_rank_v<T>
  struct rank_of<T>
  {
    static_assert(rank_and_dimension_consistent_v<T>,
                  "A type naming both rank and dimension must agree on their value");

    constexpr static std::size_t value{T::rank};
  };

  template<class T>
    requires (!has_rank_v<T>) && has_dimension_v<T>
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
      @brief concept for partial M-torsors, the most primitive of the spaces

      A partial M-torsor may be a free module. Otherwise, it comprises a set and
      a free module (which may be a vector space), and must identify as a partial
      M-torsor or a refinement thereof.
   */
  template<class T>
  concept partial_m_torsor
    =  free_module<T> || (has_set_type_v<T> && identifies_as_partial_m_torsor_v<T> && defines_free_module_v<T>);

  /** @ingroup Spaces
      @brief concept for M-affine spaces, being affine spaces over a free module

      Whereas a partial M-torsor admits only a partial action of its free module,
      for an M-affine space the action is total: any displacement may be applied
      to any point and any pair of points is separated by a displacement. A free
      module is an M-affine space over itself.
   */
  template<class T>
  concept m_affine_space = free_module<T> || (partial_m_torsor<T> && identifies_as_m_affine_space_v<T>);

  /** @defgroup FreeModuleTypeOf Subgroup Free module type of
      @ingroup PropertiesOfSpaces
      @brief Extracts the free module type associated with a partial M-torsor.

      This takes into account that if the partial M-torsor is a free module, then
      the associated free module type is just the space itself.

      @{
   */
  template<class>
  struct free_module_type_of {};

  template<class T>
  using free_module_type_of_t = free_module_type_of<T>::type;

  template<free_module Space>
  struct free_module_type_of<Space>
  {
    using type = Space;
  };

  template<class T>
    requires defines_free_module_v<T>
  struct free_module_type_of<T>
  {
    using type = nested_free_module_type_t<T>;
  };

  /** @} */

  /** @ingroup PropertiesOfSpaces
      @brief Extracts the commutative ring type of the free module associated with a partial M-torsor.

      This takes into account that if the free module is a vector space, then the commutative ring is actually a field.

      @{
   */
  template<partial_m_torsor Space>
  struct commutative_ring_type_of
  {
    using type = nested_commutative_ring_type_t<free_module_type_of_t<Space>>;
  };

  template<partial_m_torsor Space>
  using commutative_ring_type_of_t = commutative_ring_type_of<Space>::type;

  /** @} */

  /** @ingroup Spaces
      @brief concept for affine spaces: M-affine spaces over a field.

      Note that there is no need for the space to identify as anything beyond
      an M-affine space. The fact that the free module is required to be
      over a field fixes the space as an affine space.
   */
  template<class T>
  concept affine_space = m_affine_space<T> && field<commutative_ring_type_of_t<T>>;

  /** @ingroup Spaces
      @brief concept for convex spaces

      Convexity is closure under linear interpolation between pairs of points.
      Two things are required, and neither implies the other.

      First, the commutative ring must be an ordered field, so that the
      interpolation \f$ r = (1 - t)p + t q \f$ both makes sense and yields
      points properly between \f$ p \f$ and \f$ q \f$; see the introduction
      for why anything weaker will not do.

      Secondly, the space must be closed under that operation, which no amount
      of reflection on the ring can establish: a partial M-torsor over the reals
      is free to comprise, say, two disjoint intervals. Hence a space must
      identify as convex - unless it is affine, for which the action of the free
      module is total and closure is therefore automatic.
   */
  template<class T>
  concept convex_space =    partial_m_torsor<T>
                         && (affine_space<T> || identifies_as_convex_space_v<T>)
                         && ordered_field<commutative_ring_type_of_t<T>>;


  /** @ingroup PropertiesOfSpaces
      @brief Extracts the rank of the free module associated with a partial
             M-torsor; for a vector space, this is the dimension.

      The program is ill-formed if the space defines its own rank inconsistently
      with that of the free module.
   */
  template<partial_m_torsor Space>
  inline constexpr std::size_t dimension_of_v{
    [](){
      constexpr std::size_t rank{rank_of_v<free_module_type_of_t<Space>>};

      if constexpr(defines_rank_v<Space>)
      {
        static_assert(rank_of_v<Space> == rank);
      }

      return rank;
    }()
  };

  /** @defgroup Basis Basis
      @brief Concepts and helpers for bases of free modules.

      By definition, a free module admits a basis, and bases are an essential
      ingredient in our approach. The introduction describes the primary
      considerations; here we focus on the nuances.

      Understanding how to specify a basis requires some care. Consider first
      the case of \f$ \bb{R}^d \f$, understood to be a vector space. (This
      latter statement is to avoid the ambiguity whereby, by \f$ \bb{R}^d \f$,
      we could just mean the set, without any additional structure.) Really,
      \f$ \bb{R}^d \f$ is the special case of \f$ \bb{R}^S \f$ in which
      \f$ S \f$ is \f$ \{0, \ldots, d-1\} \f$. The general construction runs as
      follows:
      -# \f$ S \f$ is an index set of cardinality \f$ d \f$;
      -# \f$ \bb{R}^S \f$ is the set of all functions from
         \f$ S \to \bb{R} \f$, equipped with pointwise addition and scalar
         multiplication.

      Though seemingly very abstract, this maps nicely onto C++ intuition:
      -# Take an element of \f$ \bb{R}^S \f$, \f$ v \f$ - a vector;
      -# Take an element of \f$ S \f$, \f$ i \f$ - an index;
      -# Consider \f$ v(i) \f$ (from a C++ perspective \f$ v[i] \f$ may be even
         more natural): this returns the \f$ i \f$th coordinate, which in this
         example is just a real number.

      As a concrete example in \f$ d = 2 \f$, suppose we take
      \f$ v(0) = 0.5 \f$ and \f$ v(1) = -0.5 \f$, which we might express as
      \f$ v = (0.5, -0.5) \f$. But thinking about this more carefully seems to
      suggest a paradox. We claimed in the introduction that the components of
      a vector must be written with respect to a basis. And yet here we haven't
      introduced a basis, just a function which in no way requires a basis for
      its definition.

      Resolving the apparent paradox requires carefully picking apart both what
      we have done, and the notation we have used to express it. A function on
      \f$ S \f$ needs no basis; but presenting the space as \f$ \bb{R}^S \f$
      amounts to a choice of structure that carries a distinguished basis. With
      respect to that basis, the values \f$ v(s) \f$ *are* the coordinates.
      Notationally, when we write \f$ v = (0.5, -0.5) \f$ we are adopting the
      following convention:

      -# Taking the mathematical definition of a d-tuple to be a *function* on
         \f$ \{0, \ldots, d-1\} \f$;
      -# Taking the notation for a 2-tuple to be \f$ (x, y) \f$.

      This is a perfectly reasonable thing to do. The problem is that
      \f$ (0.5, -0.5) \f$ may also quite plausibly be read as the coordinates
      of a vector with respect to some basis which is implicitly part of the
      context of the discussion. We took care to anticipate this in the
      introduction by writing coordinates with square brackets, viz.
      \f$ [0.5, -0.5] \f$. Nevertheless, devoid of context, there is a danger
      that round brackets may be misinterpreted. Therefore, unless stated
      otherwise, we will interpret round brackets in the same way as square
      ones, generally preferring to be completely explicit if we want the
      function interpretation:

      \f[
        v : \{0, 1\} \to \bb{R}, \quad v(0) = 0.5, \quad v(1) = -0.5.
      \f]

      The distinguished basis promised above is built by running through the
      elements of the index set, taking the basis vector at \f$ s \f$ to be the
      function whose values are all zero except at \f$ s \f$, where the value
      is 1. These are the standard, or canonical, basis vectors:

      \f[
        e_s : S \to \bb{R}, \quad e_s(t) = \delta_{st},
      \f]

      where \f$ \delta_{st} \f$, the Kronecker delta, is 1 when \f$ s = t \f$
      and 0 otherwise. Put differently, \f$ \bb{R}^S \f$ admits a canonical
      basis.

      With this established, we are now in a position to talk about changes of
      basis. It is the canonical basis which makes this possible: with one
      basis distinguished, any other may be specified by an isomorphism of the
      space to itself. Such a special case is known as an automorphism, and
      here is an element of \f$ GL(\bb{R}^S) \f$.

      The final issue to discuss before moving on from \f$ \bb{R}^S \f$ is that
      of ordered versus unordered bases. Nothing so far has required \f$ S \f$
      to be more than a set of cardinality \f$ d \f$. We can impose the
      structure \f$ \{0, \ldots, d-1\} \f$, thereby defining an ordering, which
      can be carried through to give an ordered basis
      \f$ (e_0, \ldots, e_{d-1}) \f$ - a tuple in the sense described above,
      with the parentheses marking that the order is now part of the data. But
      the index set may not carry an intrinsic ordering, for example
      \f$ \{\mathrm{foo}, \mathrm{bar}, \mathrm{baz}\} \f$. Of course, there is
      nothing to stop us from imposing an order, which is typically what we do
      in the case \f$ \{x, y, z\} \f$. But that is a choice and it would be
      equally valid (if perverse) to identify \f$ e_x \f$ with \f$ e_2 \f$. C++
      naturally supplies such an order, as the members of an index set are
      necessarily declared in some sequence. Henceforth we adopt the convention
      that a basis is ordered according to the order in which its index set is
      stated.

      To summarize what we have discovered for \f$ \bb{R}^S \f$: a basis is
      specified by an index set together with an automorphism, which may be the
      identity.

      Let us now move to the general case: an arbitrary free module, \f$ M \f$,
      of rank \f$ d \f$ over a commutative ring, \f$ R \f$. The considerations
      of this section carry over unchanged, but with one key difference. For
      \f$ R^S \f$ we were able to name a basis by associating it with an
      automorphism because there is a canonical basis against which to measure
      it. However, for \f$ M \f$ the most we can say is that it is isomorphic
      to \f$ R^S \f$, and no one such isomorphism is singled out. The
      isomorphism is therefore part of the specification of a basis, which
      requires stating:
      -# An index set, \f$ S \f$, of cardinality \f$ d \f$;
      -# An isomorphism \f$ \varphi : R^S \to M \f$. This goes by a few
         different names - basepoint in the torsor literature - but we opt for
         the one preferred by differential geometry, frame.

      The automorphisms have not disappeared; they are subsumed by the
      isomorphism \f$ \varphi : R^S \to M \f$. If two bases share a frame, so
      that one is \f$ \varphi \f$ and the other \f$ \varphi \circ g \f$, then
      \f$ g \f$ is the automorphism specifying the change of basis. If they do
      not share a frame, nothing relates them: the frames form a torsor under
      \f$ GL(R^S) \f$, and in a torsor a group element is meaningful only as a
      difference, so an automorphism measured against one frame says nothing
      about coordinates referred to another.

      Whether a client must nominate a frame at all depends on the module. One
      presented as \f$ R^S \f$ carries the canonical basis, which serves as its
      default and needs no nomination. One presented abstractly carries no
      distinguished basis, so there is nothing for a default to name and the
      client has to choose.
   */

  struct identity_isomorphism {};

  // TO DO: temporary. A stand-in for a genuine element of GL(S) - the
  // automorphism negating every coordinate - pending machinery for specifying
  // such elements. It exists to exercise the client experience of a
  // non-identity automorphism.
  struct reflection {};

  /** @ingroup Basis
      @brief Compile time constant reflecting whether a nested type named `frame` exists.
   */
  template<class T>
  inline constexpr bool has_frame_v{
    requires { typename T::frame; }
  };

  /** @ingroup Basis
      @brief Compile time constant reflecting whether a nested type named `index_set` exists.
   */
  template<class T>
  inline constexpr bool has_index_set_v{
    requires { typename T::index_set; }
  };

  /** @ingroup Basis
      @brief The basis data for the canonical basis of a free module of rank \f$ d \f$,
             presented as \f$ R^S \f$.

      Only admissible for modules which declare that they admit a canonical basis; one
      presented abstractly does not, and its clients must nominate a frame. The rank is
      supplied by the client, since basis data is written independently of the module it will
      be paired with; `basis_data_for` checks that the two agree.
   */
  template<std::size_t D>
  struct canonical_basis_data
  {
    using frame     = identity_isomorphism;
    using index_set = std::make_index_sequence<D>;
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
      @brief Whether the index set is consistent with the rank of M.

      A basis of M has rank(M) elements, so an index set must supply exactly
      that many indices. An `index_sequence` can be asked its size; an
      enumeration - the natural way to write an index set whose elements have
      names rather than numbers - cannot be similarly queried, at least until
      reflection is supported. Anything else is not an index set at all.
   */
  template<class BasisData, free_module M>
    requires has_index_set_v<BasisData>
  inline constexpr bool has_consistent_index_set_v{
    []{
      using index_set_type = BasisData::index_set;

      if constexpr(requires { index_set_type::size(); })
        return index_set_type::size() == rank_of_v<M>;
      else if constexpr(std::is_enum_v<index_set_type>)
        // TO DO std::meta::enumerators_of(^^index_set_type).size() == rank_of_v<M>;
        return true;
      else
        return false;
    }()
  };

  /** @ingroup Basis
      @brief Whether the frame named by the basis data is one M may legitimately be given.

      Naming the identity asserts that M *is* \f$ R^S \f$, so only a module
      which declares as much may do so. Any other frame is a nomination, and a
      client may always nominate.
   */
  template<class BasisData, free_module M>
    requires has_frame_v<BasisData>
  inline constexpr bool names_admissible_frame_v{
       !std::same_as<typename BasisData::frame, identity_isomorphism>
    || admits_canonical_basis_v<M>
  };

  /** @ingroup Basis
      @brief Concept for data specifying a basis of a particular free module.

      A type supplies basis data by naming an index set and a frame - precisely
      the two ingredients the introduction identifies.

      Both arguments may be arbitrary types. The conditions are checked in the
      order stated, so that each of the last two finds what it needs already
      established: an index set and a module for the first, a frame and a module
      for the second. Each is nevertheless constrained to say what it assumes,
      rather than trusting this ordering silently.
   */
  template<class BasisData, class M>
  concept basis_data_for
    =  has_frame_v<BasisData>
    && has_index_set_v<BasisData>
    && free_module<M>
    && has_consistent_index_set_v<BasisData, M>
    && names_admissible_frame_v<BasisData, M>;

  /** @ingroup Basis
      @brief The basis of a free module, M, determined by supplied basis data.

      Clients supply only the basis data; the association with M is made here,
      since it is fixed by the space the coordinates belong to. A module
      presented as \f$ R^S \f$ carries the canonical basis, which therefore
      serves as the default. One presented abstractly has no distinguished
      basis, so the constraint on the data rejects the default and clients must
      nominate a frame.
   */
  template<free_module M, basis_data_for<M> BasisData=canonical_basis_data<rank_of_v<M>>>
  struct basis
  {
    using free_module_type = M;
    using basis_data_type  = BasisData;
    using frame_type       = BasisData::frame;
    using index_set_type   = BasisData::index_set;
  };

  /** @ingroup Basis
      @brief Whether two sets of basis data may be compared, and how to rebuild one of them.

      Frames are opaque: nothing about a type named as a frame reveals how it is related to
      any other. So this is a customisation point, false until a client specialises it to
      declare that two conventions are related. That is a library policy rather than a
      mathematical fact - see the discussion of torsors in the group introduction.

      A `true_type` specialisation declares the two comparable, and that is all most clients
      need. An operation which *combines* two coordinate types to make a third additionally
      requires the specialisation to supply
      `template<class U, std::size_t D> using rebind_type`, naming data of the same kind for
      unit `U` over a module of rank `D` - the third space's rank being neither operand's.
   */
  template<class BasisData1, class BasisData2>
  struct consistent_basis_data : std::false_type {};

  template<class BasisData1, class BasisData2>
  inline constexpr bool consistent_basis_data_v{consistent_basis_data<BasisData1, BasisData2>::value};

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

      Note that there is not a two template parameter analogue of
      is_addable_to_v. The problem is that for any putative
      `is_multiplicable_by_v`, there doesn't seem to be any
      reasonable concept satisfied by the return value. (Multiplying
      an n * m matrix by an m * p matrix is one example of why this
      is problematic.) We could just require that t * u exists, but
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

      This section seeks to provide compile time mechanisms to specify (and subsequently query)
      whether arithmetic types, be they built-in (int, float etc) or user-defined, exhibit various properties.
      A fundamental problem of attempting this is the difference
      between a mathematical structure and an approximate representation of that structure.
      This is sharpened in the current context of attempting to do this in code:     
      ints model the integers, but not exactly since there is a maximum representable value.
      Similarly, floating-point numbers model the reals but only in an approximate sense.
      However, it is useful to capture such 'best efforts', for which we use the signifier
      'weak'. For example to signify the fact that neither integer nor floating-point addition exactly models an
      abelian group, trait `weakly_abelian_group_under_addition` is used. Note, however, that addition of unsigned integral
      types does precisely model an abelian group and so 'weak' is a minimum requirement.
      
      Entertainingly, the only fundamental type in C++ which exactly models a field is bool.
   */

  /** @defgroup WeaklyAbelianGroupUnderAddition Subgroup weakly abelian group under addition
      @ingroup AlgebraicTraits
      @brief Trait for specifying whether a type behaves (approximately) as an abelian group under addition.

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
      @brief Trait for specifying whether a type behaves (approximately) as an abelian group under multiplication.

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

  template<class Bounds, class Space>
  concept bounds_for
    =      bounds<Bounds> && partial_m_torsor<Space>
        && (   ((dimension_of_v<Space> == 1) && checks_single_val_against_bounds_v<Bounds>)
            || ((dimension_of_v<Space>  > 1) && checks_array_against_bounds_v<Bounds, dimension_of_v<Space>>));

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
  template<class R, class Space>
  inline constexpr bool representation_for_single_value{
        (dimension_of_v<Space> == 1)
     && requires(R& r, const typename R::value_type& val) {
          { r.to_underlying(val)   } -> std::convertible_to<decltype(val)>;
          { r.from_underlying(val) } -> std::convertible_to<decltype(val)>;
        }
  };

  template<class R, class Space>
  inline constexpr bool representation_for_span{
     requires(R& r, std::span<const typename R::value_type, dimension_of_v<Space>> vals) {
       { r.to_underlying(vals)   } -> std::convertible_to<std::array<typename R::value_type, dimension_of_v<Space>>>;
       { r.from_underlying(vals) } -> std::convertible_to<std::array<typename R::value_type, dimension_of_v<Space>>>;
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
  struct weakly_represented_by : std::false_type {};

  template<class Algebra, class Rep>
  using weakly_represented_by_t = weakly_represented_by<Algebra, Rep>::type;

  template<class Algebra, class Rep>
  inline constexpr bool weakly_represented_by_v = weakly_represented_by<Algebra, Rep>::value;

  template<class Rep, class Algebra>
  concept weak_representation_for = weakly_represented_by_v<Algebra, Rep>;
  
  // TO DO constrain coordinates_type to hold things satisfying a coords concept?
  template<class R>
  concept representation = std::default_initializable<R>
                        && has_value_type_v<R>
                        && has_free_module_representation_v<R>
                        && (has_coordinates_type_v<R> || has_bounds_v<R>);

  template<class R, class Space>
  concept representation_for
    =    partial_m_torsor<Space>
      && representation<R>
    // TO DO not this, since the set could be anything and the rep applies to the coordinates
    // but maybe something along these lines
    // && weak_representation_for<value_type_of_t<R>, set_type_of_t<Space>>
    // TO DO: this seems to massively slow down compilation  && bounds_for<decltype(R::bounds_v), Space>
      && (representation_for_single_value<R, Space> || representation_for_span<R, Space>);

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

  template<partial_m_torsor Space, representation_for<Space> R>
  inline constexpr bool defines_scalar_multiplication_for_v{
    requires(const R& r, std::span<const value_type_of_t<R>, dimension_of_v<Space>> vals, value_type_of_t<R> s) {
      { r.mul(vals, s) } -> std::convertible_to<std::array<value_type_of_t<R>, dimension_of_v<Space>>>;
    }
  };

  template<partial_m_torsor Space, representation_for<Space> R>
  inline constexpr bool defines_scalar_multiplication_for_single_value_v{
       (dimension_of_v<Space> == 1)
       && requires(const R& r, value_type_of_t<R> val, value_type_of_t<R> s) {
         { r.mul(val, s) } -> std::convertible_to<value_type_of_t<R>>;
       }
  };

  template<partial_m_torsor Space, representation_for<Space> R>
  inline constexpr bool defines_scalar_division_for_v{
    requires(const R& r, std::span<const value_type_of_t<R>, dimension_of_v<Space>> vals, value_type_of_t<R> s) {
      { r.div(vals, s) } -> std::convertible_to<std::array<value_type_of_t<R>, dimension_of_v<Space>>>;
    }
  };

  template<partial_m_torsor Space, representation_for<Space> R>
  inline constexpr bool defines_scalar_division_for_single_value_v{
       (dimension_of_v<Space> == 1)
    && requires(const R& r, value_type_of_t<R> val, value_type_of_t<R> s) {
         { r.div(val, s) } -> std::convertible_to<value_type_of_t<R>>;
       }
  };

  template<partial_m_torsor Space, representation_for<Space> R>
  inline constexpr bool defines_addition_for_v{
    requires(const R& r,
             std::span<const value_type_of_t<R>, dimension_of_v<Space>> lhs,
             std::span<const value_type_of_t<R>, dimension_of_v<Space>> rhs) {
      { r.add(lhs, rhs) } -> std::convertible_to<std::array<value_type_of_t<R>, dimension_of_v<Space>>>;
    }
  };

  template<partial_m_torsor Space, representation_for<Space> R>
  inline constexpr bool defines_addition_for_single_value_v{
    (dimension_of_v<Space> == 1)
    && requires(const R& r, value_type_of_t<R> lhs, value_type_of_t<R> rhs) {
        { r.add(lhs, rhs) } -> std::convertible_to<value_type_of_t<R>>;
      }
  };

  template<partial_m_torsor Space, representation_for<Space> R>
  inline constexpr bool defines_subtraction_for_v{
    requires(const R& r,
             std::span<const value_type_of_t<R>, dimension_of_v<Space>> lhs,
             std::span<const value_type_of_t<R>, dimension_of_v<Space>> rhs) {
      { r.sub(lhs, rhs) } -> std::convertible_to<std::array<value_type_of_t<R>, dimension_of_v<Space>>>;
    }
  };

  template<partial_m_torsor Space, representation_for<Space> R>
  inline constexpr bool defines_subtraction_for_single_value_v{
       (dimension_of_v<Space> == 1)
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
  template<class V, partial_m_torsor Space, representation_for<Space> Representation>
  inline constexpr bool validator_for_single_value{
       (dimension_of_v<Space> == 1)
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
  template<class V, partial_m_torsor Space, representation_for<Space> Representation>
  inline constexpr bool validator_for_array{
    requires (V& v, const std::array<typename Representation::value_type, dimension_of_v<Space>>& values) {
      { v(Representation::bounds_v, values) } -> std::convertible_to<decltype(values)>;
    }
  };

  /** @ingroup Validators
      @brief concept to check if a validator is compatible with a partial M-torsor.
   */
  template<class V, class Space, class Representation>
  concept validator_for =
       partial_m_torsor<Space>
    && representation_for<Representation, Space>
    && std::default_initializable<V>
    && std::constructible_from<V, V>
    && (    (   has_coordinates_type_v<Representation>) // TO DO 
         || (  !has_coordinates_type_v<Representation>
             && (validator_for_single_value<V, Space, Representation> || validator_for_array<V, Space, Representation>)));

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

      For now, we do not handle the general case. Thus, we may only construct the direct
      product of free modules if they are, roughly speaking, over the same ring. To be
      precise, the free modules' commutative rings must either all satisfy the weak_field
      concept or none of them do; on top of which they must share a common type in the C++
      sense.
   */

   /** @defgroup SpacesUtilities Convex Space Utilities
      @brief Utilities for extracting properties of partial M-torsors
   */

  template<partial_m_torsor C>
  struct is_non_negative_orthant : std::false_type
  {};

  template<partial_m_torsor C>
  using is_non_negative_orthant_t = is_non_negative_orthant<C>::type;

  template<partial_m_torsor C>
  inline constexpr bool is_non_negative_orthant_v{is_non_negative_orthant<C>::value};

  template<partial_m_torsor Space>
  inline constexpr bool identifies_as_non_negative_orthant_v{
    requires {
      typename Space::non_negative_orthant;
      requires std::convertible_to<typename Space::non_negative_orthant, std::true_type>;
    }
  };

  template<partial_m_torsor C>
      requires identifies_as_non_negative_orthant_v<C>
  struct is_non_negative_orthant<C> : std::true_type
  {
    static_assert(!affine_space<C>);
  };  

  template<partial_m_torsor Space>
  inline constexpr bool has_distinguished_origin_type_v{
    requires {
      typename Space::distinguished_origin;
    }
  };
  
  template<partial_m_torsor Space>
  struct has_distinguished_origin : std::false_type
  {};
  
  template<partial_m_torsor Space>
  using has_distinguished_origin_t = has_distinguished_origin<Space>::type;

  template<partial_m_torsor Space>
  inline constexpr bool has_distinguished_origin_v{has_distinguished_origin<Space>::value};

  template<partial_m_torsor Space>
    requires has_distinguished_origin_type_v<Space> && std::convertible_to<typename Space::distinguished_origin, std::true_type>
  struct has_distinguished_origin<Space> : std::true_type
  {
  };

  template<partial_m_torsor Space>
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
    constexpr static std::size_t dimension{(rank_of_v<Ts> * ...)};
  };

  template<partial_m_torsor... Ts>
    requires (sizeof...(Ts) >= 1)
  && (has_distinguished_origin_v<Ts> && ...)
  // TO DO: distinguished origin
          && ((!affine_space<Ts> && ...) || ((free_module<Ts> || ...) && (!free_module<Ts> || ...)))
  struct tensor_product<Ts...>
  {
    using set_type         = tensor_product<typename Ts::set_type...>;
    using free_module_type = tensor_product<free_module_type_of_t<Ts>...>;
    using structure        = std::conditional_t<(convex_space<Ts> && ...), convex_space_tag_t, partial_m_torsor_tag_t>;
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
      since all we are required to do is name the structure and not attempt
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

        It is tempting to constrain the class To to be a partial M-torsor. However,
        without additional work, rings and fields do not satisfy the partial_m_torsor
        concept as introduced, above.

        TO DO: now that the domain is a partial M-torsor rather than a convex
        space, the name of this template overstates what it holds.
     */
    template<partial_m_torsor From, class To>
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
      @brief Specialization for defining duals of partial M-torsors via convex functionals
   */
  template<partial_m_torsor C>
    requires (!affine_space<C>)
  struct dual<C>
  {
    using set_type         = sets::convex_functionals<C, commutative_ring_type_of_t<C>>;
    using free_module_type = dual<free_module_type_of_t<C>>;
    using structure        = std::conditional_t<convex_space<C>, convex_space_tag_t, partial_m_torsor_tag_t>;
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
    using structure        = m_affine_space_tag_t;
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

  template<partial_m_torsor Space>
  struct has_distinguished_origin<dual<Space>> : has_distinguished_origin<Space>
  {
  };

  template<partial_m_torsor C>
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

      The dual of the dual of a finite dimensional vector space, V, is isomorphic
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
  requires (!partial_m_torsor<T>) || (partial_m_torsor<T> /* TO DO && weak_field<commutative_ring_type_of_t<T>>*/)
  struct dual_of<dual<T>> {
    using type = T;
  };

  /** @defgroup SpaceConversions Conversions Between Spaces
   */

  template<class T>
  inline constexpr bool has_base_space_v{
    requires { typename T::base_space; }
  };

  template<partial_m_torsor T>
  struct to_base_space
  {
    using type = T;
  };

  template<partial_m_torsor T>
  using to_base_space_t = to_base_space<T>::type;

  template<partial_m_torsor T>
    requires has_base_space_v<T>
  struct to_base_space<T>
  {
    using type = T::base_space;
  };

  template<partial_m_torsor T>
  struct to_base_space<dual<T>>
  {
    using type = dual<T>;
  };

  template<partial_m_torsor T>
    requires has_base_space_v<T>
  struct to_base_space<dual<T>>
  {
    using type = dual<typename T::base_space>;
  };
  
  template<partial_m_torsor T, partial_m_torsor U>
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
    partial_m_torsor Space,
    basis_data_for<free_module_type_of_t<Space>> BasisData,
    class... Ts
  >
  class coordinates;

  /** @ingroup Coordinates
      @brief Alias for coordinates of a point in an M-affine space with respect to a particular origin.

      The basis belongs to the associated free module, allowing the coordinates type for the
      M-affine space to be aware of the type of the coordinate representation for displacements
   */
  template<
    m_affine_space MAffineSpace,
    basis_data_for<free_module_type_of_t<MAffineSpace>> BasisData,
    representation_for<MAffineSpace> Representation,
    class Origin,
    validator_for<MAffineSpace, Representation> Validator
  >
  using m_affine_coordinates = coordinates<MAffineSpace, BasisData, Origin, Representation, Validator>;

  /** @ingroup Coordinates
      @brief Alias for coordinates of a point in an affine space with respect to a particular origin.

      The basis belongs to the associated vector space, allowing the coordinates type for the affine
      space to be aware of the type of the coordinate representation for displacements
   */
  template<
    affine_space AffineSpace,
    basis_data_for<free_module_type_of_t<AffineSpace>> BasisData,
    representation_for<AffineSpace> Representation,    
    class Origin,
    validator_for<AffineSpace, Representation> Validator
  >
  using affine_coordinates = coordinates<AffineSpace, BasisData, Origin, Representation, Validator>;

  /** @ingroup Coordinates
      @brief Alias for coordinates of an element of a vector space with respect to a particular basis.
   */
  template<
    vector_space VectorSpace,
    basis_data_for<free_module_type_of_t<VectorSpace>> BasisData,
    representation_for<VectorSpace> Representation,
    validator_for<VectorSpace, Representation> Validator
  >
  using vector_coordinates = coordinates<VectorSpace, BasisData, Representation, Validator>;

  /** @ingroup Coordinates
      @brief Alias for coordinates of an element of a free module with respect to a particular basis.
   */
  template<
    free_module FreeModule,
    basis_data_for<free_module_type_of_t<FreeModule>> BasisData,
    representation_for<FreeModule> Representation,
    validator_for<FreeModule, Representation> Validator
  >
  using free_module_coordinates = coordinates<FreeModule, BasisData, Representation, Validator>;
  
  /** @ingroup Coordinates
      @brief Class designed for inheritance by concrete coordinate types.

      The type has protected special member functions (including the destructor) and uses
      deducing-this patterns as a type-rich alternative to virtual dispatch. The purpose
      of this approach is solely code reduction. In the maths namespace the coordinates
      namespace derives from coordinates_base, and it turns out to be convenient for
      the former to have several different specializations.

      Furthermore, there are applications in physics which have enough in common
      with maths::coordinates, but are sufficiently distinct, for a base class to be extremely
      useful in terms of reducing what would otherwise be very significant code duplication.

      One of the novelties in the context of physics is the notion of units and quantities
      of different types that can nevertheless be multiplied and in some cases (like widths
      and heights) added.

      Morally, for a space of dimension D, coordinates_base wraps D values of the appropriate
      arithmetic type. However, this wrapping does introduce some subtleties. Most notable,
      the rules for arithmetic promotion are not those of the fundamental types. For example,
      unary plus simply returns a copy, without attempting to promote the return type such
      that it wraps the appropriately promoted arithmetic type.
   */


  namespace impl
  {
    template<class B, class Rep, class...>
    struct is_units_terminated_pack : std::false_type {};

    template<class B, class Rep, class... Args, std::size_t... Is>
      requires (sizeof...(Args) == sizeof...(Is) + 1)
            && std::same_as<std::tuple_element_t<sizeof...(Is), std::tuple<Args...>>, typename B::frame_type>
            && (std::convertible_to<std::tuple_element_t<Is, std::tuple<Args...>>, Rep> && ...)
    struct is_units_terminated_pack<B, Rep, std::tuple<Args...>, std::index_sequence<Is...>> : std::true_type
    {
    };
  }

  template<class B, class Rep, class... Args>
  struct is_units_terminated_pack : std::false_type
  {};
  
  template<class B, class Rep, class... Args>
    requires (sizeof...(Args) > 1)
  struct is_units_terminated_pack<B, Rep, Args...>
    : impl::is_units_terminated_pack<B, Rep, std::tuple<Args...>, std::make_index_sequence<sizeof...(Args) - 1>>
  {};

  template<class B, class Rep, class... Args>
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
    partial_m_torsor Space,
    basis_data_for<free_module_type_of_t<Space>> BasisData,
    representation_for<Space> Representation,
    validator_for<Space, Representation> Validator,
    class DisplacementCoordinates=free_module_coordinates<free_module_type_of_t<Space>,
                                                          BasisData,
                                                          typename Representation::free_module_representation,
                                                          Validator>
  >
  class coordinates_base
  {
  public:
    using space_type                    = Space;
    using basis_data_type               = BasisData;
    using basis_type                    = basis<free_module_type_of_t<Space>, BasisData>;
    using representation_type           = Representation;
    using displacement_coordinates_type = DisplacementCoordinates;
    using set_type                      = Space::set_type;
    using free_module_type              = free_module_type_of_t<Space>;
    using value_type                    = Representation::value_type;    
    using displacement_value_type       = Representation::free_module_representation::value_type;
    using frame_type                    = basis_type::frame_type;
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
      : coordinates_base{vals, frame_type{}}
    {}

    constexpr coordinates_base(std::span<const value_type, D> vals, frame_type) noexcept(has_identity_validator)
      requires has_homogeneous_rep
      : m_Values{validate(vals, m_Validator)}
    {}

    template<class... Ts>
      requires admits_canonical_basis && has_homogeneous_rep && (D > 1) && (std::convertible_to<Ts, value_type> && ...)
    constexpr explicit(sizeof...(Ts) == 1) coordinates_base(Ts... ts) noexcept(has_identity_validator)
      : coordinates_base{ts..., frame_type{}}
    {}

    template<class... Ts>
      requires has_homogeneous_rep && (D > 1) && (sizeof...(Ts) > 1) && is_units_terminated_pack_v<basis_type, value_type, Ts...>
    constexpr coordinates_base(Ts... ts) noexcept(has_identity_validator)
      : coordinates_base{std::make_index_sequence<sizeof...(Ts) - 1>{}, std::tuple{ts...}}
    {}

    constexpr explicit coordinates_base(value_type val) noexcept(has_identity_validator)      
      requires admits_canonical_basis && (D == 1)
      : coordinates_base{val, frame_type{}}
    {}

    constexpr coordinates_base(value_type val, frame_type) noexcept(has_identity_validator)
      requires (D == 1)
      : m_Values{m_Validator(representation_type::bounds_v, val)}
    {}

    template<class... Coords>
      requires (sizeof...(Coords) > 1)
               // TO DO: requires that these fulfill a coords concept
            && ((0 + ... + Coords::dimension) == dimension)
            && ((Coords::dimension == 1) && ...) // TO DO: ultimately remove this restriction
            && (consistent_basis_data_v<basis_data_type, typename Coords::basis_data_type> && ...)
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
            return {representation_type{}.sub(lhs.values(), rhs.values()), frame_type{}};
          }
          else if constexpr(defines_subtraction_for_single_value_v<space_type, representation_type>)
          {
            return {representation_type{}.sub(lhs.values()[0], rhs.values()[0]), frame_type{}};
          }
          else
          {
            const auto transLHS{Derived::to_underlying(lhs.values())}, transRHS{Derived::to_underlying(rhs.values())};
            if constexpr(std::is_unsigned_v<value_type>)
            {
              static_assert(sizeof(displacement_value_type) >= 2 * sizeof(value_type));
            }
            return {Derived::from_underlying(std::array{(static_cast<displacement_value_type>(transLHS[Is]) - static_cast<displacement_value_type>(transRHS[Is]))...}), frame_type{}};
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
        return {representation_type{}.add(c.values(), v.values()), frame_type{}};
      }
      else if constexpr(defines_addition_for_single_value_v<space_type, representation_type>)
      {
        return {representation_type{}.add(c.values()[0], v.values()[0]), frame_type{}};
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
        return {representation_type{}.add(c.values(), v.values()), frame_type{}};
      }
      else if constexpr(defines_addition_for_single_value_v<space_type, representation_type>)
      {
        return {representation_type{}.add(c.values()[0], v.values()[0]), frame_type{}};
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
        return {representation_type{}.sub(c.values(), v.values()), frame_type{}};
      }
      else if constexpr(defines_subtraction_for_single_value_v<space_type, representation_type>)
      {
        return {representation_type{}.sub(c.values()[0], v.values()[0]), frame_type{}};
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
          return {representation_type{}.mul(v.values(), u), frame_type{}};
        else
          return
            make_from_separate_coords(v,
                                      [u](std::span<const value_type, D> vals){
                                        return representation_type{}.mul(vals, u);
                                      });
      }
      else if constexpr(defines_scalar_multiplication_for_single_value_v<space_type, representation_type>)
      {
        return {representation_type{}.mul(v.values()[0], u), frame_type{}};
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
          return {representation_type{}.div(v.values(), u), frame_type{}};
        else
          return
            make_from_separate_coords(v,
                                      [u](std::span<const value_type, D> vals){
                                        return representation_type{}.div(vals, u);
                                      });
      }
      else if constexpr(defines_scalar_division_for_single_value_v<space_type, representation_type>)
      {
        return {representation_type{}.div(v.values()[0], u), frame_type{}};
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
    partial_m_torsor Space,
    basis_data_for<free_module_type_of_t<Space>> BasisData,
    class Origin,
    representation_for<Space> Representation,
    validator_for<Space, Representation> Validator
  >
  class coordinates<Space, BasisData, Origin, Representation, Validator> final
    : public coordinates_base<Space, BasisData, Representation, Validator>
  {
  public:
    using origin_type = Origin;

    using coordinates_base<Space, BasisData, Representation, Validator>::coordinates_base;
  };

  template<
    partial_m_torsor Space,
    basis_data_for<free_module_type_of_t<Space>> BasisData,
    representation_for<Space> Representation,
    validator_for<Space, Representation> Validator
  >
    requires has_distinguished_origin_v<Space> && (!free_module<Space>)
  class coordinates<Space, BasisData, Representation, Validator> final
    : public coordinates_base<Space, BasisData, Representation, Validator>
  {
  public:
    using coordinates_base<Space, BasisData, Representation, Validator>::coordinates_base;
  };

  template<
    affine_space AffineSpace,
    basis_data_for<free_module_type_of_t<AffineSpace>> BasisData,
    class Origin,    
    representation_for<AffineSpace> Representation,
    validator_for<AffineSpace, Representation> Validator
  >
    requires (!free_module<AffineSpace>)
  class coordinates<AffineSpace, BasisData, Origin, Representation, Validator> final
    : public coordinates_base<AffineSpace, BasisData, Representation, Validator>
  {
  public:
    using origin_type = Origin;
    
    using coordinates_base<AffineSpace, BasisData, Representation, Validator>::coordinates_base;
  };

  template<
    free_module M,
    basis_data_for<free_module_type_of_t<M>> BasisData,
    representation_for<M> Representation,
    validator_for<M, Representation> Validator
  >    
  class coordinates<M, BasisData, Representation, Validator> final
    : public coordinates_base<M, BasisData, Representation, Validator>
  {
  public:
    using coordinates_base<M, BasisData, Representation, Validator>::coordinates_base;
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
      // Only \f$ \bb{R} \f$ itself is a field. Under componentwise multiplication
      // \f$ \bb{R}^N \f$ has zero divisors, and its product order is not total, so
      // for N > 1 the weakest honest claim is a commutative ring.
      using structure = std::conditional_t<N == 1, ordered_field_tag_t, commutative_ring_tag_t>;
    };

    template<std::size_t N>
    struct integers
    {
      using set_type  = sets::Z<N>;
      using structure = ordered_ring_tag_t;
    };

    struct complexes
    {
      using set_type  = sets::C<1>;
      using structure = field_tag_t;
    };
  }

  template<std::integral Rep>
    requires std::is_signed_v<Rep>
  struct weakly_represented_by<commutative_rings::integers<1>, Rep> : std::true_type {};

  // Allow signed as well as unsigned
  //template<std::integral Rep>
  //struct weakly_represented_by<sets::N_0<1>, Rep> : std::true_type {};

  template<std::floating_point Rep>
  struct weakly_represented_by<commutative_rings::reals<1>, Rep> : std::true_type {};

  template<std::floating_point F>
  struct weakly_represented_by<commutative_rings::complexes, std::complex<F>> : std::true_type {};

  //template<std::floating_point Rep>
  //struct weakly_represented_by<sets::orthant<1>, Rep> : std::true_type {};

  /*template<sets::boundedness Lower, sets::boundedness Upper, std::floating_point Rep>
  struct weakly_represented_by<sets::real_line_segment<Lower, Upper>, Rep>
    : std::true_type
  {};

  template<sets::boundedness Lower, sets::boundedness Upper, std::integral Rep>
  requires    (     ((Lower == sets::boundedness::negative_infty) && (Lower == sets::boundedness::negative_finite))  && std::is_signed_v<Rep>  )
           || (not (((Lower == sets::boundedness::negative_infty) && (Lower == sets::boundedness::negative_finite))) && std::is_unsigned_v<Rep>)
  struct weakly_represented_by<sets::integral_line_segment<Lower, Upper>, Rep>
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

    template<class BasisData, representation_for<euclidean_vector_space> Representation, validator_for<euclidean_vector_space, Representation> Validator>
      requires is_orthonormal_basis_v<BasisData>
    [[nodiscard]]
    friend constexpr field_type inner_product(
      const vector_coordinates<euclidean_vector_space, BasisData, Representation, Validator>& v,
      const vector_coordinates<euclidean_vector_space, BasisData, Representation, Validator>& w
    )
    {
      return
        std::ranges::fold_left(
          std::views::zip(v.values(), w.values()), // TO DO: transform_view using repr. or be smarter... e.g. for polar
          field_type{},
          [](field_type f, const auto& z){ return f + std::get<0>(z) * std::get<1>(z); }
        );
    }

    template<class BasisData, representation_for<euclidean_vector_space> Representation, validator_for<euclidean_vector_space, Representation> Validator>
      requires is_orthonormal_basis_v<BasisData>
    [[nodiscard]]
    friend constexpr field_type dot(
      const vector_coordinates<euclidean_vector_space, BasisData, Representation, Validator>& v,
      const vector_coordinates<euclidean_vector_space, BasisData, Representation, Validator>& w
    )
    {
      return inner_product(v, w);
    }

    template<class BasisData, representation_for<euclidean_vector_space> Representation, validator_for<euclidean_vector_space, Representation> Validator>
      requires is_orthonormal_basis_v<BasisData>
    [[nodiscard]]
    friend constexpr field_type norm(const vector_coordinates<euclidean_vector_space, BasisData, Representation, Validator>& v)
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
    using structure         = m_affine_space_tag_t;
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

  template<partial_m_torsor T>
    requires (!has_arena_type_v<dual<T>>)
  struct arena_type_of<dual<T>>
  {
    using type = arena_type_of_t<T>;
  };

  template<partial_m_torsor... Ts>
    requires (!has_arena_type_v<tensor_product<Ts...>>)
  struct arena_type_of<tensor_product<Ts...>>
  {
    using type = std::common_type_t<arena_type_of_t<Ts>...>;
  };
  
  template<
    std::size_t D,
    class BasisData,
    representation Representation,
    class Origin,
    class Validator,
    class Arena=mathematical_arena
  >
  using euclidean_affine_coordinates = affine_coordinates<euclidean_affine_space<D, Arena>, BasisData, Representation, Origin, Validator>;

  template<
    std::size_t D,
    class BasisData,
    representation Representation,
    class Validator,
    class Arena=mathematical_arena
  >
  using euclidean_vector_coordinates = vector_coordinates<euclidean_vector_space<D, Arena>, BasisData, Representation, Validator>;

  template<
    std::size_t D,
    class BasisData,
    representation Representation,
    class Validator,
    class Arena=mathematical_arena
  >
  using euclidean_nonnegative_coordinates = coordinates<euclidean_nonnegative_space<D, Arena>, BasisData, Representation, Validator>;

  template<std::size_t D>
  struct dual_of<canonical_basis_data<D>>
  {
    using type = canonical_basis_data<D>;
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

  template<class Ratio, class ReferenceBasis>
  struct orthogonal_basis<orthogonal_similarity_transformation<dilatation<Ratio>>, ReferenceBasis>
  {
    using reference_basis_type = ReferenceBasis;
    using frame                = ReferenceBasis::frame;
  };

  template<std::floating_point T, std::size_t D, class Validator=identity_validator, class Arena=mathematical_arena>
  using vec_coords
    = euclidean_vector_coordinates<D, canonical_basis_data<D>, canonical_representation<T, no_bounds<T>>, Validator, Arena>;
}
