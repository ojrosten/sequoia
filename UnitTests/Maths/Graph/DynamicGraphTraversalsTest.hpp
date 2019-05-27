////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "GraphTraversalTestingUtilities.hpp"
#include "DynamicGraphTestingUtilities.hpp"

#include "ConcurrencyModels.hpp"

#include <functional>

namespace sequoia::unit_testing
{
  class NodeTracker
  {
  public:
    void clear() noexcept { m_Order.clear(); }
      
    void operator()(const std::size_t index) { m_Order.push_back(index); }

    const std::vector<std::size_t>& order() const noexcept { return m_Order; }
  private:
    std::vector<std::size_t> m_Order;
  };

  template<class G, class T>
  class EdgeTracker
  {
  public:
    using result_type = std::vector<std::pair<std::size_t, std::size_t>>;
      
    EdgeTracker(const G& graph) : m_Graph(graph) {}
      
    void clear() noexcept { m_Order.clear(); }

    template<class I> void operator()(I iter)
    {
      const auto pos = dist(typename std::is_same<typename T::type, DFS>::type(), iter);
      m_Order.emplace_back(iter.partition_index(), static_cast<std::size_t>(pos));
    }

    const result_type& order() const noexcept { return m_Order; }
  private:    
    result_type m_Order;
    const G& m_Graph;
    
    template<class I> auto dist(std::true_type, I iter)
    {
      return distance(m_Graph.crbegin_edges(iter.partition_index()), iter);
    }

    template<class I> auto dist(std::false_type, I iter)
    {
      return distance(m_Graph.cbegin_edges(iter.partition_index()), iter);
    }
  };

  template<class F> void clear(F& f)
  {
    f.clear();     
  }

  template<> inline void clear<maths::null_functor>(maths::null_functor&) {};
    
  template<class F, class... Fn>
  void clear(F& f, Fn&... fn)
  {
    f.clear();
    clear(fn...);
  }

  class test_graph_traversals : public graph_unit_test
  {
  public:
    using graph_unit_test::graph_unit_test;

  private:      
    struct null_weight {};

    void run_tests();

    void test_PRS_helpers();
  };

