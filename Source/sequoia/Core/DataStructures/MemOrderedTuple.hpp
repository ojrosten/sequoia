////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** @file */

#include <utility>

namespace sequoia::datastructures
{
  namespace impl
  {
    template<class T, std::size_t I>
    struct mem_ordered_tuple_element
    {      
      constexpr static std::size_t index{I};
      T value;
    };

    template<class... Ts>
    struct mem_ordered_tuple;

    template<std::size_t... Is, class... Ts>
      requires (sizeof...(Is) == sizeof...(Ts))
    struct mem_ordered_tuple<std::index_sequence<Is...>, Ts...> : mem_ordered_tuple_element<Ts, Is>...
    {
      constexpr mem_ordered_tuple()
        requires (std::is_default_constructible_v<Ts> && ...)
        : mem_ordered_tuple_element<Ts, Is>{Ts{}}...
      {}

      constexpr explicit(sizeof...(Ts) == 1) mem_ordered_tuple(const Ts&... ts)
        requires (sizeof...(Ts) >= 1) && (std::is_copy_constructible_v<Ts> && ...)
        : mem_ordered_tuple_element<Ts, Is>{ts}...
      {}
    };
  }
  
  template<class... Ts>
  struct mem_ordered_tuple : impl::mem_ordered_tuple<std::make_index_sequence<sizeof...(Ts)>, Ts...>
  {
    constexpr mem_ordered_tuple() requires (std::is_default_constructible_v<Ts> && ...) = default;

    constexpr explicit(sizeof...(Ts) == 1) mem_ordered_tuple(const Ts&... ts)
        requires (sizeof...(Ts) >= 1) && (std::is_copy_constructible_v<Ts> && ...)
      : impl::mem_ordered_tuple<std::make_index_sequence<sizeof...(Ts)>, Ts...>{ts...}
    {}
  };

  template<std::size_t I, class... Ts>
  Ts...[I]& get(mem_ordered_tuple<Ts...>& t) noexcept {
    return static_cast<impl::mem_ordered_tuple_element<Ts...[I], I>&>(t).value;
  }

  template<std::size_t I, class... Ts>
  const Ts...[I]& get(const mem_ordered_tuple<Ts...>& t) noexcept {
    return static_cast<const impl::mem_ordered_tuple_element<Ts...[I], I>&>(t).value;
  }
}
