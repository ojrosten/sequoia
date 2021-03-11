////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "GraphTraversalTestingUtilities.hpp"
#include "DynamicGraphPerformanceTestingUtilities.hpp"

#include "sequoia/Core/Concurrency/ConcurrencyModels.hpp"

#include <functional>

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

  class test_graph_traversals final : public graph_unit_test
  {
  public:
    using graph_unit_test::graph_unit_test;
    
    [[nodiscard]]
    std::string_view source_file() const noexcept final;
  private:      
    struct null_weight {};

    void run_tests() final;

    void test_prs_details();
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
  class tracker_test
    : public graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>
  {
  private:
    using base_t = graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>;
    
    using graph_t   = typename base_t::graph_type;
    using checker_t = typename base_t::checker_type;

    using edge_results = std::vector<std::pair<std::size_t, std::size_t>>;

    using checker_t::check_equality;      
    using checker_t::check_exception_thrown;
    using checker_t::check;
      
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
    void test_square_graph(const NTracker& tracker, const ETracker& eTracker, const ETracker2& eTracker2, const std::size_t start, const bool, std::true_type);

    template<class NTracker, class ETracker>
    void test_square_graph(const NTracker& tracker, const ETracker& eTracker, const std::size_t start, const bool mutualInfo, std::true_type);

    template<class NTracker, class ETracker, class ETracker2>
    void test_square_graph(const NTracker& tracker, const ETracker& eTracker, const ETracker2& eTracker2, const std::size_t start, const bool, std::false_type);

    template<class NTracker, class ETracker>
    void test_square_graph(const NTracker& tracker, const ETracker& eTracker, const std::size_t start, const bool mutualInfo, std::false_type);
  };

    //=======================================================================================//
    
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
    class test_priority_traversal :
      public graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>
    {
    private:
      using UndirectedType = std::bool_constant<maths::undirected(GraphFlavour)>;

      using base_t = graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>;
      
      using graph_t = typename base_t::graph_type;
      using checker_t = typename base_t::checker_type;

      using checker_t::check_equality;      
      using checker_t::check_exception_thrown;
      using checker_t::check;
      
      void execute_operations() override
      {
        test_prs();
      }
      
      void test_prs();
      
      auto generate_test_graph() -> graph_t;
    };
    
    
    //=======================================================================================//

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
    class test_weighted_BFS_tasks
      : public graph_performance_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>
    {
    private:
      using UndirectedType = std::bool_constant<maths::undirected(GraphFlavour)>;

      using base_t = graph_performance_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>;

      using graph_t   = typename base_t::graph_type;
      using checker_t = typename base_t::checker_type;

      using checker_t::check_equality;      
      using checker_t::check_relative_performance;
      using checker_t::check;
      
      void execute_operations() override
      {
        test_node_and_first_edge_traversal();
        test_edge_second_traversal(UndirectedType());
      }      
      
      void test_edge_second_traversal(std::false_type) {}

      void test_edge_second_traversal(std::true_type);

      void test_node_and_first_edge_traversal();

      template<class ProcessingModel, class... Args>
      static std::vector<int> task(graph_t& graph, const int upper, const bool early, const std::chrono::microseconds pause, Args&&... args);

      template<class ProcessingModel, class... Args>
      static std::vector<int> edge_first_traversal_task(graph_t& graph, const int upper, Args&&... args);      

      template<class ProcessingModel, class... Args>
      static std::vector<int> edge_second_traversal_task(graph_t& graph, const int upper, Args&&... args);

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

      static auto generate_test_graph() -> graph_t;
    };
  }