    template
    <
      maths::graph_flavour GraphFlavour,
      class EdgeWeight,
      class NodeWeight,      
      template <class> class EdgeWeightPooling,
      template <class> class NodeWeightPooling,
      template <maths::graph_flavour, class, template<class> class> class EdgeStorageTraits,
      template <class, template<class> class, bool> class NodeWeightStorageTraits
    >
    class tracker_test
      : public graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>
    {
    private:
      using Connectivity = typename
        graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>::graph_type;

      using edge_results = std::vector<std::pair<std::size_t, std::size_t>>;

      // TO DO: bespoke check_equality to include demangled traversal tag
      using graph_checker<unit_test_logger<test_mode::standard>>::check_equality;      
      using graph_checker<unit_test_logger<test_mode::standard>>::check_exception_thrown;
      using graph_checker<unit_test_logger<test_mode::standard>>::check;
      
      void execute_operations() override
      {        
        test_tracker_algorithm<Traverser<BFS>>();
        test_tracker_algorithm<Traverser<DFS>>();        
      }

      template<class Traverser, class G, class... Fn>
      void traverse_graph(const G& g, const bool findDisconnected, const std::size_t start, Fn&&... fn)
      {
        clear(std::forward<Fn>(fn)...);
        Traverser::traverse(g, findDisconnected, start, std::forward<Fn>(fn)...);
      }

      template<class Traverser>
      void test_tracker_algorithm();

      // true_types correspond BFS
      template<class NTracker, class ETracker, class ETracker2>
      void test_square_graph(const NTracker& tracker, const ETracker& eTracker, const ETracker2& eTracker2, const std::size_t start, const bool mutualInfo, std::true_type)
      {
        std::vector<std::size_t> answers;
        edge_results edgeAnswers, edgeAnswers2;
        if(start == 0)
        {
          answers = std::vector<std::size_t>{0,1,3,2};
          edgeAnswers = edge_results{{0, 0}, {0, 1}, {1, 1}, {3, 0}};
          edgeAnswers2 = edge_results{{1, 0}, {3, 1}, {2, 0}, {2, 1}};
        }
        else if(start == 2)
        {
          answers = std::vector<std::size_t>{2,1,3,0};
          edgeAnswers = edge_results{{2, 0}, {2, 1}, {1, 0}, {3, 1}};
          edgeAnswers2 = edge_results{{1, 1}, {3, 0}, {0, 0}, {0, 1}};
        }
        check_equality(answers, tracker.order(), LINE(""));
        check_equality(edgeAnswers, eTracker.order(), LINE("First edge traversal"));
        check_equality(edgeAnswers2, eTracker2.order(), LINE("Second edge traversal"));
      }

      template<class NTracker, class ETracker>
      void test_square_graph(const NTracker& tracker, const ETracker& eTracker, const std::size_t start, const bool mutualInfo, std::true_type)
      {
        std::vector<std::size_t> answers;
        edge_results edgeAnswers;
        if(start == 0)
        {
          answers = std::vector<std::size_t>{0,1,2,3};
          edgeAnswers = mutualInfo ? edge_results{{0, 0}, {1, 1}, {2, 1}, {3, 1}} : edge_results{{0, 0}, {1, 0}, {2, 0}, {3, 0}};
        }
        else if(start ==2)
        {
          answers = std::vector<std::size_t>{2,3,0,1};
          edgeAnswers = mutualInfo ? edge_results{{2, 1}, {3, 1}, {0, 0}, {1, 1}} : edge_results{{2, 0}, {3, 0}, {0, 0}, {1, 0}};
        }
        check_equality(answers, tracker.order(), LINE("start = " + std::to_string(start) + " "));
        check_equality(edgeAnswers, eTracker.order(), LINE("First edge traversal, start = " + std::to_string(start) + " "));
      }

      template<class NTracker, class ETracker, class ETracker2>
      void test_square_graph(const NTracker& tracker, const ETracker& eTracker, const ETracker2& eTracker2, const std::size_t start, const bool mutualInfo, std::false_type)
      {
        std::vector<std::size_t> answers;
        edge_results edgeAnswers, edgeAnswers2;
        if(start == 0)
        {
          answers = std::vector<std::size_t>{0,1,2,3};
          edgeAnswers = edge_results{{0, 0}, {0, 1}, {1, 0}, {2, 0}};
          edgeAnswers2 = edge_results{{1, 1}, {2, 1}, {3, 0}, {3, 1}};
        }
        else if(start ==2)
        {
          answers = std::vector<std::size_t>{2,1,0,3};
          edgeAnswers = edge_results{{2, 0}, {2, 1}, {1, 1}, {0, 0}};
          edgeAnswers2 = edge_results{{1, 0}, {0, 1}, {3, 0}, {3, 1}};
        }
        check_equality(answers, tracker.order(), LINE(""));
        check_equality(edgeAnswers, eTracker.order(), LINE("First edge traversal"));
        check_equality(edgeAnswers2, eTracker2.order(), LINE("Second edge traversal"));
      }

      template<class NTracker, class ETracker>
      void test_square_graph(const NTracker& tracker, const ETracker& eTracker, const std::size_t start, const bool mutualInfo, std::false_type)
      {
        std::vector<std::size_t> answers;
        edge_results edgeAnswers;
        if(start == 0)
        {
          answers = std::vector<std::size_t>{0,1,2,3};
          edgeAnswers = mutualInfo ? edge_results{{0, 1}, {1, 0}, {2, 0}, {3, 0}} : edge_results{{0, 0}, {1, 0}, {2, 0}, {3, 0}};
        }
        else if(start ==2)
        {
          answers = std::vector<std::size_t>{2,3,0,1};
          edgeAnswers = mutualInfo ? edge_results{{2, 0}, {3, 0}, {0, 1}, {1, 0}} : edge_results{{2, 0}, {3, 0}, {0, 0}, {1, 0}};
        }
        check_equality(answers, tracker.order(), LINE("start = " + std::to_string(start) + " "));
        check_equality(edgeAnswers, eTracker.order(), LINE("First edge traversal, start = " + std::to_string(start) + " "));
      }
    };

