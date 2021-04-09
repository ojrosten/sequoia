////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "sequoia/TestFramework/TestPreprocessorDefinitions.hpp"

#include "GraphTestingUtilities.hpp"
#include "NodeStorageTestingUtilities.hpp"
#include "Core/DataStructures/PartitionedDataTestingUtilities.hpp"

#include "sequoia/Maths/Graph/DynamicGraph.hpp"

#include <variant>

namespace sequoia::testing
{
  // Details Checkers

  template
  <
    maths::directed_flavour Directedness,
    class EdgeWeight,
    class NodeWeight,
    class EdgeWeightPooling,
    class NodeWeightPooling,
    class EdgeStorageTraits,
    class NodeWeightStorageTraits
  >
  struct detailed_equality_checker<maths::graph<Directedness, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>>
    : impl::graph_detailed_equality_checker<maths::graph<Directedness, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>>
  {};

  template
  <
    maths::directed_flavour Directedness,
    class EdgeWeight,
    class NodeWeight,
    class EdgeWeightPooling,
    class NodeWeightPooling,
    class EdgeStorageTraits,
    class NodeWeightStorageTraits
  >
  struct detailed_equality_checker<maths::embedded_graph<Directedness, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>>
    : impl::graph_detailed_equality_checker<maths::embedded_graph<Directedness, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>>
  {};

  // Equivalence Checkers

  template
  <
    maths::directed_flavour Directedness,
    class EdgeWeight,
    class NodeWeight,
    class EdgeWeightPooling,
    class NodeWeightPooling,
    class EdgeStorageTraits,
    class NodeWeightStorageTraits
  >
  struct equivalence_checker<maths::graph<Directedness, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>>
    : impl::graph_equivalence_checker<maths::graph<Directedness, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>>
  {};

  template
  <
    maths::directed_flavour Directedness,
    class EdgeWeight,
    class NodeWeight,
    class EdgeWeightPooling,
    class NodeWeightPooling,
    class EdgeStorageTraits,
    class NodeWeightStorageTraits
  >
  struct equivalence_checker<maths::embedded_graph<Directedness, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>>
    : impl::graph_equivalence_checker<maths::embedded_graph<Directedness, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>>
  {};

  // Weak Equivalence

  template
  <
    maths::directed_flavour Directedness,
    class EdgeWeight,
    class NodeWeight,
    class EdgeWeightPooling,
    class NodeWeightPooling,
    class EdgeStorageTraits,
    class NodeWeightStorageTraits
  >
  struct weak_equivalence_checker<maths::embedded_graph<Directedness, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>>
    : impl::graph_weak_equivalence_checker<maths::embedded_graph<Directedness, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>>
  {};

  // Edge Storage Traits

  struct independent_contiguous_edge_storage_traits
  {
    template <class T, class Sharing, class Traits> using storage_type = data_structures::partitioned_sequence<T, Sharing, Traits>;
    template <class T, class Sharing> using traits_type = data_structures::partitioned_sequence_traits<T, Sharing>;

    constexpr static maths::edge_sharing_preference edge_sharing{maths::edge_sharing_preference::independent};
  };

  struct independent_bucketed_edge_storage_traits
  {
    template <class T, class Sharing, class Traits> using storage_type = data_structures::bucketed_storage<T, Sharing, Traits>;
    template <class T, class Sharing> using traits_type = data_structures::bucketed_storage_traits<T, Sharing>;

    constexpr static maths::edge_sharing_preference edge_sharing{maths::edge_sharing_preference::independent};
  };

  struct shared_weight_contiguous_edge_storage_traits
  {
    template <class T, class Sharing, class Traits> using storage_type = data_structures::partitioned_sequence<T, Sharing, Traits>;
    template <class T, class Sharing> using traits_type = data_structures::partitioned_sequence_traits<T, Sharing>;

    constexpr static maths::edge_sharing_preference edge_sharing{maths::edge_sharing_preference::shared_weight};
  };

  struct shared_weight_bucketed_edge_storage_traits
  {
    template <class T, class Sharing, class Traits> using storage_type = data_structures::bucketed_storage<T, Sharing, Traits>;
    template <class T, class Sharing> using traits_type = data_structures::bucketed_storage_traits<T, Sharing>;

    constexpr static maths::edge_sharing_preference edge_sharing{maths::edge_sharing_preference::shared_weight};
  };

  // Meta

