////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "GraphTraversalTestingUtilities.hpp"

namespace sequoia::testing
{
  class node_tracker
  {
  public:
    void clear() noexcept { m_Order.clear(); }

    void operator()(const std::size_t index) { m_Order.push_back(index); }

    [[nodiscard]]
    const std::vector<std::size_t>& order() const noexcept { return m_Order; }
  private:
    std::vector<std::size_t> m_Order;
  };

  template<class G, class T>
  class edge_tracker
  {
  public:
    using result_type = std::vector<std::pair<std::size_t, std::size_t>>;

    edge_tracker(const G& graph) : m_Graph(graph) {}

    void clear() noexcept { m_Order.clear(); }

    template<class I> void operator()(I iter)
    {
      const auto pos = dist(typename std::is_same<typename T::type, DFS>::type(), iter);
      m_Order.emplace_back(iter.partition_index(), static_cast<std::size_t>(pos));
    }

    [[nodiscard]]
    const result_type& order() const noexcept { return m_Order; }
  private:
    result_type m_Order;
    const G& m_Graph;

    template<class I> [[nodiscard]] auto dist(std::true_type, I iter)
    {
      return distance(m_Graph.crbegin_edges(iter.partition_index()), iter);
    }

    template<class I> [[nodiscard]] auto dist(std::false_type, I iter)
    {
      return distance(m_Graph.cbegin_edges(iter.partition_index()), iter);
    }
  };

  template<class F> void clear(F& f)
  {
    f.clear();
  }

  inline void clear(maths::null_functor&) {}

  template<class F, class... Fn>
  void clear(F& f, Fn&... fn)
  {
    f.clear();
    clear(fn...);
  }

  // TO DO: separate out performance test
  class test_graph_traversals final : public performance_test
  {
  public:
    using performance_test::performance_test;

    [[nodiscard]]
    std::string_view source_file() const noexcept final;
  private:
    struct null_weight {};

    template<class, class, class>
    friend class graph_test_helper;

    void run_tests() final;

    void test_prs_details();

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
    void execute_operations();

    template<class Graph, class Traverser>
    void tracker_test();

    template<class Graph>
    void test_weighted_BFS_tasks();

    template<class Graph>
    void test_priority_traversal();

    //=================== For tracker =================//
    template<class Traverser, class G, class... Fn>
    void traverse_graph(const G& g, const maths::find_disconnected findDisconnected, const std::size_t start, Fn&&... fn)
    {
      clear(std::forward<Fn>(fn)...);
      Traverser::traverse(g, findDisconnected, start, std::forward<Fn>(fn)...);
    }

    // true_types correspond BFS
    template<class NTracker, class ETracker, class ETracker2>
    void test_square_graph(const NTracker& tracker, const ETracker& eTracker, const ETracker2& eTracker2, const std::size_t start, const bool, std::true_type);

    template<class NTracker, class ETracker>
    void test_square_graph(const NTracker& tracker, const ETracker& eTracker, const std::size_t start, const bool mutualInfo, std::true_type);

    template<class NTracker, class ETracker, class ETracker2>
    void test_square_graph(const NTracker& tracker, const ETracker& eTracker, const ETracker2& eTracker2, const std::size_t start, const bool, std::false_type);

    template<class NTracker, class ETracker>
    void test_square_graph(const NTracker& tracker, const ETracker& eTracker, const std::size_t start, const bool mutualInfo, std::false_type);

    //=================== For priority  =================//
    template<class Graph>
    Graph generate_priority_test_graph();

    //=================== For weighted BFS  =================//

    template<class Graph>
    void test_edge_second_traversal(std::false_type) {}

    template<class Graph>
    void test_edge_second_traversal(std::true_type);

    template<class Graph>
    void test_node_and_first_edge_traversal();

    template<class ProcessingModel, class Graph, class... Args>
    [[nodiscard]]
    static std::vector<int> task(Graph& graph, const int upper, const bool early, const std::chrono::microseconds pause, Args&&... args);

    template<class ProcessingModel, class Graph, class... Args>
    [[nodiscard]]
    static std::vector<int> edge_first_traversal_task(Graph& graph, const int upper, Args&&... args);

    template<class ProcessingModel, class Graph, class... Args>
    [[nodiscard]]
    static std::vector<int> edge_second_traversal_task(Graph& graph, const int upper, Args&&... args);

    [[nodiscard]]
    static std::vector<int> node_task_answers(const int upper);

    [[nodiscard]]
    static std::vector<int> edge_task_answers(const int upper);

    template<class Graph>
    [[nodiscard]]
    static Graph generate_weighted_bfs_test_graph();
  };
}
