#pragma once

#include "UnitTestUtils.hpp"

#include "TestPartitionedDataHelper.hpp"
#include "TestEdgeHelper.hpp"
#include "ProtectiveWrapper.hpp"
#include "PartitionedData.hpp"
#include "DataPool.hpp"
#include "Graph.hpp"
#include "GraphAlgorithms.hpp"

namespace sequoia
{
  namespace unit_testing
  {
    struct null_weight{};

    template<class G, class = std::void_t<>>
    struct is_static_graph : std::true_type
    {};

    template<class G>
    struct is_static_graph<G, std::void_t<decltype(std::declval<G>().add_node())>> : std::false_type
    {};

    template<class G>
    constexpr bool is_static_graph_v{is_static_graph<G>::value};
    
    struct BFS {};
    struct DFS {};
    struct PRS {};

    template<class SearchType=BFS> struct Traverser
    {
      using type = BFS;

      template<class G, class... Fn>
      static void traverse(const G& g, const bool findDisconnected, const std::size_t start, Fn&&... fn)
      {
        maths::breadth_first_search(g, findDisconnected, start, std::forward<Fn>(fn)...);
      }

      constexpr static bool uses_forward_iterator() { return true; }

      static std::string iterator_description() { return "forward"; }
    };

    template<> struct Traverser<DFS>
    {
      using type = DFS;

      template<class G, class... Fn>
      static void traverse(const G& g, const bool findDisconnected, const std::size_t start, Fn&&... fn)
      {
        maths::depth_first_search(g, findDisconnected, start, std::forward<Fn>(fn)...);
      }

      constexpr static bool uses_forward_iterator() { return false; }

      static std::string iterator_description() { return "reverse"; }
    };

    template<> struct Traverser<PRS>
    {
      using type = PRS;

      template<class G, class... Fn>
      static void traverse(const G& g, const bool findDisconnected, const std::size_t start, Fn&&... fn)
      {
        maths::priority_search(g, findDisconnected, start, std::forward<Fn>(fn)...);
      }

      constexpr static bool uses_forward_iterator() { return true; }

      static std::string iterator_description() { return "forward"; }
    };

    template<> struct type_to_string<Traverser<BFS>>
    {
      static std::string str() { return "BFS"; }
    };

    template<> struct type_to_string<Traverser<DFS>>
    {
      static std::string str() { return "DFS"; }
    };

    template<> struct type_to_string<Traverser<PRS>>
    {
      static std::string str() { return "PRS"; }
    };

    template<> struct template_class_to_string<data_sharing::unpooled>
    {
      static std::string str() { return "UNPOOLED"; }
    };

    template<> struct template_class_to_string<data_sharing::data_pool>
    {
      static std::string str() { return "DATA POOL"; }
    };

    template<template <class, template<class> class> class T> struct storage_traits_to_string;

    template<> struct storage_traits_to_string<maths::bucketed_edge_storage_traits>
    {
      static std::string str() { return "BUCKETED STORAGE"; }
    };

    template<> struct storage_traits_to_string<maths::contiguous_edge_storage_traits>
    {
      static std::string str() { return "CONTIGUOUS STORAGE"; }
    };

    template<template <class, template<class> class, bool> class T> struct node_weight_storage_traits_to_string;
    
    template<> struct node_weight_storage_traits_to_string<maths::node_weight_storage_traits>
    {
      static std::string str() { return "NODE WEIGHT STORAGE TRAITS"; }
    };

    inline constexpr bool mutual_info(const maths::graph_flavour flavour) noexcept
    {
      return flavour != maths::graph_flavour::directed;
    }
    
    inline std::string to_string(const maths::graph_flavour flavour)
    {
      using graph_flavour = maths::graph_flavour;
      std::string s{};
      switch(flavour)
      {
      case graph_flavour::undirected:
        s = "Undirected";
        break;
      case graph_flavour::undirected_embedded:
        s = "Undirected Embedded";
        break;
      case graph_flavour::directed:
        s = "Directed";
        break;
      case graph_flavour::directed_embedded:
        s= "Directed Embedded";
        break;
      }
      return s;
    }
    
    template<class Edge> constexpr std::size_t get_target_for_partial(const Edge& edge, const std::size_t node)
    {
      const bool isNode{(edge.host_node() != node) && (edge.target_node() != node)};
      const std::size_t target{isNode ? node : (edge.host_node() == node ? edge.target_node() : edge.host_node())};
      return target;
    }

