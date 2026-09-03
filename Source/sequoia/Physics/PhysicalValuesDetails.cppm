////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

module;

#include "sequoia/PlatformSpecific/Macros.hpp"

export module sequoia.physics:PhysicalValuesDetails;

import std;

export import sequoia.core.container_utilities;
export import sequoia.core.meta;
export import sequoia.maths.algebra;
export import sequoia.maths.arithmetic;
export import sequoia.maths.geometry;
export import sequoia.platform_specific;

/** \file */

export namespace sequoia::physics
{
  using namespace maths;

  template<class Space>
  struct associated_displacement_space;

  template<class T>
  inline constexpr bool identifies_as_unit_v{
    requires {
      typename T::is_unit;
      requires std::convertible_to<typename T::is_unit, std::true_type>;
    }
  };

  template<class T>
  concept physical_unit = identifies_as_unit_v<T>;

  template<class... Ts>
  struct composite_space;

  template<physical_unit... Ts>
  struct composite_unit;

  template<representation... Ts>
  struct composite_representation;

  template<class T>
  struct reduction;

  struct no_unit_t;
}

export namespace sequoia::physics
{
  /** \brief The manner in which a space is formed from its provenance, in the order in which
      spaces of a common provenance are sorted.

      A space appears in a tensor product either bare, as the displacement space associated with a
      point space, as a dual, or as the dual of a displacement space. Ordering the formations is
      half of ordering the spaces; the other half is the name of the provenance itself.
   */
  enum class space_formation : std::size_t { itself, displacement, dual, dual_displacement };

  /** \brief Primary class template decomposing a space into the space it is formed from and the
      manner of its formation.

      `provenance_of_t` is not to be confused with `maths::to_base_space_t`, which is a
      different question with a similar shape: that one asks what a space reduces *to* when a
      product is formed, and answers for composites; this one strips a space back to the one it is
      formed from.
   */
  template<class Space>
  struct provenance_of
  {
    using type = Space;
    constexpr static space_formation formation{space_formation::itself};
  };

  template<class Space>
  struct provenance_of<associated_displacement_space<Space>>
  {
    using type = Space;
    constexpr static space_formation formation{space_formation::displacement};
  };

  template<class Space>
  struct provenance_of<dual<Space>>
  {
    using type = Space;
    constexpr static space_formation formation{space_formation::dual};
  };

  template<class Space>
  struct provenance_of<dual<associated_displacement_space<Space>>>
  {
    using type = Space;
    constexpr static space_formation formation{space_formation::dual_displacement};
  };
  
  template<class Space>
  using provenance_of_t = provenance_of<Space>::type;

  template<class Space>
  inline constexpr space_formation formation_of_v{provenance_of<Space>::formation};

  template<class Space>
  inline constexpr bool is_formed_space_v{!std::is_same_v<Space, provenance_of_t<Space>>};
}

export namespace sequoia::meta
{
  /** \brief Orders spaces formed from a common space by their provenance, and spaces formed from
      different ones by the space they are formed from.

      Sorting a tensor product is what makes \f$A \otimes B\f$ and \f$B \otimes A\f$ the same
      type, so the comparator must be a strict weak ordering over every space which can appear in
      one. Deriving both halves of the comparison from `provenance_of` supplies that by
      construction: the provenances are totally ordered because `type_name` is, and the formations
      are totally ordered because `space_formation` is an enumeration. Ordering the two
      independently is what makes the comparison total - a pair left unordered would leave the
      canonical form of a product dependent on the order in which it was written.
   */
  template<class T, class U>
    requires (physics::is_formed_space_v<T> || physics::is_formed_space_v<U>)
  struct type_comparator<T, U>
    : std::bool_constant<
           std::is_same_v<physics::provenance_of_t<T>, physics::provenance_of_t<U>>
        ? (physics::formation_of_v<T> < physics::formation_of_v<U>)
        : (type_name<physics::provenance_of_t<T>>() < type_name<physics::provenance_of_t<U>>())
      >
  {};
}

export namespace sequoia::physics::impl
{
  using namespace maths;

  template<class T, int I>
  struct type_counter {};

  /// \class Primary class template for counting and combining instances of various types
  template<class...>
  struct count_and_combine {};

