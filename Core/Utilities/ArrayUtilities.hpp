////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file ArrayUtilities.hpp
    \brief Utility to convert an initializer_list into an array, potentially transforming
    the inlitializer_list in the process 
 */

#include <array>
#include <utility>
#include <type_traits>

namespace sequoia::utilities
{
  namespace impl
  {
    template<class T, class InitType, class Fn>
    [[nodiscard]]
    constexpr T to_element(std::initializer_list<InitType> l, const std::size_t i, Fn fn)
    {
      if constexpr(std::is_default_constructible_v<InitType>)
      {
        return fn((i < l.size()) ? *(l.begin() + i) : InitType{});
      }
      else
      {
        if(i > l.size())
          throw std::logic_error("Insuffucient arguments to initialize an array of objects which cannot be default constructed.");

        return fn(*(l.begin() + i));
      }
    }
    
    template<class T, std::size_t N, class InitType, std::size_t... I, class Fn>
    [[nodiscard]]
    constexpr std::array<T, N> to_array(std::initializer_list<InitType> l, std::index_sequence<I...>, Fn fn)
    {
      return std::array<T, N>{ to_element<T>(l, I, fn)...};
    }
  }

  template<class T> struct identity_transform
  {
    [[nodiscard]]
    constexpr const T& operator()(const T& t) const
    {
      return t;
    }
  };
  
  
  template<class T, std::size_t N, class InitType=T, class Fn=identity_transform<T>>
  [[nodiscard]]
  constexpr std::array<T, N> to_array(std::initializer_list<InitType> l, Fn fn = Fn{})
  {
    if(l.size() > N)
      throw std::logic_error{"intializer_list too big for array"};
    
    return impl::to_array<T, N>(l, std::make_index_sequence<N>{}, fn);
  }
}
