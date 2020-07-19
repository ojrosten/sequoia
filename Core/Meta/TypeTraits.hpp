////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include <type_traits>
#include <iterator>

/*! \file TypeTraits.hpp
    \brief Traits added as required by other components of the library.

 */

namespace sequoia
{
  namespace impl
  {
    template<class H, class... T>
    struct variadic_traits_helper
    {
      using head = H;
      using tail = variadic_traits_helper<T...>;

      static constexpr std::size_t size() noexcept
      {
        return 1 + variadic_traits_helper<T...>::size();
      }
    };

    template<class H>
    struct variadic_traits_helper<H>
    {
      using head = H;
      using tail = void;
      static constexpr std::size_t size() noexcept
      {
        return 1;
      }
    };
  }

  template<class... T>
  struct variadic_traits
  {
    using head = typename impl::variadic_traits_helper<T...>::head;
    using tail = typename impl::variadic_traits_helper<T...>::tail;

    static constexpr std::size_t size() noexcept
    {
      return impl::variadic_traits_helper<T...>::size();
    }
  };
     
  template<>
  struct variadic_traits<>
  {
    using head = void;
    using tail = void;
    static constexpr std::size_t size() noexcept
    {
      return 0u;
    }
  };

  template<class... T>
  using head_of_t = typename variadic_traits<T...>::head;
  
  template<class... T>
  using tail_of_t = typename variadic_traits<T...>::tail;
 
  // is_base_of_head
  
  template<class T, class... Args>
  struct is_base_of_head
    : std::is_base_of<std::remove_cvref_t<T>, std::remove_cvref_t<typename variadic_traits<Args...>::head>>
  {};

  template<class T, class... Args>
  constexpr bool is_base_of_head_v{is_base_of_head<T, Args...>::value};

  template<class T, class... Args>
  using is_base_of_head_t = typename is_base_of_head<T, Args...>::type;

  // resolve_to_copy

  template<class T, class... Args>
  struct resolve_to_copy
    : std::bool_constant<
           (variadic_traits<Args...>::size() == 1)
        && (   std::is_same_v<std::remove_cvref_t<typename variadic_traits<Args...>::head>, std::remove_cvref_t<T>>
            || is_base_of_head_v<T, Args...>)
      >
  {};

  template<class T, class... Args>
  constexpr bool resolve_to_copy_v{resolve_to_copy<T, Args...>::value};

  template<class T, class... Args>
  using resolve_to_copy_t = typename resolve_to_copy<T, Args...>::type;

  // is_const_pointer
  
  template<class T>
  struct is_const_pointer : std::false_type {};

  template<class T>
  struct is_const_pointer<const T*> : std::true_type {};

  template<class T>
  struct is_const_pointer<const T* const> : std::true_type {};

  template<class T> constexpr bool is_const_pointer_v{is_const_pointer<T>::value};

  template<class T> using is_const_pointer_t = typename is_const_pointer<T>::type;

  // is_const_reference
  
  template<class T>
  struct is_const_reference : std::bool_constant<std::is_reference_v<T> && std::is_const_v<std::remove_reference_t<T>>> {};

  template<class T> constexpr bool is_const_reference_v{is_const_reference<T>::value};

  template<class T> using is_const_reference_t = typename is_const_reference<T>::type;

  // is_orderable
  
  template<class T, class = std::void_t<>>
  struct is_orderable : std::false_type
  {};

  template<class T>
  struct is_orderable<T, std::void_t<decltype(std::declval<T>() < std::declval<T>())>> : std::true_type
  {};

  template<class T> constexpr bool is_orderable_v{is_orderable<T>::value};

  template<class T> using is_orderable_t = typename is_orderable<T>::type;

  // is_equal_to_comparable
  
  template<class T, class = std::void_t<>>
  struct is_equal_to_comparable : std::false_type
  {};

  template<class T>
  struct is_equal_to_comparable<T, std::void_t<decltype(std::declval<T>() == std::declval<T>())>>
    : std::true_type
  {};

  template<class T> constexpr bool is_equal_to_comparable_v{is_equal_to_comparable<T>::value};

  template<class T> using is_equal_to_comparable_t = typename is_equal_to_comparable<T>::type;

  // is_not_equal_to_comparable
  
  template<class T, class = std::void_t<>>
  struct is_not_equal_to_comparable : std::false_type
  {};

  template<class T>
  struct is_not_equal_to_comparable<T, std::void_t<decltype((std::declval<T>() != std::declval<T>()))>>
    : std::true_type
  {};

  template<class T> constexpr bool is_not_equal_to_comparable_v{is_not_equal_to_comparable<T>::value};

  template<class T> using is_not_equal_to_comparable_t = typename is_not_equal_to_comparable<T>::type;

  // is_container

  template<class T, class = std::void_t<>> struct is_container : std::false_type
  {};

  template<class T> struct is_container<T, std::void_t<decltype(std::begin(std::declval<T>()))>> : std::true_type
  {};

  template<class T> constexpr bool is_container_v{is_container<T>::value};

  template<class T>
  using is_container_t = typename is_container<T>::type;

  // is_allocator

  template<class A, class = std::void_t<>> struct is_allocator : std::false_type
  {};
  
  template<class A> struct is_allocator<A, std::void_t<decltype(std::declval<A>().allocate(0))>>
    : std::true_type
  {};  

  template<class A> constexpr bool is_allocator_v{is_allocator<A>::value};

  template<class A>
  using is_allocator_t = typename is_allocator<A>::type;

  // is_serializable

  // This makelval is in part a hack is to work around a bug in the XCode 10.2 stl implementation:
  // ostream line 1036. Without this, e.g. decltype(std::stringstream{} << std::vector<int>{})
  // deduces stringstream&
  template<class T>
  std::add_lvalue_reference_t<T> makelval() noexcept;

  template<class T, class=std::void_t<>>
  struct is_serializable : public std::false_type
  {};
    
  template<class T>
  struct is_serializable<T, std::void_t<decltype(makelval<std::stringstream>() << std::declval<T>())>>
    : public std::true_type
  {};

  template<class T> constexpr bool is_serializable_v{is_serializable<T>::value};

  template<class T>
  using is_serializable_t = typename is_serializable<T>::type;

  // is_class_template_instantiable

  template <class, template<class...> class T, class... Args>
  struct class_template_is_instantiable : std::false_type
  {};

  template <template<class...> class T, class... Args>
  struct class_template_is_instantiable<std::void_t<decltype(T<Args...>{})>, T, Args...>
    : std::true_type
  {};

  template <template<class...> class T, class... Args>
  constexpr bool class_template_is_instantiable_v{class_template_is_instantiable<std::void_t<>, T, Args...>::value};

  template <template<class...> class T, class... Args>
  using class_template_is_instantiable_t = typename class_template_is_instantiable<std::void_t<>, T, Args...>::type;  

  // has_allocator_type

  template<class T, class = std::void_t<>>
  struct has_allocator_type : std::false_type
  {};

  template<class T>
  struct has_allocator_type<T, std::void_t<typename T::allocator_type>> : std::true_type
  {};

  template<class T> constexpr bool has_allocator_type_v = has_allocator_type<T>::value;

  template<class T> using has_allocator_type_t = typename has_allocator_type<T>::type;  

  // dependent_false
  
  template<class T> struct dependent_false : std::false_type {};
}
