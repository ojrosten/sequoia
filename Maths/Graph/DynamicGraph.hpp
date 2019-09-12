////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file DynamicGraph.hpp
    \brief Edge & Node storage traits, base class and final classes for dynamic graphs.
  
*/

#include "GraphImpl.hpp"
#include "DynamicGraphImpl.hpp"

#include "NodeStorage.hpp"

namespace sequoia::maths
{
  template<graph_flavour GraphFlavour, class EdgeWeight, template <class> class EdgeWeightPooling>
  struct contiguous_edge_storage_traits
  {
    template <class T, class Sharing, class Traits> using storage_type = data_structures::contiguous_storage<T, Sharing, Traits>;
    template <class T, class Sharing> using traits_type = data_structures::contiguous_storage_traits<T, Sharing>;

    constexpr static edge_sharing_preference edge_sharing{edge_sharing_preference::agnostic};
  };

  template<graph_flavour GraphFlavour, class EdgeWeight, template <class> class EdgeWeightPooling>
  struct bucketed_edge_storage_traits
  {
    template <class T, class Sharing, class Traits> using storage_type = data_structures::bucketed_storage<T, Sharing, Traits>;
    template <class T, class Sharing> using traits_type = data_structures::bucketed_storage_traits<T, Sharing>;

    constexpr static edge_sharing_preference edge_sharing{edge_sharing_preference::agnostic};
  };

  template<class NodeWeight, template <class> class NodeWeightPooling, bool=std::is_empty_v<NodeWeight>>
  struct node_weight_storage_traits
  {
    constexpr static bool throw_on_range_error{true};
    constexpr static bool static_storage_v{};
    template<class S> using container_type = std::vector<S, std::allocator<S>>;
  };

  template<class NodeWeight, template <class> class NodeWeightPooling>
  struct node_weight_storage_traits<NodeWeight, NodeWeightPooling, true>
  {
  };
  
