////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "EdgeTestingUtilities.hpp"
#include "NodeStorageTestingUtilities.hpp"

#include "ProtectiveWrapper.hpp"
#include "PartitionedData.hpp"
#include "DataPool.hpp"
#include "GraphImpl.hpp"

namespace sequoia::unit_testing
{
  namespace impl
  {
    template<class Edge> struct use_weak_equiv : std::false_type {};

    template<class Weight, class WeightProxy, class IndexType>
    struct use_weak_equiv<maths::edge<Weight, WeightProxy, IndexType>> : std::true_type {};

    template<class Edge> constexpr bool use_weak_equiv_v{use_weak_equiv<Edge>::value};

    // Details Checker
    
    template<class Graph> struct graph_detailed_equality_checker
    {
      using type = Graph;
      
      template<class Logger>
      static void check(std::string_view description, Logger& logger, const Graph& graph, const Graph& prediction)
      {
        using connectivity_t = typename type::connectivity_type;
        using nodes_t = typename type::nodes_type;

        check_equality(description, logger, static_cast<const connectivity_t&>(graph), static_cast<const connectivity_t&>(prediction));
        check_equality(description, logger, static_cast<const nodes_t&>(graph), static_cast<const nodes_t&>(prediction));
      }
    };

    // Equivalence Checker

    template<class Graph> struct graph_equivalence_checker
    {
      using type = Graph;

      using connectivity_equivalent_type = std::initializer_list<std::initializer_list<typename type::edge_init_type>>;
      using node_weight_type = typename type::node_weight_type;
      using nodes_equivalent_type = std::initializer_list<node_weight_type>;

      template<class Logger, class W=node_weight_type, std::enable_if_t<!std::is_empty_v<W>, int> = 0>
      static void check(std::string_view description, Logger& logger, const type& graph, connectivity_equivalent_type connPrediction, nodes_equivalent_type nodesPrediction)
      {
        using connectivity_t = typename type::connectivity_type;
        using nodes_t = typename type::nodes_type;

        check_equivalence(description, logger, static_cast<const connectivity_t&>(graph), connPrediction);
        check_equivalence(description, logger, static_cast<const nodes_t&>(graph), nodesPrediction);
      }

      template<class Logger, class W=node_weight_type, std::enable_if_t<std::is_empty_v<W>, int> = 0>
      static void check(std::string_view description, Logger& logger, const type& graph, connectivity_equivalent_type connPrediction)
      {
        using connectivity_t = typename type::connectivity_type;

        check_equivalence(description, logger, static_cast<const connectivity_t&>(graph), connPrediction);
      }
    };

    // Weak Equivalence Checker

    template<class Graph> struct graph_weak_equivalence_checker
    {
      using type = Graph;

      using connectivity_equivalent_type = std::initializer_list<std::initializer_list<typename type::edge_init_type>>;
      using node_weight_type = typename type::node_weight_type;
      using nodes_equivalent_type = std::initializer_list<node_weight_type>;

      template<class Logger, class W=node_weight_type, std::enable_if_t<!std::is_empty_v<W>, int> = 0>
      static void check(std::string_view description, Logger& logger, const type& graph, connectivity_equivalent_type connPrediction, nodes_equivalent_type nodesPrediction)
      {
        using connectivity_t = typename type::connectivity_type;
        using nodes_t = typename type::nodes_type;

        check_weak_equivalence(description, logger, static_cast<const connectivity_t&>(graph), connPrediction);
        check_equivalence(description, logger, static_cast<const nodes_t&>(graph), nodesPrediction);
      }

      template<class Logger, class W=node_weight_type, std::enable_if_t<std::is_empty_v<W>, int> = 0>
      static void check(std::string_view description, Logger& logger, const type& graph, connectivity_equivalent_type connPrediction)
      {
        using connectivity_t = typename type::connectivity_type;

        check_weak_equivalence(description, logger, static_cast<const connectivity_t&>(graph), connPrediction);
      }
    };
  }
  
  template
  <      
    maths::directed_flavour Directedness,     
    class EdgeTraits,
    class WeightMaker
  >
  struct detailed_equality_checker<maths::connectivity<Directedness, EdgeTraits, WeightMaker>>
  {
    using type = maths::connectivity<Directedness, EdgeTraits, WeightMaker>;

