////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "DynamicGraphTraversalsTest.hpp"
#include "DynamicGraphTestingUtilities.hpp"

#include "sequoia/Core/Concurrency/ConcurrencyModels.hpp"

namespace sequoia::testing
{
  namespace
  {
    template<maths::dynamic_network G, traversal_flavour Flavour>
    struct trackers
    {
      explicit trackers(const G& g) : edgeFirst{g}, edgeSecond{g} {}

      node_tracker nodeBefore, nodeAfter;
      edge_tracker<G, Flavour> edgeFirst, edgeSecond;
    };

    template<class Traverser, maths::dynamic_network G, maths::disconnected_discovery_mode Mode>
    [[nodiscard]]
    trackers<G, Traverser::flavour> traverse_graph(const G& g, const maths::traversal_conditions<Mode> conditions)
    {
      trackers<G, Traverser::flavour> t{g};
      if constexpr(maths::undirected(G::flavour))
      {
        Traverser::traverse(g, conditions, t.nodeBefore, t.nodeAfter, t.edgeFirst, t.edgeSecond);
      }
      else
      {
        Traverser::traverse(g, conditions, t.nodeBefore, t.nodeAfter, t.edgeFirst);
      }

      return t;
    }

    template<maths::dynamic_network Graph>
    [[nodiscard]]
    Graph generate_weighted_bfs_test_graph()
    {
      Graph graph;

      std::size_t depth{};
      int value = 1;
      std::queue<std::size_t> parents;
      parents.push(graph.add_node(value));

      //    0
      //   / \
      //  0   0
      //  /\
      // 0  0

      auto add_daughters{
        [&graph](const std::size_t index, std::pair<int, int> values)
          -> std::pair<std::size_t, std::size_t> {
          const std::size_t index0{graph.add_node(values.first)};
          const std::size_t index1{graph.add_node(values.second)};

          graph.join(index, index0);
          graph.join(index, index1);

          return std::make_pair(index0, index1);
        }
      };

      while(depth < 2)
      {
        ++value;
        const auto node = parents.front();
        auto daughters = add_daughters(node, std::make_pair(value, value));

        parents.pop();
        parents.push(daughters.first);
        parents.push(daughters.second);
        ++depth;
      }

      return graph;
    }


    template<maths::dynamic_network Graph>
    Graph generate_priority_test_graph()
    {
      Graph graph;

      graph.add_node(1);
      graph.add_node(5);
      graph.add_node(10);
      graph.add_node(7);
      graph.add_node(8);
      graph.add_node(4);
      graph.add_node(6);

      graph.join(0, 1);
      graph.join(0, 2);
      graph.join(0, 3);
      graph.join(0, 4);
      graph.join(3, 5);
      graph.join(3, 6);

      //     1
      //     |
      // --------
      // |  | | |
      // 5 10 7 8
      //     /\
      //    4  6

      return graph;
    }

    template<class ProcessingModel, maths::dynamic_network Graph, class... Args>
    [[nodiscard]]
    std::vector<int> task(Graph& graph, const int upper, const bool early, const std::chrono::microseconds pause, Args&&... args)
    {
      auto fn = [upper, pause](const std::size_t index) {
        std::this_thread::sleep_for(pause);
        const auto n{upper + static_cast<int>(index)};
        return n * (n + 1) / 2;
      };

      using namespace maths;
      if constexpr(Graph::directedness == directed_flavour::directed)
      {
        return early
          ? breadth_first_search(graph, ignore_disconnected_t{}, fn, null_func_obj{}, null_func_obj{}, ProcessingModel{std::forward<Args>(args)...})
          : breadth_first_search(graph, ignore_disconnected_t{}, null_func_obj{}, fn, null_func_obj{}, ProcessingModel{std::forward<Args>(args)...});
      }
      else
      {
        return early
          ? breadth_first_search(graph, ignore_disconnected_t{}, fn, null_func_obj{}, null_func_obj{}, null_func_obj{}, ProcessingModel{std::forward<Args>(args)...})
          : breadth_first_search(graph, ignore_disconnected_t{}, null_func_obj{}, fn, null_func_obj{}, null_func_obj{}, ProcessingModel{std::forward<Args>(args)...});
      }
    }

