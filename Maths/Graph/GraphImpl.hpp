////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file GraphImpl.hpp
    \brief Underlying class for the various different graph species.
 */

#include "Connectivity.hpp"
#include "HeterogeneousNodeDetails.hpp"

namespace sequoia
{
  namespace maths
  {
    template<class Connectivity, class Nodes>
    class graph_primitive : public Connectivity, public Nodes
    {
    private:
      using node_weight_type = typename Nodes::weight_type;

    public:
      using connectivity_type = Connectivity;
      using nodes_type        = Nodes;
      using edge_init_type    = typename Connectivity::edge_init_type;
      using edge_index_type   = typename Connectivity::edge_index_type;      
      using size_type         = std::common_type_t<typename Connectivity::size_type, typename Nodes::size_type>;

      using edges_initializer = std::initializer_list<std::initializer_list<edge_init_type>>;

      constexpr graph_primitive() = default;
      
      constexpr graph_primitive(edges_initializer edges)
        : graph_primitive(get_init_type(), edges)
      {
        static_assert(std::is_empty_v<node_weight_type> || std::is_default_constructible_v<node_weight_type>);
      }
      
      template<class N=node_weight_type, class=std::enable_if_t<!std::is_empty_v<N> && !std::is_same_v<N, graph_impl::heterogeneous_tag>>>
      constexpr graph_primitive(edges_initializer edges, std::initializer_list<N> nodeWeights)
        : graph_primitive(homo_init_type{}, edges, nodeWeights)
      {
        if(nodeWeights.size() != edges.size())
          throw std::logic_error("Node weight initializer and edges top-level initializer must be of same size");
      }

      template<class... NodeWeights, class N=node_weight_type, class=std::enable_if_t<std::is_same_v<N, graph_impl::heterogeneous_tag>>>
      constexpr graph_primitive(edges_initializer edges, NodeWeights&&... nodeWeights)
        : graph_primitive(hetero_init_type{}, edges, std::forward<NodeWeights>(nodeWeights)...)
      {}
      
      constexpr graph_primitive(const graph_primitive& in)
        : Connectivity{static_cast<const Connectivity&>(in)}
        , Nodes{static_cast<const Nodes&>(in)}
      {}

      [[nodiscard]]
      constexpr size_type size() const noexcept
      {
        return connectivity_type::size();
      }
      
      //===============================equality (not isomorphism) operators================================//

      [[nodiscard]]
      friend constexpr bool operator==(const graph_primitive& lhs, const graph_primitive& rhs) noexcept
      {
        return (static_cast<const Connectivity&>(lhs) == static_cast<const Connectivity&>(rhs))
            && (static_cast<const Nodes&>(lhs) == static_cast<const Nodes&>(rhs));
      }

      [[nodiscard]]
      friend constexpr bool operator!=(const graph_primitive& lhs, const graph_primitive& rhs) noexcept
      {
        return !(lhs == rhs);
      }
    protected:
      // Constructors with allocators

      template<class N>
      constexpr static bool enableNodeAllocation{!std::is_same_v<N, graph_impl::heterogeneous_tag> && !std::is_empty_v<N>};

      template
      <
        class EdgePartitionsAllocator,
        class NodeAllocator,
        class N=node_weight_type,
        std::enable_if_t<enableNodeAllocation<N>, int>  = 0
      >
      constexpr graph_primitive(const EdgePartitionsAllocator& edgeParitionsAlloc, const NodeAllocator& nodeAlloc)
        : Connectivity(edgeParitionsAlloc)
        , Nodes(nodeAlloc)
      {}

      template
      <
        class EdgePartitionsAllocator,
        class EdgeAllocator,
        class NodeAllocator,
        class N=node_weight_type,
        std::enable_if_t<enableNodeAllocation<N>, int>  = 0
      >
      constexpr graph_primitive(const EdgePartitionsAllocator& edgeParitionsAlloc, const EdgeAllocator& edgeAlloc, const NodeAllocator& nodeAlloc)
        : Connectivity(edgeParitionsAlloc, edgeAlloc)
        , Nodes(nodeAlloc)
      {}

      template
      <
        class EdgePartitionsAllocator,
        class N=node_weight_type,
        std::enable_if_t<!enableNodeAllocation<N>, int>  = 0
      >
      constexpr graph_primitive(const EdgePartitionsAllocator& edgeParitionsAlloc)
        : Connectivity(edgeParitionsAlloc)
      {}

      template
      <
        class EdgePartitionsAllocator,
        class EdgeAllocator,
        class N=node_weight_type,
        std::enable_if_t<!enableNodeAllocation<N>, int>  = 0
      >
      constexpr graph_primitive(const EdgePartitionsAllocator& edgeParitionsAlloc, const EdgeAllocator& edgeAlloc)
        : Connectivity(edgeParitionsAlloc, edgeAlloc)
      {}

