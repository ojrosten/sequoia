////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "DynamicGraphTestingUtilities.hpp"
#include "NodeStorageAllocationTestingUtilities.hpp"
#include "Core/DataStructures/PartitionedDataAllocationTestingUtilities.hpp"

namespace sequoia::testing
{
  struct custom_allocator_contiguous_edge_storage_traits
  {
    template <class T, class Sharing, class Traits> using storage_type = data_structures::partitioned_sequence<T, Sharing, Traits>;
    template <class T, class Sharing> using traits_type = custom_partitioned_sequence_traits<T, Sharing>;

    constexpr static maths::edge_sharing_preference edge_sharing{maths::edge_sharing_preference::agnostic};
  };

  struct custom_allocator_bucketed_edge_storage_traits
  {
    template <class T, class Sharing, class Traits> using storage_type = data_structures::bucketed_storage<T, Sharing, Traits>;
    template <class T, class Sharing> using traits_type = custom_bucketed_storage_traits<T, Sharing>;

    constexpr static maths::edge_sharing_preference edge_sharing{maths::edge_sharing_preference::agnostic};
  };

  template<class NodeWeight>
  struct node_traits
  {
    constexpr static bool throw_on_range_error{true};
    constexpr static bool static_storage_v{};
    constexpr static bool has_allocator{true};
    template<class S> using container_type = std::vector<S, shared_counting_allocator<S, true, true, true>>;
  };

  template<class NodeWeight>
    requires empty<NodeWeight>
  struct node_traits<NodeWeight>
  {
    constexpr static bool has_allocator{};
  };

  template
  <
    maths::graph_flavour GraphFlavour,
    class EdgeWeight,
    class NodeWeight,
    class EdgeWeightPooling,
    class NodeWeightPooling,
    class EdgeStorageTraits,
    class NodeWeightStorageTraits,
    test_mode Mode
  >
  using canonical_graph_allocation_operations
    = basic_graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits, Mode, regular_allocation_extender<Mode>>;

  template
  <
    maths::graph_flavour GraphFlavour,
    class EdgeWeight,
    class NodeWeight,
    class EdgeWeightPooling,
    class NodeWeightPooling,
    class EdgeStorageTraits,
    class NodeWeightStorageTraits
  >
  using graph_allocation_operations
    = canonical_graph_allocation_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits, test_mode::standard>;

  template<class Graph>
  struct edge_alloc_getter
  {
    using edge_allocator = typename Graph::edge_allocator_type;

    using alloc_equivalence_class = allocation_equivalence_classes::container_of_pointers<edge_allocator>;

    [[nodiscard]]
    edge_allocator operator()(const Graph& g) const
    {
      return g.get_edge_allocator();
    }
  };

  template<class Graph>
  struct edge_partitions_alloc_getter
  {
    using edge_partitions_allocator = decltype(Graph{}.get_edge_allocator(maths::partitions_allocator_tag{}));
    using alloc_equivalence_class = allocation_equivalence_classes::container_of_pointers<edge_partitions_allocator>;

    [[nodiscard]]
    edge_partitions_allocator operator()(const Graph& g) const
    {
      return g.get_edge_allocator(maths::partitions_allocator_tag{});
    }
  };

  template<class Graph, alloc Allocator>
  struct node_alloc_getter
  {
    using alloc_equivalence_class = equiv_class_generator_t<typename Graph::node_storage_type, Allocator>;

    [[nodiscard]]
    Allocator operator()(const Graph& g) const
    {
      return g.get_node_allocator();
    }
  };

}