    template<class Logger>
    static void check(std::string_view description, Logger& logger, const type& connectivity, const type& prediction)
    {
      check_equality(combine_messages(description, "Connectivity sizes different", "\n"), logger, connectivity.size(), prediction.size());
      
      if(check_equality(combine_messages(description, "Connectivity orders different", "\n"), logger, connectivity.order(), prediction.order()))
      {
        for(std::size_t i{}; i<connectivity.order(); ++i)
        {
          const std::string message{combine_messages(description, "Partition " + std::to_string(i), "\n")};
          check_range(combine_messages(message, "cedge_iterator"), logger, connectivity.cbegin_edges(i), connectivity.cend_edges(i), prediction.cbegin_edges(i), prediction.cend_edges(i));

          check_range(combine_messages(message, "credge_iterator"), logger, connectivity.crbegin_edges(i), connectivity.crend_edges(i), prediction.crbegin_edges(i), prediction.crend_edges(i));
        }
      }
    } 
  };
  
  template
  <      
    maths::directed_flavour Directedness,     
    class EdgeTraits,
    class WeightMaker
  >
  struct equivalence_checker<maths::connectivity<Directedness, EdgeTraits, WeightMaker>>
  {
    using type            = maths::connectivity<Directedness, EdgeTraits, WeightMaker>;
    using edge_init_type  = typename type::edge_init_type;
    using edge_type       = typename type::edge_type;
    using equivalent_type = std::initializer_list<std::initializer_list<edge_init_type>>;

    template<class Logger>
    static void check(std::string_view description, Logger& logger, const type& connectivity, equivalent_type prediction)
    {
      if(check_equality(combine_messages(description, "Connectivity order wrong"), logger, connectivity.order(), prediction.size()))
      {
        for(std::size_t i{}; i<connectivity.order(); ++i)
        {
          const std::string message{combine_messages(description,"Partition " + std::to_string(i))};

          if constexpr(std::is_same_v<edge_type, edge_init_type>)
          {
            check_range(combine_messages(message, "cedge_iterator"), logger, connectivity.cbegin_edges(i), connectivity.cend_edges(i), (prediction.begin()+i)->begin(), (prediction.begin() + i)->end());
          }
          else
          {
            check_range_equivalence(combine_messages(message, "cedge_iterator"), logger, connectivity.cbegin_edges(i), connectivity.cend_edges(i), (prediction.begin()+i)->begin(), (prediction.begin() + i)->end());
          }                
        }
      }
    }    
  };

  template
  <      
    maths::directed_flavour Directedness,     
    class EdgeTraits,
    class WeightMaker
  >
  struct weak_equivalence_checker<maths::connectivity<Directedness, EdgeTraits, WeightMaker>>
  {
    using type            = maths::connectivity<Directedness, EdgeTraits, WeightMaker>;
    using edge_init_type  = typename type::edge_init_type;
    using edge_type       = typename type::edge_type;
    using equivalent_type = std::initializer_list<std::initializer_list<edge_init_type>>;

    template<class Logger>
    static void check(std::string_view description, Logger& logger, const type& connectivity, equivalent_type prediction)
    {
      if(check_equality(combine_messages(description, "Connectivity order wrong"), logger, connectivity.order(), prediction.size()))
      {
        for(std::size_t i{}; i<connectivity.order(); ++i)
        {
          const std::string message{combine_messages(description,"Partition " + std::to_string(i))};
          check_range_weak_equivalence(combine_messages(message, "cedge_iterator"), logger, connectivity.cbegin_edges(i), connectivity.cend_edges(i), (prediction.begin()+i)->begin(), (prediction.begin() + i)->end());
        }
      }
    }    
  };

  struct null_weight{};

  template<class G, class = std::void_t<>>
  struct is_static_graph : std::true_type
  {};

  template<class G>
  struct is_static_graph<G, std::void_t<decltype(std::declval<G>().add_node())>> : std::false_type
  {};

  template<class G>
  constexpr bool is_static_graph_v{is_static_graph<G>::value};    

  constexpr bool mutual_info(const maths::graph_flavour flavour) noexcept
  {
    return flavour != maths::graph_flavour::directed;
  }

  template<class Logger>
  class graph_checker : protected checker<Logger, performance_extender<Logger>, allocator_extender<Logger>>
  {
  public:
    using checker_t = checker<Logger, performance_extender<Logger>, allocator_extender<Logger>>;
    
    using checker_t::check_equality;
    using checker_t::check;
    using checker_t::check_exception_thrown;
    using checker_t::check_equivalence;
    using checker_t::check_regular_semantics;

