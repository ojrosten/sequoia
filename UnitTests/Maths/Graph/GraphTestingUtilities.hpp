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
      static void check(Logger& logger, const Graph& graph, const Graph& prediction, std::string_view description)
      {
        using connectivity_t = typename type::connectivity_type;
        using nodes_t = typename type::nodes_type;

        check_equality(logger, static_cast<const connectivity_t&>(graph), static_cast<const connectivity_t&>(prediction), description);
        check_equality(logger, static_cast<const nodes_t&>(graph), static_cast<const nodes_t&>(prediction), description);
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
      static void check(Logger& logger, const type& graph, connectivity_equivalent_type connPrediction, nodes_equivalent_type nodesPrediction, std::string_view description)
      {
        using connectivity_t = typename type::connectivity_type;
        using nodes_t = typename type::nodes_type;

        check_equivalence(logger, static_cast<const connectivity_t&>(graph), connPrediction, description);
        check_equivalence(logger, static_cast<const nodes_t&>(graph), nodesPrediction, description);
      }

      template<class Logger, class W=node_weight_type, std::enable_if_t<std::is_empty_v<W>, int> = 0>
      static void check(Logger& logger, const type& graph, connectivity_equivalent_type connPrediction, std::string_view description)
      {
        using connectivity_t = typename type::connectivity_type;

        check_equivalence(logger, static_cast<const connectivity_t&>(graph), connPrediction, description);
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
      static void check(Logger& logger, const type& graph, connectivity_equivalent_type connPrediction, nodes_equivalent_type nodesPrediction, std::string_view description)
      {
        using connectivity_t = typename type::connectivity_type;
        using nodes_t = typename type::nodes_type;

        check_weak_equivalence(logger, static_cast<const connectivity_t&>(graph), connPrediction, description);
        check_equivalence(logger, static_cast<const nodes_t&>(graph), nodesPrediction, description);
      }

      template<class Logger, class W=node_weight_type, std::enable_if_t<std::is_empty_v<W>, int> = 0>
      static void check(Logger& logger, const type& graph, connectivity_equivalent_type connPrediction, std::string_view description)
      {
        using connectivity_t = typename type::connectivity_type;

        check_weak_equivalence(logger, static_cast<const connectivity_t&>(graph), connPrediction, description);
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
    static void check(Logger& logger, const type& connectivity, const type& prediction, std::string_view description)
    {
      check_equality(logger, connectivity.size(), prediction.size(), impl::combine_messages(description, "Connectivity sizes different", "\n"));
      
      if(check_equality(logger, connectivity.order(), prediction.order(), impl::combine_messages(description, "Connectivity orders different", "\n")))
      {
        for(std::size_t i{}; i<connectivity.order(); ++i)
        {
          const std::string message{impl::combine_messages(description, "Partition " + std::to_string(i), "\n")};
          check_range(logger, connectivity.cbegin_edges(i), connectivity.cend_edges(i), prediction.cbegin_edges(i), prediction.cend_edges(i), impl::combine_messages(message, "cedge_iterator"));

          check_range(logger, connectivity.crbegin_edges(i), connectivity.crend_edges(i), prediction.crbegin_edges(i), prediction.crend_edges(i), impl::combine_messages(message, "credge_iterator"));
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
    static void check(Logger& logger, const type& connectivity, equivalent_type prediction, std::string_view description)
    {
      if(check_equality(logger, connectivity.order(), prediction.size(), impl::combine_messages(description, "Connectivity order wrong")))
      {
        for(std::size_t i{}; i<connectivity.order(); ++i)
        {
          const std::string message{impl::combine_messages(description,"Partition " + std::to_string(i))};

          if constexpr(std::is_same_v<edge_type, edge_init_type>)
          {
            check_range(logger, connectivity.cbegin_edges(i), connectivity.cend_edges(i), (prediction.begin()+i)->begin(), (prediction.begin() + i)->end(), impl::combine_messages(message, "cedge_iterator"));
          }
          else
          {
            check_range_equivalence(logger, connectivity.cbegin_edges(i), connectivity.cend_edges(i), (prediction.begin()+i)->begin(), (prediction.begin() + i)->end(), impl::combine_messages(message, "cedge_iterator"));
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
    static void check(Logger& logger, const type& connectivity, equivalent_type prediction, std::string_view description)
    {
      if(check_equality(logger, connectivity.order(), prediction.size(), impl::combine_messages(description, "Connectivity order wrong")))
      {
        for(std::size_t i{}; i<connectivity.order(); ++i)
        {
          const std::string message{impl::combine_messages(description,"Partition " + std::to_string(i))};
          check_range_weak_equivalence(logger, connectivity.cbegin_edges(i), connectivity.cend_edges(i), (prediction.begin()+i)->begin(), (prediction.begin() + i)->end(), impl::combine_messages(message, "cedge_iterator"));
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
  class graph_checker : protected checker<Logger>
  {
  public:      
    using checker<Logger>::check_equality;
    using checker<Logger>::check;
    using checker<Logger>::check_exception_thrown;
    using checker<Logger>::check_equivalence;
    using checker<Logger>::check_regular_semantics;

    template<class G, class... NodeWeights, class E=typename G::edge_init_type>
    void check_graph(const G& graph, std::initializer_list<std::initializer_list<E>> edges, const std::tuple<NodeWeights...>& nodeWeights, std::string_view description)
    {
      checker<Logger>::template check_equivalence<G, std::initializer_list<std::initializer_list<E>>, const std::tuple<NodeWeights...>&>(graph, std::move(edges), nodeWeights, description);
    }

    template<
      class G,
      class E=typename G::edge_init_type,
      class W=typename G::node_weight_type,
      std::enable_if_t<!std::is_empty_v<W>, int> = 0
    >
    void check_graph(const G& graph, std::initializer_list<std::initializer_list<E>> edges, std::initializer_list<typename G::node_weight_type> nodeWeights, std::string_view description)
    {
      if constexpr(impl::use_weak_equiv_v<typename G::edge_type>)
      {
        checker<Logger>::template check_weak_equivalence<G, std::initializer_list<std::initializer_list<E>>, std::initializer_list<typename G::node_weight_type>>(graph, std::move(edges), std::move(nodeWeights), description);
      }
      else
      {
        checker<Logger>::template check_equivalence<G, std::initializer_list<std::initializer_list<E>>, std::initializer_list<typename G::node_weight_type>>(graph, std::move(edges), std::move(nodeWeights), description);
      }
    }

    template<
      class G,
      class E=typename G::edge_init_type,
      class W=typename G::node_weight_type,
      std::enable_if_t<std::is_empty_v<W>, int> = 0
    >
    void check_graph(const G& graph, std::initializer_list<std::initializer_list<E>> edges, std::string_view description)
    {
      if constexpr(impl::use_weak_equiv_v<typename G::edge_type>)
      {
        checker<Logger>::check_weak_equivalence(graph, std::move(edges), description);
      }
      else
      {
        checker<Logger>::check_equivalence(graph, std::move(edges), description);
      }
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
    using basic_test<Logger, graph_checker<Logger>>::check_regular_semantics;

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
