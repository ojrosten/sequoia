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

#include <map>
#include <ranges>
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
  struct to_variant_or_unqiue_type;

  template<class T, class Transform>
  using to_variant_or_unique_type_t = typename to_variant_or_unqiue_type<T, Transform>::type;

  template<class... Ts, class Transform>
  struct to_variant_or_unqiue_type<std::tuple<Ts...>, Transform>
  {
    using type = std::variant<std::remove_cvref_t<std::invoke_result_t<Transform, Ts>>...>;
  };

  template<class T, class... Ts, class Transform>
    requires (std::is_same_v<std::remove_cvref_t<std::invoke_result_t<Transform, T>>, std::remove_cvref_t<std::invoke_result_t<Transform, Ts>>> && ...)
  struct to_variant_or_unqiue_type<std::tuple<T, Ts...>, Transform>
  {
    using type = std::remove_cvref_t<std::invoke_result_t<Transform, T>>;
  };

  template<class... Ts, class Transform>
  struct to_variant_or_unqiue_type<suite<Ts...>, Transform>
  {
    using type = typename to_variant_or_unqiue_type<extract_leaves_t<suite<Ts...>>, Transform>::type;
  };

  namespace impl
  {
    template<class... Ts, class Filter, class Transform, class Container, class... PreviousSuites>
      requires (is_suite_v<PreviousSuites> && ...) && ((!is_suite_v<Ts>) && ...)
    static Container&& get(suite<Ts...>& s, Filter&& filter, Transform t, Container&& c, const PreviousSuites&... previous)
    {
      auto emplacer{
        [&] <class V> (V&& val) {
          if(filter(val, previous..., s)) c.emplace_back(t(std::forward<V>(val)));
        }
      };

      [emplacer, &s] <std::size_t... Is> (std::index_sequence<Is...>) {
        (emplacer(std::move(std::get<Is>(s.values))), ...);
      }(std::make_index_sequence<sizeof...(Ts)>{});

      return std::forward<Container>(c);
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
           class Transform = std::identity,
           class Container = std::vector<to_variant_or_unique_type_t<Suite, Transform>>>
    requires is_suite_v<Suite>
  [[nodiscard]]
  Container extract(Suite s, Filter&& filter, Transform t = {})
  {
    return impl::get(s, std::forward<Filter>(filter), std::move(t), Container{});
  }

  class filter_by_names
  {
  public:
    filter_by_names(const std::vector<std::string>& selectedSuites, const std::vector<std::string>& selectedItems)
      : m_SelectedSuites{make(selectedSuites)}
      , m_SelectedItems{make(selectedItems)}
    {}

    template<class T, class... Suites>
      requires (is_suite_v<Suites> && ...)
    [[nodiscard]]
    bool operator()(const T& val, const Suites&... suites)
    {
      auto findSuite{
        [&] <class... Us> (const suite<Us...>& s) {
          if(m_SelectedSuites.empty()) return m_SelectedItems.empty();
        
          auto found{m_SelectedSuites.find(s.name)};
          if(found != m_SelectedSuites.end())
            found->second = true;
        
          return found != m_SelectedSuites.end();
        }
      };

      auto findItem{
        [&](const auto& item) {
          if(m_SelectedItems.empty()) return m_SelectedSuites.empty();

          auto found{m_SelectedItems.find(item.name)}; // Generalize this
          if(found != m_SelectedItems.end())
            found->second = true;

          return found != m_SelectedItems.end();
        }
      };

      return (findSuite(suites) || ...) || findItem(val);
    }
  private:
    using map_t = std::map<std::string, bool>;
    map_t m_SelectedSuites{}, m_SelectedItems{};

    [[nodiscard]]
    static map_t make(const std::vector<std::string>& selected)
    {
      map_t selection{};
      std::ranges::transform(selected, std::inserter(selection, selection.end()), [](const std::string& s) -> std::pair<std::string, bool> { return {s, false}; });
      return selection;
    }
  };
}
