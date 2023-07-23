////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/TestPreprocessorDefinitions.hpp"

#include "GraphTestingUtilities.hpp"
#include "NodeStorageTestingUtilities.hpp"
#include "Core/DataStructures/PartitionedDataTestingUtilities.hpp"

#include "sequoia/Maths/Graph/DynamicGraph.hpp"

#include <variant>

namespace sequoia::testing
{
  // Edge Storage Traits

  struct independent_contiguous_edge_storage_traits
  {
    template <class T, class Sharing, class Traits> using storage_type = data_structures::partitioned_sequence<T, Sharing, Traits>;
    template <class T, class Sharing> using traits_type = data_structures::partitioned_sequence_traits<T, Sharing>;

    constexpr static maths::edge_sharing_preference edge_sharing{maths::edge_sharing_preference::independent};
  };

  struct independent_bucketed_edge_storage_traits
  {
    template <class T, class Sharing, class Traits> using storage_type = data_structures::bucketed_sequence<T, Sharing, Traits>;
    template <class T, class Sharing> using traits_type = data_structures::bucketed_sequence_traits<T, Sharing>;

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
    template <class T, class Sharing, class Traits> using storage_type = data_structures::bucketed_sequence<T, Sharing, Traits>;
    template <class T, class Sharing> using traits_type = data_structures::bucketed_sequence_traits<T, Sharing>;

    constexpr static maths::edge_sharing_preference edge_sharing{maths::edge_sharing_preference::shared_weight};
  };

  struct shared_edge_contiguous_edge_storage_traits
  {
    template <class T, class Sharing, class Traits> using storage_type = data_structures::partitioned_sequence<T, Sharing, Traits>;
    template <class T, class Sharing> using traits_type = data_structures::partitioned_sequence_traits<T, Sharing>;

    constexpr static maths::edge_sharing_preference edge_sharing{maths::edge_sharing_preference::shared_edge};
  };

  struct shared_edge_bucketed_edge_storage_traits
  {
    template <class T, class Sharing, class Traits> using storage_type = data_structures::bucketed_sequence<T, Sharing, Traits>;
    template <class T, class Sharing> using traits_type = data_structures::bucketed_sequence_traits<T, Sharing>;

    constexpr static maths::edge_sharing_preference edge_sharing{maths::edge_sharing_preference::shared_edge};
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
    class EdgeWeightCreator,
    class NodeWeightStorage,
    class EdgeStorageTraits,
    class NodeWeightStorageTraits,
    bool=embedded(GraphFlavour)
  >
    requires object::creator<EdgeWeightCreator>
  struct graph_type_generator
  {
    using graph_type = maths::graph<maths::to_directedness(GraphFlavour), EdgeWeight, NodeWeight, EdgeWeightCreator, NodeWeightStorage, EdgeStorageTraits, NodeWeightStorageTraits>;
  };

  template
  <
    maths::graph_flavour GraphFlavour,
    class EdgeWeight,
    class NodeWeight,
    class EdgeWeightCreator,
    class NodeWeightCreator,
    class EdgeStorageTraits,
    class NodeWeightStorageTraits
  >
    requires (object::creator<EdgeWeightCreator> && object::creator<NodeWeightCreator>)
  struct graph_type_generator<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightCreator, NodeWeightCreator, EdgeStorageTraits, NodeWeightStorageTraits, true>
  {
    using graph_type = maths::embedded_graph<maths::to_directedness(GraphFlavour), EdgeWeight, NodeWeight, EdgeWeightCreator, NodeWeightCreator, EdgeStorageTraits, NodeWeightStorageTraits>;
  };

  template
  <
    maths::graph_flavour GraphFlavour,
    class EdgeWeight,
    class NodeWeight,
    class EdgeWeightCreator,
    class NodeWeightCreator,
    class EdgeStorageTraits,
    class NodeWeightStorageTraits
  >
    requires (object::creator<EdgeWeightCreator> && object::creator<NodeWeightCreator>)
  using graph_type_generator_t = typename graph_type_generator<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightCreator, NodeWeightCreator, EdgeStorageTraits, NodeWeightStorageTraits>::graph_type;

  template <class EdgeWeight, class NodeWeight, concrete_test Test>
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
      using namespace object;
      using EW = EdgeWeight;
      using NW = NodeWeight;
      using ESTraits = EdgeStorageTraits;
      using NSTraits = NodeWeightStorageTraits;

      run_tests<GraphFlavour, faithful_producer<EW>, faithful_producer<NW>, ESTraits, NSTraits>();

      if constexpr(!minimal_graph_tests())
      {
        if constexpr(!std::is_empty_v<NodeWeight>)
        {
          run_tests<GraphFlavour, faithful_producer<EW>, data_pool<NW>, ESTraits, NSTraits>();
        }

        if constexpr(!std::is_empty_v<EdgeWeight>)
        {
          run_tests<GraphFlavour, data_pool<EW>, faithful_producer<NW>, ESTraits, NSTraits>();
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

  template<maths::network G>
  struct graph_initialization_checker
  {
    using edge_type        = typename G::edge_init_type;
    using node_weight_type = typename G::node_weight_type;

    template<class Test>
    [[nodiscard]]
    static G make_and_check(Test& t, std::string_view description, std::initializer_list<std::initializer_list<edge_type>> init)
    {
      G g{init};
      t.check(equivalence, description, g, init);
      return g;
    }

    template<class Test>
    [[nodiscard]]
    static G make_and_check(Test& t, std::string_view description, std::initializer_list<std::initializer_list<edge_type>> edgeInit, std::initializer_list<node_weight_type> nodeInit)
    {
      G g{edgeInit, nodeInit};
      t.check(equivalence, description, g, edgeInit, nodeInit);
      return g;
    }
  };
}
