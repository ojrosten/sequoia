////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include <array>
#include <concepts>
#include <iterator>
#include <type_traits>
#include <tuple>
#include <utility>
#include <variant>

/*! \file
    \brief Traits which are sufficiently general to appear in the `sequoia` namespace.

 */

namespace sequoia
{
  /*! @defgroup variadic_traits The variadic_traits Group
      This group provides mechanisms for extracting the head and tail from a parameter pack.
      @{
   */

  namespace impl
  {
    template<class H, class... T>
    struct variadic_traits_helper
    {
      using head = H;
      using tail = variadic_traits_helper<T...>;
    };

    template<class H>
    struct variadic_traits_helper<H>
    {
      using head = H;
      using tail = void;
    };
  }

  /*! \brief Primary template for extracting the head/tail from a parameter pack. */
  template<class... T>
  struct variadic_traits
  {
    using head = typename impl::variadic_traits_helper<T...>::head;
    using tail = typename impl::variadic_traits_helper<T...>::tail;
  };

  template<>
  struct variadic_traits<>
  {
    using head = void;
    using tail = void;
  };

  /*! Helper to conveniently extract the head-type from a parameter pack */
  template<class... T>
  using head_of_t = typename variadic_traits<T...>::head;

  /*! @} */ // end of variadic_traits Group

  /*! \brief class template for determining if a type is the base class of the head of a parameter pack */
  template<class T, class... Args>
  struct is_base_of_head
    : std::is_base_of<std::remove_cvref_t<T>, std::remove_cvref_t<head_of_t<Args...>>>
  {};

  template<class T, class... Args>
  inline constexpr bool is_base_of_head_v{is_base_of_head<T, Args...>::value};

  template<class T, class... Args>
  using is_base_of_head_t = typename is_base_of_head<T, Args...>::type;


  /*! \brief class template for determining whether a constructor template should resolve to the copy constructor */
  template<class T, class... Args>
  struct resolve_to_copy
    : std::bool_constant<
           (sizeof...(Args) == 1)
        && (   std::is_same_v<std::remove_cvref_t<typename variadic_traits<Args...>::head>, std::remove_cvref_t<T>>
            || is_base_of_head_v<T, Args...>)
      >
  {};

  template<class T, class... Args>
  inline constexpr bool resolve_to_copy_v{resolve_to_copy<T, Args...>::value};

  template<class T, class... Args>
  using resolve_to_copy_t = typename resolve_to_copy<T, Args...>::type;


  /*! \brief Primary template for determining if a type is a pointer */
  template<class T>
  struct is_const_pointer : std::false_type {};

  template<class T>
  struct is_const_pointer<const T*> : std::true_type {};

  template<class T>
  struct is_const_pointer<const T* const> : std::true_type {};

  template<class T>
  inline constexpr bool is_const_pointer_v{is_const_pointer<T>::value};

  template<class T>
  using is_const_pointer_t = typename is_const_pointer<T>::type;

  // is_const_reference

  template<class T>
  struct is_const_reference
    : std::bool_constant<std::is_reference_v<T> && std::is_const_v<std::remove_reference_t<T>>>
  {};

  template<class T>
  inline constexpr bool is_const_reference_v{is_const_reference<T>::value};

  template<class T>
  using is_const_reference_t = typename is_const_reference<T>::type;

  // is_tuple

  template<class T>
  struct is_tuple : std::false_type{};

  template<class... Ts>
  struct is_tuple<std::tuple<Ts...>> : std::true_type{};

  template<class... Ts>
  using is_tuple_t = typename is_tuple<Ts...>::type;

  template<class... Ts>
  inline constexpr bool is_tuple_v{is_tuple<Ts...>::value};

  // dependent_false

  template<class T>
  struct dependent_false : std::false_type {};

  // has_allocator_type_v

  template<class T>
  struct has_allocator_type : std::bool_constant< requires { typename T::allocator_type; } >
  {};

  template<class T>
  using has_allocator_type_t = typename has_allocator_type<T>::type;

  template<class T>
  inline constexpr bool has_allocator_type_v{has_allocator_type<T>::value};

  // Machinery for deep equality checking, which will hopefully one day
  // be obviated if the stl properly constrains operator==

  /*! @defgroup deep_equality The deep_equality Group
      The stl currently underconstrains `operator==`. For example, consider
      a type, `T`, which is not equality comparable. Nevertheless, In C++20, `std::vector<T>`
      counter-intuitively satisfies the `std::equality_comparable` concept.
      To work around this defect, machinery is provided for checking deep equality.
      This exploits the uniformity of the stl:
        - If a type defines a nested type `value_type` it is treated as a homogeneous container.
        - If a type does not defines a nested type `value_type` but elements may be accessed with
          `std::get` it is treated as a heterogeneous container.
      @{
   */

  template<class T>
  inline constexpr bool has_value_type{ requires { typename T::value_type; }};

  template<class T>
  inline constexpr bool has_gettable_elements{requires (T & t) { std::get<0>(t); }};

  template<class T>
  struct is_deep_equality_comparable;

  template<class T, std::size_t... I>
  inline constexpr bool heterogeneous_deep_equality_v{
    requires(T & t, std::index_sequence<I...>) {
      requires (is_deep_equality_comparable<std::remove_cvref_t<decltype(std::get<I>(t))>>::value && ...);
    }
  };

  template<class T, std::size_t...I>
  constexpr bool has_heterogeneous_deep_equality(std::index_sequence<I...>)
  {
    return heterogeneous_deep_equality_v<T, I...>;
  }

  template<class T>
  struct heterogeneous_deep_equality;

  template<template<class...> class T, class... Ts>
  struct heterogeneous_deep_equality<T<Ts...>>
    : std::bool_constant<has_heterogeneous_deep_equality<T<Ts...>>(std::make_index_sequence<sizeof...(Ts)>{})>
  {};

  template<class T>
  struct is_deep_equality_comparable : std::bool_constant<std::equality_comparable<T>>
  {};

  template<class T>
    requires has_value_type<T>
  struct is_deep_equality_comparable<T> : std::bool_constant<std::equality_comparable<T>&& is_deep_equality_comparable<typename T::value_type>::value>
  {};

  template<template<class...> class T, class... Ts>
    requires (!has_value_type<T<Ts...>> && has_gettable_elements<T<Ts...>>)
  struct is_deep_equality_comparable<T<Ts...>> : std::bool_constant<std::equality_comparable<T<Ts...>> && heterogeneous_deep_equality<T<Ts...>>::value>
  {};

  template<class T>
  inline constexpr bool is_deep_equality_comparable_v{is_deep_equality_comparable<T>::value};

  template<class T>
  using is_deep_equality_comparable_t = typename is_deep_equality_comparable<T>::type;

  /*! @} */
}