    template<class ProcessingModel, maths::dynamic_network Graph, class... Args>
    [[nodiscard]]
    std::vector<int> edge_first_traversal_task(Graph& graph, const int upper, Args&&... args)
    {
      auto fn = [upper](auto edgeIter) {
        const auto n{upper - static_cast<int>(edgeIter.partition_index())};
        return n * (n + 1) / 2;
      };

      using namespace maths;
      if constexpr(Graph::directedness == maths::directed_flavour::directed)
      {
        return breadth_first_search(graph, ignore_disconnected_t{}, null_func_obj{}, null_func_obj{}, fn, ProcessingModel{std::forward<Args>(args)...});
      }
      else
      {
        return breadth_first_search(graph, ignore_disconnected_t{}, null_func_obj{}, null_func_obj{}, fn, null_func_obj{}, ProcessingModel{std::forward<Args>(args)...});
      }
    }

    template<class ProcessingModel, maths::dynamic_network Graph, class... Args>
    [[nodiscard]]
    std::vector<int> edge_second_traversal_task(Graph& graph, const int upper, Args&&... args)
    {
      auto fn = [upper](auto edgeIter) {
        const auto n{upper - static_cast<int>(edgeIter.partition_index())};
        return n * (n + 1) / 2;
      };

      using namespace maths;
      return breadth_first_search(graph, ignore_disconnected_t{}, null_func_obj{}, null_func_obj{}, null_func_obj{}, fn, ProcessingModel{std::forward<Args>(args)...});
    }

    [[nodiscard]]
    std::vector<int> node_task_answers(const int upper)
    {
      std::vector<int> answers;
      int shift = 0;
      for(int i = 0; i < 5; ++i)
      {
        answers.push_back((upper + shift) * (upper + shift + 1) / 2);
        ++shift;
      }

      return answers;
    }

    [[nodiscard]]
    std::vector<int> edge_task_answers(const int upper)
    {
      std::vector<int> answers;

      int shift = 0;
      for(int i = 0; i < 4; ++i)
      {
        if(i == 2) --shift;
        answers.push_back((upper + shift) * (upper + shift + 1) / 2);
      }

      return answers;
    }
  }

  [[nodiscard]]
  std::string_view test_graph_traversals::source_file() const noexcept
  {
    return __FILE__;
  }

  void test_graph_traversals::run_tests()
  {
    test_prs_details();

    {
      graph_test_helper<null_weight, null_weight, test_graph_traversals> helper{*this};
      helper.run_tests();
    }

    {
      graph_test_helper<null_weight, int, test_graph_traversals> helper{*this};
      helper.run_tests();
    }
  }

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
  void test_graph_traversals::execute_operations()
  {
    using ESTraits = EdgeStorageTraits;
    using NSTraits = NodeWeightStorageTraits;
    using graph_type = graph_type_generator_t<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightCreator, NodeWeightCreator, ESTraits, NSTraits>;

    tracker_test<graph_type, Traverser<traversal_flavour::BFS>>();
    //tracker_test<graph_type, Traverser<traversal_flavour::DFS>>();
    tracker_test<graph_type, Traverser<traversal_flavour::PDFS>>();

    if constexpr(!std::is_empty_v<NodeWeight>)
    {
      test_weighted_BFS_tasks<graph_type>();
      test_priority_traversal<graph_type>();
    }
  }

