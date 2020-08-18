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
#include "GraphTraits.hpp"

#include "PerformanceTestCore.hpp"

namespace sequoia::testing
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
      
      template<test_mode Mode>
      static void check(test_logger<Mode>& logger, const Graph& graph, const Graph& prediction)
      {
        using connectivity_t = typename type::connectivity_type;
        using nodes_t = typename type::nodes_type;

        check_equality("", logger, static_cast<const connectivity_t&>(graph), static_cast<const connectivity_t&>(prediction));
        check_equality("", logger, static_cast<const nodes_t&>(graph), static_cast<const nodes_t&>(prediction));
      }
    };

    // Equivalence Checker

    template<class Graph> struct graph_equivalence_checker
    {
      using type = Graph;

      using connectivity_equivalent_type = std::initializer_list<std::initializer_list<typename type::edge_init_type>>;
      using node_weight_type = typename type::node_weight_type;
      using nodes_equivalent_type = std::initializer_list<node_weight_type>;

      template<test_mode Mode, class W=node_weight_type>
        requires (!empty<W>)
      static void check(test_logger<Mode>& logger, const type& graph, connectivity_equivalent_type connPrediction, nodes_equivalent_type nodesPrediction)
      {
        using connectivity_t = typename type::connectivity_type;
        using nodes_t = typename type::nodes_type;

        check_equivalence("", logger, static_cast<const connectivity_t&>(graph), connPrediction);
        check_equivalence("", logger, static_cast<const nodes_t&>(graph), nodesPrediction);
      }

      template<test_mode Mode, class W=node_weight_type>
        requires empty<W>
      static void check(test_logger<Mode>& logger, const type& graph, connectivity_equivalent_type connPrediction)
      {
        using connectivity_t = typename type::connectivity_type;

        check_equivalence("", logger, static_cast<const connectivity_t&>(graph), connPrediction);
      }
    };

    // Weak Equivalence Checker

    template<class Graph> struct graph_weak_equivalence_checker
    {
      using type = Graph;

      using connectivity_equivalent_type = std::initializer_list<std::initializer_list<typename type::edge_init_type>>;
      using node_weight_type = typename type::node_weight_type;
      using nodes_equivalent_type = std::initializer_list<node_weight_type>;

      template<test_mode Mode, class W=node_weight_type>
        requires (!empty<W>)
      static void check(test_logger<Mode>& logger, const type& graph, connectivity_equivalent_type connPrediction, nodes_equivalent_type nodesPrediction)
      {
        using connectivity_t = typename type::connectivity_type;
        using nodes_t = typename type::nodes_type;

        check_weak_equivalence("", logger, static_cast<const connectivity_t&>(graph), connPrediction);
        check_equivalence("", logger, static_cast<const nodes_t&>(graph), nodesPrediction);
      }

      template<test_mode Mode, class W=node_weight_type>
        requires empty<W>
      static void check(test_logger<Mode>& logger, const type& graph, connectivity_equivalent_type connPrediction)
      {
        using connectivity_t = typename type::connectivity_type;

        check_weak_equivalence("", logger, static_cast<const connectivity_t&>(graph), connPrediction);
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

    template<test_mode Mode>
    static void check(test_logger<Mode>& logger, const type& connectivity, const type& prediction)
    {
      check_equality("Connectivity sizes different", logger, connectivity.size(), prediction.size());
      
      if(check_equality("Connectivity orders different", logger, connectivity.order(), prediction.order()))
      {
        for(std::size_t i{}; i<connectivity.order(); ++i)
        {
          const auto message{std::string{"Partition "}.append(std::to_string(i))};
          check_range(append_lines(message, "cedge_iterator"), logger, connectivity.cbegin_edges(i), connectivity.cend_edges(i), prediction.cbegin_edges(i), prediction.cend_edges(i));

          check_range(append_lines(message, "credge_iterator"), logger, connectivity.crbegin_edges(i), connectivity.crend_edges(i), prediction.crbegin_edges(i), prediction.crend_edges(i));
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

    template<test_mode Mode>
    static void check(test_logger<Mode>& logger, const type& connectivity, equivalent_type prediction)
    {
      if(check_equality("Connectivity order wrong", logger, connectivity.order(), prediction.size()))
      {
        for(std::size_t i{}; i<connectivity.order(); ++i)
        {
          const auto message{std::string{"Partition "}.append(std::to_string(i))};

          if constexpr(std::is_same_v<edge_type, edge_init_type>)
          {
            check_range(append_lines(message, "cedge_iterator"), logger, connectivity.cbegin_edges(i), connectivity.cend_edges(i), (prediction.begin()+i)->begin(), (prediction.begin() + i)->end());
          }
          else
          {
            check_range_equivalence(append_lines(message, "cedge_iterator"), logger, connectivity.cbegin_edges(i), connectivity.cend_edges(i), (prediction.begin()+i)->begin(), (prediction.begin() + i)->end());
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

    template<test_mode Mode>
    static void check(test_logger<Mode>& logger, const type& connectivity, equivalent_type prediction)
    {
      if(check_equality("Connectivity order wrong", logger, connectivity.order(), prediction.size()))
      {
        for(std::size_t i{}; i<connectivity.order(); ++i)
        {
          const std::string message{"Partition " + std::to_string(i)};
          check_range_weak_equivalence(append_lines(message, "cedge_iterator"), logger, connectivity.cbegin_edges(i), connectivity.cend_edges(i), (prediction.begin()+i)->begin(), (prediction.begin() + i)->end());
        }
      }
    }    
  };

  struct null_weight{};  

  constexpr bool mutual_info(const maths::graph_flavour flavour) noexcept
  {
    return flavour != maths::graph_flavour::directed;
  }

  template<test_mode Mode, class... Extenders>
  class graph_checker : public checker<Mode, Extenders...>
  {
  public:
    using checker_t = checker<Mode, Extenders...>;

    using checker<Mode, Extenders...>::checker;
    graph_checker(const graph_checker&) = delete;

    graph_checker& operator=(const graph_checker&) = delete;
    graph_checker& operator=(graph_checker&&)      = delete;

    template<class G, class... NodeWeights, class E=typename G::edge_init_type>
    void check_graph(std::string_view description, const G& graph, std::initializer_list<std::initializer_list<E>> edges, const std::tuple<NodeWeights...>& nodeWeights)
    {
      checker_t::check_equivalence(description, graph, std::move(edges), nodeWeights);
    }

    template
    <
      class G,
      class E=typename G::edge_init_type,
      class W=typename G::node_weight_type
    >
      requires (!empty<W>)
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

    template
    <
      class G,
      class E=typename G::edge_init_type,
      class W=typename G::node_weight_type
    >
      requires empty<W>
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
  protected:
    graph_checker(graph_checker&&) noexcept = default;

    ~graph_checker() = default;
  };

  template<test_mode Mode, class... Extenders>
  class graph_basic_test : public basic_test<graph_checker<Mode, Extenders...>>
  {
  public:
    using base_t = basic_test<graph_checker<Mode, Extenders...>>;
    
    using base_t::check_exception_thrown;
    using base_t::check_equality;
    using base_t::check_graph;
    using base_t::check_semantics;
    using base_t::check;

    graph_basic_test(std::string_view name, concurrency_mode mode)
      : basic_test<graph_checker<Mode, Extenders...>>{name}
      , m_ConcurrencyMode{mode}
    {}
      
    void merge(const log_summary& summary)
    {
      m_AccumulatedSummaries += summary;
    }

    void report_async_exception(std::string_view sv)
    {
      check(append_lines("Exception thrown during asynchronous execution of graph test:", sv), test_logger<Mode>::mode == test_mode::false_positive);
    }
  protected:
    using time_point = typename base_t::time_point;

    [[nodiscard]]
    log_summary summarize(const time_point start) const override
    {
      return base_t::summarize(start) += m_AccumulatedSummaries;
    }

    [[nodiscard]]
    concurrency_mode concurrent_execution() const noexcept { return m_ConcurrencyMode; }
  private:
      
    log_summary m_AccumulatedSummaries{};
    concurrency_mode m_ConcurrencyMode{};
  };

  template<test_mode mode>
  using regular_graph_test = graph_basic_test<mode, regular_extender<mode>>;

  using graph_unit_test = regular_graph_test<test_mode::standard>;
  using graph_false_positive_test = regular_graph_test<test_mode::false_positive>;    
       
  struct unsortable
  {
    int x{};

    [[nodiscard]]
    friend constexpr bool operator==(const unsortable& lhs, const unsortable& rhs) noexcept = default;

    [[nodiscard]]
    friend constexpr bool operator!=(const unsortable& lhs, const unsortable& rhs) noexcept = default;
      
    template<class Stream> friend Stream& operator<<(Stream& s, const unsortable& u)
    {
      s << std::to_string(u.x);
      return s;
    }
  };

  struct big_unsortable
  {
    int w{}, x{1}, y{2}, z{3};

    friend constexpr bool operator==(const big_unsortable& lhs, const big_unsortable& rhs) noexcept = default;

    friend constexpr bool operator!=(const big_unsortable& lhs, const big_unsortable& rhs) noexcept = default;

    template<class Stream> friend Stream& operator<<(Stream& s, const big_unsortable& u)
    {
      s << std::to_string(u.w) << ' ' << std::to_string(u.x) << ' ' << std::to_string(u.y) << ' ' << std::to_string(u.z);
      return s;
    }
  };
}
