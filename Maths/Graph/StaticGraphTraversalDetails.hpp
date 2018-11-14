#pragma once

#include "StaticStack.hpp"
#include "StaticQueue.hpp"
#include "StaticPriorityQueue.hpp"

namespace sequoia::maths::graph_impl
{
  template<class Q> struct traversal_traits_base;
  
  template<std::size_t MaxPushes, class Compare> struct traversal_traits_base<data_structures::static_priority_queue<std::size_t, MaxPushes, Compare>>
  {
    constexpr static bool uses_forward_iterator() { return true; }
    constexpr static auto get_container_element(const data_structures::static_priority_queue<std::size_t, MaxPushes, Compare>& q)
    {
      return q.top();
    }
  };

  template<std::size_t MaxDepth>
  struct traversal_traits_base<data_structures::static_stack<std::size_t, MaxDepth>>
  {
    constexpr static bool uses_forward_iterator() { return false; }
    constexpr static auto get_container_element(const data_structures::static_stack<std::size_t, MaxDepth>& s)
    {
      return s.top();
    }
  };

  template<std::size_t MaxPushes>
  struct traversal_traits_base<data_structures::static_queue<std::size_t, MaxPushes>>
  {
    constexpr static bool uses_forward_iterator() { return true; }
    constexpr static auto get_container_element(const data_structures::static_queue<std::size_t, MaxPushes>& q)
    {
      return q.front();
    }
  };

  template <class G>
  struct queue_constructor<G, data_structures::static_stack<std::size_t, G::order()>>
  {
    constexpr static auto make(const G& g)
    {
      return data_structures::static_stack<std::size_t, G::order()>{};
    }
  };

  template <class G>
  struct queue_constructor<G, data_structures::static_queue<std::size_t, G::order()>>
  {
    constexpr static auto make(const G& g)
    {
      return data_structures::static_queue<std::size_t, G::order()>{};
    }
  };

  template <class G, class Comparer>
  struct queue_constructor<G, data_structures::static_priority_queue<std::size_t, G::order(), Comparer>>
  {
    constexpr static auto make(const G& g)
    {
      return data_structures::static_priority_queue<std::size_t, G::order(), Comparer>{Comparer{g}};
    }
  };

  template<class G, bool> struct stack_selector;
 
  template<class G>
  struct stack_selector<G, true>
  {
    using stack_type = data_structures::static_stack<std::size_t, G::order()>;
  };

  template<class G, bool> struct queue_selector;

  template<class G>
  struct queue_selector<G, true>
  {
    using queue_type = data_structures::static_queue<std::size_t, G::order()>;
  };

  template<class G, class Compare, bool> struct priority_queue_selector;

  template<class G, class Compare>
  struct priority_queue_selector<G, Compare, true>
  {
    using queue_type = data_structures::static_priority_queue<std::size_t, G::order(), Compare>;
  };
  
}
