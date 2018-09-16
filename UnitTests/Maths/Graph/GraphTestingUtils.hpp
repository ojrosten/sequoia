#pragma once

#include "UnitTestUtils.hpp"

#include "ProtectiveWrapper.hpp"
#include "PartitionedData.hpp"
#include "DataPool.hpp"
#include "GraphImpl.hpp"

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

      std::size_t counter{};
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

      template<class G, class... NodeWeights, class E=typename G::edge_init_type>
      bool check_graph(const G& graph, const std::initializer_list<std::initializer_list<E>>& edges, const std::tuple<NodeWeights...>& nodeWeights, std::string_view failureMessage="")
      {
        check_equality(nodeWeights, graph.all_node_weights());
        
        return check_graph_edges(graph, edges, failureMessage);
      }

      template<class G, class E=typename G::edge_init_type>
      bool check_graph(const G& graph, std::initializer_list<std::initializer_list<E>> edges, std::initializer_list<typename G::node_weight_type> nodeWeights, std::string_view failureMessage="")
      {
        constexpr bool nullNodeWeight{std::is_empty_v<typename G::node_weight_type>};
          
        if constexpr(!nullNodeWeight)       
        {
          if(check_equality(nodeWeights.size(), static_cast<std::size_t>(distance(graph.cbegin_node_weights(), graph.cend_node_weights())), "Number of nodes wrong"))
          {
            for(auto iter{graph.cbegin_node_weights()}; iter != graph.cend_node_weights(); ++iter)
            {
              const auto i{distance(graph.cbegin_node_weights(), iter)};
              const std::string nodeMessage{"Node weight wrong for node " + std::to_string(i)};
              check_equality(*(nodeWeights.begin() + i), *(graph.cbegin_node_weights()+i), nodeMessage);
            }
          }
        }

        return check_graph_edges(graph, edges, failureMessage); 
      }
      
    private:
      template<class G, class E=typename G::edge_init_type>
      bool check_graph_edges(const G& graph, std::initializer_list<std::initializer_list<E>> edges, std::string_view failureMessage="")
      {
        using Edge = typename G::edge_type;
        using RefEdge = E;
        constexpr bool nullEdgeWeight{std::is_empty_v<typename G::edge_weight_type>};

        auto r{checker<Logger>::make_sentinel(failureMessage)};
        
        const auto numFailures{failures()};
        const auto numNodes{edges.size()};
        double nEdges{};
        if(check_equality(numNodes, graph.order(), "Graph order wrong"))
        {
          for(auto edgesIter{edges.begin()}; edgesIter != edges.end(); ++edgesIter)
          {
            const auto i{std::distance(edges.begin(), edgesIter)};
            const std::string partStr{std::to_string(i)};

            auto beginEdges{graph.cbegin_edges(i)};
            auto endEdges{graph.cend_edges(i)};
            auto rbeginEdges{graph.crbegin_edges(i)};
            auto rendEdges{graph.crend_edges(i)};

            const auto nNodeEdges{static_cast<std::size_t>(distance(beginEdges, endEdges))};
            const auto nrNodeEdges{static_cast<std::size_t>(distance(rbeginEdges, rendEdges))};

            const auto nodeEdges{*edgesIter};
            if(check_equality(nodeEdges.size(), nNodeEdges, "Number of edges for partition " + partStr + " wrong")
               && check_equality(nodeEdges.size(), nrNodeEdges, "Number of edges for reversed partition " + partStr + " wrong")
               )
            {
              auto ansIter{nodeEdges.begin()};
              auto redge{rendEdges};
              for(auto edge{beginEdges}; edge != endEdges; ++edge, ++ansIter)
              {
                --redge;
                check(*edge == *redge, "Disagreement between forward and reverse itereators");
                auto dist{distance(beginEdges, edge)};
                const std::string message{" wrong for partition " + partStr + ", edge " + to_string(dist)};
            
                const auto target{[edge=*ansIter](const std::size_t node){
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
                }(i)};
                
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
          post_message('\t' + std::string{failureMessage} + '\n');
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