  template
  <
    graph_flavour GraphFlavour,      
    class EdgeWeight,
    class NodeWeight,      
    template <class> class EdgeWeightPooling,
    template <class> class NodeWeightPooling,
    template<graph_flavour, class, template<class> class> class EdgeStorageTraits,
    template<class, template<class> class, bool> class NodeWeightStorageTraits
  >
  class graph_base : public
    graph_primitive
    <
      connectivity
      <
        to_directedness(GraphFlavour),
        graph_impl::dynamic_edge_traits<GraphFlavour, EdgeWeight, EdgeWeightPooling, EdgeStorageTraits, std::size_t>,
        graph_impl::weight_maker<EdgeWeightPooling<EdgeWeight>>
      >,
      graph_impl::node_storage
      <     
        graph_impl::weight_maker<NodeWeightPooling<NodeWeight>>,
        NodeWeightStorageTraits<NodeWeight, NodeWeightPooling, std::is_empty_v<NodeWeight>>
      >
    >
  {
  private:
    using edge_traits_type = graph_impl::dynamic_edge_traits<GraphFlavour, EdgeWeight, EdgeWeightPooling, EdgeStorageTraits, std::size_t>;
    using node_storage_type
      = graph_impl::node_storage
        <
          graph_impl::weight_maker<NodeWeightPooling<NodeWeight>>,
          NodeWeightStorageTraits<NodeWeight, NodeWeightPooling, std::is_empty_v<NodeWeight>>
        >; 
  public:
    using primitive_type =
      graph_primitive
      <
        connectivity<
          to_directedness(GraphFlavour),
          edge_traits_type,
          graph_impl::weight_maker<EdgeWeightPooling<EdgeWeight>>
        >,      
        graph_impl::node_storage
        <
          graph_impl::weight_maker<NodeWeightPooling<NodeWeight>>,
          NodeWeightStorageTraits<NodeWeight, NodeWeightPooling, std::is_empty_v<NodeWeight>>
        >
      >;
    
    using node_weight_type = NodeWeight;
    using size_type = typename primitive_type::size_type;
    using edges_initializer = typename primitive_type::edges_initializer;
    
    using edge_partitions_allocator_type = typename edge_traits_type::edge_partitions_allocator_type;
    using edge_allocator_type            = typename edge_traits_type::edge_allocator_type;

    graph_base() = default;

    template
    <
      class EdgeStorage = typename edge_traits_type::edge_storage_type,
      class EdgePartitionsAllocator = edge_partitions_allocator_type,
      class N = NodeWeight,
      std::enable_if_t<is_constructible_with_v<EdgeStorage, EdgePartitionsAllocator> && std::is_empty_v<N>, int> = 0
    >
    graph_base(const EdgePartitionsAllocator& edgePartitionsAllocator)
      : primitive_type(edgePartitionsAllocator)
    {}

    template
    <
      class EdgeStorage = typename edge_traits_type::edge_storage_type,
      class EdgePartitionsAllocator = edge_partitions_allocator_type,
      class N = node_storage_type,
      class NodeWeightAllocator = typename graph_impl::node_allocator_generator<N>::allocator_type,
      std::enable_if_t<is_constructible_with_v<EdgeStorage, EdgePartitionsAllocator> && !std::is_empty_v<N>, int> = 0
    >
    graph_base(const EdgePartitionsAllocator& edgePartitionsAllocator, const NodeWeightAllocator& nodeWeightAllocator)
      : primitive_type(edgePartitionsAllocator, nodeWeightAllocator)
    {}

    template
    <
      class EdgeStorage = typename edge_traits_type::edge_storage_type,
      class EdgePartitionsAllocator = edge_partitions_allocator_type,
      class EdgeAllocator = edge_allocator_type,
      class N = NodeWeight,
      std::enable_if_t<is_constructible_with_v<EdgeStorage, EdgePartitionsAllocator, EdgeAllocator> && std::is_empty_v<N>, int> = 0
    >
    graph_base(const EdgePartitionsAllocator& edgePartitionsAllocator, const EdgeAllocator& edgeAllocator)
      : primitive_type(edgePartitionsAllocator, edgeAllocator)
    {}

    template
    <
      class EdgeStorage = typename edge_traits_type::edge_storage_type,
      class EdgePartitionsAllocator = edge_partitions_allocator_type,
      class EdgeAllocator = edge_allocator_type,
      class N = node_storage_type,
      class NodeWeightAllocator = typename graph_impl::node_allocator_generator<N>::allocator_type,
      std::enable_if_t<is_constructible_with_v<EdgeStorage, EdgePartitionsAllocator, EdgeAllocator> && !std::is_empty_v<N>, int> = 0
    >
    graph_base(const EdgePartitionsAllocator& edgePartitionsAllocator, const EdgeAllocator& edgeAllocator, const NodeWeightAllocator& nodeWeightAllocator)
      : primitive_type(edgePartitionsAllocator, edgeAllocator, nodeWeightAllocator)
    {}

    graph_base(edges_initializer edges) : primitive_type{edges} {}

    template
    <
      class EdgeStorage = typename edge_traits_type::edge_storage_type,
      class EdgePartitionsAllocator = edge_partitions_allocator_type,
      class EdgeAllocator = edge_allocator_type,
      class N = NodeWeight,
      std::enable_if_t<std::is_empty_v<N>, int> = 0
    >
    graph_base(edges_initializer edges, const EdgePartitionsAllocator& edgePartitionsAllocator, const EdgeAllocator& edgeAllocator)
      : primitive_type{edges, edgePartitionsAllocator, edgeAllocator}
    {}

    template
    <
      class EdgeStorage = typename edge_traits_type::edge_storage_type,
      class EdgePartitionsAllocator = edge_partitions_allocator_type,
      class EdgeAllocator = edge_allocator_type,
      class N = node_storage_type,
      class NodeWeightAllocator = typename graph_impl::node_allocator_generator<N>::allocator_type,
      std::enable_if_t<!std::is_empty_v<N>, int> = 0
    >
    graph_base(edges_initializer edges, const EdgePartitionsAllocator& edgePartitionsAllocator, const EdgeAllocator& edgeAllocator, const NodeWeightAllocator& nodeWeightAllocator)
      : primitive_type{edges, edgePartitionsAllocator, edgeAllocator, nodeWeightAllocator}
    {}

    template
    <
      class N=NodeWeight,
      std::enable_if_t<!std::is_empty_v<N>, int> = 0
    >
    graph_base(edges_initializer edges, std::initializer_list<node_weight_type> nodeWeights)
      : primitive_type{edges, nodeWeights}
    {}

    template
    <
      class EdgeStorage = typename edge_traits_type::edge_storage_type,
      class EdgePartitionsAllocator = edge_partitions_allocator_type,
      class EdgeAllocator = edge_allocator_type,
      class N = node_storage_type,
      class NodeWeightAllocator = typename graph_impl::node_allocator_generator<N>::allocator_type,
      std::enable_if_t<!std::is_empty_v<N>, int> = 0
    >
    graph_base(edges_initializer edges, const EdgePartitionsAllocator& edgePartitionsAllocator, const EdgeAllocator& edgeAllocator, std::initializer_list<node_weight_type> nodeWeights, const NodeWeightAllocator& nodeWeightAllocator)
      : primitive_type{edges, edgePartitionsAllocator, edgeAllocator, nodeWeights, nodeWeightAllocator}
    {}

    graph_base(const graph_base&) = default;

    template
    <
      class EdgeStorage = typename edge_traits_type::edge_storage_type,
      class EdgePartitionsAllocator = edge_partitions_allocator_type,
      class EdgeAllocator = edge_allocator_type,
      class N = NodeWeight,
      std::enable_if_t<std::is_empty_v<N>, int> = 0
    >
    graph_base(const graph_base& in, const EdgePartitionsAllocator& edgePartitionsAllocator, const EdgeAllocator& edgeAllocator)
      : primitive_type{in, edgePartitionsAllocator, edgeAllocator}
    {}

    template
    <
      class EdgeStorage = typename edge_traits_type::edge_storage_type,
      class EdgePartitionsAllocator = edge_partitions_allocator_type,
      class EdgeAllocator = edge_allocator_type,
      class N = node_storage_type,
      class NodeWeightAllocator = typename graph_impl::node_allocator_generator<N>::allocator_type,
      std::enable_if_t<!std::is_empty_v<N>, int> = 0
    >
    graph_base(const graph_base& in, const EdgePartitionsAllocator& edgePartitionsAllocator, const EdgeAllocator& edgeAllocator, const NodeWeightAllocator& nodeWeightAllocator)
      : primitive_type{in, edgePartitionsAllocator, edgeAllocator, nodeWeightAllocator}
    {}
    
    graph_base(graph_base&&) noexcept = default;

    template
    <
      class EdgeStorage = typename edge_traits_type::edge_storage_type,
      class EdgePartitionsAllocator = edge_partitions_allocator_type,
      class EdgeAllocator = edge_allocator_type,
      class N = NodeWeight,
      std::enable_if_t<std::is_empty_v<N>, int> = 0
    >
    graph_base(graph_base&& in, const EdgePartitionsAllocator& edgePartitionsAllocator, const EdgeAllocator& edgeAllocator)
      : primitive_type{std::move(in), edgePartitionsAllocator, edgeAllocator}
    {}

    template
    <
      class EdgeStorage = typename edge_traits_type::edge_storage_type,
      class EdgePartitionsAllocator = edge_partitions_allocator_type,
      class EdgeAllocator = edge_allocator_type,
      class N = node_storage_type,
      class NodeWeightAllocator = typename graph_impl::node_allocator_generator<N>::allocator_type,
      std::enable_if_t<!std::is_empty_v<N>, int> = 0
    >
    graph_base(graph_base&& in, const EdgePartitionsAllocator& edgePartitionsAllocator, const EdgeAllocator& edgeAllocator, const NodeWeightAllocator& nodeWeightAllocator)
      : primitive_type{std::move(in), edgePartitionsAllocator, edgeAllocator, nodeWeightAllocator}
    {}
    
    graph_base& operator=(const graph_base&) = default;

    graph_base& operator=(graph_base&&) = default;

    using primitive_type::swap;
    using primitive_type::swap_nodes;

    friend void swap(graph_base& lhs, graph_base& rhs) noexcept(noexcept(lhs.swap(rhs)))
    {
      lhs.swap(rhs);
    }

    template
    <
      class... Args,
      std::enable_if_t<(sizeof...(Args) == 0) || !is_allocator_v<typename variadic_traits<Args...>::head>, int> = 0
    >
    size_type add_node(Args&&... args)
    {
      return primitive_type::add_node(std::forward<Args>(args)...);
    }

    template
    <    
      class EdgeAllocator = edge_allocator_type,
      class... Args,
      class EdgeStorage = typename edge_traits_type::edge_storage_type,
      class EdgePartitionsAllocator = edge_partitions_allocator_type,
      std::enable_if_t<!is_constructible_with_v<EdgeStorage, EdgePartitionsAllocator, EdgeAllocator>, int> = 0
    >
    size_type add_node(const EdgeAllocator& alloc, Args&&... args)
    {
      return primitive_type::add_node(alloc, std::forward<Args>(args)...);
    }
    
    template
    <
      class... Args,
      std::enable_if_t<(sizeof...(Args) == 0) || !is_allocator_v<typename variadic_traits<Args...>::head>, int> = 0
    >
    size_type insert_node(const size_type pos, Args&&... args)
    {
      return primitive_type::insert_node(pos, std::forward<Args>(args)...);
    }

    template
    <      
      class EdgeAllocator = edge_allocator_type,
      class... Args,
      class EdgeStorage = typename edge_traits_type::edge_storage_type,
      class EdgePartitionsAllocator = edge_partitions_allocator_type,
      std::enable_if_t<!is_constructible_with_v<EdgeStorage, EdgePartitionsAllocator, EdgeAllocator>, int> = 0
    >
    size_type insert_node(const size_type pos, const EdgeAllocator& allocator, Args&&... args)
    {
      return primitive_type::insert_node(pos, allocator, std::forward<Args>(args)...);
    }

    using primitive_type::erase_node;

    using primitive_type::join;      
    using primitive_type::erase_edge;

    using primitive_type::clear;

    using primitive_type::reserve_edges;
    using primitive_type::edges_capacity;
    using primitive_type::reserve_nodes;
    using primitive_type::node_capacity;
    using primitive_type::shrink_to_fit;
      
    constexpr static graph_flavour flavour{GraphFlavour};
  protected:
    ~graph_base() = default;
  };

