////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "DynamicGraphPerformanceTest.hpp"
#include "Maths/Graph/GraphTestingUtilities.hpp"

#include "sequoia/Core/Numeric/IntegerConversions.hpp"
#include "sequoia/Maths/Graph/DynamicGraph.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <random>
#include <ranges>
#include <vector>

namespace sequoia::testing
{
  using namespace maths;

  namespace
  {
    /** \brief The number of nodes in every graph the sort tests build, and of
               the joins from each node, so that each graph is small and
               sparse.
     */
    constexpr std::size_t nodes_per_graph{64},
                          joins_per_node{4};

    /** \brief The numbers of nodes in the smaller and larger graph of a
               comparison of build times.
     */
    constexpr std::size_t nodes_in_smaller_graph{256},
                          growth_factor{8},
                          nodes_in_larger_graph{growth_factor * nodes_in_smaller_graph};

    /** \brief The number of nodes in each graph whose sort is checked for
               stability: enough that std::ranges::sort, which is not stable,
               reorders some equivalent nodes under libc++ and libstdc++.
     */
    constexpr std::size_t nodes_in_stability_check{64};

    static_assert(nodes_per_graph > 1 && nodes_in_smaller_graph > 1);

    struct nodes_to_join
    {
      std::size_t node1{}, node2{};
    };

    /** \brief Returns the weights `0` to `nodes_per_graph - 1`, shuffled. */
    [[nodiscard]]
    std::vector<std::size_t> shuffled_weights()
    {
      auto weights{std::views::iota(0uz, nodes_per_graph) | std::ranges::to<std::vector>()};
      std::mt19937 generator{std::mt19937::default_seed};
      std::ranges::shuffle(weights, generator);
      return weights;
    }

    /** \brief Returns `joins_per_node` joins from each of `nodeCount` nodes,
               each to another node chosen at random.
     */
    [[nodiscard]]
    std::vector<nodes_to_join> random_joins(const std::size_t nodeCount)
    {
      std::mt19937 generator{std::mt19937::default_seed};
      std::vector<nodes_to_join> joins{};
      joins.reserve(nodeCount * joins_per_node);
      for(const auto node1 : std::views::iota(0uz, nodeCount))
      {
        for(std::size_t i{}; i < joins_per_node; ++i)
        {
          // An offset in [1, nodeCount - 1] joins node1 to another node
          const auto node2{(node1 + 1 + generator() % (nodeCount - 1)) % nodeCount};
          joins.push_back({.node1{node1}, .node2{node2}});
        }
      }

      return joins;
    }

    /** \brief The node weights and joins of a graph, from which to build the
               graph with its nodes either sorted by weight or shuffled.

        Both builds make the same joins, between nodes of the same weights,
        in the same order. So sorting the nodes of the shuffled graph by
        weight gives the sorted graph, if the sort keeps the order of each
        node's edges.
     */
    class graph_blueprint
    {
    public:
      /** \brief Builds the graph with its nodes sorted by weight. */
      template<dynamic_network Graph>
      [[nodiscard]]
      Graph sorted_graph() const
      {
        Graph g{};
        for(const auto weight : std::views::iota(0uz, nodes_per_graph))
        {
          g.add_node(weight);
        }

        for(const auto [node1, node2] : m_Joins)
        {
          g.join(m_ShuffledWeights[node1], m_ShuffledWeights[node2]);
        }

        return g;
      }

      /** \brief Builds the graph with its nodes in the shuffled order. */
      template<dynamic_network Graph>
      [[nodiscard]]
      Graph shuffled_graph() const
      {
        Graph g{};
        for(const auto weight : m_ShuffledWeights)
        {
          g.add_node(weight);
        }

        for(const auto [node1, node2] : m_Joins)
        {
          g.join(node1, node2);
        }

        return g;
      }
    private:
      /** \brief The weight of each node of the shuffled graph, which is the
                 node's index in the sorted graph.
       */
      std::vector<std::size_t> m_ShuffledWeights{shuffled_weights()};

      /** \brief The joins, between the nodes of the shuffled graph. */
      std::vector<nodes_to_join> m_Joins{random_joins(nodes_per_graph)};
    };

    /** \brief Returns a graph with `nodeCount` nodes and `joins` between them. */
    template<dynamic_network Graph>
    [[nodiscard]]
    Graph graph_with_nodes(const std::size_t nodeCount, const std::vector<nodes_to_join>& joins)
    {
      Graph g{};
      for(const auto weight : std::views::iota(0uz, nodeCount))
      {
        g.add_node(weight);
      }

      for(const auto [node1, node2] : joins)
      {
        g.join(node1, node2);
      }

      return g;
    }
  }

  [[nodiscard]]
  std::filesystem::path dynamic_graph_performance_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void dynamic_graph_performance_test::run_tests()
  {
    test_sort_nodes<directed_graph<null_weight, std::size_t, bucketed_edge_storage_config>>("directed graph");
    test_sort_nodes<undirected_graph<null_weight, std::size_t, null_meta_data, bucketed_edge_storage_config>>(
      "undirected graph"
    );
    test_sort_nodes<embedded_graph<null_weight, std::size_t, null_meta_data, bucketed_edge_storage_config>>(
      "embedded graph"
    );

    test_sort_nodes_is_stable<directed_graph<null_weight, std::size_t, bucketed_edge_storage_config>>(
      "directed graph"
    );
    test_sort_nodes_is_stable<undirected_graph<null_weight, std::size_t, null_meta_data, bucketed_edge_storage_config>>(
      "undirected graph"
    );
    test_sort_nodes_is_stable<embedded_graph<null_weight, std::size_t, null_meta_data, bucketed_edge_storage_config>>(
      "embedded graph"
    );

    test_build_time_scales_linearly<directed_graph<null_weight, std::size_t, bucketed_edge_storage_config>>(
      "directed graph"
    );
    test_build_time_scales_linearly<
      undirected_graph<null_weight, std::size_t, null_meta_data, bucketed_edge_storage_config>
    >("undirected graph");
    test_build_time_scales_linearly<
      embedded_graph<null_weight, std::size_t, null_meta_data, bucketed_edge_storage_config>
    >("embedded graph");
  }

