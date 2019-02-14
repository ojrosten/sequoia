////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include <type_traits>

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

  // is_base_of_head
  
  template<class T, class... Args>
  struct is_base_of_head
    : std::is_base_of<std::decay_t<T>, std::decay_t<typename variadic_traits<Args...>::head>>
  {};

  template<class T, class... Args>
  constexpr bool is_base_of_head_v{is_base_of_head<T, Args...>::value};

  template<class T, class... Args>
  using is_base_of_head_t = typename is_base_of_head<T, Args...>::type;

  // resolve_to_copy_constructor

  template<class T, class... Args>
  struct resolve_to_copy_constructor
    : std::bool_constant<
           (variadic_traits<Args...>::size() == 1)
        && (   std::is_same_v<std::decay_t<typename variadic_traits<Args...>::head>, std::decay_t<T>>
            || is_base_of_head_v<T, Args...>)
      >
  {};

  template<class T, class... Args>
  constexpr bool resolve_to_copy_constructor_v{resolve_to_copy_constructor<T, Args...>::value};

  template<class T, class... Args>
  using resolve_to_copy_constructor_t = typename resolve_to_copy_constructor<T, Args...>::type;

  // is_const_pointer
  
  template<class T>
  struct is_const_pointer : std::false_type {};

  template<class T>
  struct is_const_pointer<const T*> : std::true_type {};

  template<class T> constexpr bool is_const_pointer_v{is_const_pointer<T>::value};

  template<class T> using is_const_pointer_t = typename is_const_pointer<T>::type;

  // is_const_reference
  
  template<class T>
  struct is_const_reference : std::bool_constant<std::is_reference_v<T> && std::is_const_v<std::remove_reference_t<T>>> {};

  template<class T> constexpr bool is_const_reference_v{is_const_reference<T>::value};

  template<class T> using is_const_reference_t = typename is_const_reference<T>::type;

  // has_member_type
  
  template<class T, class M, class = std::void_t<>>
  struct has_member_type : std::false_type
  {};

  template<class T, class M>
  struct has_member_type<T, M, std::void_t<typename T::M>> : std::true_type
  {};

  template<class T, class M>
  constexpr bool has_member_type_v{has_member_type<T,M>::value};
  
  template<class T, class M>
  using has_member_type_t = typename has_member_type<T,M>::type;

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

  // has_default_constructor

  template<class T, class = std::void_t<>>
  struct has_default_constructor : std::false_type
  {};

  template<class T>
  struct has_default_constructor<T, std::void_t<decltype(new T{})>> : std::true_type
  {};

  template<class T>
  constexpr bool has_default_constructor_v{has_default_constructor<T>::value};

  template<class T>
  using has_default_constructor_t = typename has_default_constructor<T>::type;  
}
