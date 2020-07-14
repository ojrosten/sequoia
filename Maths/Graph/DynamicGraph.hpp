////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Edge & Node storage traits, base class and final classes for dynamic graphs.
  
*/

#include "GraphImpl.hpp"
#include "DynamicGraphImpl.hpp"

#include "NodeStorage.hpp"

namespace sequoia::maths
{
  struct contiguous_edge_storage_traits
  {
    template <class T, class Sharing, class Traits> using storage_type = data_structures::partitioned_sequence<T, Sharing, Traits>;
    template <class T, class Sharing> using traits_type = data_structures::partitioned_sequence_traits<T, Sharing>;

    constexpr static edge_sharing_preference edge_sharing{edge_sharing_preference::agnostic};
  };

  struct bucketed_edge_storage_traits
  {
    template <class T, class Sharing, class Traits> using storage_type = data_structures::bucketed_storage<T, Sharing, Traits>;
    template <class T, class Sharing> using traits_type = data_structures::bucketed_storage_traits<T, Sharing>;

    constexpr static edge_sharing_preference edge_sharing{edge_sharing_preference::agnostic};
  };

  template<class NodeWeight>
  struct node_weight_storage_traits
  {
    constexpr static bool throw_on_range_error{true};
    constexpr static bool static_storage_v{};
    constexpr static bool has_allocator{true};
    template<class S> using container_type = std::vector<S, std::allocator<S>>;
  };

  template<empty NodeWeight>
  struct node_weight_storage_traits<NodeWeight>
  {
    constexpr static bool has_allocator{};
  };
  
