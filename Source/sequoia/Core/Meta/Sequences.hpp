////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include <utility>

namespace sequoia
{
  template<class T>
  struct is_index_sequence : std::false_type {};

  template<std::size_t... Is>
  struct is_index_sequence<std::index_sequence<Is...>> : std::true_type {};

  template<class T>
  using is_index_sequence_t = typename is_index_sequence<T>::type;

  template<class T>
  inline constexpr bool is_index_sequence_v{is_index_sequence<T>::value};

  template<class...>
  struct concat_sequences;

  template<class... Ts>
  using concat_sequences_t = typename concat_sequences<Ts...>::type;

  template<std::size_t... Is>
  struct concat_sequences<std::index_sequence<Is...>>
  {
    using type = std::index_sequence<Is...>;
  };

  template<std::size_t... Is, std::size_t... Js, class... Sequences>
    requires (is_index_sequence_v<Sequences> && ...)
  struct concat_sequences<std::index_sequence<Is...>, std::index_sequence<Js...>, Sequences...>
  {
    using type = concat_sequences_t<std::index_sequence<Is..., Js...>, Sequences...>;
  };
}

namespace sequoia::impl
{
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
      using appended = concat_sequences_t<std::index_sequence<M...>, add>;

  public:
      using type = typename aggregate<appended, Total, Excluded, TypeToType, Ts...>::type;
  };
}

namespace sequoia
{
  //==================================================== filtered_sequence ===================================================//

  template<class Excluded, template<class> class TypeToType, class... Ts>
  struct filtered_sequence
  {
      using type = typename impl::aggregate<std::index_sequence<>, sizeof...(Ts), Excluded, TypeToType, Ts...>::type;
  };

  template<class Excluded, template<class> class TypeToType, class... Ts>
  using make_filtered_sequence = typename filtered_sequence<Excluded, TypeToType, Ts...>::type;

  //==================================================== rotate_sequence ===================================================//

  template<class> struct rotate_sequence;

  template<std::size_t I, std::size_t... Is>
  struct rotate_sequence<std::index_sequence<I, Is...>>
  {
    using type = std::index_sequence<Is..., I>;
  };

  template<class T>
  using rotate_sequence_t = typename rotate_sequence<T>::type;


  //==================================================== reverse_sequence ===================================================//

  template<class> struct reverse_sequence;

  template<class T>
  using reverse_sequence_t = typename reverse_sequence<T>::type;

  template<std::size_t I>
  struct reverse_sequence<std::index_sequence<I>>
  {
    using type = std::index_sequence<I>;
  };

  template<std::size_t I, std::size_t... Is>
  struct reverse_sequence<std::index_sequence<I, Is...>>
  {
    using type = concat_sequences_t<reverse_sequence_t<std::index_sequence<Is...>>, std::index_sequence<I>>;
  };

  //==================================================== sequence_partial_sum ===================================================//

  namespace impl
  {
    template<class, std::size_t...>
    struct sequence_partial_sum;

    template<class T, std::size_t... Is>
    using sequence_partial_sum_t = typename sequence_partial_sum<T, Is...>::type;

    template<std::size_t I, std::size_t... Is>
    struct sequence_partial_sum<std::index_sequence<I, Is...>>
    {
      using type = std::index_sequence<I, Is...>;
    };

    template<std::size_t I, std::size_t... Is, std::size_t Next, std::size_t... Remaining>
    struct sequence_partial_sum<std::index_sequence<I, Is...>, Next, Remaining...>
    {
      using type = sequence_partial_sum_t<std::index_sequence<I + Next, I, Is...>, Remaining...>;
    };
  }

  template<std::size_t I, std::size_t... Is>
  struct sequence_partial_sum
  {
    using type = reverse_sequence_t<impl::sequence_partial_sum_t<std::index_sequence<I>, Is...>>;
  };

  template<std::size_t... Is>
  using sequence_partial_sum_t = typename sequence_partial_sum<Is...>::type;
}
