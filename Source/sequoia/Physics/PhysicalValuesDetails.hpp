////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/Core/Meta/TypeAlgorithms.hpp"
#include "sequoia/Maths/Geometry/Spaces.hpp"

namespace sequoia::physics
{
  template<class Space>
  struct associated_displacement_space;

  template<class Space>
  struct is_associated_displacement_space : std::false_type {};

  template<class Space>
  struct is_associated_displacement_space<associated_displacement_space<Space>> : std::true_type {};

  template<class Space>
  using is_associated_displacement_space_t = is_associated_displacement_space<Space>::type;

  template<class Space>
  inline constexpr bool is_associated_displacement_space_v{is_associated_displacement_space<Space>::value};

  template<class T>
  inline constexpr bool identifies_as_unit_v{
    requires {
      typename T::is_unit;
      requires std::convertible_to<typename T::is_unit, std::true_type>;
    }
  };

  template<class T>
  concept physical_unit
    =    identifies_as_unit_v<T>
      && requires { typename T::validator_type; };

  struct no_unit_t;

  template<class... Ts>
  struct composite_space;

  template<physical_unit... Ts>
  struct composite_unit;

  template<class T>
  struct reduction;
}

namespace sequoia::meta
{
  template<class T, class U>
    requires (!std::is_same_v<T, U>)
  struct type_comparator<maths::dual<T>, U> : std::bool_constant<type_name<T>() < type_name<U>()>
  {};

  template<class T, class U>
    requires (!std::is_same_v<T, U>) && (!physics::is_associated_displacement_space_v<T>) && (!physics::is_associated_displacement_space_v<U>)
  struct type_comparator<T, maths::dual<U>> : std::bool_constant<type_name<T>() < type_name<U>()>
  {};

  template<class T, class U>
  struct type_comparator<maths::dual<T>, maths::dual<U>> : std::bool_constant<type_name<T>() < type_name<U>()>
  {};

  template<class T, class U>    
    requires (!maths::is_dual_v<U>)
  struct type_comparator<physics::associated_displacement_space<T>, U> : std::bool_constant<type_name<T>() < type_name<U>()>
  {};

  template<class T, class U>
    requires (!maths::is_dual_v<T>)
  struct type_comparator<T, physics::associated_displacement_space<U>> : std::bool_constant<type_name<T>() < type_name<U>()>
  {};

  template<class T, class U>
    requires (!std::is_same_v<physics::associated_displacement_space<T>, U>) && (!physics::is_associated_displacement_space_v<U>)
  struct type_comparator<physics::associated_displacement_space<T>, maths::dual<U>> : std::bool_constant<type_name<T>() < type_name<U>()>
  {};

  template<class T, class U>
    requires (!std::is_same_v<T, physics::associated_displacement_space<U>>)
  struct type_comparator<maths::dual<T>, physics::associated_displacement_space<U>> : std::bool_constant<type_name<T>() < type_name<U>()>
  {};

  template<class T, class U>
    requires (!std::is_same_v<physics::associated_displacement_space<T>, U>)
  struct type_comparator<maths::dual<physics::associated_displacement_space<T>>, U>
  : std::bool_constant<type_name<T>() < type_name<U>()>
  {};

  template<class T, class U>
    requires (!std::is_same_v<T, physics::associated_displacement_space<U>>) && (!physics::is_associated_displacement_space_v<T>)
  struct type_comparator<T, maths::dual<physics::associated_displacement_space<U>>>
  : std::bool_constant<type_name<T>() < type_name<U>()>
  {};

  template<class T, class U>
  struct type_comparator<physics::associated_displacement_space<T>, physics::associated_displacement_space<U>>
    : std::bool_constant<type_name<T>() < type_name<U>()>
  {};

  template<class T, class U>
  struct type_comparator<maths::dual<physics::associated_displacement_space<T>>, maths::dual<physics::associated_displacement_space<U>>>
    : std::bool_constant<type_name<T>() < type_name<U>()>
  {};

  // Ordering is T, associated_displacement_space<T>, dual<T>, dual<associated_displacement_space<T>>
  template<class T>
  struct type_comparator<T, maths::dual<T>> : std::true_type
  {};

