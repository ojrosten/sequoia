#pragma once

#include <type_traits>
#include <vector>

namespace sequoia
{
  namespace utilities
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

    template<class... T>
    struct variadic_traits
    {
      using head = typename variadic_traits_helper<T...>::head;
      static constexpr std::size_t size()
      {
        return variadic_traits_helper<T...>::size();
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
  }
}
