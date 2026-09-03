////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/** \file */

#include "SpaceOrderingMetaFreeTest.hpp"

#include "sequoia/PlatformSpecific/Macros.hpp"

import std;
import sequoia.physics;

namespace sequoia::testing
{
  using namespace physics;

  namespace
  {
    using mass_space_t   =        mass_space<implicit_common_arena>;
    using length_space_t =               length_space<implicit_common_arena>;
    using time_space_t   =        time_space<implicit_common_arena>;

    using euc_vec_space_t  = euclidean_vector_space<1, implicit_common_arena>;

    using delta_mass_space_t = associated_displacement_space<mass_space_t>;
    using delta_len_space_t  = associated_displacement_space<length_space_t>;

    template<class... Ts>
    struct space_list {};

    /** A space appears in a tensor product bare, as an associated displacement space, as a dual,
        or as the dual of a displacement space. Sorting is over all four.
     */
    template<class Space>
    using spaces_formed_from = space_list<Space, associated_displacement_space<Space>, dual<Space>, dual<associated_displacement_space<Space>>>;

    /** Two spaces whose names bracket the names of the formations themselves -
        `associated_displacement_space` begins with an `a`, so `angular_space` sorts on the wrong
        side of it if a comparison ever reaches a formed name instead of the name of the provenance
        - together with a `maths` space, which sorts before every `physics` one, and its dual.
        Displacement spaces are defined only where the space has a dimension, which is why the
        Euclidean entry is not a full quartet.
     */
    using ordered_spaces
      = meta::concat_t<
          meta::concat_t<spaces_formed_from<angular_space<implicit_common_arena>>,
                         spaces_formed_from<length_space_t>>,
          meta::concat_t<spaces_formed_from<mass_space_t>,
                         space_list<euc_vec_space_t, dual<euc_vec_space_t>>>
        >;

    /** Trichotomy: of `T` precedes `U`, `U` precedes `T`, and `T` is `U`, exactly one holds.

        Together with transitivity this is a strict *total* order, which is more than
        `meta::stable_sort` demands. A strict weak ordering may leave two distinct elements
        equivalent, and the sort then keeps them in input order - which is exactly the difference
        between a tensor product and its reverse that the ordering exists to abolish.

        Stated as "exactly one of three" rather than as separate irreflexivity, asymmetry and
        comparability, all of which follow from it: `std::is_same_v` then appears as the third
        alternative rather than as an exemption for the diagonal.
     */
    template<class T, class U>
    inline constexpr bool trichotomy_v{
      (meta::type_comparator_v<T, U> + meta::type_comparator_v<U, T> + std::is_same_v<T, U>) == 1};

    template<class T, class U, class V>
    inline constexpr bool transitive_v{!(meta::type_comparator_v<T, U> && meta::type_comparator_v<U, V>) || meta::type_comparator_v<T, V>};

    /** `T` stands in trichotomy with every space in the list, and every space in the list with
        every other. The two are separate because a fold expression expands one pack, so
        quantifying over pairs takes two.
     */
    template<class T, class List>
    struct trichotomous_with;

    template<class T, class... Us>
    struct trichotomous_with<T, space_list<Us...>>
    {
      constexpr static bool value{(... && trichotomy_v<T, Us>)};
    };

    template<class List>
    struct trichotomous_over;

    template<class... Ts>
    struct trichotomous_over<space_list<Ts...>>
    {
      constexpr static bool value{(... && trichotomous_with<Ts, space_list<Ts...>>::value)};
    };

    /** The same, one level deeper, for a law over three spaces. */
    template<class T, class U, class List>
    struct transitive_with;

    template<class T, class U, class... Vs>
    struct transitive_with<T, U, space_list<Vs...>>
    {
      constexpr static bool value{(... && transitive_v<T, U, Vs>)};
    };

    template<class T, class List>
    struct transitive_from;

    template<class T, class... Us>
    struct transitive_from<T, space_list<Us...>>
    {
      constexpr static bool value{(... && transitive_with<T, Us, space_list<Us...>>::value)};
    };

    template<class List>
    struct transitive_over;

    template<class... Ts>
    struct transitive_over<space_list<Ts...>>
    {
      constexpr static bool value{(... && transitive_from<Ts, space_list<Ts...>>::value)};
    };

    /** `T` precedes every space in the list, and none of them precedes `T`. Both halves are
        asserted: the reversal is where a comparison that answers by accident shows up.
     */
    template<class T, class List>
    struct precedes_all;

    template<class T, class... Us>
    struct precedes_all<T, space_list<Us...>>
    {
      constexpr static bool value{(... && (meta::type_comparator_v<T, Us> && !meta::type_comparator_v<Us, T>))};
    };

    /** The four spaces formed from one space, in the order `count_and_combine` depends on: it
        folds only the head of the counted list, so cancellation requires them to be contiguous
        *and* in this order.

        Asserted for **every** space in the list rather than for one of them, because the
        convention is invisible to the laws below: permuting the enumerators of `space_formation`
        is an order isomorphism, so both trichotomy and transitivity survive it.
     */
    template<class Space>
    struct formations_ordered
    {
      using displacement = associated_displacement_space<Space>;

      constexpr static bool value{
           precedes_all<Space,             space_list<displacement, dual<Space>, dual<displacement>>>::value
        && precedes_all<displacement,      space_list<dual<Space>, dual<displacement>>>::value
        && precedes_all<dual<Space>,       space_list<dual<displacement>>>::value};
    };