      template
      <
        class EdgePartitionsAllocator,
        class EdgeAllocator,
        class NodeAllocator,
        class N=node_weight_type,
        std::enable_if_t<enableNodeAllocation<N>, int> = 0
      >
      constexpr graph_primitive(edges_initializer edges, const EdgePartitionsAllocator& edgeParitionsAlloc, const EdgeAllocator& edgeAlloc, std::initializer_list<node_weight_type> nodeWeights, const NodeAllocator& nodeAlloc)
        : Connectivity{edges, edgeParitionsAlloc, edgeAlloc}
        , Nodes{nodeWeights, nodeAlloc}
      {}

      template
      <
        class EdgePartitionsAllocator,
        class EdgeAllocator,
        class... NodeWeights,
        class N=node_weight_type,
        std::enable_if_t<std::is_same_v<N, graph_impl::heterogeneous_tag>, int> = 0
      >
      constexpr graph_primitive(edges_initializer edges, const EdgePartitionsAllocator& edgeParitionsAlloc, const EdgeAllocator& edgeAlloc, NodeWeights&&... nodeWeights)
        : Connectivity{edges, edgeParitionsAlloc, edgeAlloc}
        , Nodes{std::forward<NodeWeights>(nodeWeights)...}
      {}

      template
      <
        class EdgePartitionsAllocator,
        class EdgeAllocator,
        class NodeAllocator,
        class N=node_weight_type,
        std::enable_if_t<enableNodeAllocation<N>, int> = 0
      >
      constexpr graph_primitive(edges_initializer edges, const EdgePartitionsAllocator& edgeParitionsAlloc, const EdgeAllocator& edgeAlloc, const NodeAllocator& nodeAlloc)
        : Connectivity{edges, edgeParitionsAlloc, edgeAlloc}
        , Nodes(edges.size(), nodeAlloc)
      {}

      template
      <
        class EdgePartitionsAllocator,
        class EdgeAllocator,
        class N=node_weight_type,
        std::enable_if_t<!enableNodeAllocation<N>, int> = 0
      >
      constexpr graph_primitive(edges_initializer edges, const EdgePartitionsAllocator& edgeParitionsAlloc, const EdgeAllocator& edgeAlloc)
        : Connectivity{edges, edgeParitionsAlloc, edgeAlloc}
        , Nodes{}
      {}

      // Copy-like constructors with allocators

      template
      <
        class EdgePartitionsAllocator,
        class EdgeAllocator,
        class NodeAllocator,
        class N=node_weight_type,
        std::enable_if_t<enableNodeAllocation<N>, int> = 0
      >
      constexpr graph_primitive(const graph_primitive& in, const EdgePartitionsAllocator& edgeParitionsAlloc, const EdgeAllocator& edgeAlloc, const NodeAllocator& nodeAlloc)
        : Connectivity{static_cast<const Connectivity&>(in), edgeParitionsAlloc, edgeAlloc}
        , Nodes{static_cast<const Nodes&>(in, nodeAlloc)}
      {}

      template
      <
        class EdgePartitionsAllocator,
        class EdgeAllocator,
        class NodeAllocator,
        class N=node_weight_type,
        std::enable_if_t<!enableNodeAllocation<N>, int> = 0
      >
      constexpr graph_primitive(const graph_primitive& in, const EdgePartitionsAllocator& edgeParitionsAlloc, const EdgeAllocator& edgeAlloc)
        : Connectivity{static_cast<const Connectivity&>(in), edgeParitionsAlloc, edgeAlloc}
        , Nodes{static_cast<const Nodes&>(in)}
      {}

      constexpr graph_primitive(graph_primitive&&) noexcept = default;

      // Move-like constructors with allocators

      template
      <
        class EdgePartitionsAllocator,
        class EdgeAllocator,
        class NodeAllocator,
        class N=node_weight_type,
        std::enable_if_t<enableNodeAllocation<N>, int> = 0
      >
      constexpr graph_primitive(graph_primitive&& in, const EdgePartitionsAllocator& edgeParitionsAlloc, const EdgeAllocator& edgeAlloc, const NodeAllocator& nodeAlloc)
        : Connectivity{static_cast<Connectivity&&>(in), edgeParitionsAlloc, edgeAlloc}
        , Nodes{static_cast<Nodes&&>(in, nodeAlloc)}
      {}

      template
      <
        class EdgePartitionsAllocator,
        class EdgeAllocator,
        class NodeAllocator,
        class N=node_weight_type,
        std::enable_if_t<!enableNodeAllocation<N>, int> = 0
      >
      constexpr graph_primitive(graph_primitive&& in, const EdgePartitionsAllocator& edgeParitionsAlloc, const EdgeAllocator& edgeAlloc)
        : Connectivity{static_cast<Connectivity&&>(in), edgeParitionsAlloc, edgeAlloc}
        , Nodes{static_cast<Nodes&&>(in)}
      {}
      
      ~graph_primitive() = default;

      constexpr graph_primitive& operator=(graph_primitive&&) noexcept = default;
      
      constexpr graph_primitive& operator=(const graph_primitive& in)
      {
        auto tmp{in};
        *this = std::move(tmp);
        return *this;
      }