  void test_graph_traversals::test_prs_details()
  {
    using namespace maths;

    using graph_type = embedded_graph<directed_flavour::undirected, null_weight, int>;
    graph_type graph;

    graph.add_node(3);
    graph.add_node(2);

    using node_comparer = graph_impl::node_comparer<graph_type, std::less<int>>;
    node_comparer compare(graph);

    check(LINE("node_comparer sees that weight_0 > weight_1 and so returns false"), !compare(0, 1));

    auto stack = graph_impl::queue_constructor<graph_type, std::stack<std::size_t>>::make(graph);
    stack.push(0);
    stack.push(1);
    check_equality(LINE(""), stack.top(), 1_sz);
    stack.pop();
    check_equality(LINE(""), stack.top(), 0_sz);

    using PQ_t = std::priority_queue<std::size_t, std::vector<std::size_t>, graph_impl::node_comparer<graph_type, std::less<int>>>;

    auto pqueue = graph_impl::queue_constructor<graph_type, PQ_t>::make(graph);
    pqueue.push(0);
    pqueue.push(1);

    check_equality(LINE(""), pqueue.top(), 0_sz);
  }

  //=============================== Tracker Test ===============================//

  template<maths::dynamic_network Graph, class Traverser>
  void test_graph_traversals::tracker_test()
  {
    using namespace maths;
    using TraversalType = traversal_constant<Traverser::flavour>;

    constexpr auto GraphFlavour{Graph::flavour};
    constexpr bool mutualInfo{mutual_info(GraphFlavour)};
    constexpr bool undirected{maths::undirected(GraphFlavour)};
    constexpr bool isBFS{Traverser::flavour == traversal_flavour::BFS};
    constexpr bool forwardIter{Traverser::uses_forward_iterator()};

    auto make_message{
      [](std::string_view message) {
        return append_lines(demangle<Traverser>(), message);
      }
    };

    Graph g{};
    using edge_results = typename edge_tracker<Graph, Traverser::flavour>::result_type;

    {
      const auto[nodeDiscovery1, nodeDiscovery2, edgeDiscovery1, edgeDiscovery2]{traverse_graph<Traverser>(g, find_disconnected_t{})};

      check_equivalence(LINE(make_message("No nodes to discover")), nodeDiscovery1, std::vector<std::size_t>{});
      check_equivalence(LINE(make_message("No nodes to discover")), nodeDiscovery2, std::vector<std::size_t>{});
      check_equivalence(LINE(make_message("No edges to discover")), edgeDiscovery1, edge_results{});
      if constexpr(undirected)
      {
        check_equivalence(LINE(make_message("No edges to discover")), edgeDiscovery2, edge_results{});
      }
    }

    check_equality(LINE(make_message("First node added")), g.add_node(), 0_sz);
    // 0

    {
      const auto[nodeDiscovery1, nodeDiscovery2, edgeDiscovery1, edgeDiscovery2]{traverse_graph<Traverser>(g, find_disconnected_t{})};

      check_equivalence(LINE(make_message("One nodes to discover")), nodeDiscovery1, std::vector<std::size_t>{0});
      check_equivalence(LINE(make_message("One nodes to discover")), nodeDiscovery2, std::vector<std::size_t>{0});
      check_equivalence(LINE(make_message("No edges to discover")), edgeDiscovery1, edge_results{});
      if constexpr(undirected)
      {
        check_equivalence(LINE(make_message("No edges to discover")), edgeDiscovery2, edge_results{});
      }
    }

    check_equality(LINE(make_message("Second node added")), g.add_node(), 1_sz);
    // 0 0

    {
      const auto[nodeDiscovery1, nodeDiscovery2, edgeDiscovery1, edgeDiscovery2]{traverse_graph<Traverser>(g, find_disconnected_t{})};

      check_equivalence(LINE(make_message("Two nodes to discover")), nodeDiscovery1, std::vector<std::size_t>{0, 1});
      check_equivalence(LINE(make_message("Two nodes to discover")), nodeDiscovery2, std::vector<std::size_t>{0, 1});
      check_equivalence(LINE(make_message("No edges to discover")), edgeDiscovery1, edge_results{});
      if constexpr(undirected)
      {
        check_equivalence(make_message("No edges to discover"), edgeDiscovery2, edge_results{});
      }
    }

    {
      const auto[nodeDiscovery1, nodeDiscovery2, edgeDiscovery1, edgeDiscovery2]{traverse_graph<Traverser>(g, find_disconnected_t{1})};

      check_equivalence(LINE(make_message("Two nodes to discover in reverse")), nodeDiscovery1, std::vector<std::size_t>{1, 0});
      check_equivalence(LINE(make_message("Two nodes to discover in reverse")), nodeDiscovery2, std::vector<std::size_t>{1, 0});
      check_equivalence(LINE(make_message("No edges to discover")), edgeDiscovery1, edge_results{});
      if constexpr(undirected)
      {
        check_equivalence(LINE(make_message("No edges to discover")), edgeDiscovery2, edge_results{});
      }
    }

    {
      const auto[nodeDiscovery1, nodeDiscovery2, edgeDiscovery1, edgeDiscovery2]{traverse_graph<Traverser>(g, ignore_disconnected_t{})};

      check_equivalence(LINE(make_message("Two nodes; one to discover")), nodeDiscovery1, std::vector<std::size_t>{0});
      check_equivalence(LINE(make_message("Two nodes; one to discover")), nodeDiscovery2, std::vector<std::size_t>{0});
      check_equivalence(LINE(make_message("No edges to discover")), edgeDiscovery1, edge_results{});
      if constexpr(undirected)
      {
        check_equivalence(LINE(make_message("No edges to discover")), edgeDiscovery2, edge_results{});
      }
    }

    check_equality(LINE(make_message("Third node added")), g.add_node(), 2_sz);
    g.join(0, 1);
    g.join(1, 2);
    // 0----0----0

    {
      const auto[nodeDiscovery1, nodeDiscovery2, edgeDiscovery1, edgeDiscovery2]{traverse_graph<Traverser>(g, find_disconnected_t{})};

      check_equivalence(LINE(make_message("Three nodes to discover")), nodeDiscovery1, std::vector<std::size_t>{0, 1, 2});
      check_equivalence(LINE(make_message("Three nodes to discover")), nodeDiscovery2, std::vector<std::size_t>{0, 1, 2});
      check_equivalence(LINE(make_message("Two edges to discover")), edgeDiscovery1, edge_results{{0,0}, {1, mutualInfo && forwardIter ? 1 : 0}});
      if constexpr(undirected)
      {
        check_equivalence(LINE(make_message("Two edges to discover")), edgeDiscovery2, isBFS ? edge_results{{1,0}, {2,0}} : edge_results{{1, 1}, {2, 0}});
      }
    }

    {
      const auto[nodeDiscovery1, nodeDiscovery2, edgeDiscovery1, edgeDiscovery2]{traverse_graph<Traverser>(g, ignore_disconnected_t{1})};

      if constexpr(undirected)
      {
        check_equivalence(LINE(make_message("Three nodes to discover")), nodeDiscovery1, std::vector<std::size_t>{1, 0, 2});
        check_equivalence(LINE(make_message("Three nodes to discover")), nodeDiscovery2, std::vector<std::size_t>{1, 0, 2});
        check_equivalence(LINE(make_message("Two edges to discover")), edgeDiscovery1, edge_results{{1,0}, {1, 1}});
        check_equivalence(LINE(make_message("Two edges to discover")), edgeDiscovery2, edge_results{{0, 0}, {2, 0}});
      }
      else
      {
        check_equivalence(LINE(make_message("Two nodes to discover")), nodeDiscovery1, std::vector<std::size_t>{1, 2});
        check_equivalence(LINE(make_message("Two nodes to discover")), nodeDiscovery2, std::vector<std::size_t>{1, 2});
        check_equivalence(LINE(make_message("One edge to discover")), edgeDiscovery1, edge_results{{1, mutualInfo && forwardIter ? 1 : 0}});
      }
    }

    {
      const auto[nodeDiscovery1, nodeDiscovery2, edgeDiscovery1, edgeDiscovery2]{traverse_graph<Traverser>(g, ignore_disconnected_t{2})};

      if constexpr(undirected)
      {
        check_equivalence(LINE(make_message("Three nodes to discover")), nodeDiscovery1, std::vector<std::size_t>{2, 1, 0});
        check_equivalence(LINE(make_message("Three nodes to discover")), nodeDiscovery2, std::vector<std::size_t>{2, 1, 0});
        check_equivalence(LINE(make_message("Two edges to discover")), edgeDiscovery1, edge_results{{2,0}, {1, forwardIter ? 0 : 1}});
        check_equivalence(LINE(make_message("Two edges to discover")), edgeDiscovery2, isBFS ? edge_results{{1, 1}, {0, 0}} : edge_results{{1, 0}, {0, 0}});
      }
      else
      {
        check_equivalence(LINE(make_message("One node to discover")), nodeDiscovery1, std::vector<std::size_t>{2});
        check_equivalence(LINE(make_message("One node to discover")), nodeDiscovery2, std::vector<std::size_t>{2});
        check_equivalence(LINE(make_message("No edges to discover")), edgeDiscovery1, edge_results{});
      }
    }

    check_equality(LINE(make_message("Fourth node added")), g.add_node(), 3_sz);
    g.join(2, 3);
    g.join(3, 0);
    //  0----0
    //  |    |
    //  |    |
    //  0----0

    test_square_graph(g, 0, TraversalType{});
    test_square_graph(g, 2, TraversalType{});
  }

