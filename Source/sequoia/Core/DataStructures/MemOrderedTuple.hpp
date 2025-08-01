////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** @file */

#include <utility>

#ifndef __cpp_pack_indexing
  #include <tuple>
#endif

namespace sequoia
{
  namespace impl
  {
    template<class T, std::size_t I>
    struct indexed_element
    {      
      constexpr static std::size_t index{I};
      T value;

      [[nodiscard]]
      friend auto constexpr operator<=>(const indexed_element&, const indexed_element&) noexcept = default;
    };

    template<class... Ts>
    struct mem_ordered_tuple;

    template<std::size_t... Is, class... Ts>
      requires (sizeof...(Is) == sizeof...(Ts))
    struct mem_ordered_tuple<std::index_sequence<Is...>, Ts...> : indexed_element<Ts, Is>...
    {
      constexpr mem_ordered_tuple()
        requires (std::is_default_constructible_v<Ts> && ...)
        : indexed_element<Ts, Is>{Ts{}}...
      {}

      constexpr explicit(sizeof...(Ts) == 1) mem_ordered_tuple(const Ts&... ts)
        requires (sizeof...(Ts) >= 1) && (std::is_copy_constructible_v<Ts> && ...)
        : indexed_element<Ts, Is>{ts}...
      {}

      [[nodiscard]]
      friend auto constexpr operator<=>(const mem_ordered_tuple&, const mem_ordered_tuple&) noexcept = default;
    };
  }

  template<std::size_t I, class T>
  struct mem_ordered_tuple_element;

  template<std::size_t I, class T>
  using mem_ordered_tuple_element_t = mem_ordered_tuple_element<I, T>::type;
  
  template<class... Ts>
  struct mem_ordered_tuple : private impl::mem_ordered_tuple<std::make_index_sequence<sizeof...(Ts)>, Ts...>
  {
    constexpr mem_ordered_tuple() requires (std::is_default_constructible_v<Ts> && ...) = default;

    constexpr explicit(!(std::convertible_to<const Ts&, Ts> && ...)) mem_ordered_tuple(const Ts&... ts)
        requires (sizeof...(Ts) >= 1) && (std::is_copy_constructible_v<Ts> && ...)
      : impl::mem_ordered_tuple<std::make_index_sequence<sizeof...(Ts)>, Ts...>{ts...}
    {}

    [[nodiscard]]
    friend auto constexpr operator<=>(const mem_ordered_tuple&, const mem_ordered_tuple&) noexcept = default;
  private:
    template<std::size_t I, class... Us>
    friend constexpr mem_ordered_tuple_element_t<I, mem_ordered_tuple<Us...>>& get(mem_ordered_tuple<Us...>&) noexcept;

    template<std::size_t I, class... Us>
    friend constexpr mem_ordered_tuple_element_t<I, mem_ordered_tuple<Us...>> const& get(mem_ordered_tuple<Us...> const&) noexcept;

    template<std::size_t I, class... Us>
    friend constexpr mem_ordered_tuple_element_t<I, mem_ordered_tuple<Us...>>&& get(mem_ordered_tuple<Us...>&&) noexcept;

    template<std::size_t I, class... Us>
    friend constexpr mem_ordered_tuple_element_t<I, mem_ordered_tuple<Us...>> const&& get(mem_ordered_tuple<Us...> const&&) noexcept;
  };

  template<std::size_t I, class... Ts>
  struct mem_ordered_tuple_element<I, mem_ordered_tuple<Ts...>>
  {
    #ifdef __cpp_pack_indexing
      using type = Ts...[I];
    #else
      using type = std::tuple_element_t<I, std::tuple<Ts...>>;
    #endif
  };

  // TO DO: restore direct use of pack-indexing, once MSVC supports it
  template<std::size_t I, class... Ts>
  [[nodiscard]]
  constexpr mem_ordered_tuple_element_t<I, mem_ordered_tuple<Ts...>>& get(mem_ordered_tuple<Ts...>& t) noexcept {
    return static_cast<impl::indexed_element<mem_ordered_tuple_element_t<I, mem_ordered_tuple<Ts...>>, I>&>(t).value;
  }

  template<std::size_t I, class... Ts>
  [[nodiscard]]
  constexpr const mem_ordered_tuple_element_t<I, mem_ordered_tuple<Ts...>>& get(const mem_ordered_tuple<Ts...>& t) noexcept {
    return static_cast<const impl::indexed_element<mem_ordered_tuple_element_t<I, mem_ordered_tuple<Ts...>>, I>&>(t).value;
  }

  template<std::size_t I, class... Ts>
  [[nodiscard]]
  constexpr mem_ordered_tuple_element_t<I, mem_ordered_tuple<Ts...>>&& get(mem_ordered_tuple<Ts...>&& t) noexcept {
    return static_cast<impl::indexed_element<mem_ordered_tuple_element_t<I, mem_ordered_tuple<Ts...>>, I>&&>(t).value;
  }

  template<std::size_t I, class... Ts>
  [[nodiscard]]
  constexpr const mem_ordered_tuple_element_t<I, mem_ordered_tuple<Ts...>>&& get(const mem_ordered_tuple<Ts...>&& t) noexcept {
    return static_cast<const impl::indexed_element<mem_ordered_tuple_element_t<I, mem_ordered_tuple<Ts...>>, I>&&>(t).value;
  }
}