    template<class G, class... NodeWeights, class E=typename G::edge_init_type>
    void check_graph(std::string_view description, const G& graph, std::initializer_list<std::initializer_list<E>> edges, const std::tuple<NodeWeights...>& nodeWeights)
    {
      checker_t::check_equivalence(description, graph, std::move(edges), nodeWeights);
    }

    template<
      class G,
      class E=typename G::edge_init_type,
      class W=typename G::node_weight_type,
      std::enable_if_t<!std::is_empty_v<W>, int> = 0
    >
    void check_graph(std::string_view description, const G& graph, std::initializer_list<std::initializer_list<E>> edges, std::initializer_list<typename G::node_weight_type> nodeWeights)
    {
      if constexpr(impl::use_weak_equiv_v<typename G::edge_type>)
      {
        checker_t::check_weak_equivalence(description, graph, edges, nodeWeights);
      }
      else
      {
        checker_t::check_equivalence(description, graph, edges, nodeWeights);
      }
    }

    template<
      class G,
      class E=typename G::edge_init_type,
      class W=typename G::node_weight_type,
      std::enable_if_t<std::is_empty_v<W>, int> = 0
    >
    void check_graph(std::string_view description, const G& graph, std::initializer_list<std::initializer_list<E>> edges)
    {
      if constexpr(impl::use_weak_equiv_v<typename G::edge_type>)
      {
        checker_t::check_weak_equivalence(description, graph, std::move(edges));
      }
      else
      {
        checker_t::check_equivalence(description, graph, std::move(edges));
      }
    }
  };

  template<class Logger>
  class graph_basic_test : public basic_test<Logger, graph_checker<Logger>>
  {
  public:
    using basic_test<Logger, graph_checker<Logger>>::check_exception_thrown;
    using basic_test<Logger, graph_checker<Logger>>::check_equality;
    using basic_test<Logger, graph_checker<Logger>>::check_graph;
    using basic_test<Logger, graph_checker<Logger>>::check_regular_semantics;
    using basic_test<Logger, graph_checker<Logger>>::check;

    graph_basic_test(std::string_view name, concurrency_mode mode)
      : basic_test<Logger, graph_checker<Logger>>{name}
      , m_ConcurrencyMode{mode}
    {}
    
    log_summary execute() override
    {        
      return base_t::execute() + m_AccumulatedSummaries;
    }
      
    void merge(const log_summary& summary)
    {
      m_AccumulatedSummaries += summary;
    }

    void report_async_exception(std::string_view sv)
    {
      check(combine_messages("Exception thrown during asynchronous execution of graph test:", sv, "\n"), Logger::mode == test_mode::false_positive);
    }
  protected:
    virtual std::string current_message() const override
    {
      const log_summary s{this->summary("", log_summary::duration{}) + m_AccumulatedSummaries};
      return s.current_message();
    }

    [[nodiscard]]
    concurrency_mode concurrent_execution() const noexcept { return m_ConcurrencyMode; }
  private:
    using base_t = basic_test<Logger, graph_checker<Logger>>;
      
    log_summary m_AccumulatedSummaries{};
    concurrency_mode m_ConcurrencyMode{};
  };

  using graph_unit_test = graph_basic_test<unit_test_logger<test_mode::standard>>;
  using graph_false_positive_test = graph_basic_test<unit_test_logger<test_mode::false_positive>>;    
       
  struct unsortable
  {
    int x{};

    friend constexpr bool operator==(const unsortable& lhs, const unsortable& rhs) noexcept
    {
      return lhs.x == rhs.x;
    }

    friend constexpr bool operator!=(const unsortable& lhs, const unsortable& rhs) noexcept
    {
      return !(lhs == rhs);
    }
      
    template<class Stream> friend Stream& operator<<(Stream& s, const unsortable& u)
    {
      s << std::to_string(u.x);
      return s;
    }
  };

  struct big_unsortable
  {
    int w{}, x{1}, y{2}, z{3};

    friend constexpr bool operator==(const big_unsortable& lhs, const big_unsortable& rhs) noexcept
    {
      return (lhs.w == rhs.w)
        && (lhs.x == rhs.x)
        && (lhs.y == rhs.y)
        && (lhs.z == rhs.z);
    }

    friend constexpr bool operator!=(const big_unsortable& lhs, const big_unsortable& rhs) noexcept
    {
      return !(lhs == rhs);
    }

    template<class Stream> friend Stream& operator<<(Stream& s, const big_unsortable& u)
    {
      s << std::to_string(u.w) << ' ' << std::to_string(u.x) << ' ' << std::to_string(u.y) << ' ' << std::to_string(u.z);
      return s;
    }
  };
}