  template<class T>
  struct type_comparator<maths::dual<T>, T> : std::false_type
  {};

  template<class T>
  struct type_comparator<T, physics::associated_displacement_space<T>> : std::true_type
  {};

  template<class T>
  struct type_comparator<physics::associated_displacement_space<T>, T> : std::false_type
  {};
    
  template<class T>
  struct type_comparator<T, maths::dual<physics::associated_displacement_space<T>>> : std::true_type
  {};

  template<class T>
  struct type_comparator<maths::dual<physics::associated_displacement_space<T>>, T> : std::false_type
  {};
    
  template<class T>
  struct type_comparator<physics::associated_displacement_space<T>, maths::dual<T>> : std::true_type
  {};

  template<class T>
  struct type_comparator<maths::dual<T>, physics::associated_displacement_space<T>> : std::false_type
  {};

  template<class T>
  struct type_comparator<maths::dual<T>, maths::dual<physics::associated_displacement_space<T>>> : std::true_type
  {};

  template<class T>
  struct type_comparator<maths::dual<physics::associated_displacement_space<T>>, maths::dual<T>> : std::false_type
  {};
}

namespace sequoia::physics::impl
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
  struct count_and_combine<direct_product<>>
  {
    using type = direct_product<>;
  };

  template<class T>
  struct count_and_combine<T>
  {
    using type = direct_product<type_counter<T, 1>>;
  };

  template<class T>
  struct count_and_combine<direct_product<T>>
  {
    using type = direct_product<type_counter<T, 1>>;
  };
    
  template<class T, class... Ts>
  struct count_and_combine<direct_product<T, Ts...>>
    : count_and_combine<direct_product<Ts...>, count_and_combine_t<T>> 
  {};

  template<class T, class... Us, int... Is>
  struct count_and_combine<direct_product<T>, direct_product<type_counter<Us, Is>...>>
    : count_and_combine<T, direct_product<type_counter<Us, Is>...>>
  {};

  template<class T, class... Ts, class... Us, int... Is>
    requires (sizeof...(Ts) > 0)
  struct count_and_combine<direct_product<T, Ts...>, direct_product<type_counter<Us, Is>...>>
    : count_and_combine<direct_product<Ts...>, count_and_combine_t<T, direct_product<type_counter<Us, Is>...>>>
  {};

  template<class S, class T, int I, class... Ts, int... Is>
    requires (!is_direct_product_v<S> && !is_dual_v<S> && !std::is_same_v<S, T> && !std::is_same_v<S, associated_displacement_space<T>>)
  struct count_and_combine<S, direct_product<type_counter<T, I>, type_counter<Ts, Is>...>>
  {
    using type = direct_product<type_counter<S, 1>, type_counter<T, I>, type_counter<Ts, Is>...>;
  };

  template<class S, class T, int I, class... Ts, int... Is>
    requires (!std::is_same_v<S, T> && !is_dual_v<T> && !is_associated_displacement_space_v<S> && !is_associated_displacement_space_v<T>)
  struct count_and_combine<dual<S>, direct_product<type_counter<T, I>, type_counter<Ts, Is>...>>
  {
    using type = direct_product<type_counter<dual<S>, 1>, type_counter<T, I>, type_counter<Ts, Is>...>;
  };

  template<class T, int I, class... Ts, int... Is>
  struct count_and_combine<T, direct_product<type_counter<T, I>, type_counter<Ts, Is>...>>
  {
    using type = direct_product<type_counter<T, I+1>, type_counter<Ts, Is>...>;
  };

  template<class T, int I, class... Ts, int... Is>
  struct count_and_combine<dual<T>, direct_product<type_counter<T, I>, type_counter<Ts, Is>...>>
  {
    using type = direct_product<type_counter<T, I-1>, type_counter<Ts, Is>...>;
  };

  /// Promote all T to associated_displacement_space<T>
  template<class T, int I, class... Ts, int... Is>
  struct count_and_combine<associated_displacement_space<T>, direct_product<type_counter<T, I>, type_counter<Ts, Is>...>>
  {
    using type = direct_product<type_counter<associated_displacement_space<T>, I+1>, type_counter<Ts, Is>...>;
  };

  /// Promote all T to associated_displacement_space<T>
  template<class T, int I, class... Ts, int... Is>
  struct count_and_combine<dual<associated_displacement_space<T>>, direct_product<type_counter<T, I>, type_counter<Ts, Is>...>>
  {
    using type = direct_product<type_counter<associated_displacement_space<T>, I-1>, type_counter<Ts, Is>...>;
  };

  /// Promote dual<T> to associated_displacement_space<T>
  template<class T, int I, class... Ts, int... Is>
  struct count_and_combine<dual<T>, direct_product<type_counter<associated_displacement_space<T>, I>, type_counter<Ts, Is>...>>
  {
    using type = direct_product<type_counter<associated_displacement_space<T>, I-1>, type_counter<Ts, Is>...>;
  };

  /// Promote dual<T> to dual<associated_displacement_space<T>>
  template<class T, int I, class... Ts, int... Is>
  struct count_and_combine<dual<associated_displacement_space<T>>, direct_product<type_counter<dual<T>, I>, type_counter<Ts, Is>...>>
  {
    using type = direct_product<type_counter<dual<associated_displacement_space<T>>, I+1>, type_counter<Ts, Is>...>;
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
  struct unpack<type_counter<T, I>> : unpack<type_counter<T, I - 1>, direct_product<T>>
  {};

  template<class T, int I>
    requires (I < 0)
  struct unpack<type_counter<T, I>> : unpack<type_counter<T, I + 1>, direct_product<dual<T>>>
  {};

  template<class T, int I, class... Ts>
    requires (I > 0)
  struct unpack<type_counter<T, I>, direct_product<Ts...>> : unpack<type_counter<T, I - 1>, direct_product<T, Ts...>>
  {};

  template<class T, int I, class... Ts>
    requires (I < 0)
  struct unpack<type_counter<T, I>, direct_product<Ts...>> : unpack<type_counter<T, I + 1>, direct_product<dual<T>, Ts...>>
  {};

  template<class T, class... Ts>
  struct unpack<type_counter<T, 0>, direct_product<Ts...>>
  {
    using type = direct_product<Ts...>;
  };

  template<class... Ts, int... Is>
  //  requires (sizeof...(Ts) > 1)
  struct unpack<direct_product<type_counter<Ts, Is>...>>
    : meta::flatten<direct_product<unpack_t<type_counter<Ts, Is>>...>>
  {
  };

  template<class>
  struct maximally_reducible : std::false_type {};

  template<class T>
  inline constexpr bool maximally_reducible_v{maximally_reducible<T>::value};

  template<class T>
  struct maximally_reducible<type_counter<T, 0>> : std::true_type
  {
  };

  template<class T, class Arena, int I>
    requires (I != 0)
  struct maximally_reducible<type_counter<euclidean_vector_space<T, 1, Arena>, I>> : std::true_type
  {
  };

  template<class T, class Arena, int I>
    requires (I != 0)
  struct maximally_reducible<type_counter<euclidean_half_space<T, Arena>, I>> : std::true_type
  {
  };

  template<physics::physical_unit U, int I>
    requires std::derived_from<U, no_unit_t> && (I != 0)
  struct maximally_reducible<type_counter<U, I>> : std::true_type
  {
  };

  template<class T>
  struct not_maximally_reducible : std::negation<maximally_reducible<T>> {};
  
  /// \class Primary class template for aiding the reduction of direct products to a lower dimensional space    
  template<class...>
  struct reduce;

  template<class... Ts>
  using reduce_t = reduce<Ts...>::type;

  template<vector_space T>
  struct reduce<direct_product<type_counter<T, 0>>>
  {
    using arena_type = T::arena_type;
    using type = direct_product<euclidean_vector_space<commutative_ring_type_of_t<T>, 1, arena_type>>;
  };

  template<class T, class Arena, int I>
  struct reduce<direct_product<type_counter<euclidean_vector_space<T, 1, Arena>, I>>>
  {
    using type = direct_product<euclidean_vector_space<T, 1, Arena>>;
  };
  
  template<convex_space T>
    requires (!affine_space<T> && !vector_space<T>)
  struct reduce<direct_product<type_counter<T, 0>>>
  {
    using arena_type = T::arena_type;
    using type = direct_product<euclidean_half_space<commutative_ring_type_of_t<free_module_type_of_t<T>>, arena_type>>;
  };

  template<class T, class Arena, int I>
  struct reduce<direct_product<type_counter<euclidean_half_space<T, Arena>, I>>>
  {
    using type = direct_product<euclidean_half_space<T, Arena>>;
  };

  // TO DO: check that count_and_combine recognizes euc_vec as the displacement space of euc_half

  template<physics::physical_unit U>
  struct reduce<direct_product<type_counter<U, 0>>>
  {
    using type = direct_product<no_unit_t>;
  };

  /*template<physics::physical_unit U, int I>
    requires std::derived_from<U, no_unit_t> && (I > 0)
  struct reduce<direct_product<type_counter<U, I>>>
  {
    using type = direct_product<U>;
    };*/

  template<int I>
  struct reduce<direct_product<type_counter<no_unit_t, I>>>
  {
    using type = direct_product<no_unit_t>;
  }; 

  template<class... Ts, int... Is>
  struct reduce<direct_product<type_counter<Ts, Is>...>>
  {
    using type = unpack_t<meta::filter_by_trait_t<direct_product<type_counter<Ts, Is>...>, not_maximally_reducible>>;
  };

  template<class, class>
  struct concat;

  template<class T, class U>
  using concat_t = concat<T, U>::type;

  template<template<class...> class TT, class... Ts, class... Us>
  struct concat<TT<Ts...>, TT<Us...>>
  {
    using type = TT<Ts..., Us...>;
  };

  template<class> struct reverse;

  template<class T>
  using reverse_t = typename reverse<T>::type;

  template<template<class...> class TT>
  struct reverse<TT<>>
  {
    using type = TT<>;
  };

  template<template<class...> class TT, class T>
  struct reverse<TT<T>>
  {
    using type = TT<T>;
  };

  template<template<class...> class TT, class T, class... Ts>
  struct reverse<TT<T, Ts...>>
  {
    using type = concat_t<reverse_t<TT<Ts...>>, TT<T>>;
  };

  /// \class Primary class template to aid reduction of direct products and composite units
  template<class...>
  struct simplify;

  template<class... Ts>
  using simplify_t = simplify<Ts...>::type;

  template<class... Ts>
  struct simplify<direct_product<Ts...>>
  {
    using type = reduction<reverse_t<reduce_t<count_and_combine_t<meta::stable_sort_t<direct_product<Ts...>, meta::type_comparator>>>>>;
  };

  // Assume direct_products are already sorted
  template<class... Ts, class... Us>
  struct simplify<direct_product<Ts...>, direct_product<Us...>>
  {
    using type = reduction<reverse_t<reduce_t<count_and_combine_t<meta::merge_t<direct_product<Ts...>, direct_product<Us...>, meta::type_comparator>>>>>;
  };

  template<class T>
  struct to_composite_space;

  template<convex_space... Ts>
  struct to_composite_space<reduction<direct_product<Ts...>>>
  {
    using type = composite_space<Ts...>;
  };

  template<physical_unit... Ts>
  struct to_composite_space<reduction<direct_product<Ts...>>>
  {
    using type = composite_unit<Ts...>;
  };

  template<class T>
  struct to_composite_space<reduction<direct_product<T>>>
  {
    using type = T;
  };

  template<class T>
  using to_composite_space_t = to_composite_space<T>::type;
}

namespace sequoia::maths
{
  template<convex_space... Ts>
  struct dual_of<physics::composite_space<Ts...>>
  {
    using type = physics::composite_space<dual_of_t<Ts>...>;
  };

  template<physics::physical_unit... Ts>
  struct dual_of<physics::composite_unit<Ts...>>
  {
    using type = physics::composite_unit<dual_of_t<Ts>...>;
  };
}