    inline constexpr bool embedded(const maths::graph_flavour graphFlavour)
    {
      using gf = maths::graph_flavour;
      return (graphFlavour == gf::directed_embedded) || (graphFlavour == gf::undirected_embedded);
    }

        template<class G>
    std::size_t local_edge_index(const G& graph, const std::size_t node1, const std::size_t node2, const std::size_t n)
    {
      if(node1 >= graph.order() || node2 >= graph.order())
        throw std::out_of_range("graph_utilities::local_edge_index - nodex index out of range");

      std::size_t counter = 0;
      auto found = graph.cend_edges(node1);
      for(auto citer = graph.cbegin_edges(node1); citer != graph.cend_edges(node1); ++citer)
      {
        if(citer->target_node() == node2)
        {
          if(counter == n)
          {
            found = citer;
            break;
          }
          else
          {
            ++counter;
          }
        }
      }

      if(found == graph.cend_edges(node1))
        throw std::out_of_range("Graph::find_nth_connection - nth connection out of range");

      return static_cast<std::size_t>(distance(graph.cbegin_edges(node1), found));
    }
    
    template<class G>
    const auto& get_edge(const G& graph, const std::size_t node1, const std::size_t node2, const std::size_t nthConnection)
    {
      const std::size_t pos{local_edge_index(graph, node1, node2, nthConnection)};
      if(pos == (G::npos)) throw std::out_of_range("graph_utilities::get_edge - index out of range!\n");

      auto iter = graph.cbegin_edges(node1);
      return *(iter + pos);
    }
    
    template
    <
      maths::graph_flavour GraphFlavour,      
      class EdgeWeight,
      class NodeWeight,      
      template <class> class EdgeWeightPooling,
      template <class> class NodeWeightStorage,
      template<class, template<class> class> class EdgeStorageTraits,
      template<class, template<class> class, bool> class NodeWeightStorageTraits,
      bool=embedded(GraphFlavour)
    >
    struct graph_type_generator
    {
      using graph_type = maths::graph<maths::to_directedness(GraphFlavour), EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightStorage, EdgeStorageTraits, NodeWeightStorageTraits>;
    };

    template
    <
      maths::graph_flavour GraphFlavour,     
      class EdgeWeight,
      class NodeWeight,      
      template <class> class EdgeWeightPooling,
      template <class> class NodeWeightPooling,
      template<class, template<class> class> class EdgeStorageTraits,
      template<class, template<class> class, bool> class NodeWeightStorageTraits
    >
    struct graph_type_generator<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits, true>
    {
      using graph_type = maths::embedded_graph<maths::to_directedness(GraphFlavour), EdgeWeight, NodeWeight, NodeWeightPooling, EdgeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>;
    };

    template<class Logger>
    class graph_checker : protected checker<Logger>
    {
    private:
      using checker<Logger>::failures;
      using checker<Logger>::post_message;

    public:      
      using checker<Logger>::check_equality;
      using checker<Logger>::check;
      using checker<Logger>::check_exception_thrown;
      
