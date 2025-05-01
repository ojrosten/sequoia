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
  template<class G>
    requires dynamic_network<G> || dynamic_tree<G>
  struct traversal_tracking_traits<G>
  {
    using bitset = std::vector<bool>;

    [[nodiscard]]
    static bitset make_bitset(const G& g)
    {
      return bitset(g.order(), false);
    }
  };

  template<dynamic_network G>
  struct traversal_traits_base<G, traversal_flavour::breadth_first>
  {
    using queue_type = std::queue<std::size_t>;

    [[nodiscard]]
    static auto get_container_element(const queue_type& q) { return q.front(); }
  };

  template<dynamic_network G>
  struct traversal_traits_base<G, traversal_flavour::pseudo_depth_first>
  {
    using queue_type = std::stack<std::size_t>;

    [[nodiscard]]
    static auto get_container_element(const queue_type& s) { return s.top(); }
  };

  template<dynamic_network G, class Compare>
  struct traversal_traits_base<G, traversal_flavour::priority, Compare>
  {
    using queue_type = std::priority_queue<std::size_t, std::vector<size_t>, Compare>;

    [[nodiscard]]
    static auto get_container_element(const queue_type& q) { return q.top(); }
  };
}
