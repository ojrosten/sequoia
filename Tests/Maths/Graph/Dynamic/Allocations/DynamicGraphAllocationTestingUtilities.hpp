////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "Maths/Graph/Dynamic/DynamicGraphTestingUtilities.hpp"
#include "Core/DataStructures/PartitionedDataAllocationTestingUtilities.hpp"

namespace sequoia::testing
{
  struct custom_allocator_contiguous_edge_storage_config
  {
    template <class T> using storage_type = typename custom_partitioned_sequence_generator<T, true, false, false>::storage_type;

    constexpr static maths::edge_sharing_preference edge_sharing{maths::edge_sharing_preference::agnostic};
  };

  struct custom_allocator_bucketed_edge_storage_config
  {
    template <class T> using storage_type = typename custom_bucketed_sequence_generator<T, true, false, false>::storage_type;

    constexpr static maths::edge_sharing_preference edge_sharing{maths::edge_sharing_preference::agnostic};
  };

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

  template<class Graph>
  struct node_alloc_getter
  {
    using node_allocator = typename Graph::node_weight_allocator_type;
    using alloc_equivalence_class = allocation_equivalence_classes::container_of_values<node_allocator>;

    [[nodiscard]]
    node_allocator operator()(const Graph& g) const
    {
      return g.get_node_allocator();
    }
  };

  template<class T>
  struct node_storage_generator
  {
    using type = maths::node_storage<T, std::vector<T, shared_counting_allocator<T, true, true, true>>>;
  };

  template<class T>
  using node_storage_generator_t = typename node_storage_generator<T>::type;
}