  template
  <
    directed_flavour Directedness,      
    class EdgeWeight,
    class NodeWeight,      
    template <class> class EdgeWeightPooling=data_sharing::unpooled,
    template <class> class NodeWeightPooling=data_sharing::unpooled,
    template<graph_flavour, class, template<class> class> class EdgeStorageTraits = bucketed_edge_storage_traits,
    template<class, template<class> class, bool> class NodeWeightStorageTraits = node_weight_storage_traits
  >
  class graph final : public
    graph_base
    <
      (Directedness == directed_flavour::directed) ? graph_flavour::directed : graph_flavour::undirected,      
      EdgeWeight,
      NodeWeight,      
      EdgeWeightPooling,
      NodeWeightPooling,
      EdgeStorageTraits,
      NodeWeightStorageTraits    
    >
  {
  private:
    using base =
      graph_base
      <
        (Directedness == directed_flavour::directed) ? graph_flavour::directed : graph_flavour::undirected,      
         EdgeWeight,
         NodeWeight,     
         EdgeWeightPooling,
         NodeWeightPooling,
         EdgeStorageTraits,
         NodeWeightStorageTraits
      >;
  public:
    using node_weight_type = NodeWeight;
    
    using
      graph_base
      <
        (Directedness == directed_flavour::directed) ? graph_flavour::directed : graph_flavour::undirected,      
         EdgeWeight,
         NodeWeight, 
         EdgeWeightPooling,
         NodeWeightPooling,
         EdgeStorageTraits,
         NodeWeightStorageTraits
      >::graph_base;

    using base::sort_edges;
    using base::swap_edges;
  };

