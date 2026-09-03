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

/** \file
    \brief Traits which are sufficiently general to appear in the `sequoia` namespace.

 */

namespace sequoia
{
  /** \brief Standard meta-programming utility */
  template<class T>
  struct dependent_false : std::false_type {};

  /** \brief class template for determining whether a constructor template should resolve to the copy constructor */
  template<class T, class... Args>
  struct resolve_to_copy
    : std::bool_constant<
           (sizeof...(Args) == 1)
        && (std::is_same_v<std::remove_cvref_t<Args>, std::remove_cvref_t<T>> && ...)
      >
  {};

  template<class T, class... Args>
  inline constexpr bool resolve_to_copy_v{resolve_to_copy<T, Args...>::value};

  template<class T, class... Args>
  using resolve_to_copy_t = resolve_to_copy<T, Args...>::type;


  /** \brief Primary class template for determining if a type is a `const` pointer */
  template<class T>
  struct is_const_pointer : std::false_type {};

  template<class T>
  struct is_const_pointer<const T*> : std::true_type {};

  template<class T>
  struct is_const_pointer<const T* volatile> : std::true_type {};

  template<class T>
  struct is_const_pointer<const T* const> : std::true_type {};

  template<class T>
  struct is_const_pointer<const T* const volatile> : std::true_type {};

  template<class T>
  inline constexpr bool is_const_pointer_v{is_const_pointer<T>::value};

  template<class T>
  using is_const_pointer_t = is_const_pointer<T>::type;

  /** \brief class template for determining if a type is a `const` reference */
  template<class T>
  struct is_const_reference
    : std::bool_constant<std::is_reference_v<T> && std::is_const_v<std::remove_reference_t<T>>>
  {};

  template<class T>
  inline constexpr bool is_const_reference_v{is_const_reference<T>::value};

  template<class T>
  using is_const_reference_t = is_const_reference<T>::type;

  /** \brief Primary class template for determining if a type is a `std::tuple` */
  template<class T>
  struct is_tuple : std::false_type{};

  template<class... Ts>
  struct is_tuple<std::tuple<Ts...>> : std::true_type{};

  template<class... Ts>
  using is_tuple_t = is_tuple<Ts...>::type;

  template<class... Ts>
  inline constexpr bool is_tuple_v{is_tuple<Ts...>::value};

  /** \brief Primary class template for determining if a type can be brace-initialized
      by Args...
 ` */
  template<class T, class... Args>
  struct is_initializable : std::false_type
  {};

  template<class T, class... Args>
    requires requires (Args&&... args) { T{std::forward<Args>(args)...}; }
  struct is_initializable<T, Args...> : std::true_type {};

  template<class T, class... Args>
  using is_initializable_t = is_initializable<T, Args...>::type;

  template<class T, class... Args>
  inline constexpr bool is_initializable_v{is_initializable<T, Args...>::value};


  /** \brief Class template for determining if a type defines a nested type `allocator_type` */
  template<class T>
  struct has_allocator_type : std::bool_constant< requires { typename T::allocator_type; } >
  {};

  template<class T>
  using has_allocator_type_t = has_allocator_type<T>::type;

  template<class T>
  inline constexpr bool has_allocator_type_v{has_allocator_type<T>::value};

  /** \brief Checks for dependent type `value_type` */
  template<class T>
  inline constexpr bool has_value_type_v{requires { typename T::value_type; }};

  /** \brief Extracts the dependent type `value_type` */
  template<class T>
    requires has_value_type_v<T>
  struct value_type_of
  {
    using type = T::value_type;
  };

  template<class T>
  using value_type_of_t = value_type_of<T>::type;

  /** \brief Checks for dependent type `element_type` */
  template<class T>
  inline constexpr bool has_element_type_v{requires { typename T::element_type; }};

  // Machinery for deep equality checking, which will hopefully one day
  // be obviated if the stl properly constrains operator==

  /** @defgroup deep_equality The deep_equality Group
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
  inline constexpr bool has_gettable_elements{requires (T & t) { std::get<0>(t); }};

  /** \brief The `I`th element of a heterogeneous container, as a value type.

      Named rather than spelled inline at each use, where the same `decltype` appeared twice.
      Backported from `modules-native`, which needs it for a further reason that does not apply
      here: MSVC evaluates the inline form as `false` for `std::variant` when the enclosing
      variable template is instantiated across a module boundary. Reduced repro in `sequoia-LLM`,
      `msvc-bugs/E-module-fold-get.cpp`.
   */
  template<class T, std::size_t I>
  using gettable_element_t = std::remove_cvref_t<decltype(std::get<I>(std::declval<T&>()))>;