      template<class G, class E=typename G::edge_init_type> bool
      check_graph(const G& graph, const std::initializer_list<std::initializer_list<E>>& edges, const std::initializer_list<typename G::node_weight_type>& nodeWeights, const std::string& failureMessage="")
      {
        using Edge = typename G::edge_type;
        using RefEdge = E;
        constexpr bool nullNodeWeight{std::is_empty_v<typename G::node_weight_type>};
        constexpr bool nullEdgeWeight{std::is_empty_v<typename G::edge_weight_type>};

        auto r{checker<Logger>::make_sentinel(failureMessage)};
        
        const auto numFailures = failures();
        const auto numNodes = edges.size();
        double nEdges{};
        if(check_equality(numNodes, graph.order(), "Graph order wrong"))
        {
          if(!nullNodeWeight && check_equality(nodeWeights.size(), numNodes, "Number of node weights wrong"))
          {
            check_equality(nodeWeights.size(), numNodes, "Number of node weights wrong");
          }

          for(auto edgesIter{edges.begin()}; edgesIter != edges.end(); ++edgesIter)
          {
            const auto i{std::distance(edges.begin(), edgesIter)};
            const std::string partStr{std::to_string(i)};

            const std::string nodeMessage{"Node weight wrong for node " + partStr};
            if constexpr(!nullNodeWeight)
            {
              if(i < nodeWeights.size())
                check_equality(*(nodeWeights.begin() + i), *(graph.cbegin_node_weights()+i), nodeMessage);
            }

            auto beginEdges = graph.cbegin_edges(i);
            auto endEdges = graph.cend_edges(i);
            auto rbeginEdges = graph.crbegin_edges(i);
            auto rendEdges = graph.crend_edges(i);

            const auto nNodeEdges{static_cast<std::size_t>(distance(beginEdges, endEdges))};
            const auto nrNodeEdges{static_cast<std::size_t>(distance(rbeginEdges, rendEdges))};

            const auto nodeEdges{*edgesIter};
            if(check_equality(nodeEdges.size(), nNodeEdges, "Number of edges for partition " + partStr + " wrong")
               && check_equality(nodeEdges.size(), nrNodeEdges, "Number of edges for reversed partition " + partStr + " wrong")
               )
            {
              auto ansIter = nodeEdges.begin();
              auto redge = rendEdges;
              for(auto edge = beginEdges; edge != endEdges; ++edge, ++ansIter)
              {
                --redge;
                check(*edge == *redge, "Disagreement between forward and reverse itereators");
                auto dist = distance(beginEdges, edge);
                const std::string message{" wrong for partition " + partStr + ", edge " + to_string(dist)};
            
                const auto target = [edge=*ansIter](const std::size_t node){
                  if constexpr (   (G::flavour != maths::graph_flavour::directed_embedded)
                              && (E::flavour != maths::edge_flavour::partial)
                              && (E::flavour != maths::edge_flavour::partial_embedded)
                  )
                  {
                    return (edge.host_node() == node) ? edge.target_node() : edge.host_node();
                  }
                  else
                  {
                    return edge.target_node();
                  } 
                }(i);
                check_equality(target, edge->target_node(), "target_node" + message);

                if constexpr ((Edge::flavour == maths::edge_flavour::partial_embedded)
                              && ((RefEdge::flavour == maths::edge_flavour::full_embedded) || (RefEdge::flavour == maths::edge_flavour::partial_embedded)))
                {
                  check_equality(ansIter->complementary_index(), edge->complementary_index(), "complementary_index" + message);
                }
                else if constexpr ((Edge::flavour == maths::edge_flavour::full_embedded) && (RefEdge::flavour == maths::edge_flavour::full_embedded))
                {
                  check_equality(ansIter->host_node(), edge->host_node(), "host_node " + message);
                  check_equality(ansIter->complementary_index(), edge->complementary_index(), "complementary_index" + message);
                  check_equality(ansIter->inverted(), edge->inverted(), "inverted" + message);
                }
                else if constexpr ((Edge::flavour == maths::edge_flavour::full) && (RefEdge::flavour == maths::edge_flavour::full))
                {
                  check_equality(ansIter->host_node(), edge->host_node(), "host_node" + message);
                  check_equality(ansIter->inverted(), edge->inverted(), "inverted" + message);
                }
              
                if constexpr(!nullEdgeWeight) check_equality(ansIter->weight(), edge->weight(), "Weight" + message);
              
                (G::flavour != maths::graph_flavour::directed) ? nEdges += 0.5 : ++nEdges;
              }
            }
          }
        }
        check_equality(static_cast<std::size_t>(nEdges), graph.size(), "Graph size wrong");

        constexpr bool falsePosMode{checker<Logger>::mode == test_mode::false_positive};
        const bool passed{(falsePosMode && (failures() != numFailures)) || (!falsePosMode && (failures() == numFailures))};
        if(!passed && !failureMessage.empty())
        {
          post_message('\t' + failureMessage + '\n');
        }
      
        return passed;
      }
    };

    template<class Logger>
    class graph_basic_test : public basic_test<Logger, graph_checker<Logger>>
    {
    public:
      using basic_test<Logger, graph_checker<Logger>>::basic_test;
      using basic_test<Logger, graph_checker<Logger>>::check_exception_thrown;
      using basic_test<Logger, graph_checker<Logger>>::check_equality;
      using basic_test<Logger, graph_checker<Logger>>::check_graph;

      log_summary execute() override
      {        
        return base_t::execute() + m_AccumulatedSummaries;
      }
      
      void merge(const log_summary& summary)
      {
        m_AccumulatedSummaries += summary;
      }