  template<maths::dynamic_network G>
  void test_graph_traversals::test_square_graph(const G& g, const std::size_t start, bfs_type)
  {
    const auto[nodeDiscovery1, nodeDiscovery2, edgeDiscovery1, edgeDiscovery2]{traverse_graph<Traverser<bfs_type::value>>(g, maths::ignore_disconnected_t{start})};

    using edge_results = typename decltype(edgeDiscovery1)::result_type;
    if constexpr(maths::undirected(G::flavour))
    {
      std::vector<std::size_t> expected;
      edge_results edgeAnswers, edgeAnswers2;
      if(start == 0)
      {
        expected = std::vector<std::size_t>{0,1,3,2};
        edgeAnswers = edge_results{{0, 0}, {0, 1}, {1, 1}, {3, 0}};
        edgeAnswers2 = edge_results{{1, 0}, {3, 1}, {2, 0}, {2, 1}};
      }
      else if(start == 2)
      {
        expected = std::vector<std::size_t>{2,1,3,0};
        edgeAnswers = edge_results{{2, 0}, {2, 1}, {1, 0}, {3, 1}};
        edgeAnswers2 = edge_results{{1, 1}, {3, 0}, {0, 0}, {0, 1}};
      }
      check_equivalence(LINE(""), nodeDiscovery1, expected);
      check_equivalence(LINE("First edge traversal"), edgeDiscovery1, edgeAnswers);
      check_equivalence(LINE("Second edge traversal"), edgeDiscovery2, edgeAnswers2);
    }
    else
    {
      std::vector<std::size_t> expected;
      edge_results edgeAnswers;
      constexpr auto mutualInfo{mutual_info(G::flavour)};
      if(start == 0)
      {
        expected = std::vector<std::size_t>{0,1,2,3};
        edgeAnswers = mutualInfo ? edge_results{{0, 0}, {1, 1}, {2, 1}, {3, 1}} : edge_results{{0, 0}, {1, 0}, {2, 0}, {3, 0}};
      }
      else if(start == 2)
      {
        expected = std::vector<std::size_t>{2,3,0,1};
        edgeAnswers = mutualInfo ? edge_results{{2, 1}, {3, 1}, {0, 0}, {1, 1}} : edge_results{{2, 0}, {3, 0}, {0, 0}, {1, 0}};
      }
      check_equivalence(LINE("start = " + std::to_string(start) + " "), nodeDiscovery1, expected);
      check_equivalence(LINE("First edge traversal, start = " + std::to_string(start) + " "), edgeDiscovery1, edgeAnswers);
    }
  }

