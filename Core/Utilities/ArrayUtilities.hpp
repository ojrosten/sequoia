////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file ArrayUtilities.hpp
    \brief Utilities to convert an initializer_list into an array.
 */

#include <array>
#include <utility>

namespace sequoia::utilities
{
  namespace impl
  {
    template<std::size_t N, class T, std::size_t... I, class Fn>
    [[nodiscard]]
    constexpr std::array<T, N> to_array(std::initializer_list<T> l, std::index_sequence<I...>, Fn fn)
    {
      return std::array<T, N>{ fn(l, I)...};
    }
  }
  
  template<class T> struct to_element
  {
    [[nodiscard]]
    constexpr T operator()(std::initializer_list<T> l, const std::size_t i)
    {
      return (i < l.size()) ?  *(l.begin() + i) : T{};
    }
  };
  
  template<std::size_t N, class T, class Fn=to_element<T>>
  [[nodiscard]]
  constexpr std::array<T, N> to_array(std::initializer_list<T> l, Fn fn = Fn{})
  {
    if(l.size() > N)
      throw std::logic_error("Intializer list too big for array");
    
    return impl::to_array<N>(l, std::make_index_sequence<N>{}, fn);
  }
}
