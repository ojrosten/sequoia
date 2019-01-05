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
      static constexpr std::size_t size()
      {
        return 1 + variadic_traits_helper<T...>::size();
      }
    };

    template<class H>
    struct variadic_traits_helper<H>
    {
      using head = H;
      static constexpr std::size_t size()
      {
        return 1;
      }
    };
  }

  template<class... T>
  struct variadic_traits
  {
    using head = typename impl::variadic_traits_helper<T...>::head;
    static constexpr std::size_t size()
    {
      return impl::variadic_traits_helper<T...>::size();
    }
  };
    
  template<>
  struct variadic_traits<>
  {
    using head = void;
    static constexpr std::size_t size()
    {
      return 0u;
    }
  };

  template<class T, class... Args>
  struct same_decay
    : std::bool_constant<
    (variadic_traits<Args...>::size() == 1)
    && std::is_same_v<std::decay_t<typename variadic_traits<Args...>::head>, std::decay_t<T>>>
  {
  };

  template<class T, class... Args>
  constexpr bool same_decay_v{same_decay<T, Args...>::value};

  template<class T, class... Args>
  struct is_base_of_head : std::is_base_of<std::decay_t<T>, std::decay_t<typename variadic_traits<Args...>::head>>
  {
  };

  template<class T, class... Args>
  constexpr bool is_base_of_head_v{is_base_of_head<T, Args...>::value};

  template<class T, class = std::void_t<>>
  struct is_orderable : std::false_type
  {};

  template<class T>
  struct is_orderable<T, std::void_t<decltype((std::declval<T>() < std::declval<T>()))>> : std::true_type
  {};

  template<class T> constexpr bool is_orderable_v{is_orderable<T>::value};

  template<class T, class = std::void_t<>>
  struct is_equal_to_comparable : std::false_type
  {};

  template<class T>
  struct is_equal_to_comparable<T, std::void_t<decltype((std::declval<T>() == std::declval<T>()))>> : std::true_type
  {};

  template<class T> constexpr bool is_equal_to_comparable_v{is_equal_to_comparable<T>::value};

  template<class T, class = std::void_t<>>
  struct is_not_equal_to_comparable : std::false_type
  {};

  template<class T>
  struct is_not_equal_to_comparable<T, std::void_t<decltype((std::declval<T>() != std::declval<T>()))>> : std::true_type
  {};

  template<class T> constexpr bool is_not_equal_to_comparable_v{is_not_equal_to_comparable<T>::value};
}
