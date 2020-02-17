////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "DynamicGraphTestingUtilities.hpp"
#include "PartitionedDataAllocationTestingUtilities.hpp"

namespace sequoia::unit_testing
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

  template<class NodeWeight, bool=std::is_empty_v<NodeWeight>>
  struct custom_node_weight_storage_traits
  {
    constexpr static bool throw_on_range_error{true};
    constexpr static bool static_storage_v{};
    constexpr static bool has_allocator{true};
    template<class S> using container_type = std::vector<S, shared_counting_allocator<S, true, true, true>>;
  };

  template<class NodeWeight>
  struct custom_node_weight_storage_traits<NodeWeight, true>
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
    = basic_graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits, unit_test_logger<Mode>, allocation_extender<unit_test_logger<Mode>>>;

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
  class graph_contiguous_memory
    : public graph_allocation_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>
  {
  public:
      
  private:
    using base_t = graph_allocation_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>;
      
    using graph_t = typename base_t::graph_type;
    using checker_t = typename base_t::checker_type;
      
    using checker_t::check_equality;
    using checker_t::check_regular_semantics;
    using graph_checker<unit_test_logger<test_mode::standard>, allocation_extender<unit_test_logger<test_mode::standard>>>::check_exception_thrown;


    void execute_operations() override;
  };

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
  class graph_bucketed_memory
    : public graph_allocation_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>
  {
  private:
    using base_t = graph_allocation_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>;
      
    using graph_t = typename base_t::graph_type;
    using checker_t = typename base_t::checker_type;

    using checker_t::check_equality;
    using checker_t::check_regular_semantics;
    using graph_checker<unit_test_logger<test_mode::standard>, allocation_extender<unit_test_logger<test_mode::standard>>>::check_exception_thrown;

    void execute_operations() override;
  };
  
  template<class Weight, bool Empty>
  using node_traits = custom_node_weight_storage_traits<Weight, Empty>;
}

#include "DynamicGraphAllocationTestingUtilities.impl"
