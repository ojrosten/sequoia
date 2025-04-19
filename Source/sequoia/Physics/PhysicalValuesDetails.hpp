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
  struct displacement_space;

  template<class Space>
  struct is_displacement_space : std::false_type {};

  template<class Space>
  struct is_displacement_space<displacement_space<Space>> : std::true_type {};

  template<class Space>
  using is_displacement_space_t = is_displacement_space<Space>::type;

  template<class Space>
  inline constexpr bool is_displacement_space_v{is_displacement_space<Space>::value};

  struct no_unit_t;

  template<class T>
  struct composite_unit;
    
  template<class T>
  concept physical_unit = requires {
    typename T::validator_type;
  };

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
    requires (!std::is_same_v<T, U>) && (!physics::is_displacement_space_v<T>) && (!physics::is_displacement_space_v<U>)
  struct type_comparator<T, maths::dual<U>> : std::bool_constant<type_name<T>() < type_name<U>()>
  {};

  template<class T, class U>
  struct type_comparator<maths::dual<T>, maths::dual<U>> : std::bool_constant<type_name<T>() < type_name<U>()>
  {};

  template<class T, class U>    
    requires (!maths::is_dual_v<U>)
  struct type_comparator<physics::displacement_space<T>, U> : std::bool_constant<type_name<T>() < type_name<U>()>
  {};

  template<class T, class U>
    requires (!maths::is_dual_v<T>)
  struct type_comparator<T, physics::displacement_space<U>> : std::bool_constant<type_name<T>() < type_name<U>()>
  {};

  template<class T, class U>
    requires (!std::is_same_v<physics::displacement_space<T>, U>) && (!physics::is_displacement_space_v<U>)
  struct type_comparator<physics::displacement_space<T>, maths::dual<U>> : std::bool_constant<type_name<T>() < type_name<U>()>
  {};

  template<class T, class U>
    requires (!std::is_same_v<T, physics::displacement_space<U>>)
  struct type_comparator<maths::dual<T>, physics::displacement_space<U>> : std::bool_constant<type_name<T>() < type_name<U>()>
  {};

  template<class T, class U>
    requires (!std::is_same_v<physics::displacement_space<T>, U>)
  struct type_comparator<maths::dual<physics::displacement_space<T>>, U>
  : std::bool_constant<type_name<T>() < type_name<U>()>
  {};

  template<class T, class U>
    requires (!std::is_same_v<T, physics::displacement_space<U>>) && (!physics::is_displacement_space_v<T>)
  struct type_comparator<T, maths::dual<physics::displacement_space<U>>>
  : std::bool_constant<type_name<T>() < type_name<U>()>
  {};

  template<class T, class U>
  struct type_comparator<physics::displacement_space<T>, physics::displacement_space<U>>
    : std::bool_constant<type_name<T>() < type_name<U>()>
  {};

  template<class T, class U>
  struct type_comparator<maths::dual<physics::displacement_space<T>>, maths::dual<physics::displacement_space<U>>>
    : std::bool_constant<type_name<T>() < type_name<U>()>
  {};

  // Ordering is T, displacement_space<T>, dual<T>, dual<displacement_space<T>>
  template<class T>
  struct type_comparator<T, maths::dual<T>> : std::true_type
  {};

  template<class T>
  struct type_comparator<maths::dual<T>, T> : std::false_type
  {};

  template<class T>
  struct type_comparator<T, physics::displacement_space<T>> : std::true_type
  {};

  template<class T>
  struct type_comparator<physics::displacement_space<T>, T> : std::false_type
  {};
    
  template<class T>
  struct type_comparator<T, maths::dual<physics::displacement_space<T>>> : std::true_type
  {};

  template<class T>
  struct type_comparator<maths::dual<physics::displacement_space<T>>, T> : std::false_type
  {};
    
  template<class T>
  struct type_comparator<physics::displacement_space<T>, maths::dual<T>> : std::true_type
  {};

  template<class T>
  struct type_comparator<maths::dual<T>, physics::displacement_space<T>> : std::false_type
  {};

  template<class T>
  struct type_comparator<maths::dual<T>, maths::dual<physics::displacement_space<T>>> : std::true_type
  {};

  template<class T>
  struct type_comparator<maths::dual<physics::displacement_space<T>>, maths::dual<T>> : std::false_type
  {};
}

namespace sequoia::physics::impl
{
  using namespace maths;

  template<class T, int I>
  struct type_counter {};

  /// \class Primary class template for counting and combinging instances of various types
  template<class...>
  struct count_and_combine;

  template<class... Ts>
  using count_and_combine_t = count_and_combine<Ts...>::type;

  template<>
  struct count_and_combine<std::tuple<>>
  {
    using type = std::tuple<>;
  };