  /* Each performance check claims that building a graph shuffled and then
     sorting its nodes takes the same order of magnitude of time as building
     the graph sorted. A speed-up is of the same order of magnitude as 1 if
     its logarithm to base ten rounds to zero. So the speed-up must be less
     than the square root of ten, the prediction's upper end. The claim
     bounds the speed-up only from above. `check_relative_performance`
     requires the prediction's lower end to exceed 1, so it is put just
     above 1.

     With bucketed edge storage, building a graph shuffled takes about as
     long as building it sorted. So the speed-up reflects the cost of sorting
     the nodes, and not a difference between the two builds.
  */
  template<dynamic_network Graph>
  void dynamic_graph_performance_test::test_sort_nodes(std::string_view graphName)
  {
    const graph_blueprint blueprint{};

    auto buildSortedGraph{[&blueprint]() { return blueprint.sorted_graph<Graph>(); }};

    auto buildShuffledGraphAndSortNodes{
      [&blueprint]() {
        auto g{blueprint.shuffled_graph<Graph>()};
        auto weightOf{
          [&g](std::size_t node) {
            return g.cbegin_node_weights()[numeric::unchecked_conversion_to<std::ptrdiff_t>(node)];
          }
        };

        g.sort_nodes([weightOf](std::size_t i, std::size_t j) { return weightOf(i) < weightOf(j); });
        return g;
      }
    };

    if(check(equality,
             std::format("Sorting the nodes of a shuffled {} gives the graph built sorted", graphName),
             buildShuffledGraphAndSortNodes(),
             buildSortedGraph()))
    {
      check_relative_performance(
        std::format("Building a shuffled {} and sorting its nodes takes the same order of magnitude of time as "
                    "building the {} sorted",
                    graphName,
                    graphName),
        buildSortedGraph,
        buildShuffledGraphAndSortNodes,
        {.prediction{.lower{1.01}, .upper{std::sqrt(10.0)}}});
    }
  }

  /* Sorting by the remainder of each weight on division by
     `equivalence_class_count` treats as equivalent the nodes whose weights
     share a remainder.
  */
  template<dynamic_network Graph>
  void dynamic_graph_performance_test::test_sort_nodes_is_stable(std::string_view graphName)
  {
    constexpr std::size_t equivalence_class_count{4};
    auto byRemainder{
      [](std::size_t lhs, std::size_t rhs) { return (lhs % equivalence_class_count) < (rhs % equivalence_class_count); }
    };

    auto g{graph_with_nodes<Graph>(nodes_in_stability_check, {})};
    std::vector<std::size_t> stablySortedWeights(g.cbegin_node_weights(), g.cend_node_weights());
    std::ranges::stable_sort(stablySortedWeights, byRemainder);

    g.sort_nodes([&g, byRemainder](std::size_t i, std::size_t j) {
      return byRemainder(g.cbegin_node_weights()[numeric::unchecked_conversion_to<std::ptrdiff_t>(i)],
                         g.cbegin_node_weights()[numeric::unchecked_conversion_to<std::ptrdiff_t>(j)]);
    });

    check(equality,
          std::format("Sorting the nodes of the {} keeps the order of nodes the comparator treats as equivalent",
                      graphName),
          std::vector<std::size_t>(g.cbegin_node_weights(), g.cend_node_weights()),
          stablySortedWeights);
  }

  /* Each performance check claims that building a graph takes time
     proportional to its size. The graphs have `joins_per_node` joins from each
     node, so their numbers of nodes and of joins grow together. Building the
     graph with `growth_factor` times as many nodes should take about
     `growth_factor` times as long. If each join cost time proportional to the
     number of nodes, it would take about `growth_factor` squared times as
     long. The prediction's ends lie a factor of two either side of
     `growth_factor`, so the squared time lies outside the prediction. The
     lower end also exceeds 1, as `check_relative_performance` requires.
  */
  template<dynamic_network Graph>
  void dynamic_graph_performance_test::test_build_time_scales_linearly(std::string_view graphName)
  {
    const auto joinsOfSmallerGraph{random_joins(nodes_in_smaller_graph)},
               joinsOfLargerGraph{random_joins(nodes_in_larger_graph)};

    auto buildSmallerGraph{
      [&joinsOfSmallerGraph]() { return graph_with_nodes<Graph>(nodes_in_smaller_graph, joinsOfSmallerGraph); }
    };

    auto buildLargerGraph{
      [&joinsOfLargerGraph]() { return graph_with_nodes<Graph>(nodes_in_larger_graph, joinsOfLargerGraph); }
    };

    check_relative_performance(
      std::format("Building the {} takes time proportional to its size", graphName),
      buildSmallerGraph,
      buildLargerGraph,
      {.prediction{.lower{growth_factor / 2.0}, .upper{growth_factor * 2.0}}});
  }
}
