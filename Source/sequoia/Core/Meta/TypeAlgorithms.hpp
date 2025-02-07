////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2025.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "sequoia/Core/Meta/Sequences.hpp"

#include <source_location>
#include <string_view>
#include <tuple>

/*! \file */

namespace sequoia::meta
{
  //==================================================== type_comparator ===================================================//

  namespace impl
  {
    using trial_type = void;
    constexpr std::string_view trial_type_name{"void"};

    namespace wrapped_type
    {
      template <typename T>
      [[nodiscard]]
      constexpr std::string_view name() noexcept
      {
        return std::source_location::current().function_name();
      }

      [[nodiscard]]
      constexpr std::size_t prefix_length() noexcept
      { 
        return name<trial_type>().find(trial_type_name); 
      }

      [[nodiscard]]
      constexpr std::size_t suffix_length() noexcept
      { 
        return name<trial_type>().length() - prefix_length() - trial_type_name.length();
      }
    }
  }

  template<class T>
  [[nodiscard]]
  consteval std::string_view type_name()
  {
    using namespace impl::wrapped_type;
    constexpr auto wrappedName{name<T>()};
    constexpr auto prefixLength{prefix_length()};
    constexpr auto nameLength{wrappedName.length() - prefixLength - suffix_length()};
    return wrappedName.substr(prefixLength, nameLength);
  }

  template<class T, class U>
  struct type_comparator : std::bool_constant<type_name<T>() < type_name<U>()>
  {};

  template<class T, class U>
  using type_comparator_t = type_comparator<T, U>::type;

  template<class T, class U>
  inline constexpr bool type_comparator_v{type_comparator<T, U>::value};
  
  //==================================================== lower_bound ===================================================//

  template<class T, class U, template<class, class> class Compare>
  struct lower_bound;

  template<class T, class U, template<class, class> class Compare>
  inline constexpr auto lower_bound_v{lower_bound<T, U, Compare>::value};

  template<class... Ts, class U, template<class, class> class Compare>
  struct lower_bound<std::tuple<Ts...>, U, Compare>
  {
    constexpr static std::size_t N{sizeof...(Ts)};

    template<std::size_t Lower, std::size_t Upper>
      requires (Lower <= Upper) && (Upper <= N)
    constexpr static std::size_t get() noexcept {      
      if constexpr (Lower == Upper)
        return Lower;
      else
      {
        constexpr auto partition{(Lower + Upper) / 2};
        if constexpr(Compare<std::tuple_element_t<partition, std::tuple<Ts...>>, U>::value)
          return get<partition + 1, Upper>();
        else
          return get<Lower, partition>();
      }
    }

    constexpr static std::size_t value{get<0, N>()};
  };

  //==================================================== filter ===================================================//

  template<class T, class U>
  struct filter;

  template<class T, class U>
  using filter_t = filter<T, U>::type;

  template<class... Ts, std::size_t... Is>
    requires ((Is < sizeof...(Ts)) && ...)
  struct filter<std::tuple<Ts...>, std::index_sequence<Is...>>
  {
    using type = std::tuple<std::tuple_element_t<Is, std::tuple<Ts...>> ...>;
  };

  //==================================================== drop ===================================================//

  template<class T, std::size_t N>
  struct drop;

  template<class T, std::size_t N>
  using drop_t = drop<T, N>::type;

  template<class... Ts, std::size_t N>
    requires (N >= sizeof...(Ts))
  struct drop<std::tuple<Ts...>, N>
  {
    using type = std::tuple<>;
  };
    
  template<class... Ts, std::size_t N>
  struct drop<std::tuple<Ts...>, N>
  {
    using type = filter_t<std::tuple<Ts...>, shift_sequence_t<std::make_index_sequence<sizeof...(Ts) - N>, N>>;
  };

  //==================================================== keep ===================================================//

  template<class T, std::size_t N>
  struct keep;

  template<class T, std::size_t N>
  using keep_t = keep<T, N>::type;

  template<class... Ts, std::size_t N>
    requires (N >= sizeof...(Ts))
  struct keep<std::tuple<Ts...>, N>
  {
    using type = std::tuple<Ts...>;
  };
    
  template<class... Ts, std::size_t N>
  struct keep<std::tuple<Ts...>, N>
  {
    using type = filter_t<std::tuple<Ts...>, std::make_index_sequence<N>>;
  };

  //==================================================== insert ===================================================//

  template<class T, class U, std::size_t I>
  struct insert;

  template<class T, class U, std::size_t I>
  using insert_t = insert<T, U, I>::type;

  template<class U>
  struct insert<std::tuple<>, U, 0>
  {
    using type = std::tuple<U>;
  };

  namespace impl
  {
    template<class...>
    struct do_insert;

    template<class... Ts>
    using do_insert_t = do_insert<Ts...>::type;

    template<class T, class... Us, std::size_t... Is, std::size_t... Js>
    struct do_insert<T, std::tuple<Us...>, std::index_sequence<Is...>, std::index_sequence<Js...>>
    {
      using type = std::tuple<std::tuple_element_t<Is, std::tuple<Us...>>..., T, std::tuple_element_t<Js, std::tuple<Us...>>...>;
    };
  }