  template<class T>
  struct count_and_combine<T>
  {
    using type = std::tuple<type_counter<T, 1>>;
  };

  template<class T>
  struct count_and_combine<std::tuple<T>>
  {
    using type = std::tuple<type_counter<T, 1>>;
  };
    
  template<class T, class... Ts>
  struct count_and_combine<std::tuple<T, Ts...>>
  {
    using type = count_and_combine_t<std::tuple<Ts...>, count_and_combine_t<T>>;
  };

  template<class T, class... Us, int... Is>
  struct count_and_combine<std::tuple<T>, std::tuple<type_counter<Us, Is>...>>
  {
    using type = count_and_combine_t<T, std::tuple<type_counter<Us, Is>...>>;
  };

  template<class T, class... Ts, class... Us, int... Is>
    requires (sizeof...(Ts) > 0)
  struct count_and_combine<std::tuple<T, Ts...>, std::tuple<type_counter<Us, Is>...>>
  {
    using type = count_and_combine_t<std::tuple<Ts...>, count_and_combine_t<T, std::tuple<type_counter<Us, Is>...>>>;
  };

  template<class S, class T, int I, class... Ts, int... Is>
    requires (!is_tuple_v<S> && !is_dual_v<S> && !std::is_same_v<S, T> && !std::is_same_v<S, displacement_space<T>>)
  struct count_and_combine<S, std::tuple<type_counter<T, I>, type_counter<Ts, Is>...>>
  {
    using type = std::tuple<type_counter<S, 1>, type_counter<T, I>, type_counter<Ts, Is>...>;
  };

  template<class S, class T, int I, class... Ts, int... Is>
    requires (!std::is_same_v<S, T> && !is_dual_v<T> && !is_displacement_space_v<S> && !is_displacement_space_v<T>)
  struct count_and_combine<dual<S>, std::tuple<type_counter<T, I>, type_counter<Ts, Is>...>>
  {
    using type = std::tuple<type_counter<dual<S>, 1>, type_counter<T, I>, type_counter<Ts, Is>...>;
  };

  template<class T, int I, class... Ts, int... Is>
  struct count_and_combine<T, std::tuple<type_counter<T, I>, type_counter<Ts, Is>...>>
  {
    using type = std::tuple<type_counter<T, I+1>, type_counter<Ts, Is>...>;
  };

  template<class T, int I, class... Ts, int... Is>
  struct count_and_combine<dual<T>, std::tuple<type_counter<T, I>, type_counter<Ts, Is>...>>
  {
    using type = std::tuple<type_counter<T, I-1>, type_counter<Ts, Is>...>;
  };

  /// Promote all T to displacement_space<T>
  template<class T, int I, class... Ts, int... Is>
  struct count_and_combine<displacement_space<T>, std::tuple<type_counter<T, I>, type_counter<Ts, Is>...>>
  {
    using type = std::tuple<type_counter<displacement_space<T>, I+1>, type_counter<Ts, Is>...>;
  };

  /// Promote all T to displacement_space<T>
  template<class T, int I, class... Ts, int... Is>
  struct count_and_combine<dual<displacement_space<T>>, std::tuple<type_counter<T, I>, type_counter<Ts, Is>...>>
  {
    using type = std::tuple<type_counter<displacement_space<T>, I-1>, type_counter<Ts, Is>...>;
  };

  /// Promote dual<T> to displacement_space<T>
  template<class T, int I, class... Ts, int... Is>
  struct count_and_combine<dual<T>, std::tuple<type_counter<displacement_space<T>, I>, type_counter<Ts, Is>...>>
  {
    using type = std::tuple<type_counter<displacement_space<T>, I-1>, type_counter<Ts, Is>...>;
  };

  /// Promote dual<T> to dual<displacement_space<T>>
  template<class T, int I, class... Ts, int... Is>
  struct count_and_combine<dual<displacement_space<T>>, std::tuple<type_counter<dual<T>, I>, type_counter<Ts, Is>...>>
  {
    using type = std::tuple<type_counter<dual<displacement_space<T>>, I+1>, type_counter<Ts, Is>...>;
  };

  /// Unpack counted types into tuples
  /// TO DO: Consider whether the unpacking roundtrip is necessary;
  /// it may be better just work in terms of type_counter.
  template<class...>
  struct unpack;

  template<class... Ts>
  using unpack_t = unpack<Ts...>::type;

  template<class T, int I>
    requires (I > 0)
  struct unpack<type_counter<T, I>>
  {
    using type = unpack_t<type_counter<T, I - 1>, std::tuple<T>>;
  };

  template<class T, int I>
    requires (I < 0)
  struct unpack<type_counter<T, I>>
  {
    using type = unpack_t<type_counter<T, I + 1>, std::tuple<dual<T>>>;
  };