  template<class... Ts>
  using count_and_combine_t = count_and_combine<Ts...>::type;

  template<>
  struct count_and_combine<tensor_product<>>
  {
    using type = tensor_product<>;
  };

  template<class T>
  struct count_and_combine<T>
  {
    using type = tensor_product<type_counter<T, 1>>;
  };

  template<class T>
  struct count_and_combine<tensor_product<T>>
  {
    using type = tensor_product<type_counter<T, 1>>;
  };
    
  template<class T, class... Ts>
  struct count_and_combine<tensor_product<T, Ts...>>
    : count_and_combine<tensor_product<Ts...>, count_and_combine_t<T>> 
  {};

  template<class T, class... Us, int... Is>
  struct count_and_combine<tensor_product<T>, tensor_product<type_counter<Us, Is>...>>
    : count_and_combine<T, tensor_product<type_counter<Us, Is>...>>
  {};

  template<class T, class... Ts, class... Us, int... Is>
    requires (sizeof...(Ts) > 0)
  struct count_and_combine<tensor_product<T, Ts...>, tensor_product<type_counter<Us, Is>...>>
    : count_and_combine<tensor_product<Ts...>, count_and_combine_t<T, tensor_product<type_counter<Us, Is>...>>>
  {};

  template<class S, class T, int I, class... Ts, int... Is>
    requires (!is_tensor_product_v<S> && !is_dual_v<S> && !std::is_same_v<S, T> && !std::is_same_v<S, associated_displacement_space<T>>)
  struct count_and_combine<S, tensor_product<type_counter<T, I>, type_counter<Ts, Is>...>>
  {
    using type = tensor_product<type_counter<S, 1>, type_counter<T, I>, type_counter<Ts, Is>...>;
  };

  template<class S, class T, int I, class... Ts, int... Is>
  requires (!std::is_same_v<S, T> && !std::is_same_v<S, dual_of_t<T>> && !std::is_same_v<S, associated_displacement_space<T>> && !std::is_same_v<S, dual_of_t<associated_displacement_space<T>>> && !std::is_same_v<S, associated_displacement_space<dual_of_t<T>>>
                                  && !std::is_same_v<T, dual_of_t<S>> && !std::is_same_v<T, associated_displacement_space<S>> && !std::is_same_v<T, dual_of_t<associated_displacement_space<S>>> && !std::is_same_v<T, associated_displacement_space<dual_of_t<S>>>) 
  struct count_and_combine<dual<S>, tensor_product<type_counter<T, I>, type_counter<Ts, Is>...>>
  {
    using type = tensor_product<type_counter<dual<S>, 1>, type_counter<T, I>, type_counter<Ts, Is>...>;
  };

  template<class T, int I, class... Ts, int... Is>
  struct count_and_combine<T, tensor_product<type_counter<T, I>, type_counter<Ts, Is>...>>
  {
    using type = tensor_product<type_counter<T, I+1>, type_counter<Ts, Is>...>;
  };

  template<class T, int I, class... Ts, int... Is>
  struct count_and_combine<dual<T>, tensor_product<type_counter<T, I>, type_counter<Ts, Is>...>>
  {
    using type = tensor_product<type_counter<T, I-1>, type_counter<Ts, Is>...>;
  };
  
  // TO DO: associated_displacement_space is specific to physical quantities so
  // doesn't recognize euc_vec as the displacement space of euc_half

  /// Promote all T to associated_displacement_space<T>
  template<class T, int I, class... Ts, int... Is>
  struct count_and_combine<associated_displacement_space<T>, tensor_product<type_counter<T, I>, type_counter<Ts, Is>...>>
  {
    using type = tensor_product<type_counter<associated_displacement_space<T>, I+1>, type_counter<Ts, Is>...>;
  };

  /// Promote all T to associated_displacement_space<T>
  template<class T, int I, class... Ts, int... Is>
  struct count_and_combine<dual<associated_displacement_space<T>>, tensor_product<type_counter<T, I>, type_counter<Ts, Is>...>>
  {
    using type = tensor_product<type_counter<associated_displacement_space<T>, I-1>, type_counter<Ts, Is>...>;
  };

