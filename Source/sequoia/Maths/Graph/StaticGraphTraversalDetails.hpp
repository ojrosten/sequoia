////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file StaticGraphTraversalDetails.hpp
    \brief Meta-programming utilities for traversals of static graphs.

 */

#include "sequoia/Maths/Graph/GraphTraversalDetails.hpp"
#include "sequoia/Core/DataStructures/StaticStack.hpp"
#include "sequoia/Core/DataStructures/StaticQueue.hpp"
#include "sequoia/Core/DataStructures/StaticPriorityQueue.hpp"

namespace sequoia::maths::graph_impl
{
  template<static_network G> struct traversal_tracking_traits<G>
  {
    using bitset = std::array<bool, G::order()>;

    [[nodiscard]]
    constexpr static bitset make_bitset(const G&)
    {
      return bitset{};
    }
  };

  template<std::integral IndexType, std::size_t MaxDepth, class Compare> struct traversal_traits_base<data_structures::static_priority_queue<IndexType, MaxDepth, Compare>>
  {
    [[nodiscard]]
    constexpr static bool uses_forward_iterator() noexcept { return true; }

    [[nodiscard]]
    constexpr static auto get_container_element(const data_structures::static_priority_queue<IndexType, MaxDepth, Compare>& q)
    {
      return q.top();
    }
  };

  template<std::integral IndexType, std::size_t MaxDepth>
  struct traversal_traits_base<data_structures::static_stack<IndexType, MaxDepth>>
  {
    [[nodiscard]]
    constexpr static bool uses_forward_iterator() noexcept { return false; }

    [[nodiscard]]
    constexpr static auto get_container_element(const data_structures::static_stack<IndexType, MaxDepth>& s)
    {
      return s.top();
    }
  };

  template<std::integral IndexType, std::size_t MaxDepth>
  struct traversal_traits_base<data_structures::static_queue<IndexType, MaxDepth>>
  {
    [[nodiscard]]
    constexpr static bool uses_forward_iterator() noexcept { return true; }

    [[nodiscard]]
    constexpr static auto get_container_element(const data_structures::static_queue<IndexType, MaxDepth>& q)
    {
      return q.front();
    }
  };

  template<static_network G, class Q>
  struct traversal_traits<G, Q> : public traversal_traits_base<Q>
  {
    [[nodiscard]]
    constexpr static auto begin(const G& graph, const typename G::edge_index_type nodeIndex)
    {
      return iterator_getter<traversal_traits_base<Q>::uses_forward_iterator()>::begin(graph, nodeIndex);
    }

    [[nodiscard]]
    constexpr static auto end(const G& graph, const typename G::edge_index_type nodeIndex)
    {
      return iterator_getter<traversal_traits_base<Q>::uses_forward_iterator()>::end(graph, nodeIndex);
    }
  };

  template<static_network G>
  struct queue_traits<G, traversal_flavour::breadth_first>
  {
    using queue_type = data_structures::static_queue<typename G::edge_index_type, G::order()>;

    [[nodiscard]]
    constexpr static queue_type make(const G&)
    {
      return {};
    }
  };

  template<static_network G>
  struct queue_traits<G, traversal_flavour::pseudo_depth_first>
  {
    using queue_type = data_structures::static_stack<typename G::edge_index_type, G::order()>;

    [[nodiscard]]
    constexpr static queue_type make(const G&)
    {
      return {};
    }
  };

  template<static_network G, class Compare>
  struct queue_traits<G, traversal_flavour::priority, Compare>
  {
    using queue_type = data_structures::static_priority_queue<typename G::edge_index_type, G::order(), Compare>;

    [[nodiscard]]
    constexpr static queue_type make(const G& g)
    {
      return queue_type{Compare{g}};
    }
  };

}
