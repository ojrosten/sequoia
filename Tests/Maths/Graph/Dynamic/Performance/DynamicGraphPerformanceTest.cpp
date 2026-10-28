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
    /** \brief The number of nodes in each graph which `test_sort_nodes`
               builds, and the number of joins from each node of every
               graph.

        Each graph which `test_sort_nodes` builds is small, and every graph
        is sparse.
     */
    constexpr std::size_t nodes_per_graph{64},
                          joins_per_node{4};

    /** \brief The numbers of nodes in the smaller and larger graph of each
               comparison of graphs of two sizes.
     */
    constexpr std::size_t nodes_in_smaller_graph{256},
                          growth_factor{8},
                          nodes_in_larger_graph{growth_factor * nodes_in_smaller_graph};

    /** \brief The interval of speed-ups predicted by each comparison of
               graphs of two sizes.

        Each speed-up is the time taken for the larger graph divided by the
        time taken for the smaller. The interval's ends lie a factor of two
        either side of `growth_factor`.
     */
    constexpr relative_performance_interval growth_prediction{.lower{growth_factor / 2.0},
                                                              .upper{growth_factor * 2.0}};

    static_assert(nodes_per_graph > 1 && nodes_in_smaller_graph > 1);

    struct nodes_to_join
    {
      std::size_t node1{}, node2{};
    };

    /** \brief Returns the weights `0` to `nodeCount - 1`, shuffled. */
    [[nodiscard]]
    std::vector<std::size_t> shuffled_weights(const std::size_t nodeCount)
    {
      auto weights{std::views::iota(0uz, nodeCount) | std::ranges::to<std::vector>()};
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
      explicit graph_blueprint(const std::size_t nodeCount)
        : m_ShuffledWeights{shuffled_weights(nodeCount)}
        , m_Joins{random_joins(nodeCount)}
      {}

      /** \brief Builds the graph with its nodes sorted by weight. */
      template<dynamic_network Graph>
      [[nodiscard]]
      Graph sorted_graph() const
      {
        Graph g{};
        for(const auto weight : std::views::iota(0uz, m_ShuffledWeights.size()))
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
      std::vector<std::size_t> m_ShuffledWeights{};

      /** \brief The joins, between the nodes of the shuffled graph. */
      std::vector<nodes_to_join> m_Joins{};
    };

    /** \brief Sorts the nodes of `g` by weight. */
    template<dynamic_network Graph>
    void sort_nodes_by_weight(Graph& g)
    {
      auto weightOf{
        [&g](std::size_t node) {
          return g.cbegin_node_weights()[numeric::unchecked_conversion_to<std::ptrdiff_t>(node)];
        }
      };

      g.sort_nodes([weightOf](std::size_t i, std::size_t j) { return weightOf(i) < weightOf(j); });
    }

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

    test_build_time_scales_linearly<directed_graph<null_weight, std::size_t, bucketed_edge_storage_config>>(
      "directed graph"
    );
    test_build_time_scales_linearly<
      undirected_graph<null_weight, std::size_t, null_meta_data, bucketed_edge_storage_config>
    >("undirected graph");
    test_build_time_scales_linearly<
      embedded_graph<null_weight, std::size_t, null_meta_data, bucketed_edge_storage_config>
    >("embedded graph");

    test_sort_time_scales_as_n_log_n<directed_graph<null_weight, std::size_t, bucketed_edge_storage_config>>(
      "directed graph"
    );
    test_sort_time_scales_as_n_log_n<
      undirected_graph<null_weight, std::size_t, null_meta_data, bucketed_edge_storage_config>
    >("undirected graph");
    test_sort_time_scales_as_n_log_n<
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
    const graph_blueprint blueprint{nodes_per_graph};

    auto buildSortedGraph{[&blueprint]() { return blueprint.sorted_graph<Graph>(); }};

    auto buildShuffledGraphAndSortNodes{
      [&blueprint]() {
        auto g{blueprint.shuffled_graph<Graph>()};
        sort_nodes_by_weight(g);
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

  /* Each performance check claims that building a graph takes time
     proportional to its size. The graphs have `joins_per_node` joins from each
     node, so their numbers of nodes and of joins grow together. Building the
     graph with `growth_factor` times as many nodes should take about
     `growth_factor` times as long. If each join cost time proportional to the
     number of nodes, it would take about `growth_factor` squared times as
     long. `growth_prediction` excludes such a time, and its lower end exceeds
     1, as `check_relative_performance` requires.
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
      {.prediction{growth_prediction}});
  }

  /* Each performance check claims that sorting the nodes of a graph with n
     nodes takes time proportional to n log n. The sort's comparisons take
     that long. Moving the nodes and their edges takes time proportional to
     the size of the graph. The graphs have `joins_per_node` joins from each
     node, so their numbers of nodes and of joins grow together.
     `check_relative_performance` copies each task before timing it, so each
     trial sorts a fresh copy of a shuffled graph, and the copy is not
     timed.

     Sorting the graph with `growth_factor` times as many nodes should take a
     little more than `growth_factor` times as long. If each swap of two
     nodes visited the edges of every node, sorting would take time
     proportional to n squared times log n, and so more than `growth_factor`
     squared times as long. `growth_prediction` excludes such a time.

     A directed graph holds each edge only at the node the edge leaves. So a
     swap of two nodes which relabels the edges pointing at them must visit
     the edges of every node. The check fails a sort made of such swaps. An
     undirected or embedded graph holds each edge at both of its nodes. So
     such a swap need visit only the edges of the two nodes and of their
     neighbours. The average number of those edges does not grow with the
     graph, so the check passes the sort. `test_sort_nodes` bounds the cost
     of such a sort instead.
  */
  template<dynamic_network Graph>
  void dynamic_graph_performance_test::test_sort_time_scales_as_n_log_n(std::string_view graphName)
  {
    auto nodeSortingTask{
      [](const std::size_t nodeCount) {
        return [g{graph_blueprint{nodeCount}.shuffled_graph<Graph>()}]() mutable {
          sort_nodes_by_weight(g);
          do_not_optimize_away(g);
        };
      }
    };

    check_relative_performance(
      std::format("Sorting the nodes of the {} takes time proportional to its size, up to a logarithmic factor",
                  graphName),
      nodeSortingTask(nodes_in_smaller_graph),
      nodeSortingTask(nodes_in_larger_graph),
      {.prediction{growth_prediction}});
  }
}