    protected:
      virtual std::string current_message() const override
      {
        const log_summary s{this->summary("") + m_AccumulatedSummaries};
        return s.current_message();
      }
    private:
      using base_t = basic_test<Logger, graph_checker<Logger>>;
      
      log_summary m_AccumulatedSummaries{};
    };

    using graph_unit_test = graph_basic_test<unit_test_logger<test_mode::standard>>;
    using graph_false_positive_test = graph_basic_test<unit_test_logger<test_mode::false_positive>>;


    template
    <
      maths::graph_flavour GraphFlavour,      
      class EdgeWeight,
      class NodeWeight,      
      template <class> class EdgeWeightPooling,
      template <class> class NodeWeightPooling,
      template<class, template<class> class> class EdgeStorageTraits,
      template<class, template<class> class, bool> class NodeWeightStorageTraits,
      class Logger=unit_test_logger<test_mode::standard>
    >
    class graph_operations : protected graph_checker<Logger>
    {
    public:
      using graph_type = typename graph_type_generator<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>::graph_type;      
      
      log_summary run(const std::string& helpername)
      {        
        const std::string prefix{
            to_string(GraphFlavour) + "; "           
          + template_class_to_string<EdgeWeightPooling>::str() + "; "
          + template_class_to_string<NodeWeightPooling>::str() + "; "
          + storage_traits_to_string<EdgeStorageTraits>::str() + "; "
          + node_weight_storage_traits_to_string<NodeWeightStorageTraits>::str() + "; " 
          + helpername};

        this->failure_message_prefix(prefix);

        execute_operations();

        return this->summary("");
      }

      log_summary get_summary() const { return this->summary(""); }

      std::string prefix() const { return this->failure_message_prefix(); }
      
    protected:
      virtual void execute_operations() = 0;

    };

    template <class EdgeWeight, class NodeWeight>
    class graph_test_helper
    {
    public:
      graph_test_helper(std::string_view name="") : m_Name{name} {}
      
      template
      <
        template
        <
          maths::graph_flavour,
          class,
          class,
          template <class> class,
          template <class> class,
          template <class, template<class> class> class,
          template <class, template<class> class, bool> class
        >
        class TemplateTestClass,
        class Test
      >
      void run_tests(Test& unitTest)
      {        
        using flavour = maths::graph_flavour;
        try
        {
          run_graph_flavour_tests<flavour::undirected, TemplateTestClass>();
          run_graph_flavour_tests<flavour::undirected_embedded, TemplateTestClass>();
          run_graph_flavour_tests<flavour::directed, TemplateTestClass>();
          run_graph_flavour_tests<flavour::directed_embedded, TemplateTestClass>();
          
          finish(unitTest);
        }
        catch(...)
        {
          finish(unitTest);
          throw;
        }
      }
      
      template
      <
        template <class, template<class> class> class EdgeStorage,
        template
        <
          maths::graph_flavour,
          class,
          class,
          template <class> class,
          template <class> class,
          template <class, template<class> class> class,
          template <class, template<class> class, bool> class
        >
        class TemplateTestClass,
        class Test
      >
      void run_storage_tests(Test& unitTest)
      {        
        using flavour = maths::graph_flavour;
        try
        {
          run_graph_storage_tests<flavour::undirected,          EdgeStorage, TemplateTestClass>();
          run_graph_storage_tests<flavour::undirected_embedded, EdgeStorage, TemplateTestClass>();
          run_graph_storage_tests<flavour::directed,            EdgeStorage, TemplateTestClass>();
          run_graph_storage_tests<flavour::directed_embedded,   EdgeStorage, TemplateTestClass>();
          
          finish(unitTest);
        }
        catch(...)
        {
          finish(unitTest);
          throw;
        }
      }
      
      template
      <
        maths::graph_flavour GraphFlavour,
        template
        <
          maths::graph_flavour,
          class,
          class,
          template <class> class,
          template <class> class,
          template <class, template<class> class> class,
          template <class, template<class> class, bool> class
        >
        class TemplateTestClass,
        class Test
      >
      void run_individual_test(Test& unitTest)
      {
        try
        {
          run_graph_flavour_tests<GraphFlavour, TemplateTestClass>();
          
          finish(unitTest);
        }
        catch(...)
        {
          finish(unitTest);
          throw;
        }
      }

      
    private:
      std::string m_Name;
      log_summary m_Summary{};

      template<class Test>
      void finish(Test& unitTest)
      {
        unitTest.merge(m_Summary);   
        m_Summary.clear();
      }