  template<maths::dynamic_network G>
  void test_graph_traversals::test_square_graph(const G& g, const std::size_t start, dfs_type)
  {
  }

  template<maths::dynamic_network G>
  void test_graph_traversals::test_square_graph(const G& g, const std::size_t start, pdfs_type)
  {
    const auto[nodeDiscovery1, nodeDiscovery2, edgeDiscovery1, edgeDiscovery2]{traverse_graph<Traverser<pdfs_type::value>>(g, maths::ignore_disconnected_t{start})};

    using edge_results = typename decltype(edgeDiscovery1)::result_type;
    if constexpr(maths::undirected(G::flavour))
    {
      std::vector<std::size_t> expected;
      edge_results edgeAnswers, edgeAnswers2;
      if(start == 0)
      {
        expected = std::vector<std::size_t>{0,1,2,3};
        edgeAnswers = edge_results{{0, 0}, {0, 1}, {1, 0}, {2, 0}};
        edgeAnswers2 = edge_results{{1, 1}, {2, 1}, {3, 0}, {3, 1}};
      }
      else if(start == 2)
      {
        expected = std::vector<std::size_t>{2,1,0,3};
        edgeAnswers = edge_results{{2, 0}, {2, 1}, {1, 1}, {0, 0}};
        edgeAnswers2 = edge_results{{1, 0}, {0, 1}, {3, 0}, {3, 1}};
      }
      check_equivalence(LINE(""), nodeDiscovery1, expected);
      check_equivalence(LINE("First edge traversal"), edgeDiscovery1, edgeAnswers);
      check_equivalence(LINE("Second edge traversal"), edgeDiscovery2, edgeAnswers2);
    }
    else
    {
      std::vector<std::size_t> expected;
      edge_results edgeAnswers;
      constexpr auto mutualInfo{mutual_info(G::flavour)};
      if(start == 0)
      {
        expected = std::vector<std::size_t>{0,1,2,3};
        edgeAnswers = mutualInfo ? edge_results{{0, 1}, {1, 0}, {2, 0}, {3, 0}} : edge_results{{0, 0}, {1, 0}, {2, 0}, {3, 0}};
      }
      else if(start == 2)
      {
        expected = std::vector<std::size_t>{2,3,0,1};
        edgeAnswers = mutualInfo ? edge_results{{2, 0}, {3, 0}, {0, 1}, {1, 0}} : edge_results{{2, 0}, {3, 0}, {0, 0}, {1, 0}};
      }
      check_equivalence(LINE("start = " + std::to_string(start) + " "), nodeDiscovery1, expected);
      check_equivalence(LINE("First edge traversal, start = " + std::to_string(start) + " "), edgeDiscovery1, edgeAnswers);
    }
  }

