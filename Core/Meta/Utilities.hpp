////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file Utilities.hpp
    \brief Meta-programming utilities
 */

#include <utility>

namespace sequoia::impl
{
  template<class Sequence, class Next>
  struct append_to_sequence;

  // X... can either be empty or contain a single element equal to the index desired 
  template<std::size_t... M, std::size_t... X>
  struct append_to_sequence<std::index_sequence<M...>, std::index_sequence<X...>>
  {
    using type = std::index_sequence<M..., X...>;
  };

  template<class Sequence, std::size_t Total, class Excluded, template<class> class TypeToType, class... Ts>
  struct aggregate;

  template<std::size_t... M, std::size_t Total, class Excluded, template<class> class TypeToType>
  struct aggregate<std::index_sequence<M...>, Total, Excluded, TypeToType>
  {
    using type = std::index_sequence<M...>;
  };

  template<std::size_t... M, std::size_t Total, class Excluded, template<class> class TypeToType, class T, class... Ts>
  struct aggregate<std::index_sequence<M...>, Total, Excluded, TypeToType, T, Ts...>
  {
  private:
    constexpr static bool cond{std::is_same_v<Excluded, typename TypeToType<T>::type>};
    using add = std::conditional_t<cond, std::index_sequence<>, std::index_sequence<Total - sizeof...(Ts) - 1>>;
    using appended = typename append_to_sequence<std::index_sequence<M...>, add>::type;

  public:
    using type = typename aggregate<appended, Total, Excluded, TypeToType, Ts...>::type;
  };   

  template<std::size_t I, class... Ts>
  auto get(const std::tuple<Ts...>& ts)
  {
    return std::get<I>(ts);
  }

  template<class Fn, class... Ts, std::size_t... I>
  void filter(Fn f, std::index_sequence<I...>, Ts... t)
  {
    f(get<I>(std::tuple<Ts...>{t...})...);
  }
}

namespace sequoia
{
  template<class Excluded, template<class> class TypeToType, class... Ts>
  struct filtered_sequence
  {
    using type = typename impl::aggregate<std::index_sequence<>, sizeof...(Ts), Excluded, TypeToType, Ts...>::type;
  };

  template<class Excluded, template<class> class TypeToType, class... Ts>
  using make_filtered_sequence = typename filtered_sequence<Excluded, TypeToType, Ts...>::type;
}
