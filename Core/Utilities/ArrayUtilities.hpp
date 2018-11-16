#pragma once

#include <array>
#include <utility>

namespace sequoia::utilities
{
  namespace impl
  {
    template<std::size_t N, class T, std::size_t... I, class Fn>
    constexpr std::array<T, N> to_array(std::initializer_list<T> l, std::index_sequence<I...>, Fn fn)
    {
      return std::array<T, N>{ fn(l, I)...};
    }
  }
  
  template<class T> struct to_element
  {
    constexpr T operator()(std::initializer_list<T> l, const std::size_t i)
    {
      return (i < l.size()) ?  *(l.begin() + i) : T{};
    }
  };
  
  template<std::size_t N, class T, class Fn=to_element<T>>
  constexpr std::array<T, N> to_array(std::initializer_list<T> l, Fn fn = Fn{})
  {
    if(l.size() > N)
      throw std::logic_error("Intializer list too big for array");
    
    return impl::to_array<N>(l, std::make_index_sequence<N>{}, fn);
  }
}