  template
  <
    directed_flavour Directedness,      
    class EdgeWeight,
    class NodeWeight,      
    template <class> class EdgeWeightPooling=data_sharing::unpooled,
    template <class> class NodeWeightPooling=data_sharing::unpooled,
    template<graph_flavour, class, template<class> class> class EdgeStorageTraits=bucketed_edge_storage_traits,
    template<class, template<class> class, bool> class NodeWeightStorageTraits=node_weight_storage_traits
  >
  class embedded_graph final : public
    graph_base
    <
      (Directedness == directed_flavour::directed) ? graph_flavour::directed_embedded : graph_flavour::undirected_embedded,      
      EdgeWeight,
      NodeWeight,     
      EdgeWeightPooling,
      NodeWeightPooling,
      EdgeStorageTraits,
      NodeWeightStorageTraits
    >
    {
    private:
      [[nodiscard]]
      static constexpr graph_flavour to_graph_flavour() noexcept
      {
        return (Directedness == directed_flavour::directed) ? graph_flavour::directed_embedded : graph_flavour::undirected_embedded;
      }
    public:
      using node_weight_type = NodeWeight;
      
      using
        graph_base
        <
         (Directedness == directed_flavour::directed) ? graph_flavour::directed_embedded : graph_flavour::undirected_embedded,      
          EdgeWeight,
          NodeWeight,      
          EdgeWeightPooling,
          NodeWeightPooling,
          EdgeStorageTraits,
          NodeWeightStorageTraits
        >::graph_base;
      
      using base_type =
        graph_base
        <
          (Directedness == directed_flavour::directed) ? graph_flavour::directed_embedded : graph_flavour::undirected_embedded,      
          EdgeWeight,
          NodeWeight,     
          EdgeWeightPooling,
          NodeWeightPooling,
          EdgeStorageTraits,
          NodeWeightStorageTraits
        >;

      using base_type::primitive_type::insert_join;
    };  
}
