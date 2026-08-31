////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

module;

#include "sequoia/PlatformSpecific/Macros.hpp"

#include <algorithm>
#include <array>
#include <concepts>
#include <condition_variable>
#include <execution>
#include <functional>
#include <future>
#include <iterator>
#include <memory>
#include <mutex>
#include <queue>
#include <span>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

export module sequoia.maths.graph:StaticGraphTraversalDetails;

import :Edge;
import :EdgesAndNodesUtilities;
import :GraphDetails;
import :GraphTraits;
import :GraphTraversalDetails;
export import sequoia.algorithms;
export import sequoia.core.concurrency;
export import sequoia.core.container_utilities;
export import sequoia.core.data_structures;
export import sequoia.core.meta;
export import sequoia.core.object;
export import sequoia.platform_specific;

/** \file
    \brief Meta-programming utilities for traversals of static graphs.

 */


export namespace sequoia::maths::graph_impl
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

  template<static_network G>
  struct traversal_traits_base<G, traversal_flavour::breadth_first>
  {
    using queue_type = data_structures::static_queue<typename G::edge_index_type, G::order()>;

    [[nodiscard]]
    constexpr static auto get_container_element(const queue_type& q) { return q.front(); }
  };

  template<static_network G>
  struct traversal_traits_base<G, traversal_flavour::pseudo_depth_first>
  {
    using queue_type = data_structures::static_stack<typename G::edge_index_type, G::order()>;

    [[nodiscard]]
    constexpr static auto get_container_element(const queue_type& s) { return s.top(); }
  };

  template<static_network G, class Compare>
  struct traversal_traits_base<G, traversal_flavour::priority, Compare>
  {
    using queue_type = data_structures::static_priority_queue<typename G::edge_index_type, G::order(), Compare>;

    [[nodiscard]]
    constexpr static auto get_container_element(const queue_type& q) { return q.top(); }
  };
}
