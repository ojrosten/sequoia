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
#include <stdexcept>
#include <vector>

namespace sequoia::testing
{
  using namespace maths;

  namespace
  {
    /** \brief The number of nodes in each graph which `test_sort_nodes`
               builds, so that each such graph is small.
     */
    constexpr std::size_t nodes_per_sort_versus_build_graph{64};

    /** \brief The number of joins from each node of every graph.

        So every graph is sparse. The numbers of nodes and of joins of a
        graph grow together.
     */
    constexpr std::size_t joins_per_node{4};

    /** \brief The number of nodes in the smaller graph of each scaling
               check.
     */
    constexpr std::size_t nodes_in_smaller_graph{256};

    /** \brief The factors by which the larger graph of each scaling check
               has more nodes than the smaller.
     */
    constexpr std::size_t growth_factor_of_build_check{8},
                          growth_factor_of_sort_check{16};

    /** \brief The intervals of speed-ups predicted for the scaling checks of
               building and of sorting.

        A speed-up is the duration of the larger graph's task over the
        smaller's.
     */
    constexpr relative_performance_interval prediction_of_build_check{.lower{growth_factor_of_build_check / 2.0},
                                                                      .upper{growth_factor_of_build_check * 2.0}},
                                            prediction_of_sort_check{.lower{growth_factor_of_sort_check / 2.0},
                                                                     .upper{growth_factor_of_sort_check * 2.0}};

    static_assert((nodes_per_sort_versus_build_graph > 1) && (nodes_in_smaller_graph > 1));

    // `check_relative_performance` refuses a prediction whose lower end does
    // not exceed 1
    static_assert((prediction_of_build_check.lower > 1) && (prediction_of_sort_check.lower > 1));

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

        \throws std::invalid_argument if `nodeCount` is less than 2.
     */
    [[nodiscard]]
    std::vector<nodes_to_join> random_joins(const std::size_t nodeCount)
    {
      if(nodeCount < 2)
        throw std::invalid_argument{std::format("Random joins require at least 2 nodes, not {}", nodeCount)};

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

    /** \brief Sorts the nodes of `g` into ascending order of weight. */
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
    const graph_blueprint blueprint{nodes_per_sort_versus_build_graph};

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
     proportional to its size. Building the graph with
     `growth_factor_of_build_check` times as many nodes should take about
     `growth_factor_of_build_check` times as long. If each join cost time
     proportional to the number of nodes, it would take about
     `growth_factor_of_build_check` squared times as long.
     `prediction_of_build_check` excludes such a time.
  */
  template<dynamic_network Graph>
  void dynamic_graph_performance_test::test_build_time_scales_linearly(std::string_view graphName)
  {
    constexpr auto nodesInLargerGraph{growth_factor_of_build_check * nodes_in_smaller_graph};

    const auto joinsOfSmallerGraph{random_joins(nodes_in_smaller_graph)},
               joinsOfLargerGraph{random_joins(nodesInLargerGraph)};

    auto buildSmallerGraph{
      [&joinsOfSmallerGraph]() { return graph_with_nodes<Graph>(nodes_in_smaller_graph, joinsOfSmallerGraph); }
    };

    auto buildLargerGraph{
      [&joinsOfLargerGraph]() { return graph_with_nodes<Graph>(nodesInLargerGraph, joinsOfLargerGraph); }
    };

    check_relative_performance(
      std::format("Building the {} takes time proportional to its size", graphName),
      buildSmallerGraph,
      buildLargerGraph,
      {.prediction{prediction_of_build_check}});
  }

  /* Each performance check claims that sorting the nodes of a graph takes
     time proportional to its size, up to a logarithmic factor. Each task
     holds its shuffled graph by value, so each trial sorts a fresh copy,
     made untimed.

     Each check bounds the growth of the time from above. For a graph with
     `growth_factor_of_sort_check` times as many nodes, time proportional to
     the number of nodes predicts `growth_factor_of_sort_check` times as
     long. Time proportional to n log n, for n nodes, predicts somewhat
     longer. Both lie inside `prediction_of_sort_check`, so no check tells
     them apart.

     Each check guards in particular against a step taking time
     proportional to n squared. Finding the destination of each node by a
     linear search through the sorted nodes is such a step. With that
     search, sorting the larger graph took about 100 to 110 times as long
     as sorting the smaller in a release build, and about 60 times as long
     in debug and ASan builds. `prediction_of_sort_check` excludes each of
     these.

     A directed graph holds each edge only at the node the edge leaves. So a
     swap of two nodes which relabels the edges pointing at them must visit
     the edges of every node. A sort made of such swaps takes time
     proportional to n squared times log n, and each check fails such a sort
     of a directed graph. An undirected or embedded graph holds each edge at
     both of its nodes. So such a swap need visit only the edges of the two
     nodes and of their neighbours, whose average number does not grow with
     the graph. Each check passes a sort made of such swaps of an undirected
     or embedded graph. `test_sort_nodes` fails such a sort.
  */
  template<dynamic_network Graph>
  void dynamic_graph_performance_test::test_sort_time_scales_as_n_log_n(std::string_view graphName)
  {
    constexpr auto nodesInLargerGraph{growth_factor_of_sort_check * nodes_in_smaller_graph};

    auto sortSmallerGraph{
      [g{graph_blueprint{nodes_in_smaller_graph}.shuffled_graph<Graph>()}]() mutable {
        sort_nodes_by_weight(g);
        do_not_optimize_away(g);
      }
    };

    auto sortLargerGraph{
      [g{graph_blueprint{nodesInLargerGraph}.shuffled_graph<Graph>()}]() mutable {
        sort_nodes_by_weight(g);
        do_not_optimize_away(g);
      }
    };

    check_relative_performance(
      std::format("Sorting the nodes of the {} takes time proportional to its size, up to a logarithmic factor",
                  graphName),
      sortSmallerGraph,
      sortLargerGraph,
      {.prediction{prediction_of_sort_check}});
  }
}
