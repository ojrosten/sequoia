#pragma once

#include <queue>
#include <stack>

namespace sequoia::maths::graph_impl
{
  template<bool Forward> struct iterator_getter
  {
    template<class G> static auto begin(const G& graph, const std::size_t nodeIndex) { return graph.cbegin_edges(nodeIndex); }
    template<class G> static auto end(const G& graph, const std::size_t nodeIndex) { return graph.cend_edges(nodeIndex); }
  };

  template<> struct iterator_getter<false>
  {
    template<class G> static auto begin(const G& graph, const std::size_t nodeIndex) { return graph.crbegin_edges(nodeIndex); }
    template<class G> static auto end(const G& graph, const std::size_t nodeIndex) { return graph.crend_edges(nodeIndex); }
  };
      
  template<class Q> struct traversal_traits_base
  {
    static constexpr bool uses_forward_iterator() { return true; }
    static auto get_container_element(const Q& q) { return q.top(); }
  };

  template<> struct traversal_traits_base<std::stack<std::size_t>>
  {
    static constexpr bool uses_forward_iterator() { return false; }
    static auto get_container_element(const std::stack<std::size_t>& s) { return s.top(); }
  };


  template<> struct traversal_traits_base<std::queue<std::size_t>>
  {
    static constexpr bool uses_forward_iterator() { return true; }
    static auto get_container_element(const std::queue<std::size_t>& q) { return q.front(); }
  };

  template<class Q> struct TraversalTraits : public traversal_traits_base<Q>
  {
    template<class G> static auto begin(const G& graph, const std::size_t nodeIndex)
    {
      return iterator_getter<traversal_traits_base<Q>::uses_forward_iterator()>::begin(graph, nodeIndex);
    }
    template<class G> static auto end(const G& graph, const std::size_t nodeIndex)
    {
      return iterator_getter<traversal_traits_base<Q>::uses_forward_iterator()>::end(graph, nodeIndex);
    }
  };
      
  template<class G, class Compare>
  class node_comparer
  {
  public:
    using compare_type = Compare;

    node_comparer(const G& g) : m_Graph(g) {}
    node_comparer(const node_comparer& in) : m_Graph(in.m_Graph) {};

    bool operator()(const std::size_t index1, const std::size_t index2)
    {
      Compare comparer;
      return comparer(*(m_Graph.cbegin_node_weights() + index1), *(m_Graph.cbegin_node_weights() + index2));
    }
  private:
    const G& m_Graph;
  };

  template<class G, class Q> struct queue_constructor;
      
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
    static auto make(const G& g) { return std::stack<std::size_t, Container>{}; }
  };

  template <class G, class Container>
  struct queue_constructor<G, std::queue<std::size_t, Container>>
  {
    static auto make(const G& g) { return std::queue<std::size_t, Container>{}; }
  };
}
