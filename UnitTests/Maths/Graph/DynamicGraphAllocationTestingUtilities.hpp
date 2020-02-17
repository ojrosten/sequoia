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
  struct node_traits
  {
    constexpr static bool throw_on_range_error{true};
    constexpr static bool static_storage_v{};
    constexpr static bool has_allocator{true};
    template<class S> using container_type = std::vector<S, shared_counting_allocator<S, true, true, true>>;
  };

  template<class NodeWeight>
  struct node_traits<NodeWeight, true>
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
  void graph_contiguous_memory<
      GraphFlavour,
      EdgeWeight,
      NodeWeight,
      EdgeWeightPooling,
      NodeWeightPooling,
      EdgeStorageTraits,
      NodeWeightStorageTraits
  >::execute_operations()
  {
    using namespace maths;
    using edge_partitions_allocator = decltype(graph_t{}.get_edge_allocator(partitions_allocator_tag{}));
    using edge_allocator = typename graph_t::edge_allocator_type;

    auto maker{
      [](){
        if constexpr(std::is_empty_v<NodeWeight>)
        {
          return graph_t(edge_allocator{}, edge_partitions_allocator{});
        }
        else
        {
          using node_allocator = typename graph_t::node_weight_container_type::allocator_type;
          return graph_t{edge_allocator{}, edge_partitions_allocator{}, node_allocator{}};
        }
      }
    };

    // null
    graph_t g{maker()};

    check_equality(LINE(""), g.edges_capacity(), 0ul);
    check_equality(LINE(""), g.node_capacity(), 0ul);

    g.reserve_edges(4);
    check_equality(LINE(""), g.edges_capacity(), 4ul);

    g.reserve_nodes(4);
    check_equality(LINE(""), g.node_capacity(), 4ul);

    g.shrink_to_fit();
    check_equality(LINE("May fail if stl implementation doesn't actually shrink to fit!"), g.edges_capacity(), 0ul);
    check_equality(LINE("May fail if stl implementation doesn't actually shrink to fit!"), g.node_capacity(), 0ul);
      
    // x----x
    using edge_init_t = typename graph_t::edge_init_type;
    
    auto nodeMaker{
      [](graph_t& g) { g.add_node(); }
    };

    auto allocGetter{
      [](const graph_t& g) { return g.get_edge_allocator(); }
    };

    auto partitionAllocGetter{
      [](const graph_t& g) { return g.get_edge_allocator(partitions_allocator_tag{}); }
    };

    if constexpr(std::is_empty_v<NodeWeight>)
    {      
      check_regular_semantics(LINE(""), g, graph_t{{{}}, edge_allocator{}, edge_partitions_allocator{}}, nodeMaker, allocation_info{allocGetter, {0, {0, 0}, {0, 0}}}, allocation_info{partitionAllocGetter, {0, {1, 1}, {1, 1}}});
      graph_t g2{};

      if constexpr (GraphFlavour == graph_flavour::directed)
      {
        check_regular_semantics(LINE(""), g2, graph_t{{edge_init_t{1}}, {}}, nodeMaker, allocation_info{allocGetter, {0, {1, 0}, {1, 1}}}, allocation_info{partitionAllocGetter, {0, {1, 1}, {1, 1}}});
      }
      else if constexpr(GraphFlavour == graph_flavour::undirected)
      {
        check_regular_semantics(LINE(""), g2, graph_t{{edge_init_t{1}}, {edge_init_t{0}}}, nodeMaker, allocation_info{allocGetter, {0, {1, 0}, {1, 1}}}, allocation_info{partitionAllocGetter, {0, {1, 1}, {1, 1}}});
      }
      else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
      {
        check_regular_semantics(LINE(""), g2, graph_t{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}}}, nodeMaker, allocation_info{allocGetter, {0, {1, 0}, {1, 1}}}, allocation_info{partitionAllocGetter, {0, {1, 1}, {1, 1}}});
      }
      else
      {
        check_regular_semantics(LINE(""), g2, graph_t{{edge_init_t{1,0}}, {edge_init_t{0,0}}}, nodeMaker, allocation_info{allocGetter, {0, {1, 0}, {1, 1}}}, allocation_info{partitionAllocGetter, {0, {1, 1}, {1, 1}}});
      }
    }
    else
    {
      auto nodeAllocGetter{
        [](const graph_t& g) { return g.get_node_allocator(); }
      };

      using node_allocator = typename graph_t::node_weight_container_type::allocator_type;
      check_regular_semantics(LINE(""),
                              g,
                              graph_t{{{}},edge_allocator{}, edge_partitions_allocator{}, node_allocator{}},
                              nodeMaker,
                              allocation_info{allocGetter, {0, {0, 0}, {0, 0}}},
                              allocation_info{partitionAllocGetter, {0, {1, 1}, {1, 1}}},
                              allocation_info{nodeAllocGetter, {0, {1, 1}, {1, 1}}});

      check_regular_semantics(LINE(""),
                              g,
                              graph_t{{{}}, edge_allocator{}, edge_partitions_allocator{}, {{1.0, -1.0}}, node_allocator{}},
                              nodeMaker,
                              allocation_info{allocGetter, {0, {0, 0}, {0, 0}}},
                              allocation_info{partitionAllocGetter, {0, {1, 1}, {1, 1}}},
                              allocation_info{nodeAllocGetter, {0, {1, 1}, {1, 1}}});

      graph_t g2{};

      if constexpr (GraphFlavour == graph_flavour::directed)
      {
        check_regular_semantics(LINE(""), g2, graph_t{{edge_init_t{1}}, {}}, nodeMaker, allocation_info{allocGetter, {0, {1, 0}, {1, 1}}}, allocation_info{partitionAllocGetter, {0, {1, 1}, {1, 1}}}, allocation_info{nodeAllocGetter, {0, {1, 1}, {1, 1}}});
      }
      else if constexpr(GraphFlavour == graph_flavour::undirected)
      {
        check_regular_semantics(LINE(""), g2, graph_t{{edge_init_t{1}}, {edge_init_t{0}}}, nodeMaker, allocation_info{allocGetter, {0, {1, 0}, {1, 1}}}, allocation_info{partitionAllocGetter, {0, {1, 1}, {1, 1}}}, allocation_info{nodeAllocGetter, {0, {1, 1}, {1, 1}}});
      }
      else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
      {
        check_regular_semantics(LINE(""), g2, graph_t{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}}}, nodeMaker, allocation_info{allocGetter, {0, {1, 0}, {1, 1}}}, allocation_info{partitionAllocGetter, {0, {1, 1}, {1, 1}}}, allocation_info{nodeAllocGetter, {0, {1, 1}, {1, 1}}});
      }
      else
      {
        check_regular_semantics(LINE(""), g2, graph_t{{edge_init_t{1,0}}, {edge_init_t{0,0}}}, nodeMaker, allocation_info{allocGetter, {0, {1, 0}, {1, 1}}}, allocation_info{partitionAllocGetter, {0, {1, 1}, {1, 1}}}, allocation_info{nodeAllocGetter, {0, {1, 1}, {1, 1}}});
      }
    }
  }

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
  void graph_bucketed_memory<
      GraphFlavour,
      EdgeWeight,
      NodeWeight,
      EdgeWeightPooling,
      NodeWeightPooling,
      EdgeStorageTraits,
      NodeWeightStorageTraits
  >::execute_operations()
  {
    using edge_allocator = typename graph_t::edge_allocator_type;

    auto maker{
      [](){
        if constexpr(std::is_empty_v<NodeWeight>)
        {
          return graph_t(edge_allocator{});
        }
        else
        {
          using node_allocator = typename graph_t::node_weight_container_type::allocator_type;
          return graph_t{edge_allocator{}, node_allocator{}};
        }
      }
    };
    
    graph_t g{maker()};

    check_exception_thrown<std::out_of_range>(LINE(""), [&g](){ g.reserve_edges(0, 4);});
    check_exception_thrown<std::out_of_range>(LINE(""), [&g](){ return g.edges_capacity(0);});
    check_equality(LINE(""), g.node_capacity(), 0ul);

    g.add_node();
    if constexpr(std::is_empty_v<NodeWeight>)
    {
      check_equality(LINE(""), g, graph_t{{{}}, edge_allocator{}});
    }
    else
    {
      using node_allocator = typename graph_t::node_weight_container_type::allocator_type;
      check_equality(LINE(""), g, graph_t{{{}}, edge_allocator{}, node_allocator{}});
    }

    check_equality(LINE(""), g.edges_capacity(0), 0ul);
    check_equality(LINE(""), g.node_capacity(), 1ul);
    g.reserve_edges(0, 4);
    check_equality(LINE(""), g.edges_capacity(0), 4ul);

    g.reserve_nodes(4);
    check_equality(LINE(""), g.node_capacity(), 4ul);

    g.shrink_to_fit();
    check_equality(LINE("May fail if stl implementation doesn't actually shrink to fit!"), g.edges_capacity(0), 0ul);
    check_equality(LINE("May fail if stl implementation doesn't actually shrink to fit!"), g.node_capacity(), 1ul);

    g.insert_node(0u);
    if constexpr(std::is_empty_v<NodeWeight>)
    {
      check_equality(LINE(""), g, graph_t{{{}, {}}, edge_allocator{}});
    }
    else
    {
      using node_allocator = typename graph_t::node_weight_container_type::allocator_type;
      check_equality(LINE(""), g, graph_t{{{}, {}}, edge_allocator{}, node_allocator{}});
    }

    // x----x
    using edge_init_t = typename graph_t::edge_init_type;
    using namespace maths;
    using edge_allocator = typename graph_t::edge_allocator_type;

    auto nodeMaker{
      [](graph_t& g) { g.add_node(); }
    };

    auto allocGetter{
      [](const graph_t& g) { return g.get_edge_allocator(); }
    };

    if constexpr(std::is_empty_v<NodeWeight>)
    {
      graph_t g2{};

      check_regular_semantics(LINE(""),
                              g2,
                              graph_t{{{}}, edge_allocator{}},
                              nodeMaker,
                              allocation_info{allocGetter, {{0, {1, 1}, {1, 1}}, {0, {0, 0}, {0, 0}}}}
                              );

      if constexpr (GraphFlavour == graph_flavour::directed)
      {
        check_regular_semantics(LINE(""), g2, graph_t{{edge_init_t{1}}, {}}, nodeMaker, allocation_info{allocGetter, {{0, {1, 1}, {1, 1}}, {0, {1, 0}, {1, 1}}}});
      }
      else if constexpr(GraphFlavour == graph_flavour::undirected)
      {
        check_regular_semantics(LINE(""), g2, graph_t{{edge_init_t{1}}, {edge_init_t{0}}}, nodeMaker, allocation_info{allocGetter, {{0, {1, 1}, {1, 1}}, {0, {2, 0}, {2, 2}}}});
      }
      else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
      {
        check_regular_semantics(LINE(""), g2, graph_t{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}}}, nodeMaker, allocation_info{allocGetter, {{0, {1, 1}, {1, 1}}, {0, {2, 0}, {2, 2}}}});
      }
      else
      {
        check_regular_semantics(LINE(""), g2, graph_t{{edge_init_t{1,0}}, {edge_init_t{0,0}}}, nodeMaker, allocation_info{allocGetter, {{0, {1, 1}, {1, 1}}, {0, {2, 0}, {2, 2}}}});
      }
    }
    else
    {
      graph_t g2{};

      auto nodeAllocGetter{
        [](const graph_t& g) { return g.get_node_allocator(); }
      };
      
      using node_allocator = typename graph_t::node_weight_container_type::allocator_type;

      check_regular_semantics(LINE(""),
                              g2,
                              graph_t{{{}}, edge_allocator{}, node_allocator{}},
                              nodeMaker,
                              allocation_info{allocGetter, {{0, {1, 1}, {1, 1}}, {0, {0, 0}, {0, 0}}}},
                              allocation_info{nodeAllocGetter, {0, {1, 1}, {1, 1}}});

      
      check_regular_semantics(LINE(""),
                              g2,
                              graph_t{{{}}, edge_allocator{}, {{1.0, -1.0}}, node_allocator{}},
                              nodeMaker,
                              allocation_info{allocGetter, {{0, {1, 1}, {1, 1}}, {0, {0, 0}, {0, 0}}}},
                              allocation_info{nodeAllocGetter, {0, {1, 1}, {1, 1}}});

      if constexpr (GraphFlavour == graph_flavour::directed)
      {
        check_regular_semantics(LINE(""), g2, graph_t{{edge_init_t{1}}, {}}, nodeMaker, allocation_info{allocGetter, {{0, {1, 1}, {1, 1}}, {0, {1, 0}, {1, 1}}}}, allocation_info{nodeAllocGetter, {0, {1, 1}, {1, 1}}});
      }
      else if constexpr(GraphFlavour == graph_flavour::undirected)
      {
        check_regular_semantics(LINE(""), g2, graph_t{{edge_init_t{1}}, {edge_init_t{0}}}, nodeMaker, allocation_info{allocGetter, {{0, {1, 1}, {1, 1}}, {0, {2, 0}, {2, 2}}}}, allocation_info{nodeAllocGetter, {0, {1, 1}, {1, 1}}});
      }
      else if constexpr(GraphFlavour == graph_flavour::directed_embedded)
      {
        check_regular_semantics(LINE(""), g2, graph_t{{edge_init_t{0,1,0}}, {edge_init_t{0,1,0}}}, nodeMaker, allocation_info{allocGetter, {{0, {1, 1}, {1, 1}}, {0, {2, 0}, {2, 2}}}}, allocation_info{nodeAllocGetter, {0, {1, 1}, {1, 1}}});
      }
      else
      {
        check_regular_semantics(LINE(""), g2, graph_t{{edge_init_t{1,0}}, {edge_init_t{0,0}}}, nodeMaker, allocation_info{allocGetter, {{0, {1, 1}, {1, 1}}, {0, {2, 0}, {2, 2}}}}, allocation_info{nodeAllocGetter, {0, {1, 1}, {1, 1}}});
      }
    }
  }
}
