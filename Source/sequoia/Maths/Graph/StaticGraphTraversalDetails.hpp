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

#include "sequoia/Core/DataStructures/StaticStack.hpp"
#include "sequoia/Core/DataStructures/StaticQueue.hpp"
#include "sequoia/Core/DataStructures/StaticPriorityQueue.hpp"

namespace sequoia::maths::graph_impl
{
  template<class Q> struct traversal_traits_base;
  
  template<std::size_t MaxDepth, class Compare> struct traversal_traits_base<data_structures::static_priority_queue<std::size_t, MaxDepth, Compare>>
  {
    [[nodiscard]]
    constexpr static bool uses_forward_iterator() noexcept { return true; }

    [[nodiscard]]
    constexpr static auto get_container_element(const data_structures::static_priority_queue<std::size_t, MaxDepth, Compare>& q)
    {
      return q.top();
    }
  };

  template<std::size_t MaxDepth>
  struct traversal_traits_base<data_structures::static_stack<std::size_t, MaxDepth>>
  {
    [[nodiscard]]
    constexpr static bool uses_forward_iterator() noexcept { return false; }

    [[nodiscard]]
    constexpr static auto get_container_element(const data_structures::static_stack<std::size_t, MaxDepth>& s)
    {
      return s.top();
    }
  };

  template<std::size_t MaxDepth>
  struct traversal_traits_base<data_structures::static_queue<std::size_t, MaxDepth>>
  {
    [[nodiscard]]
    constexpr static bool uses_forward_iterator() noexcept { return true; }

    [[nodiscard]]
    constexpr static auto get_container_element(const data_structures::static_queue<std::size_t, MaxDepth>& q)
    {
      return q.front();
    }
  };

  template <class G>
  struct queue_constructor<G, data_structures::static_stack<std::size_t, G::order()>>
  {
    constexpr static auto make(const G&)
    {
      return data_structures::static_stack<std::size_t, G::order()>{};
    }
  };

  template <class G>
  struct queue_constructor<G, data_structures::static_queue<std::size_t, G::order()>>
  {
    [[nodiscard]]
    constexpr static auto make(const G&)
    {
      return data_structures::static_queue<std::size_t, G::order()>{};
    }
  };

  template <class G, class Comparer>
  struct queue_constructor<G, data_structures::static_priority_queue<std::size_t, G::order(), Comparer>>
  {
    [[nodiscard]]
    constexpr static auto make(const G& g)
    {
      return data_structures::static_priority_queue<std::size_t, G::order(), Comparer>{Comparer{g}};
    }
  };
 
  template<static_network G>
  struct stack_selector<G>
  {
    using stack_type = data_structures::static_stack<std::size_t, G::order()>;
  };

  template<static_network G>
  struct queue_selector<G>
  {
    using queue_type = data_structures::static_queue<std::size_t, G::order()>;
  };

  template<static_network G, class Compare>
  struct priority_queue_selector<G, Compare>
  {
    using queue_type = data_structures::static_priority_queue<std::size_t, G::order(), Compare>;
  };
  
}