  /// Promote dual<T> to associated_displacement_space<T>
  template<class T, int I, class... Ts, int... Is>
  struct count_and_combine<dual<T>, tensor_product<type_counter<associated_displacement_space<T>, I>, type_counter<Ts, Is>...>>
  {
    using type = tensor_product<type_counter<associated_displacement_space<T>, I-1>, type_counter<Ts, Is>...>;
  };

  /// Promote dual<T> to dual<associated_displacement_space<T>>
  template<class T, int I, class... Ts, int... Is>
  struct count_and_combine<dual<associated_displacement_space<T>>, tensor_product<type_counter<dual<T>, I>, type_counter<Ts, Is>...>>
  {
    using type = tensor_product<type_counter<dual<associated_displacement_space<T>>, I+1>, type_counter<Ts, Is>...>;
  };

  /// Unpack counted types into direct products
  /// TO DO: Consider whether the unpacking roundtrip is necessary;
  /// it may be better just work in terms of type_counter.
  template<class...>
  struct unpack {};

  template<class... Ts>
  using unpack_t = unpack<Ts...>::type;

  template<class T, int I>
    requires (I > 0)
  struct unpack<type_counter<T, I>> : unpack<type_counter<T, I - 1>, tensor_product<T>>
  {};

  template<class T, int I>
    requires (I < 0)
  struct unpack<type_counter<T, I>> : unpack<type_counter<T, I + 1>, tensor_product<dual<T>>>
  {};

  template<class T, int I, class... Ts>
    requires (I > 0)
  struct unpack<type_counter<T, I>, tensor_product<Ts...>> : unpack<type_counter<T, I - 1>, tensor_product<T, Ts...>>
  {};

  template<class T, int I, class... Ts>
    requires (I < 0)
  struct unpack<type_counter<T, I>, tensor_product<Ts...>> : unpack<type_counter<T, I + 1>, tensor_product<dual<T>, Ts...>>
  {};

  template<class T, class... Ts>
  struct unpack<type_counter<T, 0>, tensor_product<Ts...>>
  {
    using type = tensor_product<Ts...>;
  };

  template<class... Ts, int... Is>
  struct unpack<tensor_product<type_counter<Ts, Is>...>>
    : meta::flatten<tensor_product<unpack_t<type_counter<Ts, Is>>...>>
  {
  };

  template<class>
  struct potentially_prunable : std::false_type {};

  template<class T>
  inline constexpr bool potentially_prunable_v{potentially_prunable<T>::value};

  template<class T>
  struct potentially_prunable<type_counter<T, 0>> : std::true_type
  {
  };

  template<class Arena, int I>
    requires (I != 0)
  struct potentially_prunable<type_counter<euclidean_vector_space<1, Arena>, I>> : std::true_type
  {
  };

  template<class Arena, int I>
    requires (I != 0)
  struct potentially_prunable<type_counter<dual<euclidean_vector_space<1, Arena>>, I>> : std::true_type
  {
  };

  template<class Arena, int I>
    requires (I != 0)
  struct potentially_prunable<type_counter<euclidean_nonnegative_space<1, Arena>, I>> : std::true_type
  {
  };

  template<class Arena, int I>
    requires (I != 0)
  struct potentially_prunable<type_counter<dual<euclidean_nonnegative_space<1, Arena>>, I>> : std::true_type
  {
  };

  template<physics::physical_unit U, int I>
    requires std::derived_from<U, no_unit_t> && (I != 0)
  struct potentially_prunable<type_counter<U, I>> : std::true_type
  {
  };

  template<class T>
  struct not_potentially_prunable : std::negation<potentially_prunable<T>> {};
  
  /// \class Primary class template for aiding the reduction of direct products to a lower dimensional space    
  template<class...>
  struct reduce;

  template<class... Ts>
  using reduce_t = reduce<Ts...>::type;

  template<physical_unit U>
  struct reduce<tensor_product<type_counter<U, 0>>>
  {
    using type = tensor_product<no_unit_t>;
  };

  template<physical_unit U, int I>
    requires std::derived_from<U, no_unit_t> && (I > 0)
  struct reduce<tensor_product<type_counter<U, I>>>
  {
    using type = tensor_product<U>;
  };

  template<physics::physical_unit... Ts, int... Is>
  struct reduce<tensor_product<type_counter<Ts, Is>...>>
  {
    using type = unpack_t<meta::filter_by_trait_t<tensor_product<type_counter<Ts, Is>...>, not_potentially_prunable>>;
  };