  //=============================== Priority Search  ===============================//

  template<maths::dynamic_network Graph>
  void test_graph_traversals::test_priority_traversal()
  {
    auto graph{generate_priority_test_graph<Graph>()};

    node_tracker tracker;
    maths::priority_search(graph, maths::ignore_disconnected_t{}, tracker);

    check_equivalence(LINE(""), tracker, std::vector<std::size_t>{0,2,4,3,6,1,5});
  }

  //=============================== Weighted BFS  ===============================//

  template<maths::dynamic_network Graph>
  void test_graph_traversals::test_weighted_BFS_tasks()
  {
    test_node_and_first_edge_traversal<Graph>();

    if constexpr(maths::undirected(Graph::flavour))
    {
      test_edge_second_traversal<Graph>();
    }
  }

  template<maths::dynamic_network Graph>
  void test_graph_traversals::test_edge_second_traversal()
  {
    auto graph{generate_weighted_bfs_test_graph<Graph>()};

    constexpr int upper = 6;

    auto serialFn = [&graph](){
      return edge_second_traversal_task<concurrency::serial<int>>(graph, upper);
    };

    auto asyncFn = [&graph](){
      return edge_second_traversal_task<concurrency::asynchronous<int>>(graph, upper);
    };

    auto poolFn = [&graph](){
      return edge_second_traversal_task<concurrency::thread_pool<int>>(graph, upper, 4u);
    };

    auto answers{
      [](const int upper) -> std::vector<int> {
        std::vector<int> answers;

        for(int i=0; i < 4; ++i)
        {
          const int shift = -i - 1;
          answers.push_back((upper + shift)*(upper + shift + 1) / 2);
        }

        return answers;
      }
    };

    const std::vector<int>
      serialResults = serialFn(),
      asyncResults = asyncFn(),
      poolResults = poolFn(),
      expected = answers(upper);

    check_equality(LINE("Null edge first task expected"), serialResults, expected);
    check_equality(LINE("Async edge first task expected"), asyncResults, expected);
    check_equality(LINE("Pool edge first task expected"), poolResults, expected);
  }

