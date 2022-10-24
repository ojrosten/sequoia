////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Meta-prorgamming utilities for traversals of dynamic graphs.

 */

#include "sequoia/Maths/Graph/GraphTraversalDetails.hpp"

#include <queue>
#include <stack>

namespace sequoia::maths::graph_impl
{
  template<dynamic_network G> struct traversal_tracking_traits<G>
  {
    using bitset = std::vector<bool>;

    [[nodiscard]]
    static bitset make_bitset(const G& g)
    {
      return bitset(g.order(), false);
    }
  };

  // TO DO: refine this
  template<dynamic_tree G> struct traversal_tracking_traits<G>
  {
    using bitset = std::vector<bool>;

    [[nodiscard]]
    static bitset make_bitset(const G& g)
    {
      return bitset(g.order(), false);
    }
  };

  template<class Container, class Compare>
  struct traversal_traits_base<std::priority_queue<std::size_t, Container, Compare>>
  {
    [[nodiscard]]
    constexpr static bool uses_forward_iterator() noexcept { return true; }

    [[nodiscard]]
    static auto get_container_element(const std::priority_queue<std::size_t, Container, Compare>& q)
    {
      return q.top();
    }
  };

  template<>
  struct traversal_traits_base<std::stack<std::size_t>>
  {
    [[nodiscard]]
    constexpr static bool uses_forward_iterator() noexcept { return false; }

    [[nodiscard]]
    static auto get_container_element(const std::stack<std::size_t>& s) { return s.top(); }
  };

  template<>
  struct traversal_traits_base<std::queue<std::size_t>>
  {
    [[nodiscard]]
    constexpr static bool uses_forward_iterator() noexcept { return true; }

    [[nodiscard]]
    static auto get_container_element(const std::queue<std::size_t>& q) { return q.front(); }
  };

  template<dynamic_network G, class Q>
  struct traversal_traits<G, Q> : public traversal_traits_base<Q>
  {
    [[nodiscard]]
    static auto begin(const G& graph, const std::size_t nodeIndex)
    {
      return iterator_getter<traversal_traits_base<Q>::uses_forward_iterator()>::begin(graph, nodeIndex);
    }

    [[nodiscard]]
    static auto end(const G& graph, const std::size_t nodeIndex)
    {
      return iterator_getter<traversal_traits_base<Q>::uses_forward_iterator()>::end(graph, nodeIndex);
    }
  };

  template<dynamic_network G>
  struct queue_traits<G, traversal_flavour::breadth_first>
  {
    using queue_type = std::queue<std::size_t>;

    [[nodiscard]]
    static queue_type make()
    {
      return {};
    }
  };

  template<dynamic_network G>
  struct queue_traits<G, traversal_flavour::pseudo_depth_first>
  {
    using queue_type = std::stack<std::size_t>;

    [[nodiscard]]
    static queue_type make()
    {
      return {};
    }
  };

  template<dynamic_network G, class Compare>
  struct queue_traits<G, traversal_flavour::priority, Compare>
  {
    using queue_type = std::priority_queue<std::size_t, std::vector<size_t>, Compare>;

    [[nodiscard]]
    static queue_type make(Compare c)
    {
      return queue_type{std::move(c)};
    }
  };
}