  template
  <
    graph_flavour GraphFlavour,      
    class EdgeWeight,
    class NodeWeight,      
    creator EdgeWeightCreator,
    creator NodeWeightCreator,
    class EdgeStorageTraits,
    class NodeWeightStorageTraits,
    bool = !NodeWeightStorageTraits::has_allocator
  >
  class graph_base : public
    graph_primitive
    <
      connectivity
      <
        to_directedness(GraphFlavour),
        graph_impl::dynamic_edge_traits<GraphFlavour, EdgeWeight, EdgeWeightCreator, EdgeStorageTraits, std::size_t>,
        EdgeWeightCreator
      >,
      graph_impl::node_storage
      <     
        NodeWeightCreator,
        NodeWeightStorageTraits
      >
    >
  {
  private:
    using edge_traits_type = graph_impl::dynamic_edge_traits<GraphFlavour, EdgeWeight, EdgeWeightCreator, EdgeStorageTraits, std::size_t>;
    using node_storage_type
      = graph_impl::node_storage
        <
          NodeWeightCreator,
          NodeWeightStorageTraits
        >; 
  public:
    using primitive_type =
      graph_primitive
      <
        connectivity
        <
          to_directedness(GraphFlavour),
          edge_traits_type,
          EdgeWeightCreator
        >,      
        graph_impl::node_storage
        <
          NodeWeightCreator,
          NodeWeightStorageTraits
        >
      >;
    
    using node_weight_type    = NodeWeight;
    using size_type           = typename primitive_type::size_type;
    using edges_initializer   = typename primitive_type::edges_initializer;    
    using edge_allocator_type = typename edge_traits_type::edge_allocator_type;

    graph_base() = default;

    template
    <
      alloc EdgeAllocator = edge_allocator_type,
      class N             = NodeWeight
    >
      requires empty<N>
    graph_base(const EdgeAllocator& edgeAllocator)
      : primitive_type(edgeAllocator)
    {}

    template
    <
      alloc EdgeAllocator           = edge_allocator_type,
      class N                       = node_storage_type,
      alloc NodeWeightAllocator     = typename graph_impl::node_allocator_generator<N>::allocator_type
    >
      requires (!empty<N>)
    graph_base(const EdgeAllocator& edgeAllocator, const NodeWeightAllocator& nodeWeightAllocator)
      : primitive_type(edgeAllocator, nodeWeightAllocator)
    {}

    template
    <
      alloc EdgeAllocator           = edge_allocator_type,
      class EdgeStorage             = typename edge_traits_type::edge_storage_type,
      alloc EdgePartitionsAllocator = typename EdgeStorage::partitions_allocator_type,
      class N                       = NodeWeight
    >
      requires (empty<N> && !std::is_convertible_v<EdgeAllocator&, graph_base&>)
    graph_base(const EdgeAllocator& edgeAllocator, const EdgePartitionsAllocator& edgePartitionsAllocator)
      : primitive_type(edgeAllocator, edgePartitionsAllocator)
    {}

    template
    <
      alloc EdgeAllocator           = edge_allocator_type,
      class EdgeStorage             = typename edge_traits_type::edge_storage_type,
      alloc EdgePartitionsAllocator = typename EdgeStorage::partitions_allocator_type,
      class N                       = node_storage_type,
      alloc NodeWeightAllocator     = typename graph_impl::node_allocator_generator<N>::allocator_type
    >
      requires (!empty<N> && !std::is_convertible_v<EdgeAllocator&, graph_base&>)
    graph_base(const EdgeAllocator& edgeAllocator, const EdgePartitionsAllocator& edgePartitionsAllocator, const NodeWeightAllocator& nodeWeightAllocator)
      : primitive_type(edgeAllocator, edgePartitionsAllocator, nodeWeightAllocator)
    {}

    graph_base(edges_initializer edges) : primitive_type{edges} {}

    template
    <
      alloc EdgeAllocator           = edge_allocator_type,     
      class N                       = NodeWeight
    >
      requires empty<N>
    graph_base(edges_initializer edges, const EdgeAllocator& edgeAllocator)
      : primitive_type{edges, edgeAllocator}
    {}
    
    template
    <
      alloc EdgeAllocator           = edge_allocator_type,
      class EdgeStorage             = typename edge_traits_type::edge_storage_type,
      alloc EdgePartitionsAllocator = typename EdgeStorage::partitions_allocator_type,     
      class N                       = NodeWeight
    >
      requires empty<N>
    graph_base(edges_initializer edges, const EdgeAllocator& edgeAllocator, const EdgePartitionsAllocator& edgePartitionsAllocator)
      : primitive_type{edges, edgeAllocator, edgePartitionsAllocator}
    {}

    template
    <
      alloc EdgeAllocator           = edge_allocator_type,     
      class N                       = node_storage_type,
      alloc NodeWeightAllocator     = typename graph_impl::node_allocator_generator<N>::allocator_type
    >
    requires (!empty<N>)
    graph_base(edges_initializer edges, const EdgeAllocator& edgeAllocator, const NodeWeightAllocator& nodeWeightAllocator)
      : primitive_type{edges, edgeAllocator, nodeWeightAllocator}
    {}
    
    template
    <
      alloc EdgeAllocator           = edge_allocator_type,
      class EdgeStorage             = typename edge_traits_type::edge_storage_type,
      class EdgePartitionsAllocator = typename EdgeStorage::partitions_allocator_type,     
      class N                       = node_storage_type,
      alloc NodeWeightAllocator     = typename graph_impl::node_allocator_generator<N>::allocator_type
    >
      requires (!empty<N>)
    graph_base(edges_initializer edges, const EdgeAllocator& edgeAllocator, const EdgePartitionsAllocator& edgePartitionsAllocator, const NodeWeightAllocator& nodeWeightAllocator)
      : primitive_type{edges, edgeAllocator, edgePartitionsAllocator, nodeWeightAllocator}
    {}

    template<class N=NodeWeight>
      requires (!empty<N>)
    graph_base(edges_initializer edges, std::initializer_list<node_weight_type> nodeWeights)
      : primitive_type{edges, nodeWeights}
    {}

    template
    <
      alloc EdgeAllocator           = edge_allocator_type,     
      class N                       = node_storage_type,
      alloc NodeWeightAllocator     = typename graph_impl::node_allocator_generator<N>::allocator_type
    >
    requires (!empty<N>)
    graph_base(edges_initializer edges, const EdgeAllocator& edgeAllocator, std::initializer_list<node_weight_type> nodeWeights, const NodeWeightAllocator& nodeWeightAllocator)
      : primitive_type{edges, edgeAllocator, nodeWeights, nodeWeightAllocator}
    {}

    template
    <
      alloc EdgeAllocator           = edge_allocator_type,
      class EdgeStorage             = typename edge_traits_type::edge_storage_type,
      alloc EdgePartitionsAllocator = typename EdgeStorage::partitions_allocator_type,      
      class N                       = node_storage_type,
      alloc NodeWeightAllocator     = typename graph_impl::node_allocator_generator<N>::allocator_type
    >
     requires(!empty<N>)
    graph_base(edges_initializer edges, const EdgeAllocator& edgeAllocator, const EdgePartitionsAllocator& edgePartitionsAllocator, std::initializer_list<node_weight_type> nodeWeights, const NodeWeightAllocator& nodeWeightAllocator)
      : primitive_type{edges, edgeAllocator, edgePartitionsAllocator, nodeWeights, nodeWeightAllocator}
    {}

    graph_base(const graph_base&) = default;

    template
    <   
      alloc EdgeAllocator           = edge_allocator_type,
      class N                       = NodeWeight
    >
      requires (empty<N>)
    graph_base(const graph_base& in, const EdgeAllocator& edgeAllocator)
      : primitive_type{in, edgeAllocator}
    {}

    template
    <   
      alloc EdgeAllocator           = edge_allocator_type,
      class EdgeStorage             = typename edge_traits_type::edge_storage_type,
      alloc EdgePartitionsAllocator = typename EdgeStorage::partitions_allocator_type,
      class N                       = NodeWeight
    >
      requires empty<N>
    graph_base(const graph_base& in, const EdgeAllocator& edgeAllocator, const EdgePartitionsAllocator& edgePartitionsAllocator)
      : primitive_type{in, edgeAllocator, edgePartitionsAllocator}
    {}

    template
    <
      alloc EdgeAllocator           = edge_allocator_type,
      class N                       = node_storage_type,
      alloc NodeWeightAllocator     = typename graph_impl::node_allocator_generator<N>::allocator_type
    >
      requires (!empty<N>)
    graph_base(const graph_base& in, const EdgeAllocator& edgeAllocator, const NodeWeightAllocator& nodeWeightAllocator)
      : primitive_type{in, edgeAllocator, nodeWeightAllocator}
    {}

    template
    <
      alloc EdgeAllocator           = edge_allocator_type,
      class EdgeStorage             = typename edge_traits_type::edge_storage_type,
      alloc EdgePartitionsAllocator = typename EdgeStorage::partitions_allocator_type,
      class N                       = node_storage_type,
      alloc NodeWeightAllocator     = typename graph_impl::node_allocator_generator<N>::allocator_type
    >
      requires (!empty<N>)
    graph_base(const graph_base& in, const EdgeAllocator& edgeAllocator, const EdgePartitionsAllocator& edgePartitionsAllocator, const NodeWeightAllocator& nodeWeightAllocator)
      : primitive_type{in, edgeAllocator, edgePartitionsAllocator, nodeWeightAllocator}
    {}
    
    graph_base(graph_base&&) noexcept = default;

    template
    <
      alloc EdgeAllocator           = edge_allocator_type,     
      class N                       = NodeWeight
    >
      requires empty<N>
    graph_base(graph_base&& in, const EdgeAllocator& edgeAllocator)
      : primitive_type{std::move(in), edgeAllocator}
    {}
    
    template
    <
      alloc EdgeAllocator           = edge_allocator_type,
      class EdgeStorage             = typename edge_traits_type::edge_storage_type,
      alloc EdgePartitionsAllocator = typename EdgeStorage::partitions_allocator_type,     
      class N                       = NodeWeight
    >
      requires empty<N>
    graph_base(graph_base&& in, const EdgeAllocator& edgeAllocator, const EdgePartitionsAllocator& edgePartitionsAllocator)
      : primitive_type{std::move(in), edgeAllocator, edgePartitionsAllocator}
    {}

    template
    <      
      alloc EdgeAllocator           = edge_allocator_type,
      class N                       = node_storage_type,
      alloc NodeWeightAllocator     = typename graph_impl::node_allocator_generator<N>::allocator_type
    >
      requires (!empty<N>)
    graph_base(graph_base&& in, const EdgeAllocator& edgeAllocator, const NodeWeightAllocator& nodeWeightAllocator)
      : primitive_type{std::move(in), edgeAllocator, nodeWeightAllocator}
    {}

    template
    <      
      alloc EdgeAllocator           = edge_allocator_type,
      class EdgeStorage             = typename edge_traits_type::edge_storage_type,
      alloc EdgePartitionsAllocator = typename EdgeStorage::partitions_allocator_type,
      class N                       = node_storage_type,
      alloc NodeWeightAllocator     = typename graph_impl::node_allocator_generator<N>::allocator_type
    >
      requires (!empty<N>)
    graph_base(graph_base&& in, const EdgeAllocator& edgeAllocator, const EdgePartitionsAllocator& edgePartitionsAllocator, const NodeWeightAllocator& nodeWeightAllocator)
      : primitive_type{std::move(in), edgeAllocator, edgePartitionsAllocator, nodeWeightAllocator}
    {}
    
    graph_base& operator=(const graph_base&) = default;

    graph_base& operator=(graph_base&&) = default;

    using primitive_type::swap;
    using primitive_type::swap_nodes;

    friend void swap(graph_base& lhs, graph_base& rhs) noexcept(noexcept(lhs.swap(rhs)))
    {
      lhs.swap(rhs);
    }

    using primitive_type::get_edge_allocator;

    using primitive_type::add_node;
    using primitive_type::insert_node;
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
    graph_flavour GraphFlavour,      
    class EdgeWeight,
    class NodeWeight,      
    creator EdgeWeightCreator,
    creator NodeWeightCreator,
    class EdgeStorageTraits,
    class NodeWeightStorageTraits
  >
  class graph_base<
    GraphFlavour,
    EdgeWeight,
    NodeWeight,
    EdgeWeightCreator,
    NodeWeightCreator,
    EdgeStorageTraits,
    NodeWeightStorageTraits,
    false
    > : public graph_base<
                 GraphFlavour,
                 EdgeWeight,
                 NodeWeight,
                 EdgeWeightCreator,
                 NodeWeightCreator,
                 EdgeStorageTraits,
                 NodeWeightStorageTraits,
                 true
               >
  {
  public:
    using graph_base<
                 GraphFlavour,
                 EdgeWeight,
                 NodeWeight,
                 EdgeWeightCreator,
                 NodeWeightCreator,
                 EdgeStorageTraits,
                 NodeWeightStorageTraits,
                 true
      >:: graph_base;

    graph_base(const graph_base&) = default;
    graph_base(graph_base&&) noexcept = default;

    graph_base& operator=(const graph_base&) = default;
    graph_base& operator=(graph_base&&) noexcept = default;

    using graph_base<
            GraphFlavour,
            EdgeWeight,
            NodeWeight,
            EdgeWeightCreator,
            NodeWeightCreator,
            EdgeStorageTraits,
            NodeWeightStorageTraits,
            true
          >::get_node_allocator;
  protected:
    ~graph_base() = default;
  };

  template
  <
    directed_flavour Directedness,      
    class EdgeWeight,
    class NodeWeight,      
    creator EdgeWeightCreator=data_sharing::spawner<EdgeWeight>,
    creator NodeWeightCreator=data_sharing::spawner<NodeWeight>,
    class EdgeStorageTraits = bucketed_edge_storage_traits,
    class NodeWeightStorageTraits = node_weight_storage_traits<NodeWeight>
  >
  class graph final : public
    graph_base
    <
      graph_impl::to_graph_flavour<Directedness>(),      
      EdgeWeight,
      NodeWeight,      
      EdgeWeightCreator,
      NodeWeightCreator,
      EdgeStorageTraits,
      NodeWeightStorageTraits    
    >
  {
  private:
    using base =
      graph_base
      <
        graph_impl::to_graph_flavour<Directedness>(),
        EdgeWeight,
        NodeWeight,     
        EdgeWeightCreator,
        NodeWeightCreator,
        EdgeStorageTraits,
        NodeWeightStorageTraits
      >;
  public:
    using node_weight_type = NodeWeight;
    
    using
      graph_base
      <
        graph_impl::to_graph_flavour<Directedness>(),      
        EdgeWeight,
        NodeWeight, 
        EdgeWeightCreator,
        NodeWeightCreator,
        EdgeStorageTraits,
        NodeWeightStorageTraits
      >::graph_base;

    graph(const graph&) = default;
    graph(graph&&) noexcept = default;

    graph& operator=(const graph&) = default;
    graph& operator=(graph&&) = default;

    using base::sort_edges;
    using base::swap_edges;
  };

  template
  <
    directed_flavour Directedness,      
    class EdgeWeight,
    class NodeWeight,      
    creator EdgeWeightCreator=data_sharing::spawner<EdgeWeight>,
    creator NodeWeightCreator=data_sharing::spawner<NodeWeight>,
    class EdgeStorageTraits=bucketed_edge_storage_traits,
    class NodeWeightStorageTraits=node_weight_storage_traits<NodeWeight>
  >
  class embedded_graph final : public
    graph_base
    <
      graph_impl::to_embedded_graph_flavour<Directedness>(),      
      EdgeWeight,
      NodeWeight,     
      EdgeWeightCreator,
      NodeWeightCreator,
      EdgeStorageTraits,
      NodeWeightStorageTraits
    >
    {
    public:
      using node_weight_type = NodeWeight;
      
      using
        graph_base
        <
          graph_impl::to_embedded_graph_flavour<Directedness>(),      
          EdgeWeight,
          NodeWeight,      
          EdgeWeightCreator,
          NodeWeightCreator,
          EdgeStorageTraits,
          NodeWeightStorageTraits
        >::graph_base;
      
      using base_type =
        graph_base
        <
          graph_impl::to_embedded_graph_flavour<Directedness>(),      
          EdgeWeight,
          NodeWeight,     
          EdgeWeightCreator,
          NodeWeightCreator,
          EdgeStorageTraits,
          NodeWeightStorageTraits
        >;

      using base_type::primitive_type::insert_join;
    };  
}