  template<maths::dynamic_network Graph>
  void test_graph_traversals::test_node_and_first_edge_traversal()
  {
    auto graph{generate_weighted_bfs_test_graph<Graph>()};

    //================================ Node functors =========================//

    int upper = 10;
    for(int i=0; i < 2; ++i)
    {
      const bool early{i == 0};
      using microseconds = std::chrono::microseconds;
      auto serialFn  = [&graph, upper, early](){ return task<concurrency::serial<int>>(graph, upper, early, microseconds{}); };
      auto asyncFn = [&graph, upper, early](){ return task<concurrency::asynchronous<int>>(graph, upper, early, microseconds{}); };
      auto poolFn  = [&graph, upper, early](){ return task<concurrency::thread_pool<int>>(graph, upper, early, microseconds{}, 4u); };
      const std::vector<int>
        serialResults = serialFn(),
        asyncResults = asyncFn(),
        poolResults = poolFn(),
        expected = node_task_answers(upper);

      check_equality(LINE("Null node task expected"), serialResults, expected);
      check_equality(LINE("Async node task expected"), asyncResults, expected);
      check_equality(LINE("Pool node task expected"), poolResults, expected);
    }

    //================================ Edge First Traversal functors =========================//

    {
      upper = 6;

      auto serialFn  = [&graph, upper](){
        return edge_first_traversal_task<concurrency::serial<int>>(graph, upper);
      };

      auto asyncFn = [&graph, upper](){
        return edge_first_traversal_task<concurrency::asynchronous<int>>(graph, upper);
      };

      auto poolFn  = [&graph, upper](){
        return edge_first_traversal_task<concurrency::thread_pool<int>>(graph, upper, 4u);
      };

      const std::vector<int>
        serialResults = serialFn(),
        asyncResults = asyncFn(),
        poolResults = poolFn(),
        expected = edge_task_answers(upper);

      check_equality(LINE("Null edge first task expected"), serialResults, expected);
      check_equality(LINE("Async edge first task expected"), asyncResults, expected);
      check_equality(LINE("Pool edge first task expected"), poolResults, expected);

    }


    //================================ Now check performance =========================//

    {
      upper = 10;

      const auto pause{std::chrono::milliseconds(20)};

      auto serialFn{
        [&graph, upper, pause]() {
          return task<concurrency::serial<int>>(graph, upper, true, pause);
        }
      };

      auto asyncFn{
        [&graph, upper, pause](){
          return task<concurrency::asynchronous<int>>(graph, upper, true, pause);
        }
      };

      auto poolFn{
        [&graph, upper, pause](){
          return task<concurrency::thread_pool<int>>(graph, upper, true, pause, 4u);
        }
      };

      check_relative_performance(LINE("Null versus async check"), asyncFn, serialFn, 2.0, 5.0);
      check_relative_performance(LINE("Null versus pool check"), poolFn, serialFn, 2.0, 5.0);
    }
  }
}
