////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file Utilities.hpp
    \brief Meta-programming utilities
 */

#include "sequoia/Core/Meta/Concepts.hpp"

#include <utility>
#include <tuple>
#include <scoped_allocator>

namespace sequoia
{
  template<class Fn, class... Ts, std::size_t... I>
    requires std::invocable<Fn, decltype(std::get<I>(std::declval<std::tuple<Ts&&...>>()))...>
  auto invoke_with_specified_args(Fn f, std::index_sequence<I...>, [[maybe_unused]] Ts&&... ts)
  {
    return f(std::get<I>(std::tuple<Ts&&...>{std::forward<Ts>(ts)...})...);
  }

  template<class F> struct function_signature;

  template<class R, class L, class T>
  struct function_signature<R(L::*) (T)>
  {
    using ret = R;
    using arg = T;
  };

  template<class R, class L, class T>
  struct function_signature<R(L::*) (T) const>
  {
    using ret = R;
    using arg = T;
  };

  template<class R, class L, class T>
  struct function_signature<R(L::*) (T) noexcept>
  {
    using ret = R;
    using arg = T;
  };

  template<class R, class L, class T>
  struct function_signature<R(L::*) (T) const noexcept>
  {
    using ret = R;
    using arg = T;
  };

  template<class R, class T>
  struct function_signature<R(*) (T)>
  {
    using ret = R;
    using arg = T;
  };

  template<class R, class T>
  struct function_signature<R(*) (T) noexcept>
  {
    using ret = R;
    using arg = T;
  };

  template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
  template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

  template<alloc A>
  struct alloc_count
  {
    constexpr static std::size_t size{1};
  };

  template<alloc OuterAlloc, alloc... InnerAlloc>
  struct alloc_count<std::scoped_allocator_adaptor<OuterAlloc, InnerAlloc...>>
  {
    constexpr static std::size_t size{1 + sizeof...(InnerAlloc)};
  };

  template<class Tuple, class Fn>
  constexpr void for_each(Tuple&& t, Fn&& f)
  {
    [&]<std::size_t... Is>(std::index_sequence<Is...>){
      ((std::forward<Fn>(f)(std::get<Is>(t))), ...);
    }(std::make_index_sequence<std::tuple_size_v<std::remove_cvref_t<Tuple>>>{});
  }

}
