#pragma once

#include <queue>
#include <stack>

namespace sequoia::maths::graph_impl
{
  template<class Container, class Compare> struct traversal_traits_base<std::priority_queue<std::size_t, Container, Compare>>
  {
    constexpr static bool uses_forward_iterator() { return true; }
    constexpr static auto get_container_element(const std::priority_queue<std::size_t, Container, Compare>& q)
    {
      return q.top();
    }
  };

  template<> struct traversal_traits_base<std::stack<std::size_t>>
  {
    constexpr static bool uses_forward_iterator() { return false; }
    constexpr static auto get_container_element(const std::stack<std::size_t>& s) { return s.top(); }
  };


  template<> struct traversal_traits_base<std::queue<std::size_t>>
  {
    constexpr static bool uses_forward_iterator() { return true; }
    constexpr static auto get_container_element(const std::queue<std::size_t>& q) { return q.front(); }
  };

  template<class G, class Container, class Comparer>
  struct queue_constructor<G, std::priority_queue<std::size_t, Container, Comparer>>
  {
    static auto make(const G& g)
    {
      return std::priority_queue<std::size_t, Container, Comparer>{Comparer{g}};
    }
  };

  template <class G, class Container>
  struct queue_constructor<G, std::stack<std::size_t, Container>>
  {
    static auto make(const G& g)
    {
      return std::stack<std::size_t, Container>{};
    }
  };

  template <class G, class Container>
  struct queue_constructor<G, std::queue<std::size_t, Container>>
  {
    static auto make(const G& g)
    {
      return std::queue<std::size_t, Container>{};
    }
  };

  template<class G, bool> struct stack_selector;

  template<class G>
  struct stack_selector<G, false>
  {
    using stack_type = std::stack<std::size_t>;
  };
  
  template<class G, bool> struct queue_selector; 

  template<class G>
  struct queue_selector<G, false>
  {
    using queue_type = std::queue<std::size_t>;
  };

  template<class G, class Compare, bool> struct priority_queue_selector;

  template<class G, class Compare>
  struct priority_queue_selector<G, Compare, false>
  {
    using queue_type = std::priority_queue<std::size_t, std::vector<size_t>, Compare>;
  };
}