      constexpr void swap_nodes(size_type i, size_type j)
      {
        if constexpr(!std::is_empty_v<node_weight_type>)
        {
          Nodes::swap_nodes(i, j);
        }

        Connectivity::swap_nodes(i, j);
      }

      void reserve_nodes(const size_type size)
      {
        if constexpr(!std::is_empty_v<node_weight_type>)
        {
          Nodes::reserve(size);
        }
       
        Connectivity::reserve_nodes(size);
      }

      [[nodiscard]]
      size_type node_capacity() const noexcept
      {
        if constexpr(!std::is_empty_v<node_weight_type>)
        {
          return std::min(Connectivity::node_capacity(), Nodes::capacity());
        }
        else
        {       
          return Connectivity::node_capacity();
        }
      }

      void shrink_to_fit()
      {
        if constexpr(!std::is_empty_v<node_weight_type>)
        {
          Nodes::shrink_to_fit();
        }

        Connectivity::shrink_to_fit();
      }

      template
      <
        class... Args
        // std::enable_if_t<!is_allocator<typename variadic_traits<Args...>::head>, int> = 0
      >
      size_type add_node(Args&&... args)
      {
        reserve_nodes(this->order() + 1);
        
        Connectivity::add_node();
        if constexpr (!emptyNodes) Nodes::add_node(std::forward<Args>(args)...);
        return (this->order()-1);
      }

      /*template
      <
        class Allocator,
        class... Args
      >
      size_type add_node(const Allocator& alloc, Args&&... args)
      {
        reserve_nodes(this->order() + 1);
        
        Connectivity::add_node(alloc);
        if constexpr (!emptyNodes) Nodes::add_node(std::forward<Args>(args)...);
        return (this->order()-1);
        }*/

      template<class... Args>
      size_type insert_node(const size_type pos, Args&&... args)
      {
        reserve_nodes(this->order() + 1);
        
        const auto node{(pos < this->order()) ? pos : (this->order() - 1)};
        if constexpr (!emptyNodes)
        {
          Nodes::insert_node(this->cbegin_node_weights() + pos, std::forward<Args>(args)...);
        }
        
        Connectivity::insert_node(node);

        return node;
      }

      void erase_node(const size_type node)
      {
        Connectivity::erase_node(node);
        if constexpr (!emptyNodes) Nodes::erase_node(this->cbegin_node_weights() + node);
      }

      template<class... Args>
      void join(const edge_index_type node1, const edge_index_type node2, Args&&... args)
      {
        Connectivity::join(node1, node2, std::forward<Args>(args)...);
      }

      void clear() noexcept
      {
        if constexpr (!emptyNodes) Nodes::clear();
        Connectivity::clear(); 
      }
    private:
      constexpr static bool emptyNodes{std::is_empty_v<typename Nodes::weight_type>};
      
      template<bool Hetero>
      struct node_init_constant : std::bool_constant<Hetero>
      {};
      
      using hetero_init_type = node_init_constant<true>;
      using homo_init_type   = node_init_constant<false>;

      constexpr static auto get_init_type()
      {
        if constexpr(std::is_same_v<node_weight_type, graph_impl::heterogeneous_tag>)
        {
          return hetero_init_type{};
        }
        else
        {
          return homo_init_type{};
        }    
      }

      template
      <
        class N=node_weight_type,
        std::enable_if_t<!std::is_same_v<N, graph_impl::heterogeneous_tag>, int> = 0
      >
      constexpr graph_primitive(homo_init_type, edges_initializer edges, std::initializer_list<node_weight_type> nodeWeights)
        : Connectivity{edges}
        , Nodes{nodeWeights}
      {}

      template
      <
        class... NodeWeights,
        class N=node_weight_type, std::enable_if_t<std::is_same_v<N, graph_impl::heterogeneous_tag>, int> = 0
      >
      constexpr graph_primitive(hetero_init_type, edges_initializer edges, NodeWeights&&... nodeWeights)
        : Connectivity{edges}
        , Nodes{std::forward<NodeWeights>(nodeWeights)...}
      {}

      template
      <
        class N=node_weight_type,
        std::enable_if_t<!std::is_same_v<N, graph_impl::heterogeneous_tag> && !std::is_empty_v<N>, int> = 0
      >
      constexpr graph_primitive(homo_init_type, edges_initializer edges)
        : Connectivity{edges}
        , Nodes(edges.size())
      {}

      template
      <
        class N=node_weight_type,
        std::enable_if_t<!std::is_same_v<N, graph_impl::heterogeneous_tag> && std::is_empty_v<N>, int> = 0
      >
      constexpr graph_primitive(homo_init_type, edges_initializer edges)
        : Connectivity{edges}
        , Nodes{}
      {}

      template
      <
        class N=node_weight_type,
        std::enable_if_t<std::is_same_v<N, graph_impl::heterogeneous_tag>, int> = 0
      >
      constexpr graph_primitive(hetero_init_type, edges_initializer edges)
        : Connectivity{edges},
          Nodes{}
      {}
    };
  }
}