  [[nodiscard]]
  constexpr bool embedded(const maths::graph_flavour graphFlavour) noexcept
  {
    using gf = maths::graph_flavour;
    return (graphFlavour == gf::directed_embedded) || (graphFlavour == gf::undirected_embedded);
  }

  template
  <
    maths::graph_flavour GraphFlavour,
    class EdgeWeight,
    class NodeWeight,
    class EdgeWeightPooling,
    class NodeWeightStorage,
    class EdgeStorageTraits,
    class NodeWeightStorageTraits,
    bool=embedded(GraphFlavour)
  >
  struct graph_type_generator
  {
    using graph_type = maths::graph<maths::to_directedness(GraphFlavour), EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightStorage, EdgeStorageTraits, NodeWeightStorageTraits>;
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
  struct graph_type_generator<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits, true>
  {
    using graph_type = maths::embedded_graph<maths::to_directedness(GraphFlavour), EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>;
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
  using graph_type_generator_t = typename graph_type_generator<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>::graph_type;

  template <class EdgeWeight, class NodeWeight, class Test>
  class graph_test_helper
  {
  public:
    explicit graph_test_helper(Test& t) : m_Test{t}
    {}

    void run_tests()
    {
      using flavour = maths::graph_flavour;

      run_tests<flavour::undirected>();
      if constexpr(!minimal_graph_tests())
      {
        run_tests<flavour::undirected_embedded>();
        run_tests<flavour::directed>();
        run_tests<flavour::directed_embedded>();

        using NSTraits = maths::node_weight_storage_traits<NodeWeight>;
        creation_permutations<flavour::directed_embedded, independent_contiguous_edge_storage_traits, NSTraits>();
        creation_permutations<flavour::directed_embedded, independent_bucketed_edge_storage_traits,   NSTraits>();
      }
    }

    template<class EdgeStorageTraits, class NodeStorageTraits>
    void run_tests()
    {
      using flavour = maths::graph_flavour;
      creation_permutations<flavour::undirected, EdgeStorageTraits, NodeStorageTraits>();
      if constexpr (!minimal_graph_tests())
      {
        creation_permutations<flavour::undirected_embedded, EdgeStorageTraits, NodeStorageTraits>();
        creation_permutations<flavour::directed,            EdgeStorageTraits, NodeStorageTraits>();
        creation_permutations<flavour::directed_embedded,   EdgeStorageTraits, NodeStorageTraits>();
      }
    }

    template<maths::graph_flavour GraphFlavour>
    void run_tests()
    {
      using namespace data_structures;
      using NSTraits = maths::node_weight_storage_traits<NodeWeight>;

      creation_permutations<GraphFlavour, maths::contiguous_edge_storage_traits, NSTraits>();
      creation_permutations<GraphFlavour, maths::bucketed_edge_storage_traits, NSTraits>();
    }
  private:
    Test& m_Test;

    template
    <
      maths::graph_flavour GraphFlavour,
      class EdgeStorageTraits,
      class NodeWeightStorageTraits
    >
    void creation_permutations()
    {
      using namespace ownership;
      using EW = EdgeWeight;
      using NW = NodeWeight;
      using ESTraits = EdgeStorageTraits;
      using NSTraits = NodeWeightStorageTraits;

      run_tests<GraphFlavour, spawner<EW>, spawner<NW>, ESTraits, NSTraits>();

      if constexpr(!minimal_graph_tests())
      {
        if constexpr(!std::is_empty_v<NodeWeight>)
        {
          run_tests<GraphFlavour, spawner<EW>, data_pool<NW>, ESTraits, NSTraits>();
        }

        if constexpr(!std::is_empty_v<EdgeWeight>)
        {
          run_tests<GraphFlavour, data_pool<EW>, spawner<NW>, ESTraits, NSTraits>();
        }

        if constexpr(!std::is_empty_v<EdgeWeight> && !std::is_empty_v<NodeWeight>)
        {
          run_tests<GraphFlavour, data_pool<EW>, data_pool<NW>, ESTraits, NSTraits>();
        }
      }
    }

    template
    <
      maths::graph_flavour GraphFlavour,
      class EdgeWeightCreator,
      class NodeWeightCreator,
      class EdgeStorageTraits,
      class NodeWeightStorageTraits
    >
    void run_tests()
    {
      m_Test.template execute_operations<
        GraphFlavour,
        EdgeWeight,
        NodeWeight,
        EdgeWeightCreator,
        NodeWeightCreator,
        EdgeStorageTraits,
        NodeWeightStorageTraits
      >();
    }
  };
}
