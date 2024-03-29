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
#include "sequoia/Maths/Graph/DynamicTree.hpp"
#include "sequoia/Maths/Graph/GraphTraits.hpp"

#include <algorithm>
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
  struct leaves_to_variant_or_unique_type;

  template<class T, class Transform>
  using leaves_to_variant_or_unique_type_t = typename leaves_to_variant_or_unique_type<T, Transform>::type;

  template<class... Ts, class Transform>
    requires (std::invocable<Transform, Ts> && ...)
  struct leaves_to_variant_or_unique_type<std::tuple<Ts...>, Transform>
  {
    using type = std::variant<std::remove_cvref_t<std::invoke_result_t<Transform, Ts>>...>;
  };

  template<class T, class... Ts, class Transform>
    requires (std::is_same_v<std::remove_cvref_t<std::invoke_result_t<Transform, T>>, std::remove_cvref_t<std::invoke_result_t<Transform, Ts>>> && ...)
  struct leaves_to_variant_or_unique_type<std::tuple<T, Ts...>, Transform>
  {
    using type = std::remove_cvref_t<std::invoke_result_t<Transform, T>>;
  };

  template<class... Ts, class Transform>
  struct leaves_to_variant_or_unique_type<suite<Ts...>, Transform>
  {
    using type = typename leaves_to_variant_or_unique_type<leaf_extractor_t<suite<Ts...>>, Transform>::type;
  };


  template<class...>
  struct faithful_variant;

  template<class... Ts>
  using faithful_variant_t = typename faithful_variant<Ts...>::type;

  template<>
  struct faithful_variant<>
  {};

  template<class T>
  struct faithful_variant<T>
  {
    using type = std::variant<T>;
  };

  template<class... Ts, class T>
    requires (std::is_same_v<Ts, T> || ...)
  struct faithful_variant<std::variant<Ts...>, T>
  {
    using type = std::variant<Ts...>;
  };

  template<class... Ts, class T>
  struct faithful_variant<std::variant<Ts...>, T>
  {
    using type = std::variant<Ts..., T>;
  };

  template<class... Ts, class T, class... Us>
  struct faithful_variant<std::variant<Ts...>, T, Us...>
  {
    using type = faithful_variant_t<faithful_variant_t<std::variant<Ts...>, T>, Us...>;
  };

  template<class T, class... Ts>
  struct faithful_variant<T, Ts...>
  {
    using type = faithful_variant_t<std::variant<T>, Ts...>;
  };


  template<class T, std::invocable<T> Transform>
  struct to_variant_or_unique_type
  {
    using type = std::invoke_result_t<Transform, T>;
  };

  template<class T, class Transform>
  using to_variant_or_unique_type_t = typename to_variant_or_unique_type<T, Transform>::type;

  template<class... Ts, std::invocable<suite<Ts...>> Transform>
  struct to_variant_or_unique_type<suite<Ts...>, Transform>
  {
    using variant_type = faithful_variant_t<std::invoke_result_t<Transform, suite<Ts...>>, to_variant_or_unique_type_t<Ts, Transform>...>;
    using type = std::conditional_t<std::variant_size_v<variant_type> == 1, std::variant_alternative_t<0, variant_type>, variant_type>;
  };

  namespace impl
  {
    template<class... Ts, class Fn>
    void extract_leaves(suite<Ts...>& s, Fn fn)
    {
      [&] <std::size_t... Is> (std::index_sequence<Is...>) {
        (fn(std::get<Is>(s.values)), ...);
      }(std::make_index_sequence<sizeof...(Ts)>{});
    }

    template<class... Ts, class Filter, class Transform, class Container, class... PreviousSuites>
      requires (is_suite_v<PreviousSuites> && ...) && ((!is_suite_v<Ts>) && ...)
    Container&& extract_leaves(suite<Ts...>& s, Filter&& filter, Transform t, Container&& c, const PreviousSuites&... previous)
    {
      auto emplacer{
        [&] <class V> (V&& val) {
          if(filter(val, previous..., s)) c.emplace_back(t(std::move(val)));
        }
      };

      extract_leaves(s, emplacer);
      return std::forward<Container>(c);
    }

    template<class... Ts, class Filter, class Transform, class Container, class... PreviousSuites>
      requires (is_suite_v<PreviousSuites> && ...) && ((is_suite_v<Ts>) && ...)
    Container&& extract_leaves(suite<Ts...>& s, Filter&& filter, Transform t, Container&& c, const PreviousSuites&... previous)
    {
      auto extractor{
        [&]<class V>(V&& val) { extract_leaves(std::forward<V>(val), std::forward<Filter>(filter), t, std::forward<Container>(c), previous..., s); }
      };

      extract_leaves(s, extractor);
      return std::forward<Container>(c);
    }

    template<class... Ts,
             class Transform,
             class Tree,
             std::integral SizeType = typename Tree::size_type,
             class Fn>
      requires maths::dynamic_tree<std::remove_cvref_t<Tree>>
    Tree&& extract_tree(suite<Ts...>& s, Transform transform, Tree&& tree, const SizeType parentNode, Fn fn)
    {
      const auto node{tree.add_node(parentNode, transform(s))};

      [node, fn, &s] <std::size_t... Is> (std::index_sequence<Is...>) {
        (fn(node, std::get<Is>(s.values)), ...);
      }(std::make_index_sequence<sizeof...(Ts)>{});

      if(!std::ranges::distance(tree.cedges(node)))
        tree.prune(node);

      return std::forward<Tree>(tree);
    }


    template<class... Ts, class Filter, class Transform, class Tree, std::integral SizeType = typename Tree::size_type, class... PreviousSuites>
      requires maths::dynamic_tree<std::remove_cvref_t<Tree>> && (is_suite_v<PreviousSuites> && ...) && ((!is_suite_v<Ts>) && ...)
    Tree&& extract_tree(suite<Ts...>& s, Filter&& filter, Transform transform, Tree&& tree, const SizeType parentNode, const PreviousSuites&... previous)
    {
      auto emplacer{
        [&](SizeType node, auto&& val) {
          if(filter(val, previous..., s))
            tree.add_node(node, transform(std::move(val)));
        }
      };

      return impl::extract_tree(s, transform, std::forward<Tree>(tree), parentNode, emplacer);
    }

    template<class... Ts, class Filter, class Transform, class Tree, std::integral SizeType = typename Tree::size_type, class... PreviousSuites>
      requires maths::dynamic_tree<std::remove_cvref_t<Tree>> && (is_suite_v<PreviousSuites> && ...) && ((is_suite_v<Ts>) && ...)
    Tree&& extract_tree(suite<Ts...>& s, Filter&& filter, Transform transform, Tree&& tree, const SizeType parentNode, const PreviousSuites&... previous)
    {
      auto recurser{
        [&] <class V> (SizeType node, V&& val) {
          impl::extract_tree(std::forward<V>(val), std::forward<Filter>(filter), transform, std::forward<Tree>(tree), node, previous..., s);
        }
      };

      return impl::extract_tree(s, transform, std::forward<Tree>(tree), parentNode, recurser);
    }
  }

  template<class Suite,
           class Filter,
           class Transform = std::identity,
           class Container = std::vector<leaves_to_variant_or_unique_type_t<Suite, Transform>>>
    requires is_suite_v<Suite>
  [[nodiscard]]
  Container extract_leaves(Suite s, Filter&& filter, Transform t = {})
  {
    return impl::extract_leaves(s, std::forward<Filter>(filter), std::move(t), Container{});
  }

  template<class Suite,
           class Filter,
           class Transform,
           maths::dynamic_tree Tree = maths::directed_tree<maths::tree_link_direction::forward, maths::null_weight, to_variant_or_unique_type_t<Suite, Transform>>>
    requires is_suite_v<Suite>
  [[nodiscard]]
  Tree extract_tree(Suite s, Filter&& filter, Transform transform)
  {
    return impl::extract_tree(s, std::forward<Filter>(filter), std::move(transform), Tree{}, Tree::npos);
  }

  template<class Suite,
           class Filter,
           class Transform,
           maths::dynamic_tree Tree>
    requires is_suite_v<Suite>
  Tree& extract_tree(Suite s, Filter&& filter, Transform transform, Tree& tree, typename Tree::size_type pos)
  {
    return impl::extract_tree(s, std::forward<Filter>(filter), std::move(transform), tree, pos);
  }

  struct item_to_name
  {
    template<class T>
      requires has_intrinsic_nomenclator<T> || has_extrinsic_nomenclator<T>
    [[nodiscard]]
    std::string operator()(const T& t) const
    {
      return object::nomenclature(t);
    }
  };

  template
  <
    class ItemKeyType,
    class ItemCompare,
    class ItemProjector,
    class T
  >
  inline constexpr bool granular_filter_for{
    requires (const ItemKeyType& val, ItemCompare comp, ItemProjector proj, const T& t) {
      { comp(val, proj(t)) } -> std::same_as<bool>;
    }
  };

  template
  <
    class ItemKeyType,
    class ItemCompare,
    class ItemProjector
  >
  class granular_filter
  {
  public:
    using items_key_type  = ItemKeyType;
    using suites_map_type = std::vector<std::pair<std::string, bool>>;
    using items_map_type  = std::vector<std::pair<ItemKeyType, bool>>;
    using selected_suites_iterator = typename suites_map_type::const_iterator;
    using selected_items_iterator  = typename items_map_type::const_iterator;
    using optional_suite_selection = std::optional<std::vector<std::string>>;
    using optional_item_selection  = std::optional<std::vector<items_key_type>>;

    granular_filter(ItemCompare compare = {}, ItemProjector proj = {})
      : m_Compare{std::move(compare)}
      , m_Proj{std::move(proj)}
    {}

    granular_filter(optional_suite_selection selectedSuites, optional_item_selection selectedItems, ItemCompare compare = {}, ItemProjector proj = {})
      : m_Compare{std::move(compare)}
      , m_Proj{std::move(proj)}
      , m_SelectedSuites{make(std::move(selectedSuites))}
      , m_SelectedItems{make(std::move(selectedItems))}
    {}

    void add_selected_suite(std::string name)
    {
      add(m_SelectedSuites, std::move(name));
    }

    void add_selected_item(items_key_type key)
    {
      add(m_SelectedItems, std::move(key));
    }

    template<class T, class... Suites>
      requires (is_suite_v<Suites> && ...) && granular_filter_for<ItemKeyType, ItemCompare, ItemProjector, T>
    [[nodiscard]]
    bool operator()(const T& val, const Suites&... suites)
    {
      if(!m_SelectedSuites && !m_SelectedItems) return true;

      // Don't use logical short-circuit, otherwise the maps may not accurately update
      std::array<bool, sizeof...(Suites) + 1> isFound{ find(m_SelectedItems, val, m_Proj, m_Compare), find(m_SelectedSuites, suites, item_to_name{}, std::ranges::equal_to{}) ... };

      return std::ranges::any_of(isFound, [](bool b) { return b; });
    }

    [[nodiscard]]
    std::optional<std::ranges::subrange<selected_suites_iterator>> selected_suites() const noexcept
    {
      return m_SelectedSuites ? std::optional<std::ranges::subrange<selected_suites_iterator>>{{m_SelectedSuites->begin(), m_SelectedSuites->end()}} : std::nullopt;
    }

    [[nodiscard]]
    std::optional<std::ranges::subrange<selected_items_iterator>> selected_items() const noexcept
    {
      return m_SelectedItems ? std::optional<std::ranges::subrange<selected_items_iterator>>{{m_SelectedItems->begin(), m_SelectedItems->end()}} : std::nullopt;
    }

    [[nodiscard]]
    friend bool operator==(const granular_filter&, const granular_filter&) noexcept = default;

    [[nodiscard]]
    operator bool() const noexcept
    {
      return m_SelectedSuites || m_SelectedItems;
    }
  private:
    [[no_unique_address]] ItemCompare m_Compare{};
    [[no_unique_address]] ItemProjector m_Proj{};
    std::optional<suites_map_type> m_SelectedSuites{};
    std::optional<items_map_type>  m_SelectedItems{};

    template<class Key>
    using opt_map_type = std::optional<std::vector<std::pair<Key, bool>>>;

    template<class Key>
    static void add(opt_map_type<Key>& map, Key k)
    {
      using map_type = typename opt_map_type<Key>::value_type;
      if(!map) map = map_type{};

      map->emplace_back(std::move(k), false);
    }

    template<class Key>
    [[nodiscard]]
    static opt_map_type<Key> make(std::optional<std::vector<Key>> selected)
    {
      if(!selected) return std::nullopt;

      std::vector<std::pair<Key, bool>> selection{};
      for (auto& e : *selected)
        selection.emplace_back(std::move(e), false);

      return selection;
    }

    template<class Key, class U, class Projector, class Comp>
    static bool find(opt_map_type<Key>& selected, const U& u, Projector proj, Comp compare)
    {
      if(selected)
      {
        auto found{std::ranges::find_if(*selected, [&proj, &compare, &u](const std::pair<Key, bool>& e) { return compare(e.first, proj(u)); })};

        if(found != selected->end())
        {
          found->second = true;
          return true;
        }
      }

      return false;
    }
  };
}