  template<class T>
  struct is_deep_equality_comparable;

  template<class T>
  inline constexpr bool is_deep_equality_comparable_v{is_deep_equality_comparable<T>::value};

  template<class T>
  using is_deep_equality_comparable_t = is_deep_equality_comparable<T>::type;

  template<class T, std::size_t... I>
  inline constexpr bool heterogeneous_deep_equality_v{
    requires(T & t, std::index_sequence<I...>) {
      requires (is_deep_equality_comparable_v<gettable_element_t<T, I>> && ...);
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
  struct is_deep_equality_comparable : std::bool_constant<std::equality_comparable<T> && !std::is_array_v<T>>
  {};

  template<class T>
    requires has_value_type_v<T>
  struct is_deep_equality_comparable<T> : std::bool_constant<std::equality_comparable<T>&& is_deep_equality_comparable<typename T::value_type>::value>
  {};

  template<template<class...> class T, class... Ts>
    requires (!has_value_type_v<T<Ts...>> && has_gettable_elements<T<Ts...>>)
  struct is_deep_equality_comparable<T<Ts...>> : std::bool_constant<std::equality_comparable<T<Ts...>> && heterogeneous_deep_equality<T<Ts...>>::value>
  {};

  /** @} */ // end of deep_equality group

  /** @defgroup deep_totally_ordered The deep_totally_ordered Group */
  
  template<class T>
  struct is_deep_totally_ordered;

  template<class T>
  inline constexpr bool is_deep_totally_ordered_v{is_deep_totally_ordered<T>::value};

  template<class T>
  using is_deep_totally_ordered_t = is_deep_totally_ordered<T>::type;

  template<class T, std::size_t... I>
  inline constexpr bool heterogeneous_deep_total_order_v{
    requires(T & t, std::index_sequence<I...>) {
      requires (is_deep_totally_ordered_v<gettable_element_t<T, I>> && ...);
    }
  };

  template<class T, std::size_t...I>
  constexpr bool has_heterogeneous_deep_total_order(std::index_sequence<I...>)
  {
    return heterogeneous_deep_total_order_v<T, I...>;
  }

  template<class T>
  struct heterogeneous_deep_total_order;

  template<template<class...> class T, class... Ts>
  struct heterogeneous_deep_total_order<T<Ts...>>
    : std::bool_constant<has_heterogeneous_deep_total_order<T<Ts...>>(std::make_index_sequence<sizeof...(Ts)>{})>
  {};

  template<class T>
  struct is_deep_totally_ordered : std::bool_constant<std::totally_ordered<T> && !std::is_array_v<T>>
  {};

  template<class T>
    requires has_value_type_v<T>
  struct is_deep_totally_ordered<T> : std::bool_constant<std::totally_ordered<T>&& is_deep_totally_ordered<typename T::value_type>::value>
  {};

  template<template<class...> class T, class... Ts>
    requires (!has_value_type_v<T<Ts...>> && has_gettable_elements<T<Ts...>>)
  struct is_deep_totally_ordered<T<Ts...>> : std::bool_constant<std::totally_ordered<T<Ts...>> && heterogeneous_deep_total_order<T<Ts...>>::value>
  {};
  
  /** @} */ // end of deep_totally_ordered group

  /** \brief class template for determining if a type is compatible with a floating-point type.
  
      The goal is to forbid e.g. multiplication of a float by a double, but to allow multiplication
      of a float by an integral type, even though the latter operation may also lead to a loss of
      precision. Thus, for an instance `x` of a type which both wraps a float and defines *=, this
      utility can be used to allow x *= 2 but forbid x *= 2.2. The idea is to strike a balance 
      between ergonomics and loss of precision.
   */

  template<std::floating_point T, class U>
  struct is_compatible : std::bool_constant<!std::is_same_v<U, bool> && ((std::floating_point<U> && is_initializable_v<T, U>) || std::is_integral_v<U>)> {};

  template<std::floating_point T, class U>
  using is_compatible_t = is_compatible<T, U>::type;

  template<std::floating_point T, class U>
  inline constexpr bool is_compatible_v{is_compatible<T, U>::value};

  /** \brief Trait for determining if the elements of a set of types are all the same
      
      The set must have at least one element, and in this case the trait is
      convertible to std::true_type.
   */
    
  template<class T, class... Ts>
  struct are_same : std::bool_constant<(std::same_as<T, Ts> && ...)> {};

  template<class T, class... Ts>
  using are_same_t = are_same<T, Ts...>::type;

  template<class T, class... Ts>
  constexpr inline bool are_same_v{are_same<T, Ts...>::value};
}
