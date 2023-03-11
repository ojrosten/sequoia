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

#include "sequoia/Core/Object/Nomenclator.hpp"

#include <algorithm>
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
  struct leaf_extractor;

  template<class... Ts>
  using leaf_extractor_t = typename leaf_extractor<Ts...>::type;

  template<class... Ts>
  struct leaf_extractor<std::tuple<Ts...>>
  {
    using type = std::tuple<Ts...>;
  };

  template<class... Ts>
    requires ((!is_suite_v<Ts>) && ...)
  struct leaf_extractor<suite<Ts...>>
  {
    using type = std::tuple<Ts...>;
  };

  template<class... Ts>
  struct leaf_extractor<suite<Ts...>>
  {
    using type = leaf_extractor_t<leaf_extractor_t<Ts>...>;
  };

  template<class... Ts, class... Us>
  struct leaf_extractor<std::tuple<Ts...>, std::tuple<Us...>>
  {
    using type = std::tuple<Ts..., Us...>;
  };

  template<class... Ts, class... Us>
  struct leaf_extractor<suite<Ts...>, std::tuple<Us...>>
  {
    using type = std::tuple<leaf_extractor_t<suite<Ts...>>, Us...>;
  };

  template<class... Ts, class... Us>
  struct leaf_extractor<std::tuple<Ts...>, suite<Us...>>
  {
    using type = std::tuple<Ts..., leaf_extractor_t<suite<Us...>>>;
  };

  template<class T, class U, class... Vs>
  struct leaf_extractor<T, U, Vs...>
  {
    using type = leaf_extractor_t<leaf_extractor_t<T, U>, Vs...>;
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
    using type = typename to_variant_or_unqiue_type<leaf_extractor_t<suite<Ts...>>, Transform>::type;
  };

  enum class suite_extraction_flavour { leaves, tree, leaves_and_tree };

  template<suite_extraction_flavour F>
  struct suite_extraction_constant : std::integral_constant<suite_extraction_flavour, F> {};

  using suite_leaves_t          = suite_extraction_constant<suite_extraction_flavour::leaves>;
  using suite_tree_t            = suite_extraction_constant<suite_extraction_flavour::tree>;
  using suite_leaves_and_tree_t = suite_extraction_constant<suite_extraction_flavour::leaves_and_tree>;

  constexpr suite_leaves_t suite_leaves{};
  constexpr suite_tree_t suite_tree{};
  constexpr suite_leaves_and_tree_t suite_leaves_and_tree{};

  namespace impl
  {
    template<class... Ts, class Fn>
    void extract(suite_leaves_t, suite<Ts...>& s, Fn fn)
    {
      [&] <std::size_t... Is> (std::index_sequence<Is...>) {
        (fn(std::get<Is>(s.values)), ...);
      }(std::make_index_sequence<sizeof...(Ts)>{});
    }

    template<class... Ts, class Filter, class Transform, class Container, class... PreviousSuites>
      requires (is_suite_v<PreviousSuites> && ...) && ((!is_suite_v<Ts>) && ...)
    Container&& extract(suite_leaves_t, suite<Ts...>& s, Filter&& filter, Transform t, Container&& c, const PreviousSuites&... previous)
    {
      auto emplacer{
        [&] <class V> (V&& val) {
          if(filter(val, previous..., s)) c.emplace_back(t(std::move(val)));
        }
      };

      extract(suite_leaves, s, emplacer);
      return std::forward<Container>(c);
    }

    template<class... Ts, class Filter, class Transform, class Container, class... PreviousSuites>
      requires (is_suite_v<PreviousSuites> && ...) && ((is_suite_v<Ts>) && ...)
    Container&& extract(suite_leaves_t, suite<Ts...>& s, Filter&& filter, Transform t, Container&& c, const PreviousSuites&... previous)
    {
      auto extractor{
        [&]<class V>(V&& val) { extract(suite_leaves, std::forward<V>(val), std::forward<Filter>(filter), t, std::forward<Container>(c), previous..., s); }
      };

      extract(suite_leaves, s, extractor);
      return std::forward<Container>(c);
    }

    template<class... Ts,
             class Tree,
             class SizeType = typename Tree::size_type,
             class Fn>
    Tree&& extract(suite_tree_t, suite<Ts...>& s, Tree&& tree, const SizeType parentNode, Fn fn)
    {
      const auto node{tree.add_node(parentNode, nomenclature(s))};

      [node, fn, &s] <std::size_t... Is> (std::index_sequence<Is...>) {
        (fn(node, std::get<Is>(s.values)), ...);
      }(std::make_index_sequence<sizeof...(Ts)>{});

      if(!std::distance(tree.cbegin_edges(node), tree.cend_edges(node)))
        tree.prune(node);

      return std::forward<Tree>(tree);
    }


    template<class... Ts, class Filter, class Transform, class Tree, class SizeType = typename Tree::size_type, class... PreviousSuites>
      requires (is_suite_v<PreviousSuites> && ...) && ((!is_suite_v<Ts>) && ...)
    Tree&& extract(suite_tree_t, suite<Ts...>& s, Filter&& filter, Transform transform, Tree&& tree, const SizeType parentNode, const PreviousSuites&... previous)
    {
      auto emplacer{
        [&](SizeType node, auto&& val) {
          if(filter(val, previous..., s))
            tree.add_node(node, transform(std::move(val)));
        }
      };

      return extract(suite_tree, s, std::forward<Tree>(tree), parentNode, emplacer);
    }

    template<class... Ts, class Filter, class Transform, class Tree, class SizeType = typename Tree::size_type, class... PreviousSuites>
      requires (is_suite_v<PreviousSuites> && ...) && ((is_suite_v<Ts>) && ...)
    Tree&& extract(suite_tree_t, suite<Ts...>& s, Filter&& filter, Transform transform, Tree&& tree, const SizeType parentNode, const PreviousSuites&... previous)
    {
      auto recurser{
        [&] <class V> (SizeType node, V&& val) {
          extract(suite_tree, std::forward<V>(val), std::forward<Filter>(filter), transform, tree, node, previous..., s);
        }
      };

      return extract(suite_tree, s, std::forward<Tree>(tree), parentNode, recurser);
    }
  }

  template<class Suite,
           class Filter,
           class Transform = std::identity,
           class Container = std::vector<to_variant_or_unique_type_t<Suite, Transform>>>
    requires is_suite_v<Suite>
  [[nodiscard]]
  Container extract(suite_leaves_t, Suite s, Filter&& filter, Transform t = {})
  {
    return impl::extract(suite_leaves, s, std::forward<Filter>(filter), std::move(t), Container{});
  }

  template<class Suite,
           class Filter,
           class Transform,
           class Tree = maths::tree<maths::directed_flavour::directed, maths::tree_link_direction::forward, maths::null_weight, to_variant_or_unique_type_t<Suite, Transform>>>
    requires is_suite_v<Suite>
  [[nodiscard]]
  Tree extract(suite_tree_t, Suite s, Filter&& filter, Transform transform)
  {
    return impl::extract(suite_tree, s, std::forward<Filter>(filter), std::move(transform), Tree{}, Tree::npos);
  }

  class filter_by_names
  {
  public:
    using map_type       = std::map<std::string, bool>;
    using const_iterator = typename map_type::const_iterator;

    filter_by_names(const std::vector<std::string>& selectedSuites, const std::vector<std::string>& selectedItems)
      : m_SelectedSuites{make(selectedSuites)}
      , m_SelectedItems{make(selectedItems)}
    {}

    template<class T, class... Suites>
      requires (is_suite_v<Suites> && ...)
    [[nodiscard]]
    bool operator()(const T& val, const Suites&... suites)
    {
      auto finder{
        [] <class U>(map_type& selected, const map_type& other, const U& u) {
          if(selected.empty()) return other.empty();

          auto found{selected.find(nomenclator<U>::name(u))};
          const bool isFound{found != selected.end()};
          if(isFound) found->second = true;

          return isFound;
        }
      };

      // Don't use logical short-circuit, otherwise the maps may not accurately update
      std::array<bool, sizeof...(Suites) + 1> isFound{finder(m_SelectedItems, m_SelectedSuites, val), finder(m_SelectedSuites, m_SelectedItems, suites) ...};

      return std::any_of(isFound.begin(), isFound.end(), [](bool b) { return b; });
    }

    [[nodiscard]]
    const_iterator begin_selected_suites() const noexcept { return m_SelectedSuites.begin(); }

    [[nodiscard]]
    const_iterator end_selected_suites() const noexcept { return m_SelectedSuites.end(); }

    [[nodiscard]]
    const_iterator begin_selected_items() const noexcept { return m_SelectedItems.begin(); }

    [[nodiscard]]
    const_iterator end_selected_items() const noexcept { return m_SelectedItems.end(); }

    [[nodiscard]]
    friend bool operator==(const filter_by_names&, const filter_by_names&) noexcept = default;
  private:
    map_type m_SelectedSuites{}, m_SelectedItems{};

    [[nodiscard]]
    static map_type make(const std::vector<std::string>& selected)
    {
      map_type selection{};
//      std::ranges::transform(selected, std::inserter(selection, selection.end()), [](const std::string& s) -> std::pair<std::string, bool> { return {s, false}; });
      std::transform(selected.begin(), selected.end(), std::inserter(selection, selection.end()), [](const std::string& s) -> std::pair<std::string, bool> { return {s, false}; });
      return selection;
    }
  };
}