    //=======================================================================================//
    // Graph (tree) created and searched using various traversal methods
    
    template
    <
      maths::graph_flavour GraphFlavour,
      class EdgeWeight,
      class NodeWeight,      
      template <class> class EdgeWeightPooling,
      template <class> class NodeWeightPooling,
      template <maths::graph_flavour, class, template<class> class> class EdgeStorageTraits,
      template <class, template<class> class, bool> class NodeWeightStorageTraits
    >
    class test_priority_traversal :
      public graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>
    {
    private:
      using UndirectedType = std::bool_constant<maths::undirected(GraphFlavour)>;
      using GGraph = typename
        graph_operations
        <
          GraphFlavour,
          EdgeWeight,
          NodeWeight,
          EdgeWeightPooling,
          NodeWeightPooling,
          EdgeStorageTraits,
          NodeWeightStorageTraits
        >::graph_type;

      using graph_checker<unit_test_logger<test_mode::standard>>::check_equality;      
      using graph_checker<unit_test_logger<test_mode::standard>>::check_exception_thrown;
      using graph_checker<unit_test_logger<test_mode::standard>>::check;
      
      void execute_operations() override
      {
        testPRS();
      }
      
      void testPRS()
      {
        GGraph graph{generate_test_graph()};

        NodeTracker tracker;
        maths::priority_search(graph, false, 0, tracker);

        auto order = tracker.order();
        check_equality(std::vector<std::size_t>{0,2,4,3,6,1,5}, order, LINE(""));
      }
      
      GGraph generate_test_graph()
      {
        GGraph graph;
        
        graph.add_node(1);
        graph.add_node(5);
        graph.add_node(10);
        graph.add_node(7);
        graph.add_node(8);
        graph.add_node(4);
        graph.add_node(6);

        graph.join(0,1);
        graph.join(0,2);
        graph.join(0,3);
        graph.join(0,4);
        graph.join(3,5);
        graph.join(3,6);

        //     1
        //     |
        // --------
        // |  | | |
        // 5 10 7 8
        //     /\
        //    4  6

        return graph;
      }
    };
    
    
    //=======================================================================================//
    // Graph (tree) created and searched using bfs; at each vertex,
    // a task is performed 

