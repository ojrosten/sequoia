////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Utilities for defining a suite of objects, filtered at runtime
 */

#pragma once

#include <string>
#include <tuple>
#include <variant>
#include <vector>

namespace sequoia::object
{
  template<class T>
  struct is_suite : std::false_type {};

  template<class T>
  using is_suite_t = typename is_suite<T>::type;

  template<class T>
  inline constexpr bool is_suite_v{is_suite<T>::value};

  template<class... Ts>
    requires ((is_suite_v<Ts> && ...) || ((!is_suite_v<Ts>) && ...))
  class suite;

  template<class... Ts>
  struct is_suite<suite<Ts...>> : std::true_type {};

  template<class... Ts>
    requires ((is_suite_v<Ts> && ...) || ((!is_suite_v<Ts>) && ...))
  class suite
  {
  public:
    std::string name;
    std::tuple<Ts...> values;

    suite(std::string name, Ts&&... ts)
      : name{std::move(name)}
      , values{std::forward<Ts>(ts)...} {}
  };


  template<class... Ts>
  struct extract_leaves;

  template<class... Ts>
  using extract_leaves_t = typename extract_leaves<Ts...>::type;

  template<class... Ts>
  struct extract_leaves<std::tuple<Ts...>>
  {
    using type = std::tuple<Ts...>;
  };

  template<class... Ts>
    requires ((!is_suite_v<Ts>) && ...)
  struct extract_leaves<suite<Ts...>>
  {
    using type = std::tuple<Ts...>;
  };

  template<class... Ts>
  struct extract_leaves<suite<Ts...>>
  {
    using type = extract_leaves_t<extract_leaves_t<Ts>...>;
  };

  template<class... Ts, class... Us>
  struct extract_leaves<std::tuple<Ts...>, std::tuple<Us...>>
  {
    using type = std::tuple<Ts..., Us...>;
  };

  template<class... Ts, class... Us>
  struct extract_leaves<suite<Ts...>, std::tuple<Us...>>
  {
    using type = std::tuple<extract_leaves_t<suite<Ts...>>, Us...>;
  };

  template<class... Ts, class... Us>
  struct extract_leaves<std::tuple<Ts...>, suite<Us...>>
  {
    using type = std::tuple<Ts..., extract_leaves_t<suite<Us...>>>;
  };

  template<class T, class U, class... Vs>
  struct extract_leaves<T, U, Vs...>
  {
    using type = extract_leaves_t<extract_leaves_t<T, U>, Vs...>;
  };

  template<class T, class Transform>
  struct to_variant;

  template<class T, class Transform>
  using to_variant_t = typename to_variant<T, Transform>::type;

  template<class... Ts, class Transform>
  struct to_variant<std::tuple<Ts...>, Transform>
  {
    using type = std::variant<std::remove_cvref_t<std::invoke_result_t<Transform, Ts>>...>;
  };

  template<class... Ts, class Transform>
  struct to_variant<suite<Ts...>, Transform>
  {
    using type = typename to_variant<extract_leaves_t<suite<Ts...>>, Transform>::type;
  };

  namespace impl
  {
    template<class... Ts, class Filter, class Transform, class Container, class... PreviousSuites>
      requires (is_suite_v<PreviousSuites> && ...) && ((!is_suite_v<Ts>) && ...)
    static void get(suite<Ts...>& s, Filter&& filter, Transform t, Container& c, const PreviousSuites&... previous)
    {
      auto emplacer{
        [&] <class V> (V&& val) {
          if(filter(val, previous..., s)) c.emplace_back(t(std::forward<V>(val)));
        }
      };

      [emplacer, &s] <std::size_t... Is> (std::index_sequence<Is...>) {
        (emplacer(std::move(std::get<Is>(s.values))), ...);
      }(std::make_index_sequence<sizeof...(Ts)>{});
    }

    template<class... Ts, class Filter, class Transform, class Container, class... PreviousSuites>
      requires (is_suite_v<PreviousSuites> && ...) && ((is_suite_v<Ts>) && ...)
    static Container&& get(suite<Ts...>& s, Filter&& filter, Transform t, Container&& c, const PreviousSuites&... previous)
    {
      [&] <std::size_t... Is> (std::index_sequence<Is...>) {
        (get(std::get<Is>(s.values), std::forward<Filter>(filter), t, c, previous..., s), ...);
      }(std::make_index_sequence<sizeof...(Ts)>{});

      return std::forward<Container>(c);
    }
  }

  template<class Suite,
           class Filter,
           class Transform=std::identity,
           class Container = std::vector<to_variant_t<Suite, Transform>>>
    requires is_suite_v<Suite>
  [[nodiscard]]
  Container extract(Suite s, Filter&& filter, Transform t = {})
  {
    return impl::get(s, std::forward<Filter>(filter), std::move(t), Container{});
  }
}