  template<partial_m_torsor... Ts, int... Is>
  struct reduce<tensor_product<type_counter<Ts, Is>...>>
  {
    // TO DO; potential problem here if reducible modules are floating-point but everything else is integral
    // Depends where we do any arithmetic promotions; ideally below using common_type
    // TO DO: At some point this may need generalizing to finite segments with a distinguished origin
    constexpr static bool anyOfNotReducibleFreeModule     {(( free_module<Ts>               && !potentially_prunable_v<type_counter<Ts, Is>>) || ...)};
    constexpr static bool anyOfNotReducibleHalfLine       {(( is_non_negative_orthant_v<Ts> && !potentially_prunable_v<type_counter<Ts, Is>>) || ...)};
    constexpr static bool allOfNotReducibleOrNotFreeModule{((!free_module<Ts>               || !potentially_prunable_v<type_counter<Ts, Is>>) && ...)};
    constexpr static bool allOfNotReducibleOrNotHalfLine  {((!is_non_negative_orthant_v<Ts> || !potentially_prunable_v<type_counter<Ts, Is>>) && ...)};
    
    using filtered_t = meta::filter_by_trait_t<tensor_product<type_counter<Ts, Is>...>, not_potentially_prunable>;

    using unpacked_t = unpack_t<filtered_t>;

    constexpr static bool anyFreeModule{(free_module<Ts> || ...)};
    
    using root_space_t =
      std::conditional_t<
        anyFreeModule,
        tensor_product<euclidean_vector_space<1, std::common_type_t<arena_type_of_t<Ts>...>>>,
        tensor_product<euclidean_half_line<std::common_type_t<arena_type_of_t<Ts>...>>>
      >;
    
    using type
      = std::conditional_t<
          anyOfNotReducibleFreeModule || (allOfNotReducibleOrNotFreeModule && (anyOfNotReducibleHalfLine || allOfNotReducibleOrNotHalfLine)),
          unpacked_t,
          meta::merge_t<unpacked_t, root_space_t, meta::type_comparator>
        >;
  };

  /// \class Primary class template to aid reduction of direct products and composite units
  template<class...>
  struct simplify;

  template<class... Ts>
  using simplify_t = simplify<Ts...>::type;

  template<class... Ts>
  struct simplify<tensor_product<Ts...>>
  {
    using type = reduction<meta::reverse_t<reduce_t<count_and_combine_t<meta::stable_sort_t<tensor_product<Ts...>, meta::type_comparator>>>>>;
  };

  // Assume tensor_products are already sorted
  template<class... Ts, class... Us>
  struct simplify<tensor_product<Ts...>, tensor_product<Us...>>
  {
    using type = reduction<meta::reverse_t<reduce_t<count_and_combine_t<meta::merge_t<tensor_product<Ts...>, tensor_product<Us...>, meta::type_comparator>>>>>;
  };

  template<class T>
  struct to_composite_space;

  template<partial_m_torsor... Ts>
  struct to_composite_space<reduction<tensor_product<Ts...>>>
  {
    using type = composite_space<Ts...>;
  };

  template<physical_unit... Ts>
  struct to_composite_space<reduction<tensor_product<Ts...>>>
  {
    using type = composite_unit<Ts...>;
  };

  template<representation... Ts>
  struct to_composite_space<reduction<tensor_product<Ts...>>>
  {
    using type = composite_representation<Ts...>;
  };

  template<class T>
  struct to_composite_space<reduction<tensor_product<T>>>
  {
    using type = T;
  };

  template<class T>
  using to_composite_space_t = to_composite_space<T>::type;
}

export namespace sequoia::maths
{
  template<partial_m_torsor... Ts>
  struct dual_of<physics::composite_space<Ts...>>
  {
    using type = physics::composite_space<dual_of_t<Ts>...>;
  };

  template<physics::physical_unit... Ts>
  struct dual_of<physics::composite_unit<Ts...>>
  {
    using type = physics::composite_unit<dual_of_t<Ts>...>;
  };

  template<representation... Ts>
  struct dual_of<physics::composite_representation<Ts...>>
  {
    using type = physics::composite_representation<dual_of_t<Ts>...>;
  };
}