      template
      <
        maths::graph_flavour GraphType,
        template <class, template<class> class> class EdgeStorage,
        template
        <
          maths::graph_flavour,
          class,
          class,
          template <class> class,
          template <class> class,
          template <class, template<class> class> class,
          template <class, template<class> class, bool> class
        >
        class TemplateTestClass
      >
      void run_graph_storage_tests()
      {
        using namespace data_sharing;
        
        TemplateTestClass<GraphType, EdgeWeight, NodeWeight, unpooled, unpooled, EdgeStorage, maths::node_weight_storage_traits> test0;        
        TemplateTestClass<GraphType, EdgeWeight, NodeWeight, unpooled, data_pool, EdgeStorage, maths::node_weight_storage_traits> test1;
        TemplateTestClass<GraphType, EdgeWeight, NodeWeight, data_pool, unpooled, EdgeStorage, maths::node_weight_storage_traits> test2;
        TemplateTestClass<GraphType, EdgeWeight, NodeWeight, data_pool, data_pool, EdgeStorage, maths::node_weight_storage_traits> test3;

        run_graph_test(test0);
        run_graph_test(test1);
        run_graph_test(test2);
        run_graph_test(test3);
      }
      
      template
      <
        maths::graph_flavour GraphType,
        template
        <
          maths::graph_flavour,
          class,
          class,
          template <class> class,
          template <class> class,
          template <class, template<class> class> class,
          template <class, template<class> class, bool> class
        >
        class TemplateTestClass
      >
      void run_graph_flavour_tests()
      {
        using namespace data_structures;
        
        run_graph_storage_tests<GraphType, maths::contiguous_edge_storage_traits, TemplateTestClass>();
        run_graph_storage_tests<GraphType, maths::bucketed_edge_storage_traits, TemplateTestClass>();
      }

      template<class Test>
      void run_graph_test(Test& test)
      {
        try
        {
          m_Summary += test.run(m_Name);
        }
        catch(...)
        {
          m_Summary += test.get_summary();
          m_Summary.prefix(test.prefix());
          throw;
        }
      }
    };


    template<class Checker>
    class checker_wrapper
    {
    public:
      checker_wrapper(Checker& checker) : m_Checker{checker} {}
      
    protected:
      ~checker_wrapper() = default;
      
      template<class E, class Fn>
      bool check_exception_thrown(Fn&& function, const std::string& description="")
      {
        return m_Checker.template check_exception_thrown<E>(std::forward<Fn>(function), description);
      }

      template<class G, class E=typename G::edge_init_type>
      bool check_graph(const G& graph, const std::initializer_list<std::initializer_list<E>>& edges, const std::initializer_list<typename G::node_weight_type>& nodeWeights, const std::string& failureMessage="")
      {
        return m_Checker.template check_graph(graph, edges, nodeWeights, failureMessage);
      }

      template<class T> bool check_equality(const T& reference, const T& value, const std::string& description="")
      {
        return m_Checker.template check_equality(reference, value, description);
      }


    private:
      Checker& m_Checker;
    };
    
    
    struct unsortable
    {
      int x{};
    };

    constexpr bool operator==(const unsortable& lhs, const unsortable& rhs) noexcept
    {
      return lhs.x == rhs.x;
    }

    constexpr bool operator!=(const unsortable& lhs, const unsortable& rhs) noexcept
    {
      return !(lhs == rhs);
    }

    template<class Stream> Stream& operator<<(Stream& s, const unsortable& u)
    {
      s << std::to_string(u.x);
      return s;
    }

    struct big_unsortable
    {
      int w{}, x{1}, y{2}, z{3};
    };

    constexpr bool operator==(const big_unsortable& lhs, const big_unsortable& rhs) noexcept
    {
      return (lhs.w == rhs.w)
          && (lhs.x == rhs.x)
          && (lhs.y == rhs.y)
          && (lhs.z == rhs.z);
    }

    constexpr bool operator!=(const big_unsortable& lhs, const big_unsortable& rhs) noexcept
    {
      return !(lhs == rhs);
    }

    template<class Stream> Stream& operator<<(Stream& s, const big_unsortable& u)
    {
      s << std::to_string(u.w) << ' ' << std::to_string(u.x) << ' ' << std::to_string(u.y) << ' ' << std::to_string(u.z);
      return s;
    }
  }
}
