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
#include "sequoia/Core/Object/DataPool.hpp"
#include "sequoia/Maths/Graph/GraphImpl.hpp"
#include "sequoia/Maths/Graph/DynamicGraphImpl.hpp"
#include "sequoia/Maths/Graph/NodeStorage.hpp"

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
    template <class T, class Sharing, class Traits> using storage_type = data_structures::bucketed_sequence<T, Sharing, Traits>;
    template <class T, class Sharing> using traits_type = data_structures::bucketed_sequence_traits<T, Sharing>;

    constexpr static edge_sharing_preference edge_sharing{edge_sharing_preference::agnostic};
  };

  template<class NodeWeight>
  struct node_weight_storage_traits
  {
    constexpr static bool throw_on_range_error{true};
    constexpr static bool static_storage_v{false};
    constexpr static bool has_allocator{true};
    template<class S> using container_type = std::vector<S, std::allocator<S>>;
  };

  template<class NodeWeight>
    requires std::is_empty_v<NodeWeight>
  struct node_weight_storage_traits<NodeWeight>
  {
    constexpr static bool has_allocator{false};
  };

  template<class Traits>
  concept allocatable_partitions = requires{
    typename Traits::edge_storage_type;
    typename Traits::edge_storage_type::partitions_allocator_type;
  };

  template
  <
    graph_flavour GraphFlavour,
    class EdgeWeight,
    class NodeWeight,
    class EdgeWeightCreator,
    class NodeWeightCreator,
    class EdgeStorageTraits,
    class NodeWeightStorageTraits
  >
    requires (object::creator<EdgeWeightCreator> && object::creator<NodeWeightCreator>)
  class graph_base : public
    graph_primitive
    <
      connectivity
      <
        to_directedness(GraphFlavour),
        graph_impl::dynamic_edge_traits<GraphFlavour, EdgeWeightCreator, EdgeStorageTraits, std::size_t>,
        EdgeWeightCreator
      >,
      graph_impl::node_storage
      <
        NodeWeightCreator,
        NodeWeightStorageTraits
      >
    >
  {
  public:
    using edge_traits_type  = graph_impl::dynamic_edge_traits<GraphFlavour, EdgeWeightCreator, EdgeStorageTraits, std::size_t>;
    using node_storage_type = graph_impl::node_storage<NodeWeightCreator, NodeWeightStorageTraits>;

    using primitive_type =
      graph_primitive
      <
        connectivity
        <
          to_directedness(GraphFlavour),
          edge_traits_type,
          EdgeWeightCreator
        >,
        node_storage_type
      >;

    using node_weight_type    = NodeWeight;
    using size_type           = typename primitive_type::size_type;
    using edges_initializer   = typename primitive_type::edges_initializer;
    using edge_allocator_type = typename edge_traits_type::edge_allocator_type;

    graph_base() = default;

    graph_base(const edge_allocator_type& edgeAllocator)
      : primitive_type(edgeAllocator)
    {}

    template<alloc EdgePartitionsAllocator>
      requires allocatable_partitions<edge_traits_type>
    graph_base(const edge_allocator_type& edgeAllocator, const EdgePartitionsAllocator& edgePartitionsAllocator)
      : primitive_type(edgeAllocator, edgePartitionsAllocator)
    {}

    graph_base(edges_initializer edges) : primitive_type{edges} {}

    graph_base(edges_initializer edges, const edge_allocator_type& edgeAllocator)
      : primitive_type{edges, edgeAllocator}
    {}

    template<alloc EdgePartitionsAllocator>
      requires allocatable_partitions<edge_traits_type>
    graph_base(edges_initializer edges, const edge_allocator_type& edgeAllocator, const EdgePartitionsAllocator& edgePartitionsAllocator)
      : primitive_type{edges, edgeAllocator, edgePartitionsAllocator}
    {}

    template<tree_link_direction dir>
      requires (    !std::same_as<node_weight_type, graph_impl::heterogeneous_tag>
                 && ((dir == tree_link_direction::symmetric) || (primitive_type::connectivity::directedness == directed_flavour::directed)))
    graph_base(std::initializer_list<tree_initializer<node_weight_type>> forest, tree_link_direction_constant<dir> tdc)
      : primitive_type{forest, tdc}
    {}

    template<tree_link_direction dir>
      requires (    !std::same_as<node_weight_type, graph_impl::heterogeneous_tag>
                 && ((dir == tree_link_direction::symmetric) || (primitive_type::connectivity::directedness == directed_flavour::directed)))
    graph_base(tree_initializer<node_weight_type> tree, tree_link_direction_constant<dir> tdc)
      : primitive_type{tree, tdc}
    {}

    graph_base(const graph_base&) = default;

    graph_base(const graph_base& in, const edge_allocator_type& edgeAllocator)
      : primitive_type{in, edgeAllocator}
    {}

    template<alloc EdgePartitionsAllocator>
      requires allocatable_partitions<edge_traits_type>
    graph_base(const graph_base& in, const edge_allocator_type& edgeAllocator, const EdgePartitionsAllocator& edgePartitionsAllocator)
      : primitive_type{in, edgeAllocator, edgePartitionsAllocator}
    {}

    graph_base(graph_base&&) noexcept = default;

    graph_base(graph_base&& in, const edge_allocator_type& edgeAllocator)
      : primitive_type{std::move(in), edgeAllocator}
    {}

    template<alloc EdgePartitionsAllocator>
      requires allocatable_partitions<edge_traits_type>
    graph_base(graph_base&& in, const edge_allocator_type& edgeAllocator, const EdgePartitionsAllocator& edgePartitionsAllocator)
      : primitive_type{std::move(in), edgeAllocator, edgePartitionsAllocator}
    {}

    graph_base& operator=(const graph_base&) = default;

    graph_base& operator=(graph_base&&) = default;

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
    class EdgeWeightCreator,
    class NodeWeightCreator,
    class EdgeStorageTraits,
    class NodeWeightStorageTraits
  >
  requires (   object::creator<EdgeWeightCreator>
            && object::creator<NodeWeightCreator>
            && NodeWeightStorageTraits::has_allocator)
  class graph_base<
      GraphFlavour,
      EdgeWeight,
      NodeWeight,
      EdgeWeightCreator,
      NodeWeightCreator,
      EdgeStorageTraits,
      NodeWeightStorageTraits
    > : public
    graph_primitive
    <
      connectivity
      <
        to_directedness(GraphFlavour),
        graph_impl::dynamic_edge_traits<GraphFlavour, EdgeWeightCreator, EdgeStorageTraits, std::size_t>,
        EdgeWeightCreator
      >,
      graph_impl::node_storage
      <
        NodeWeightCreator,
        NodeWeightStorageTraits
      >
    >
  {
  public:
    using edge_traits_type = graph_impl::dynamic_edge_traits<GraphFlavour, EdgeWeightCreator, EdgeStorageTraits, std::size_t>;
    using node_storage_type = graph_impl::node_storage<NodeWeightCreator, NodeWeightStorageTraits>;

    using primitive_type =
      graph_primitive
      <
        connectivity
        <
          to_directedness(GraphFlavour),
          edge_traits_type,
          EdgeWeightCreator
        >,
        node_storage_type
      >;

    using node_weight_type           = NodeWeight;
    using size_type                  = typename primitive_type::size_type;
    using edges_initializer          = typename primitive_type::edges_initializer;
    using edge_allocator_type        = typename edge_traits_type::edge_allocator_type;
    using node_weight_allocator_type = typename graph_impl::node_allocator_generator<node_storage_type>::allocator_type;

    graph_base() = default;

    graph_base(edges_initializer edges) : primitive_type{edges} {}

    graph_base(const edge_allocator_type& edgeAllocator, const node_weight_allocator_type& nodeWeightAllocator)
      : primitive_type(edgeAllocator, nodeWeightAllocator)
    {}

    template<alloc EdgePartitionsAllocator>
      requires allocatable_partitions<edge_traits_type>
    graph_base(const edge_allocator_type& edgeAllocator, const EdgePartitionsAllocator& edgePartitionsAllocator, const node_weight_allocator_type& nodeWeightAllocator)
      : primitive_type(edgeAllocator, edgePartitionsAllocator, nodeWeightAllocator)
    {}

    graph_base(edges_initializer edges, const edge_allocator_type& edgeAllocator, const node_weight_allocator_type& nodeWeightAllocator)
      : primitive_type{edges, edgeAllocator, nodeWeightAllocator}
    {}

    template<alloc EdgePartitionsAllocator>
      requires allocatable_partitions<edge_traits_type>
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
      requires allocatable_partitions<edge_traits_type>
    graph_base(edges_initializer edges, const edge_allocator_type& edgeAllocator, const EdgePartitionsAllocator& edgePartitionsAllocator, std::initializer_list<node_weight_type> nodeWeights, const node_weight_allocator_type& nodeWeightAllocator)
      : primitive_type{edges, edgeAllocator, edgePartitionsAllocator, nodeWeights, nodeWeightAllocator}
    {}

    template<tree_link_direction dir>
      requires ((dir == tree_link_direction::symmetric) || (primitive_type::connectivity::directedness == directed_flavour::directed))
    graph_base(std::initializer_list<tree_initializer<node_weight_type>> forest, tree_link_direction_constant<dir> tdc)
      : primitive_type{forest, tdc}
    {}

    template<tree_link_direction dir>
      requires (    !std::is_empty_v<node_weight_type> && !std::same_as<node_weight_type, graph_impl::heterogeneous_tag>
                 && ((dir == tree_link_direction::symmetric) || (primitive_type::connectivity::directedness == directed_flavour::directed)))
    graph_base(tree_initializer<node_weight_type> tree, tree_link_direction_constant<dir> tdc)
      : primitive_type{tree, tdc}
    {}

     graph_base(const graph_base& in, const edge_allocator_type& edgeAllocator, const node_weight_allocator_type& nodeWeightAllocator)
      : primitive_type{in, edgeAllocator, nodeWeightAllocator}
    {}

    template<alloc EdgePartitionsAllocator>
      requires allocatable_partitions<edge_traits_type>
    graph_base(const graph_base& in, const edge_allocator_type& edgeAllocator, const EdgePartitionsAllocator& edgePartitionsAllocator, const node_weight_allocator_type& nodeWeightAllocator)
      : primitive_type{in, edgeAllocator, edgePartitionsAllocator, nodeWeightAllocator}
    {}


    graph_base(graph_base&& in, const edge_allocator_type& edgeAllocator, const node_weight_allocator_type& nodeWeightAllocator)
      : primitive_type{std::move(in), edgeAllocator, nodeWeightAllocator}
    {}

    template<alloc EdgePartitionsAllocator>
      requires allocatable_partitions<edge_traits_type>
    graph_base(graph_base&& in, const edge_allocator_type& edgeAllocator, const EdgePartitionsAllocator& edgePartitionsAllocator, const node_weight_allocator_type& nodeWeightAllocator)
      : primitive_type{std::move(in), edgeAllocator, edgePartitionsAllocator, nodeWeightAllocator}
    {}

    graph_base(const graph_base&)     = default;
    graph_base(graph_base&&) noexcept = default;

    graph_base& operator=(const graph_base&)     = default;
    graph_base& operator=(graph_base&&) noexcept = default;

    using primitive_type::swap;
    using primitive_type::clear;

    using primitive_type::get_edge_allocator;
    using primitive_type::reserve_edges;
    using primitive_type::edges_capacity;
    using primitive_type::reserve_nodes;
    using primitive_type::node_capacity;
    using primitive_type::shrink_to_fit;

    using primitive_type::get_node_allocator;

    constexpr static graph_flavour flavour{GraphFlavour};
  protected:
    ~graph_base() = default;
  };

  template
  <
    directed_flavour Directedness,
    class EdgeWeight,
    class NodeWeight,
    class EdgeWeightCreator=object::faithful_producer<EdgeWeight>,
    class NodeWeightCreator=object::faithful_producer<NodeWeight>,
    class EdgeStorageTraits = bucketed_edge_storage_traits,
    class NodeWeightStorageTraits = node_weight_storage_traits<NodeWeight>
  >
    requires (object::creator<EdgeWeightCreator> && object::creator<NodeWeightCreator>)
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

    using base_type =
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

    using base_type::swap_nodes;
    using base_type::add_node;
    using base_type::insert_node;
    using base_type::erase_node;

    using base_type::join;
    using base_type::erase_edge;

    using base_type::sort_edges;
    using base_type::swap_edges;

    void swap(graph& rhs) noexcept(noexcept(this->base_type::swap(rhs)))
    {
      base_type::swap(rhs);
    }

    friend void swap(graph& lhs, graph& rhs) noexcept(noexcept(lhs.swap(rhs)))
    {
      lhs.swap(rhs);
    }
  };

  template
  <
    directed_flavour Directedness,
    class EdgeWeight,
    class NodeWeight,
    class EdgeWeightCreator=object::faithful_producer<EdgeWeight>,
    class NodeWeightCreator=object::faithful_producer<NodeWeight>,
    class EdgeStorageTraits=bucketed_edge_storage_traits,
    class NodeWeightStorageTraits=node_weight_storage_traits<NodeWeight>
  >
    requires (object::creator<EdgeWeightCreator> && object::creator<NodeWeightCreator>)
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

    using base_type::swap_nodes;
    using base_type::add_node;
    using base_type::insert_node;
    using base_type::erase_node;

    using base_type::join;
    using base_type::erase_edge;

    using base_type::primitive_type::insert_join;

    void swap(embedded_graph& rhs) noexcept(noexcept(this->base_type::swap(rhs)))
    {
      base_type::swap(rhs);
    }

    friend void swap(embedded_graph& lhs, embedded_graph& rhs) noexcept(noexcept(lhs.swap(rhs)))
    {
      lhs.swap(rhs);
    }
  };
}
