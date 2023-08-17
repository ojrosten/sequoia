////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "DynamicGraphTraversalsTest.hpp"
#include "DynamicGraphTestingUtilities.hpp"

#include "sequoia/Core/Concurrency/ConcurrencyModels.hpp"

namespace sequoia::testing
{
  namespace
  {
    template<maths::dynamic_network G, maths::traversal_flavour Flavour>
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
      if constexpr(maths::undirected(G::flavour) && (Traverser::flavour != maths::traversal_flavour::depth_first))
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
          ? traverse(breadth_first, graph, ignore_disconnected_t{}, fn, null_func_obj{}, null_func_obj{}, ProcessingModel{std::forward<Args>(args)...})
          : traverse(breadth_first, graph, ignore_disconnected_t{}, null_func_obj{}, fn, null_func_obj{}, ProcessingModel{std::forward<Args>(args)...});
      }
      else
      {
        return early
          ? traverse(breadth_first, graph, ignore_disconnected_t{}, fn, null_func_obj{}, null_func_obj{}, null_func_obj{}, ProcessingModel{std::forward<Args>(args)...})
          : traverse(breadth_first, graph, ignore_disconnected_t{}, null_func_obj{}, fn, null_func_obj{}, null_func_obj{}, ProcessingModel{std::forward<Args>(args)...});
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
        return traverse(breadth_first, graph, ignore_disconnected_t{}, null_func_obj{}, null_func_obj{}, fn, ProcessingModel{std::forward<Args>(args)...});
      }
      else
      {
        return traverse(breadth_first, graph, ignore_disconnected_t{}, null_func_obj{}, null_func_obj{}, fn, null_func_obj{}, ProcessingModel{std::forward<Args>(args)...});
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
      return traverse(breadth_first, graph, ignore_disconnected_t{}, null_func_obj{}, null_func_obj{}, null_func_obj{}, fn, ProcessingModel{std::forward<Args>(args)...});
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
  std::filesystem::path test_graph_traversals::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void test_graph_traversals::run_tests()
  {
    using namespace maths;

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
    class EdgeStorageTraits,
    class NodeWeightStorageTraits
   >
  void test_graph_traversals::execute_operations()
  {
    using ESTraits = EdgeStorageTraits;
    using NSTraits = NodeWeightStorageTraits;
    using graph_type = graph_type_generator_t<GraphFlavour, EdgeWeight, NodeWeight, ESTraits, NSTraits>;

    tracker_test<graph_type, Traverser<maths::traversal_flavour::breadth_first>>();
    tracker_test<graph_type, Traverser<maths::traversal_flavour::depth_first>>();
    tracker_test<graph_type, Traverser<maths::traversal_flavour::pseudo_depth_first>>();

    if constexpr(!std::is_empty_v<NodeWeight>)
    {
      test_weighted_BFS_tasks<graph_type>();
      test_priority_traversal<graph_type>();
    }
  }

  void test_graph_traversals::test_prs_details()
  {
    using namespace maths;

    using graph_type = embedded_graph<null_weight, int>;
    graph_type graph;

    graph.add_node(3);
    graph.add_node(2);

    using node_comparer = graph_impl::node_comparer<graph_type, std::ranges::less>;
    node_comparer compare(graph);

    check(report_line("node_comparer sees that weight_0 > weight_1 and so returns false"), !compare(0, 1));

    auto stack = graph_impl::traversal_traits<graph_type, traversal_flavour::pseudo_depth_first>::make();
    stack.push(0);
    stack.push(1);
    check(equality, report_line(""), stack.top(), 1_sz);
    stack.pop();
    check(equality, report_line(""), stack.top(), 0_sz);

    using compare_t = graph_impl::node_comparer<graph_type, std::ranges::less>;
    auto pqueue = graph_impl::traversal_traits<graph_type, traversal_flavour::priority, compare_t>::make(compare_t{graph});
    pqueue.push(0);
    pqueue.push(1);

    check(equality, report_line(""), pqueue.top(), 0_sz);
  }

  //=============================== Tracker Test ===============================//

  template<maths::dynamic_network Graph, class Traverser>
  void test_graph_traversals::tracker_test()
  {
    using namespace maths;
    using TraversalType = traversal_constant<Traverser::flavour>;

    constexpr auto GraphFlavour{Graph::flavour};
    constexpr bool mutualInfo{!is_directed(GraphFlavour)};
    constexpr bool undirected{maths::undirected(GraphFlavour)};
    constexpr bool isBFS{Traverser::flavour == traversal_flavour::breadth_first};
    constexpr bool isDFS{Traverser::flavour == traversal_flavour::depth_first};
    constexpr bool forwardIter{Traverser::uses_forward_iterator()};

    auto make_message{
      [](std::string_view message) {
        return append_lines(to_string(Traverser::flavour), message);
      }
    };

    Graph g{};
    using edge_order = typename edge_tracker<Graph, Traverser::flavour>::result_type;
    using node_order = std::vector<std::size_t>;


    {
      const auto[nodeDiscovery1, nodeDiscovery2, edgeDiscovery1, edgeDiscovery2]{traverse_graph<Traverser>(g, find_disconnected_t{})};

      check(equivalence, report_line(make_message("No nodes to discover")), nodeDiscovery1, node_order{});
      check(equivalence, report_line(make_message("No nodes to discover")), nodeDiscovery2, node_order{});
      check(equivalence, report_line(make_message("No edges to discover")), edgeDiscovery1, edge_order{});
      if constexpr(undirected && !isDFS)
      {
        check(equivalence, report_line(make_message("No edges to discover")), edgeDiscovery2, edge_order{});
      }
    }

    check(equality, report_line(make_message("First node added")), g.add_node(), 0_sz);
    // 0

    {
      const auto[nodeDiscovery1, nodeDiscovery2, edgeDiscovery1, edgeDiscovery2]{traverse_graph<Traverser>(g, find_disconnected_t{})};

      check(equivalence, report_line(make_message("One nodes to discover")), nodeDiscovery1, node_order{0});
      check(equivalence, report_line(make_message("One nodes to discover")), nodeDiscovery2, node_order{0});
      check(equivalence, report_line(make_message("No edges to discover")), edgeDiscovery1, edge_order{});
      if constexpr(undirected && !isDFS)
      {
        check(equivalence, report_line(make_message("No edges to discover")), edgeDiscovery2, edge_order{});
      }
    }

    check(equality, report_line(make_message("Second node added")), g.add_node(), 1_sz);
    // 0 0

    {
      const auto[nodeDiscovery1, nodeDiscovery2, edgeDiscovery1, edgeDiscovery2]{traverse_graph<Traverser>(g, find_disconnected_t{})};

      check(equivalence, report_line(make_message("Two nodes to discover")), nodeDiscovery1, node_order{0, 1});
      check(equivalence, report_line(make_message("Two nodes to discover")), nodeDiscovery2, node_order{0, 1});
      check(equivalence, report_line(make_message("No edges to discover")), edgeDiscovery1, edge_order{});
      if constexpr(undirected && !isDFS)
      {
        check(equivalence, make_message("No edges to discover"), edgeDiscovery2, edge_order{});
      }
    }

    {
      const auto[nodeDiscovery1, nodeDiscovery2, edgeDiscovery1, edgeDiscovery2]{traverse_graph<Traverser>(g, find_disconnected_t{1})};

      check(equivalence, report_line(make_message("Two nodes to discover in reverse")), nodeDiscovery1, node_order{1, 0});
      check(equivalence, report_line(make_message("Two nodes to discover in reverse")), nodeDiscovery2, node_order{1, 0});
      check(equivalence, report_line(make_message("No edges to discover")), edgeDiscovery1, edge_order{});
      if constexpr(undirected && !isDFS)
      {
        check(equivalence, report_line(make_message("No edges to discover")), edgeDiscovery2, edge_order{});
      }
    }

    {
      const auto[nodeDiscovery1, nodeDiscovery2, edgeDiscovery1, edgeDiscovery2]{traverse_graph<Traverser>(g, ignore_disconnected_t{})};

      check(equivalence, report_line(make_message("Two nodes; one to discover")), nodeDiscovery1, node_order{0});
      check(equivalence, report_line(make_message("Two nodes; one to discover")), nodeDiscovery2, node_order{0});
      check(equivalence, report_line(make_message("No edges to discover")), edgeDiscovery1, edge_order{});
      if constexpr(undirected && !isDFS)
      {
        check(equivalence, report_line(make_message("No edges to discover")), edgeDiscovery2, edge_order{});
      }
    }

    check(equality, report_line(make_message("Third node added")), g.add_node(), 2_sz);
    g.join(0, 1);
    g.join(1, 2);
    // 0----0----0

    {
      const auto[nodeDiscovery1, nodeDiscovery2, edgeDiscovery1, edgeDiscovery2]{traverse_graph<Traverser>(g, find_disconnected_t{})};

      check(equivalence, report_line(make_message("Three nodes to discover")), nodeDiscovery1, node_order{0, 1, 2});
      check(equivalence, report_line(make_message("Three nodes to discover")), nodeDiscovery2, isDFS ? node_order{2, 1, 0} : node_order{0, 1, 2});
      check(equivalence, report_line(make_message("Two edges to discover")), edgeDiscovery1, edge_order{{0,0}, {1, mutualInfo && forwardIter ? 1 : 0}});
      if constexpr(undirected && !isDFS)
      {
        check(equivalence, report_line(make_message("Two edges to discover")), edgeDiscovery2, isBFS  ? edge_order{{1,0}, {2,0}} : edge_order{{1, 1}, {2, 0}});
      }
    }

    // 0----0----0

    {
      const auto[nodeDiscovery1, nodeDiscovery2, edgeDiscovery1, edgeDiscovery2]{traverse_graph<Traverser>(g, ignore_disconnected_t{1})};

      if constexpr(undirected)
      {
        check(equivalence, report_line(make_message("Three nodes to discover")), nodeDiscovery1, node_order{1, 0, 2});
        check(equivalence, report_line(make_message("Three nodes to discover")), nodeDiscovery2, isDFS ? node_order{0, 2, 1} : node_order{1, 0, 2});
        check(equivalence, report_line(make_message("Two edges to discover")), edgeDiscovery1, edge_order{{1,0}, {1, 1}});
        if constexpr(!isDFS)
        {
          check(equivalence, report_line(make_message("Two edges to discover")), edgeDiscovery2, edge_order{{0, 0}, {2, 0}});
        }
      }
      else
      {
        check(equivalence, report_line(make_message("Two nodes to discover")), nodeDiscovery1, node_order{1, 2});
        check(equivalence, report_line(make_message("Two nodes to discover")), nodeDiscovery2, isDFS ? node_order{2, 1} : node_order{1, 2});
        check(equivalence, report_line(make_message("One edge to discover")), edgeDiscovery1, edge_order{{1, mutualInfo && forwardIter ? 1 : 0}});
      }
    }

    // 0----0----0

    {
      const auto[nodeDiscovery1, nodeDiscovery2, edgeDiscovery1, edgeDiscovery2]{traverse_graph<Traverser>(g, ignore_disconnected_t{2})};

      if constexpr(undirected)
      {
        check(equivalence, report_line(make_message("Three nodes to discover")), nodeDiscovery1, node_order{2, 1, 0});
        check(equivalence, report_line(make_message("Three nodes to discover")), nodeDiscovery2, isDFS ? node_order{0, 1, 2} : node_order{2, 1, 0});
        check(equivalence, report_line(make_message("Two edges to discover")), edgeDiscovery1, edge_order{{2,0}, {1, forwardIter ? 0 : 1}});
        if constexpr(!isDFS)
        {
          check(equivalence, report_line(make_message("Two edges to discover")), edgeDiscovery2,
            isBFS ? edge_order{{1, 1}, {0, 0}} : edge_order{{1, 0}, {0, 0}});
        }
      }
      else
      {
        check(equivalence, report_line(make_message("One node to discover")), nodeDiscovery1, node_order{2});
        check(equivalence, report_line(make_message("One node to discover")), nodeDiscovery2, node_order{2});
        check(equivalence, report_line(make_message("No edges to discover")), edgeDiscovery1, edge_order{});
      }
    }

    check(equality, report_line(make_message("Fourth node added")), g.add_node(), 3_sz);
    g.join(2, 3);
    g.join(3, 0);
    //  0----0
    //  |    |
    //  |    |
    //  0----0

    test_square_graph(g, 0, make_message, TraversalType{});
    test_square_graph(g, 2, make_message, TraversalType{});
  }

  template<maths::dynamic_network G, class MessageMaker>
  void test_graph_traversals::test_square_graph(const G& g, const std::size_t start, MessageMaker messageMaker, bfs_type)
  {
    using traverser_t = Traverser<bfs_type::value>;
    const auto[nodeDiscovery1, nodeDiscovery2, edgeDiscovery1, edgeDiscovery2]{traverse_graph<traverser_t>(g, maths::ignore_disconnected_t{start})};

    using edge_order = typename decltype(edgeDiscovery1)::result_type;
    std::vector<std::size_t> nodeAnswers;
    edge_order edgeAnswers;
    if constexpr(maths::undirected(G::flavour))
    {
      edge_order edgeAnswers2;
      if(start == 0)
      {
        nodeAnswers = std::vector<std::size_t>{0,1,3,2};
        edgeAnswers = edge_order{{0, 0}, {0, 1}, {1, 1}, {3, 0}};
        edgeAnswers2 = edge_order{{1, 0}, {3, 1}, {2, 0}, {2, 1}};
      }
      else if(start == 2)
      {
        nodeAnswers = std::vector<std::size_t>{2,1,3,0};
        edgeAnswers = edge_order{{2, 0}, {2, 1}, {1, 0}, {3, 1}};
        edgeAnswers2 = edge_order{{1, 1}, {3, 0}, {0, 0}, {0, 1}};
      }

      check(equivalence, report_line(messageMaker("Second edge traversal, start = " + std::to_string(start) + " ")), edgeDiscovery2, edgeAnswers2);
    }
    else
    {
      constexpr auto mutualInfo{!is_directed(G::flavour)};
      if(start == 0)
      {
        nodeAnswers = std::vector<std::size_t>{0,1,2,3};
        edgeAnswers = mutualInfo ? edge_order{{0, 0}, {1, 1}, {2, 1}, {3, 1}} : edge_order{{0, 0}, {1, 0}, {2, 0}, {3, 0}};
      }
      else if(start == 2)
      {
        nodeAnswers = std::vector<std::size_t>{2,3,0,1};
        edgeAnswers = mutualInfo ? edge_order{{2, 1}, {3, 1}, {0, 0}, {1, 1}} : edge_order{{2, 0}, {3, 0}, {0, 0}, {1, 0}};
      }
    }

    check(equivalence, report_line(messageMaker("start = " + std::to_string(start) + " ")), nodeDiscovery1, nodeAnswers);
    check(equivalence, report_line(messageMaker("start = " + std::to_string(start) + " ")), nodeDiscovery2, nodeAnswers);
    check(equivalence, report_line(messageMaker("First edge traversal, start = " + std::to_string(start) + " ")), edgeDiscovery1, edgeAnswers);
  }

  template<maths::dynamic_network G, class MessageMaker>
  void test_graph_traversals::test_square_graph(const G& g, const std::size_t start, MessageMaker messageMaker, dfs_type)
  {
    using traverser_t = Traverser<dfs_type::value>;
    const auto [nodeDiscovery1, nodeDiscovery2, edgeDiscovery1, edgeDiscovery2] {traverse_graph<traverser_t>(g, maths::ignore_disconnected_t{start})};

    using edge_order = typename decltype(edgeDiscovery1)::result_type;
    std::vector<std::size_t> nodeAnswers, nodeAnswers2;
    edge_order edgeAnswers;
    if constexpr(maths::undirected(G::flavour))
    {
      if(start == 0)
      {
        nodeAnswers = std::vector<std::size_t>{0,1,2,3};
        nodeAnswers2 = std::vector<std::size_t>{3,2,1,0};
        edgeAnswers = edge_order{{0, 0}, {1, 1}, {2, 1}};
      }
      else if(start == 2)
      {
        nodeAnswers = std::vector<std::size_t>{2,1,0,3};
        nodeAnswers2 = std::vector<std::size_t>{3,0,1,2};
        edgeAnswers = edge_order{{2, 0}, {1, 0}, {0, 1}};
      }
    }
    else
    {
      constexpr auto mutualInfo{!is_directed(G::flavour)};
      if(start == 0)
      {
        nodeAnswers = std::vector<std::size_t>{0,1,2,3};
        nodeAnswers2 = std::vector<std::size_t>{3,2,1,0};
        edgeAnswers = mutualInfo ? edge_order{{0, 0}, {1, 1}, {2, 1}} : edge_order{{0, 0}, {1, 0}, {2, 0}};
      }
      else if(start == 2)
      {
        nodeAnswers = std::vector<std::size_t>{2,3,0,1};
        nodeAnswers2 = std::vector<std::size_t>{1,0,3,2};
        edgeAnswers = mutualInfo ? edge_order{{2, 1}, {3, 1}, {0, 0}} : edge_order{{2, 0}, {3, 0}, {0, 0}};
      }
    }

    check(equivalence, report_line(messageMaker("start = " + std::to_string(start) + " ")), nodeDiscovery1, nodeAnswers);
    check(equivalence, report_line(messageMaker("start = " + std::to_string(start) + " ")), nodeDiscovery2, nodeAnswers2);
    check(equivalence, report_line(messageMaker("Edge traversal to undiscovered node, start = " + std::to_string(start) + " ")), edgeDiscovery1, edgeAnswers);
  }

  template<maths::dynamic_network G, class MessageMaker>
  void test_graph_traversals::test_square_graph(const G& g, const std::size_t start, MessageMaker messageMaker, pdfs_type)
  {
    using traverser_t = Traverser<pdfs_type::value>;
    const auto[nodeDiscovery1, nodeDiscovery2, edgeDiscovery1, edgeDiscovery2]{traverse_graph<traverser_t>(g, maths::ignore_disconnected_t{start})};

    using edge_order = typename decltype(edgeDiscovery1)::result_type;
    std::vector<std::size_t> nodeAnswers;
    edge_order edgeAnswers;
    if constexpr(maths::undirected(G::flavour))
    {
      edge_order edgeAnswers2;
      if(start == 0)
      {
        nodeAnswers = std::vector<std::size_t>{0,1,2,3};
        edgeAnswers = edge_order{{0, 0}, {0, 1}, {1, 0}, {2, 0}};
        edgeAnswers2 = edge_order{{1, 1}, {2, 1}, {3, 0}, {3, 1}};
      }
      else if(start == 2)
      {
        nodeAnswers = std::vector<std::size_t>{2,1,0,3};
        edgeAnswers = edge_order{{2, 0}, {2, 1}, {1, 1}, {0, 0}};
        edgeAnswers2 = edge_order{{1, 0}, {0, 1}, {3, 0}, {3, 1}};
      }

      check(equivalence, report_line(messageMaker("Second edge traversal, start = " + std::to_string(start) + " ")), edgeDiscovery2, edgeAnswers2);
    }
    else
    {
      constexpr auto mutualInfo{!is_directed(G::flavour)};
      if(start == 0)
      {
        nodeAnswers = std::vector<std::size_t>{0,1,2,3};
        edgeAnswers = mutualInfo ? edge_order{{0, 1}, {1, 0}, {2, 0}, {3, 0}} : edge_order{{0, 0}, {1, 0}, {2, 0}, {3, 0}};
      }
      else if(start == 2)
      {
        nodeAnswers = std::vector<std::size_t>{2,3,0,1};
        edgeAnswers = mutualInfo ? edge_order{{2, 0}, {3, 0}, {0, 1}, {1, 0}} : edge_order{{2, 0}, {3, 0}, {0, 0}, {1, 0}};
      }
    }

    check(equivalence, report_line(messageMaker("start = " + std::to_string(start) + " ")), nodeDiscovery1, nodeAnswers);
    check(equivalence, report_line(messageMaker("start = " + std::to_string(start) + " ")), nodeDiscovery2, nodeAnswers);
    check(equivalence, report_line(messageMaker("First edge traversal, start = " + std::to_string(start) + " ")), edgeDiscovery1, edgeAnswers);
  }

  //=============================== Priority Search  ===============================//

  template<maths::dynamic_network Graph>
  void test_graph_traversals::test_priority_traversal()
  {
    auto graph{generate_priority_test_graph<Graph>()};

    node_tracker tracker;
    traverse(maths::priority_first, graph, maths::ignore_disconnected_t{}, tracker);

    check(equivalence, report_line(""), tracker, std::vector<std::size_t>{0,2,4,3,6,1,5});
  }

  //=============================== Weighted breadth_first  ===============================//

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

    check(equality, report_line("Null edge first task expected"), serialResults, expected);
    check(equality, report_line("Async edge first task expected"), asyncResults, expected);
    check(equality, report_line("Pool edge first task expected"), poolResults, expected);
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

      check(equality, report_line("Null node task expected"), serialResults, expected);
      check(equality, report_line("Async node task expected"), asyncResults, expected);
      check(equality, report_line("Pool node task expected"), poolResults, expected);
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

      check(equality, report_line("Null edge first task expected"), serialResults, expected);
      check(equality, report_line("Async edge first task expected"), asyncResults, expected);
      check(equality, report_line("Pool edge first task expected"), poolResults, expected);

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

      check_relative_performance(report_line("Null versus async check"), asyncFn, serialFn, 2.0, 5.0);
      check_relative_performance(report_line("Null versus pool check"), poolFn, serialFn, 2.0, 5.0);
    }
  }
}
