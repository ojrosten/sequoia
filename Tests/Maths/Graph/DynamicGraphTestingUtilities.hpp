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
#include "Core/DataStructures/PartitionedDataTestingUtilities.hpp"

#include "sequoia/Maths/Graph/DynamicGraph.hpp"

#include <variant>

namespace sequoia::testing
{
  // Edge Storage Container

  struct independent_contiguous_edge_storage_config
  {
    template <class T> using storage_type = data_structures::partitioned_sequence<T>;

    constexpr static maths::edge_sharing_preference edge_sharing{maths::edge_sharing_preference::independent};
  };

  struct independent_bucketed_edge_storage_config
  {
    template <class T> using storage_type = data_structures::bucketed_sequence<T>;

    constexpr static maths::edge_sharing_preference edge_sharing{maths::edge_sharing_preference::independent};
  };

  struct shared_weight_contiguous_edge_storage_config
  {
    template <class T> using storage_type = data_structures::partitioned_sequence<T>;

    constexpr static maths::edge_sharing_preference edge_sharing{maths::edge_sharing_preference::shared_weight};
  };

  struct shared_weight_bucketed_edge_storage_config
  {
    template <class T> using storage_type = data_structures::bucketed_sequence<T>;

    constexpr static maths::edge_sharing_preference edge_sharing{maths::edge_sharing_preference::shared_weight};
  };

  // Meta

  [[nodiscard]]
  constexpr bool embedded(const maths::graph_flavour graphFlavour) noexcept
  {
    using gf = maths::graph_flavour;
    return graphFlavour == gf::undirected_embedded;
  }

  template
  <
    maths::graph_flavour GraphFlavour,
    class EdgeWeight,
    class NodeWeight,
    class EdgeStorageConfig,
    class NodeWeightStorage
  >
  struct graph_type_generator
  {
    using graph_type = maths::directed_graph<EdgeWeight, NodeWeight, EdgeStorageConfig, NodeWeightStorage>;
  };

  template
  <
    maths::graph_flavour GraphFlavour,
    class EdgeWeight,
    class NodeWeight,
    class EdgeStorageConfig,
    class NodeWeightStorage
  >
    requires (GraphFlavour == maths::graph_flavour::undirected)
  struct graph_type_generator<GraphFlavour, EdgeWeight, NodeWeight, EdgeStorageConfig, NodeWeightStorage>
  {
    using graph_type = maths::undirected_graph<EdgeWeight, NodeWeight, maths::null_meta_data, EdgeStorageConfig, NodeWeightStorage>;
  };

  template
  <
    maths::graph_flavour GraphFlavour,
    class EdgeWeight,
    class NodeWeight,
    class EdgeStorageConfig,
    class NodeWeightStorage
  >
    requires (GraphFlavour == maths::graph_flavour::undirected_embedded)
  struct graph_type_generator<GraphFlavour, EdgeWeight, NodeWeight, EdgeStorageConfig, NodeWeightStorage>
  {
    using graph_type = maths::embedded_graph<EdgeWeight, NodeWeight, maths::null_meta_data, EdgeStorageConfig, NodeWeightStorage>;
  };

  template
  <
    maths::graph_flavour GraphFlavour,
    class EdgeWeight,
    class NodeWeight,
    class EdgeStorageConfig,
    class NodeWeightStorage
  >
  using graph_type_generator_t = typename graph_type_generator<GraphFlavour, EdgeWeight, NodeWeight, EdgeStorageConfig, NodeWeightStorage>::graph_type;

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
      }
    }

    template<class EdgeStorageConfig, class NodeWeightStorage>
    void run_tests()
    {
      using flavour = maths::graph_flavour;
      creation_permutations<flavour::undirected, EdgeStorageConfig, NodeWeightStorage>();
      if constexpr (!minimal_graph_tests())
      {
        creation_permutations<flavour::undirected_embedded, EdgeStorageConfig, NodeWeightStorage>();
        creation_permutations<flavour::directed,            EdgeStorageConfig, NodeWeightStorage>();
      }
    }

    template<maths::graph_flavour GraphFlavour>
    void run_tests()
    {
      creation_permutations<GraphFlavour, maths::contiguous_edge_storage_config, maths::node_storage<NodeWeight>>();
      creation_permutations<GraphFlavour, maths::bucketed_edge_storage_config, maths::node_storage<NodeWeight>>();
    }
  private:
    Test& m_Test;

    template
    <
      maths::graph_flavour GraphFlavour,
      class EdgeStorageConfig,
      class NodeWeightStorage
    >
    void creation_permutations()
    {
      m_Test.template execute_operations<
        GraphFlavour,
        EdgeWeight,
        NodeWeight,
        EdgeStorageConfig,
        NodeWeightStorage
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
