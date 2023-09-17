////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Edge & Node storage traits, base class and final classes for dynamic graphs.

*/

#include "sequoia/Core/DataStructures/PartitionedData.hpp"
#include "sequoia/Maths/Graph/GraphPrimitive.hpp"
#include "sequoia/Maths/Graph/NodeStorage.hpp"

namespace sequoia::maths
{
  struct contiguous_edge_storage_config
  {
    template <class T> using storage_type = data_structures::partitioned_sequence<T>;

    constexpr static edge_sharing_preference edge_sharing{edge_sharing_preference::agnostic};
  };

  struct bucketed_edge_storage_config
  {
    template <class T> using storage_type = data_structures::bucketed_sequence<T>;

    constexpr static edge_sharing_preference edge_sharing{edge_sharing_preference::agnostic};
  };


  template<class Storage>
  concept allocatable_partitions = requires{
    typename Storage::partitions_allocator_type;
  };

  template
  <
    graph_flavour GraphFlavour,
    class EdgeWeight,
    class NodeWeight,
    class EdgeMetaData,
    class EdgeStorageConfig,
    class NodeWeightStorage
  >
  class graph_base : public
    graph_primitive
    <
      connectivity<GraphFlavour, graph_impl::edge_storage_generator_t<GraphFlavour, EdgeWeight, EdgeMetaData, std::size_t, EdgeStorageConfig>>,
      NodeWeightStorage
    >
  {
  public:
    using connectivity_type = connectivity<GraphFlavour, graph_impl::edge_storage_generator_t<GraphFlavour, EdgeWeight, EdgeMetaData, std::size_t, EdgeStorageConfig>>;
    using node_storage_type = NodeWeightStorage;
    using primitive_type    = graph_primitive<connectivity_type, node_storage_type>;

    using node_weight_type    = NodeWeight;
    using size_type           = typename primitive_type::size_type;
    using edges_initializer   = typename primitive_type::edges_initializer;
    using edge_storage_type   = typename connectivity_type::edge_storage_type;
    using edge_allocator_type = typename edge_storage_type::allocator_type;

    graph_base() = default;

    explicit graph_base(const edge_allocator_type& edgeAllocator)
      : primitive_type(edgeAllocator)
    {}

    template<alloc EdgePartitionsAllocator>
      requires allocatable_partitions<edge_storage_type>
    graph_base(const edge_allocator_type& edgeAllocator, const EdgePartitionsAllocator& edgePartitionsAllocator)
      : primitive_type(edgeAllocator, edgePartitionsAllocator)
    {}

    graph_base(edges_initializer edges) : primitive_type{edges} {}

    graph_base(edges_initializer edges, const edge_allocator_type& edgeAllocator)
      : primitive_type{edges, edgeAllocator}
    {}

    template<alloc EdgePartitionsAllocator>
      requires allocatable_partitions<edge_storage_type>
    graph_base(edges_initializer edges, const edge_allocator_type& edgeAllocator, const EdgePartitionsAllocator& edgePartitionsAllocator)
      : primitive_type{edges, edgeAllocator, edgePartitionsAllocator}
    {}

    template<tree_link_direction dir>
      requires (    !heterogeneous_nodes<graph_base>
                 && ((dir == tree_link_direction::symmetric) || is_directed(primitive_type::connectivity::flavour)))
    graph_base(std::initializer_list<tree_initializer<node_weight_type>> forest, tree_link_direction_constant<dir> tdc)
      : primitive_type{forest, tdc}
    {}

    template<tree_link_direction dir>
      requires (    !heterogeneous_nodes<graph_base>
                 && ((dir == tree_link_direction::symmetric) || is_directed(primitive_type::connectivity::flavour)))
    graph_base(tree_initializer<node_weight_type> tree, tree_link_direction_constant<dir> tdc)
      : primitive_type{tree, tdc}
    {}

    graph_base(const graph_base&) = default;

    graph_base(const graph_base& in, const edge_allocator_type& edgeAllocator)
      : primitive_type{in, edgeAllocator}
    {}

    template<alloc EdgePartitionsAllocator>
      requires allocatable_partitions<edge_storage_type>
    graph_base(const graph_base& in, const edge_allocator_type& edgeAllocator, const EdgePartitionsAllocator& edgePartitionsAllocator)
      : primitive_type{in, edgeAllocator, edgePartitionsAllocator}
    {}

    graph_base(graph_base&&) noexcept = default;

    graph_base(graph_base&& in, const edge_allocator_type& edgeAllocator)
      : primitive_type{std::move(in), edgeAllocator}
    {}

    template<alloc EdgePartitionsAllocator>
      requires allocatable_partitions<edge_storage_type>
    graph_base(graph_base&& in, const edge_allocator_type& edgeAllocator, const EdgePartitionsAllocator& edgePartitionsAllocator)
      : primitive_type{std::move(in), edgeAllocator, edgePartitionsAllocator}
    {}

    graph_base& operator=(const graph_base&) = default;

    graph_base& operator=(graph_base&&) noexcept = default;

    using primitive_type::swap;
    using primitive_type::clear;

    using primitive_type::get_edge_allocator;
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
    class EdgeMetaData,
    class EdgeStorageConfig,
    class NodeWeightStorage
  >
    requires (!std::is_empty_v<NodeWeight>)
  class graph_base<
      GraphFlavour,
      EdgeWeight,
      NodeWeight,
      EdgeMetaData,
      EdgeStorageConfig,
      NodeWeightStorage
    > : public
    graph_primitive
    <
      connectivity<GraphFlavour, graph_impl::edge_storage_generator_t<GraphFlavour, EdgeWeight, EdgeMetaData, std::size_t, EdgeStorageConfig>>,
      NodeWeightStorage
    >
  {
  public:
    using connectivity_type = connectivity<GraphFlavour, graph_impl::edge_storage_generator_t<GraphFlavour, EdgeWeight, EdgeMetaData, std::size_t, EdgeStorageConfig>>;
    using node_storage_type = NodeWeightStorage;
    using primitive_type    = graph_primitive<connectivity_type, node_storage_type>;

    using node_weight_type           = NodeWeight;
    using size_type                  = typename primitive_type::size_type;
    using edges_initializer          = typename primitive_type::edges_initializer;
    using edge_storage_type          = typename connectivity_type::edge_storage_type;
    using edge_allocator_type        = typename edge_storage_type::allocator_type;
    using node_weight_allocator_type = typename node_storage_type::node_weight_container_type::allocator_type;

    graph_base() = default;

    graph_base(edges_initializer edges) : primitive_type{edges} {}

    graph_base(const edge_allocator_type& edgeAllocator, const node_weight_allocator_type& nodeWeightAllocator)
      : primitive_type(edgeAllocator, nodeWeightAllocator)
    {}

    template<alloc EdgePartitionsAllocator>
      requires allocatable_partitions<edge_storage_type>
    graph_base(const edge_allocator_type& edgeAllocator, const EdgePartitionsAllocator& edgePartitionsAllocator, const node_weight_allocator_type& nodeWeightAllocator)
      : primitive_type(edgeAllocator, edgePartitionsAllocator, nodeWeightAllocator)
    {}

    graph_base(edges_initializer edges, const edge_allocator_type& edgeAllocator, const node_weight_allocator_type& nodeWeightAllocator)
      : primitive_type{edges, edgeAllocator, nodeWeightAllocator}
    {}

    template<alloc EdgePartitionsAllocator>
      requires allocatable_partitions<edge_storage_type>
    graph_base(edges_initializer edges, const edge_allocator_type& edgeAllocator, const EdgePartitionsAllocator& edgePartitionsAllocator, const node_weight_allocator_type& nodeWeightAllocator)
      : primitive_type{edges, edgeAllocator, edgePartitionsAllocator, nodeWeightAllocator}
    {}

    graph_base(edges_initializer edges, std::initializer_list<node_weight_type> nodeWeights)
      : primitive_type{edges, nodeWeights}
    {}

    graph_base(edges_initializer edges, const edge_allocator_type& edgeAllocator, std::initializer_list<node_weight_type> nodeWeights, const node_weight_allocator_type& nodeWeightAllocator)
      : primitive_type{edges, edgeAllocator, nodeWeights, nodeWeightAllocator}
    {}

    template<alloc EdgePartitionsAllocator>
      requires allocatable_partitions<edge_storage_type>
    graph_base(edges_initializer edges, const edge_allocator_type& edgeAllocator, const EdgePartitionsAllocator& edgePartitionsAllocator, std::initializer_list<node_weight_type> nodeWeights, const node_weight_allocator_type& nodeWeightAllocator)
      : primitive_type{edges, edgeAllocator, edgePartitionsAllocator, nodeWeights, nodeWeightAllocator}
    {}

    template<tree_link_direction dir>
      requires ((dir == tree_link_direction::symmetric) || is_directed(primitive_type::connectivity::flavour))
    graph_base(std::initializer_list<tree_initializer<node_weight_type>> forest, tree_link_direction_constant<dir> tdc)
      : primitive_type{forest, tdc}
    {}

    template<tree_link_direction dir>
      requires (    !std::is_empty_v<node_weight_type> && !heterogeneous_nodes<graph_base>
                 && ((dir == tree_link_direction::symmetric) || is_directed(primitive_type::connectivity::flavour)))
    graph_base(tree_initializer<node_weight_type> tree, tree_link_direction_constant<dir> tdc)
      : primitive_type{tree, tdc}
    {}

     graph_base(const graph_base& in, const edge_allocator_type& edgeAllocator, const node_weight_allocator_type& nodeWeightAllocator)
      : primitive_type{in, edgeAllocator, nodeWeightAllocator}
    {}

    template<alloc EdgePartitionsAllocator>
      requires allocatable_partitions<edge_storage_type>
    graph_base(const graph_base& in, const edge_allocator_type& edgeAllocator, const EdgePartitionsAllocator& edgePartitionsAllocator, const node_weight_allocator_type& nodeWeightAllocator)
      : primitive_type{in, edgeAllocator, edgePartitionsAllocator, nodeWeightAllocator}
    {}


    graph_base(graph_base&& in, const edge_allocator_type& edgeAllocator, const node_weight_allocator_type& nodeWeightAllocator)
      : primitive_type{std::move(in), edgeAllocator, nodeWeightAllocator}
    {}

    template<alloc EdgePartitionsAllocator>
      requires allocatable_partitions<edge_storage_type>
    graph_base(graph_base&& in, const edge_allocator_type& edgeAllocator, const EdgePartitionsAllocator& edgePartitionsAllocator, const node_weight_allocator_type& nodeWeightAllocator)
      : primitive_type{std::move(in), edgeAllocator, edgePartitionsAllocator, nodeWeightAllocator}
    {}

    graph_base(const graph_base&)     = default;
    graph_base(graph_base&&) noexcept = default;

    graph_base& operator=(const graph_base&)     = default;
    graph_base& operator=(graph_base&&) noexcept = default;

    constexpr static graph_flavour flavour{GraphFlavour};

    using primitive_type::clear;

    using primitive_type::get_edge_allocator;
    using primitive_type::reserve_edges;
    using primitive_type::edges_capacity;
    using primitive_type::reserve_nodes;
    using primitive_type::node_capacity;
    using primitive_type::shrink_to_fit;

    using primitive_type::get_node_allocator;

    using primitive_type::swap;

    friend void swap(graph_base& lhs, graph_base& rhs) noexcept(noexcept(lhs.swap(rhs)))
    {
      lhs.swap(rhs);
    }
  protected:
    ~graph_base() = default;
  };

  template
  <
    class EdgeWeight,
    class NodeWeight,
    class EdgeStorageConfig = bucketed_edge_storage_config,
    class NodeWeightStorage = node_storage<NodeWeight>
  >
  class directed_graph final : public
    graph_base
    <
      graph_flavour::directed,
      EdgeWeight,
      NodeWeight,
      null_meta_data,
      EdgeStorageConfig,
      NodeWeightStorage
    >
  {
  public:
    using node_weight_type = NodeWeight;

    using
      graph_base
      <
        graph_flavour::directed,
        EdgeWeight,
        NodeWeight,
        null_meta_data,
        EdgeStorageConfig,
        NodeWeightStorage
      >::graph_base;

    using base_type =
      graph_base
      <
        graph_flavour::directed,
        EdgeWeight,
        NodeWeight,
        null_meta_data,
        EdgeStorageConfig,
        NodeWeightStorage
      >;

    using base_type::swap_nodes;
    using base_type::add_node;
    using base_type::insert_node;
    using base_type::erase_node;

    using base_type::join;
    using base_type::erase_edge;

    using base_type::sort_edges;
    using base_type::stable_sort_edges;
    using base_type::swap_edges;
  };

  template
  <
    class EdgeWeight,
    class NodeWeight,
    class EdgeMetaData      = null_meta_data,
    class EdgeStorageConfig = bucketed_edge_storage_config,
    class NodeWeightStorage = node_storage<NodeWeight>
  >
  class undirected_graph final : public
    graph_base
    <
      graph_flavour::undirected,
      EdgeWeight,
      NodeWeight,
      EdgeMetaData,
      EdgeStorageConfig,
      NodeWeightStorage
    >
  {
  public:
    using node_weight_type = NodeWeight;

    using
      graph_base
      <
        graph_flavour::undirected,
        EdgeWeight,
        NodeWeight,
        EdgeMetaData,
        EdgeStorageConfig,
        NodeWeightStorage
      >::graph_base;

    using base_type =
      graph_base
      <
        graph_flavour::undirected,
        EdgeWeight,
        NodeWeight,
        EdgeMetaData,
        EdgeStorageConfig,
        NodeWeightStorage
      >;

    using base_type::swap_nodes;
    using base_type::add_node;
    using base_type::insert_node;
    using base_type::erase_node;

    using base_type::join;
    using base_type::erase_edge;

    using base_type::sort_edges;
    using base_type::stable_sort_edges;
    using base_type::swap_edges;
  };

  template
  <
    class EdgeWeight,
    class NodeWeight,
    class EdgeMetaData,
    class EdgeStorageConfig,
    class NodeWeightStorage
  >
    requires (!std::is_empty_v<EdgeMetaData>)
  class undirected_graph<EdgeWeight, NodeWeight, EdgeMetaData, EdgeStorageConfig, NodeWeightStorage> final : public
    graph_base
    <
    graph_flavour::undirected,
    EdgeWeight,
    NodeWeight,
    EdgeMetaData,
    EdgeStorageConfig,
    NodeWeightStorage
    >
  {
  public:
    using node_weight_type = NodeWeight;

    using
      graph_base
      <
      graph_flavour::undirected,
      EdgeWeight,
      NodeWeight,
      EdgeMetaData,
      EdgeStorageConfig,
      NodeWeightStorage
      >::graph_base;

    using base_type =
      graph_base
      <
      graph_flavour::undirected,
      EdgeWeight,
      NodeWeight,
      EdgeMetaData,
      EdgeStorageConfig,
      NodeWeightStorage
      >;

    using base_type::swap_nodes;
    using base_type::add_node;
    using base_type::insert_node;
    using base_type::erase_node;

    using base_type::join;

    // TO DO: reinstate this, but implementation needs to be changed
    // using base_type::erase_edge;
  };

  template
  <
    class EdgeWeight,
    class NodeWeight,
    class EdgeMetaData      = null_meta_data,
    class EdgeStorageConfig = bucketed_edge_storage_config,
    class NodeWeightStorage = node_storage<NodeWeight>
  >
  class embedded_graph final : public
    graph_base
    <
      graph_flavour::undirected_embedded,
      EdgeWeight,
      NodeWeight,
      EdgeMetaData,
      EdgeStorageConfig,
      NodeWeightStorage
    >
  {
  public:
    using node_weight_type = NodeWeight;

    using
      graph_base
      <
        graph_flavour::undirected_embedded,
        EdgeWeight,
        NodeWeight,
        EdgeMetaData,
        EdgeStorageConfig,
        NodeWeightStorage
      >::graph_base;

    using base_type =
      graph_base
      <
        graph_flavour::undirected_embedded,
        EdgeWeight,
        NodeWeight,
        EdgeMetaData,
        EdgeStorageConfig,
        NodeWeightStorage
      >;

    using base_type::swap_nodes;
    using base_type::add_node;
    using base_type::insert_node;
    using base_type::erase_node;

    using base_type::join;
    using base_type::erase_edge;

    using base_type::primitive_type::insert_join;
  };
}