  template<class T, int I, class... Ts>
    requires (I > 0)
  struct unpack<type_counter<T, I>, std::tuple<Ts...>>
  {
    using type = unpack_t<type_counter<T, I - 1>, std::tuple<T, Ts...>>;
  };

  template<class T, int I, class... Ts>
    requires (I < 0)
  struct unpack<type_counter<T, I>, std::tuple<Ts...>>
  {
    using type = unpack_t<type_counter<T, I + 1>, std::tuple<dual<T>, Ts...>>;
  };

  template<class T, class... Ts>
  struct unpack<type_counter<T, 0>, std::tuple<Ts...>>
  {
    using type = std::tuple<Ts...>;
  };

  /// \class Primary class template for aiding the reduction of direct products to a lower dimensional space    
  template<class...>
  struct reduce;

  template<class... Ts>
  using reduce_t = reduce<Ts...>::type;

  template<vector_space T>
  struct reduce<std::tuple<type_counter<T, 0>>>
  {
    using type = std::tuple<euclidean_vector_space<1, typename T::vector_space_type::field_type>>;
  };

  template<convex_space T>
    requires (!affine_space<T> && !vector_space<T>)
  struct reduce<std::tuple<type_counter<T, 0>>>
  {
    using type = std::tuple<euclidean_half_space<1, typename T::vector_space_type::field_type>>;
  };

  template<class T, class... Ts, int... Is>
  struct reduce<std::tuple<type_counter<T, 0>, type_counter<Ts, Is>...>>
  {
    using type = reduce_t<std::tuple<type_counter<Ts, Is>...>>;
  };

  template<class T, int I>
    requires (I != 0)
  struct reduce<std::tuple<type_counter<T, I>>>
  {
    using type = unpack_t<type_counter<T, I>>;
  };
 
  template<class T, int I, class... Ts, int... Is>
    requires (I != 0)
  struct reduce<std::tuple<type_counter<T, I>, type_counter<Ts, Is>...>>
  {
    using type = reduce_t<std::tuple<type_counter<Ts, Is>...>, unpack_t<type_counter<T, I>>>;
  };

  template<class T, class... Ts, int... Is, class... Us>
  struct reduce<std::tuple<type_counter<T, 0>, type_counter<Ts, Is>...>, std::tuple<Us...>>
  {
    using type = reduce_t<std::tuple<type_counter<Ts, Is>...>, std::tuple<Us...>>;
  };

  template<class T, int I, class... Ts, int... Is, class... Us>
    requires (I != 0)
  struct reduce<std::tuple<type_counter<T, I>, type_counter<Ts, Is>...>, std::tuple<Us...>>
  {
    using type = reduce_t<std::tuple<type_counter<Ts, Is>...>, unpack_t<type_counter<T, I>, std::tuple<Us...>>>;
  };

  template<class... Us>
  struct reduce<std::tuple<>, std::tuple<Us...>>
  {
    using type = std::tuple<Us...>;
  };

  template<physics::physical_unit T>
  struct reduce<std::tuple<type_counter<T, 0>>>
  {
    using type = std::tuple<physics::no_unit_t>;
  };

  /// \class Primary class template to aid reduction of direct products and composite units
  template<class>
  struct simplify;

  template<class T>
  using simplify_t = simplify<T>::type;

  template<class... Ts>
  struct simplify<std::tuple<Ts...>>
  {
    using type = reduce_t<count_and_combine_t<std::tuple<Ts...>>>;
  };

  template<convex_space... Ts>
  struct simplify<direct_product<std::tuple<Ts...>>>
  {
    using reduced_tuple_type = simplify_t<std::tuple<Ts...>>;
    using type = std::conditional_t<std::tuple_size_v<reduced_tuple_type> == 1,
                                    std::tuple_element_t<0, reduced_tuple_type>,
                                    direct_product<reduced_tuple_type>>;
  };
     
  template<physics::physical_unit... Ts>
  struct simplify<physics::composite_unit<std::tuple<Ts...>>>
  {
    using reduced_tuple_type = simplify_t<std::tuple<Ts...>>;
    using type = std::conditional_t<std::tuple_size_v<reduced_tuple_type> == 1,
                                    std::tuple_element_t<0, reduced_tuple_type>,
                                    physics::composite_unit<reduced_tuple_type>>;
  };
}

namespace sequoia::maths
{
  template<convex_space... Ts>
  struct dual_of<physics::reduction<direct_product<std::tuple<Ts...>>>>
  {
    using type = physics::reduction<direct_product<std::tuple<dual_of_t<Ts>...>>>;
  };

  template<physics::physical_unit... Ts>
  struct dual_of<physics::composite_unit<std::tuple<Ts...>>>
  {
    using type = physics::composite_unit<std::tuple<dual_of_t<Ts>...>>;
  };
}