    /** What production code actually consults is `meta::stable_sort`, not the comparator directly,
        and the two are not interchangeable: `merge`'s two-element case selects between an
        unconstrained specialization and one constrained on `Compare<T, U>::value`, so a comparator
        which is *ill-formed* for a pair leaves the constraint merely unsatisfied and the pair
        silently transposed. Asserting the sort therefore covers a failure mode that asserting the
        comparator cannot.
     */
    template<class T, class List>
    struct sorts_the_same_either_way;

    template<class T, class... Us>
    struct sorts_the_same_either_way<T, space_list<Us...>>
    {
      constexpr static bool value{
        (... && std::is_same_v<meta::stable_sort_t<space_list<T, Us>, meta::type_comparator>,
                               meta::stable_sort_t<space_list<Us, T>, meta::type_comparator>>)};
    };

    template<class List>
    struct over_sorted_pairs;

    template<class... Ts>
    struct over_sorted_pairs<space_list<Ts...>>
    {
      constexpr static bool value{(... && sorts_the_same_either_way<Ts, space_list<Ts...>>::value)};
    };

    /** Every space formed from one space precedes every space formed from another, which is what
        it means for the comparison to be settled by the space rather than by what is formed from
        it.
     */
    template<class List, class OtherList>
    struct every_pair_ordered;

    template<class... Ts, class OtherList>
    struct every_pair_ordered<space_list<Ts...>, OtherList>
    {
      constexpr static bool value{(... && precedes_all<Ts, OtherList>::value)};
    };  }

  [[nodiscard]]
  std::filesystem::path space_ordering_meta_free_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void space_ordering_meta_free_test::run_tests()
  {
    test_type_comparator();
    test_type_comparator_ordering_laws();
  }

  void space_ordering_meta_free_test::test_type_comparator()
  {
    STATIC_CHECK(meta::type_comparator_v<mass_space_t, dual<mass_space_t>>);
    STATIC_CHECK(!meta::type_comparator_v<dual<mass_space_t>, mass_space_t>);
    STATIC_CHECK(meta::type_comparator_v<mass_space_t, delta_mass_space_t>);
    STATIC_CHECK(!meta::type_comparator_v<delta_mass_space_t, mass_space_t>);
    STATIC_CHECK(meta::type_comparator_v<mass_space_t, dual<delta_mass_space_t>>);
    STATIC_CHECK(!meta::type_comparator_v<dual<delta_mass_space_t>, mass_space_t>);

    STATIC_CHECK(meta::type_comparator_v<delta_mass_space_t, dual<mass_space_t>>);
    STATIC_CHECK(!meta::type_comparator_v<dual<mass_space_t>, delta_mass_space_t>);    
    STATIC_CHECK(meta::type_comparator_v<delta_mass_space_t, dual<delta_mass_space_t>>);
    STATIC_CHECK(!meta::type_comparator_v<dual<delta_mass_space_t>, delta_mass_space_t>);
    STATIC_CHECK(meta::type_comparator_v<dual<mass_space_t>, dual<delta_mass_space_t>>);
    STATIC_CHECK(!meta::type_comparator_v<dual<delta_mass_space_t>, dual<mass_space_t>>);

    STATIC_CHECK(meta::type_comparator_v<delta_mass_space_t, dual<delta_mass_space_t>>);
    STATIC_CHECK(!meta::type_comparator_v<dual<delta_mass_space_t>, delta_mass_space_t>);
  }

  /** `meta::stable_sort` requires a strict weak ordering; `type_comparator` supplies one only if
      every space which can appear in a tensor product is ordered against every other. Enumerate
      rather than sample: what a sample establishes is that the pairs in it are ordered, and the
      property needed is that all of them are.
   */
  void space_ordering_meta_free_test::test_type_comparator_ordering_laws()
  {
    STATIC_CHECK(trichotomous_over<ordered_spaces>::value);
    STATIC_CHECK(transitive_over<ordered_spaces>::value);

    STATIC_CHECK(every_pair_ordered<spaces_formed_from<angular_space<implicit_common_arena>>, spaces_formed_from<length_space_t>>::value);
    STATIC_CHECK(every_pair_ordered<spaces_formed_from<length_space_t>, spaces_formed_from<mass_space_t>>::value);
    STATIC_CHECK(every_pair_ordered<spaces_formed_from<angular_space<implicit_common_arena>>, spaces_formed_from<mass_space_t>>::value);

    STATIC_CHECK(formations_ordered<angular_space<implicit_common_arena>>::value);
    STATIC_CHECK(formations_ordered<length_space_t>::value);
    STATIC_CHECK(formations_ordered<mass_space_t>::value);
    STATIC_CHECK(formations_ordered<time_space_t>::value);

    STATIC_CHECK(over_sorted_pairs<ordered_spaces>::value);

    // The three cross-space pairs which were matched by two partial specializations and ordered by
    // neither. Spelled out both ways round, since it is the reversal that carries the information.
    STATIC_CHECK( meta::type_comparator_v<dual<delta_len_space_t>, delta_mass_space_t>);
    STATIC_CHECK(!meta::type_comparator_v<delta_mass_space_t, dual<delta_len_space_t>>);
    STATIC_CHECK( meta::type_comparator_v<dual<delta_len_space_t>, dual<mass_space_t>>);
    STATIC_CHECK(!meta::type_comparator_v<dual<mass_space_t>, dual<delta_len_space_t>>);
    STATIC_CHECK( meta::type_comparator_v<dual<length_space_t>, dual<delta_mass_space_t>>);
    STATIC_CHECK(!meta::type_comparator_v<dual<delta_mass_space_t>, dual<length_space_t>>);
  }
}