    template
    <
      maths::graph_flavour GraphFlavour,
      class EdgeWeight,
      class NodeWeight,      
      template <class> class EdgeWeightPooling,
      template <class> class NodeWeightPooling,
      template <maths::graph_flavour, class, template<class> class> class EdgeStorageTraits,
      template <class, template<class> class, bool> class NodeWeightStorageTraits
    >
    class test_weighted_BFS_tasks
      : public graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>
    {
    private:
      using UndirectedType = std::bool_constant<maths::undirected(GraphFlavour)>;
      using GGraph = typename
        graph_operations
        <
          GraphFlavour,
          EdgeWeight,
          NodeWeight,
          EdgeWeightPooling,
          NodeWeightPooling,
          EdgeStorageTraits,
          NodeWeightStorageTraits
        >::graph_type;

      using graph_checker<unit_test_logger<test_mode::standard>>::check_equality;      
      using graph_checker<unit_test_logger<test_mode::standard>>::check_relative_performance;
      using graph_checker<unit_test_logger<test_mode::standard>>::check;
      
      void execute_operations() override
      {
        testNodeAndFirstEdgeTraversal();
        testEdgeSecondTraversal(UndirectedType());
      }      
      
      void testEdgeSecondTraversal(std::false_type)
      {
        // Do nothing: there is no second edge traversal
        // for directed graphs!
      }

      void testEdgeSecondTraversal(std::true_type)
      {
        GGraph graph{generate_test_graph()};

        {
          int upper = 6;

          auto nullFn  = [&graph, upper](){ return ESTTask<concurrency::serial<int>>(graph, upper); };
          auto asyncFn = [&graph, upper](){ return ESTTask<concurrency::asynchronous<int>>(graph, upper); };
          auto poolFn  = [&graph, upper](){ return ESTTask<concurrency::thread_pool<int>>(graph, upper, 4u); };

          std::vector<int> answers0 = nullFn(),
                           answers1 = asyncFn(),
                           answers2 = poolFn();

          check(answers0 == answers1, "Null threading and asyncronous processing the same");
          check(answers0 == answers2, "Null threading and pool processing the same");

          auto answers = edge_second_task_answers(upper);

          check_equality(answers, answers0, "Null edge first task answers");
          check_equality(answers, answers1, "Async edge first task answers");
          check_equality(answers, answers2, "Pool edge first task answers");
        }
      }

      void testNodeAndFirstEdgeTraversal()
      {
        GGraph graph{generate_test_graph()};

        //================================ Node functors =========================//

        int upper = 10;
        for(int i=0; i < 2; ++i)
        {
          const bool early{i == 0};
          using microseconds = std::chrono::microseconds;
          auto nullFn  = [&graph, upper, early](){ return task<concurrency::serial<int>>(graph, upper, early, microseconds{}); };
          auto asyncFn = [&graph, upper, early](){ return task<concurrency::asynchronous<int>>(graph, upper, early, microseconds{}); };
          auto poolFn  = [&graph, upper, early](){ return task<concurrency::thread_pool<int>>(graph, upper, early, microseconds{}, 4u); };
          std::vector<int>
            answers0 = nullFn(),
            answers1 = asyncFn(),
            answers2 = poolFn();

          check(answers0 == answers1, "Null threading and asyncronous processing the same");
          check(answers0 == answers2, "Null threading and thread pool processing the same");

          auto answers = node_task_answers(upper);

          check_equality(answers, answers0, "Null node task answers");
          check_equality(answers, answers1, "Async node task answers");
          check_equality(answers, answers2, "Pool node task answers");
        }

        //================================ Edge First Traversal functors =========================//

        {

          upper = 6;
          
          auto nullFn  = [&graph, upper](){ return EFTTask<concurrency::serial<int>>(graph, upper); };
          auto asyncFn = [&graph, upper](){ return EFTTask<concurrency::asynchronous<int>>(graph, upper); };
          auto poolFn  = [&graph, upper](){ return EFTTask<concurrency::thread_pool<int>>(graph, upper, 4u); };

          std::vector<int> answers0 = nullFn(),
                           answers1 = asyncFn(),
                           answers2 = poolFn();

          check(answers0 == answers1, "Null threading and asyncronous processing the same");
          check(answers0 == answers2, "Null threading and pool processing the same");

          auto answers = edge_task_answers(upper);

          check_equality(answers, answers0, "Null edge first task answers");
          check_equality(answers, answers1, "Async edge first task answers");
          check_equality(answers, answers2, "Pool edge first task answers");

        }


        //================================ Now check performance =========================//

        {
          upper = 10;

          const auto pause{std::chrono::microseconds(500)};
          
          auto nullFn{
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

          auto answers = node_task_answers(upper);

          auto futures = check_relative_performance(asyncFn, nullFn, 2.0, 4.5, LINE("Null versus async check"));
          auto futures2 = check_relative_performance(poolFn, nullFn, 2.0, 4.5, LINE("Null versus pool check"));

          for(auto&& fut : futures.fast_futures)
          {
            auto results = fut.get();
            check_equality(answers, results, "Async node performance task results wrong");
          }

          for(auto&& fut : futures.slow_futures)
          {
            auto results = fut.get();
            check_equality(answers, results, "Null node performance task results wrong");
          }

          for(auto&& fut : futures2.fast_futures)
          {
            auto results = fut.get();
            check_equality(answers, results, "Pool node performance task results wrong");
          }
        }
      }

      template<class ProcessingModel, class... Args>
      static std::vector<int> task(GGraph& graph, const int upper, const bool early, const std::chrono::microseconds pause, Args&&... args)
      {

        auto fn = [upper, pause](const std::size_t index) {
          std::this_thread::sleep_for(pause);
          const auto n{upper + static_cast<int>(index)};
          return n * (n + 1) /2;
        };

        using maths::null_functor;
        if constexpr(GGraph::directedness == maths::directed_flavour::directed)
        {
          return early ? maths::breadth_first_search(graph, false, 0, fn, null_functor{}, null_functor{}, ProcessingModel{std::forward<Args>(args)...})
                       : maths::breadth_first_search(graph, false, 0, null_functor{}, fn, null_functor{}, ProcessingModel{std::forward<Args>(args)...});
        }
        else
        {
          return early ? maths::breadth_first_search(graph, false, 0, fn, null_functor{}, null_functor{}, null_functor{}, ProcessingModel{std::forward<Args>(args)...})
                       : maths::breadth_first_search(graph, false, 0, null_functor{}, fn, null_functor{}, null_functor{}, ProcessingModel{std::forward<Args>(args)...});
        }
        
      }

      template<class ProcessingModel, class... Args>
      static std::vector<int> EFTTask(GGraph& graph, const int upper, Args&&... args)
      {
        auto fn = [upper](auto edgeIter) {
          const auto n{upper - static_cast<int>(edgeIter.partition_index())};
          return n*(n+1)/2;
        };

        using maths::null_functor;
        if constexpr(GGraph::directedness == maths::directed_flavour::directed)
        {
          return maths::breadth_first_search(graph, false, 0, null_functor{}, null_functor{}, fn, ProcessingModel{std::forward<Args>(args)...});
        }
        else
        {
          return maths::breadth_first_search(graph, false, 0, null_functor{}, null_functor{}, fn, null_functor{}, ProcessingModel{std::forward<Args>(args)...});
        }
      }

      template<class ProcessingModel, class... Args>
      static std::vector<int> ESTTask(GGraph& graph, const int upper, Args&&... args)
      {
        auto fn = [upper](auto edgeIter) {
          const auto n{upper - static_cast<int>(edgeIter.partition_index())};
          return n*(n+1)/2;
        };

        using maths::null_functor;
        return maths::breadth_first_search(graph, false, 0, null_functor{}, null_functor{}, null_functor{}, fn, ProcessingModel{std::forward<Args>(args)...});
      }

      static std::pair<std::size_t, std::size_t> add_daughters(GGraph& graph, const std::size_t index, std::pair<int, int> values)
      {
        const std::size_t index0{graph.add_node(values.first)};
        const std::size_t index1{graph.add_node(values.second)};

        graph.join(index, index0);
        graph.join(index, index1);

        return std::make_pair(index0, index1);
      }

      static std::vector<int> node_task_answers(const int upper)
      {
        std::vector<int> answers;
        int shift = 0;
        for(int i=0; i < 5; ++i)
        {
          answers.push_back((upper + shift)*(upper + shift + 1) / 2);
          ++shift;
        }

        return answers;
      }

      static std::vector<int> edge_task_answers(const int upper)
      {
        std::vector<int> answers;

        int shift = 0;
        for(int i=0; i < 4; ++i)
        {
          if(i == 2) --shift;
          answers.push_back((upper + shift)*(upper + shift + 1) / 2);
        }

        return answers;
      }

      static std::vector<int> edge_second_task_answers(const int upper)
      {
        std::vector<int> answers;

        for(int i=0; i < 4; ++i)
        {
          const int shift = -i - 1;
          answers.push_back((upper + shift)*(upper + shift + 1) / 2);
        }

        return answers;
      }

      static GGraph generate_test_graph()
      {
        GGraph graph;

        std::size_t depth{0};
        int value = 1;
        std::queue<std::size_t> parents;
        parents.push(graph.add_node(value));

        //    0
        //   / \
        //  0   0
        //  /\
        // 0  0
        while(depth < 2)
        {
          ++value;
          const auto node = parents.front();
          auto daughters = add_daughters(graph, node, std::make_pair(value, value));

          parents.pop();
          parents.push(daughters.first);
          parents.push(daughters.second);
          ++depth;
        }

        return graph;
      }
    };
  }