  template<class... Us, class T, std::size_t I>
  requires (I <= sizeof...(Us))
  struct insert<std::tuple<Us...>, T, I>
  {
    using type = impl::do_insert_t<T,
                                   std::tuple<Us...>,
                                   std::make_index_sequence<I>,
                                   shift_sequence_t<std::make_index_sequence<sizeof...(Us) - I>, I>>;
  };

  //==================================================== erase ===================================================//

  template<class T, std::size_t I>
  struct erase;

  template<class T, std::size_t I>
  using erase_t = erase<T, I>::type;

  template<class T, class... Ts>
  struct erase<std::tuple<T, Ts...>, 0>
  {
    using type = std::tuple<Ts...>;
  };

  template<class... Ts, std::size_t I>
  struct erase<std::tuple<Ts...>, I>
    : filter<std::tuple<Ts...>,
             concat_sequences_t<std::make_index_sequence<I>,
                                shift_sequence_t<std::make_index_sequence<sizeof...(Ts)-I-1>, I+1>>>
  {};

  //==================================================== merge ===================================================//

  template<class T, class U, template<class, class> class Compare>
  struct merge;

  template<class T, class U, template<class, class> class Compare>
  using merge_t = merge<T, U, Compare>::type;

  template<class... Ts,  template<class, class> class Compare>
  struct merge<std::tuple<Ts...>, std::tuple<>, Compare>
  {
    using type = std::tuple<Ts...>;
  };

  template<class... Ts, template<class, class> class Compare>
    requires (sizeof...(Ts) > 0)
  struct merge<std::tuple<>, std::tuple<Ts...>, Compare>
  {
    using type = std::tuple<Ts...>;
  };

  template<class T, class U, template<class, class> class Compare>
  struct merge<std::tuple<T>, std::tuple<U>, Compare>
  {
    using type = std::tuple<U, T>;
  };

  template<class T, class U, template<class, class> class Compare>
  requires (Compare<T, U>::value)
  struct merge<std::tuple<T>, std::tuple<U>, Compare>
  {
    using type = std::tuple<T, U>;
  };

  namespace impl
  {
    template<class T, class U, std::size_t I, template<class, class> class Compare>
    struct merge_from_position;

    template<class T, class U, std::size_t I, template<class, class> class Compare>
    using merge_from_position_t = merge_from_position<T, U, I, Compare>::type;

    template<class... Us, std::size_t I, template<class, class> class Compare>
    struct merge_from_position<std::tuple<>, std::tuple<Us...>, I, Compare>
    {
      using type = std::tuple<Us...>;
    };
    
    template<class T, class... Us, std::size_t I, template<class, class> class Compare>
    struct merge_from_position<std::tuple<T>, std::tuple<Us...>, I, Compare>
    {
      constexpr static auto N{sizeof...(Us)};
      constexpr static auto Pos{I + lower_bound_v<drop_t<std::tuple<Us...>, I>, T, Compare>};
      using type = insert_t<std::tuple<Us...>, T, Pos>;
    };

    template<class T, class... Ts, class... Us, std::size_t I, template<class, class> class Compare>
    struct merge_from_position<std::tuple<T, Ts...>, std::tuple<Us...>, I, Compare>
    {
      using first_merge = merge_from_position<std::tuple<T>, std::tuple<Us...>, 0, Compare>;
      using type = merge_from_position_t<std::tuple<Ts...>, typename first_merge::type, first_merge::Pos + 1, Compare>;
    };
  }  

  template<class... Ts, class... Us, template<class, class> class Compare>
    requires (sizeof...(Ts) > 0) && (sizeof...(Us) > 0)
  struct merge<std::tuple<Ts...>, std::tuple<Us...>, Compare> : impl::merge_from_position<std::tuple<Ts...>, std::tuple<Us...>, 0, Compare>
  {};

  //==================================================== stable_sort ===================================================//

  template<class T, template<class, class> class Compare>
  struct stable_sort;

  template<class T, template<class, class> class Compare>
  using stable_sort_t = stable_sort<T, Compare>::type;

  template<template<class, class> class Compare>
  struct stable_sort<std::tuple<>, Compare>
  {
    using type = std::tuple<>;
  };

  template<class T, template<class, class> class Compare>
  struct stable_sort<std::tuple<T>, Compare>
  {
    using type = std::tuple<T>;
  };
  
  template<class... Ts, template<class, class> class Compare>
  struct stable_sort<std::tuple<Ts...>, Compare>
  {
    constexpr static auto partition{sizeof...(Ts) / 2};
    using type = merge_t<stable_sort_t<keep_t<std::tuple<Ts...>, partition>, Compare>,
                         stable_sort_t<drop_t<std::tuple<Ts...>, partition>, Compare>,
                         Compare>;
  };

  //==================================================== find ===================================================//

  template<class T, class U>
  struct find;

  template<class T, class U>
  inline constexpr std::size_t find_v{find<T, U>::index};

  template<class U>
  struct find<std::tuple<>, U>
  {
    constexpr static std::size_t index{};
  };

  template<class T, class... Ts, class U>
  struct find<std::tuple<T, Ts...>, U>
  {
    constexpr static std::size_t index{std::is_same_v<T, U> ? 0 : 1 + find_v<std::tuple<Ts...>, U>};
  };  
}
