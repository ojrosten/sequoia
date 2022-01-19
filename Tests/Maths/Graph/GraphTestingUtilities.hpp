////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "EdgeTestingUtilities.hpp"
#include "NodeStorageTestingUtilities.hpp"

#include "sequoia/Core/Utilities/UniformWrapper.hpp"
#include "sequoia/Core/DataStructures/PartitionedData.hpp"
#include "sequoia/Core/Ownership/DataPool.hpp"
#include "sequoia/Maths/Graph/GraphImpl.hpp"
#include "sequoia/Maths/Graph/GraphTraits.hpp"

#include "sequoia/TestFramework/PerformanceTestCore.hpp"

namespace sequoia::testing
{
  template
  <
    maths::directed_flavour Directedness,
    class EdgeTraits,
    class WeightMaker
  >
  struct value_tester<maths::connectivity<Directedness, EdgeTraits, WeightMaker>>
  {
    using type                         = maths::connectivity<Directedness, EdgeTraits, WeightMaker>;
    using edge_index_type              = typename type::edge_index_type;

    template<class E>
    using connectivity_equivalent_type = std::initializer_list<std::initializer_list<E>>;

    template<class CheckType, test_mode Mode>
    static void test(CheckType flavour, test_logger<Mode>& logger, const type& connectivity, const type& prediction)
    {
      check(equality, "Connectivity size incorrect", logger, connectivity.size(), prediction.size());

      if(check(equality, "Connectivity order incorrect", logger, connectivity.order(), prediction.order()))
      {
        for(edge_index_type i{}; i<connectivity.order(); ++i)
        {
          const auto message{std::string{"Partition "}.append(std::to_string(i))};
          check(flavour, append_lines(message, "cedge_iterator"), logger, connectivity.cbegin_edges(i), connectivity.cend_edges(i), prediction.cbegin_edges(i), prediction.cend_edges(i));
          check(flavour, append_lines(message, "credge_iterator"), logger, connectivity.crbegin_edges(i), connectivity.crend_edges(i), prediction.crbegin_edges(i), prediction.crend_edges(i));
        }
      }
    }

    template<class CheckType, test_mode Mode, class E>
    static void test(CheckType flavour, test_logger<Mode>& logger, const type& connectivity, connectivity_equivalent_type<E> prediction)
    {
      check_connectivity(flavour, logger, connectivity, prediction);
    }
  private:
    template<class CheckType, test_mode Mode, class E>
    static void check_connectivity(CheckType flavour, test_logger<Mode>& logger, const type& connectivity, connectivity_equivalent_type<E> prediction)
    {
      if(check(equality, "Connectivity order incorrect", logger, connectivity.order(), prediction.size()))
      {
        for(edge_index_type i{}; i < connectivity.order(); ++i)
        {
          const auto message{"Partition " + std::to_string(i)};
          check(flavour, append_lines(message, "cedge_iterator"), logger, connectivity.cbegin_edges(i), connectivity.cend_edges(i), (prediction.begin() + i)->begin(), (prediction.begin() + i)->end());
        }
      }
    }
  };

  template<maths::network Graph>
  struct value_tester<Graph>
  {
    using type = Graph;

    template<class E>
    using connectivity_equivalent_type = std::initializer_list<std::initializer_list<E>>;

    using connectivity_type = typename type::connectivity_type;
    using nodes_type = typename type::nodes_type;

    template<class CheckType, test_mode Mode>
    static void test(CheckType flavour, test_logger<Mode>& logger, const Graph& graph, const Graph& prediction)
    {
      check(flavour, "", logger, static_cast<const connectivity_type&>(graph), static_cast<const connectivity_type&>(prediction));
      check(flavour, "", logger, static_cast<const nodes_type &>(graph), static_cast<const nodes_type&>(prediction));
    }

    template<class CheckType, test_mode Mode, class E, class NodesEquivalentType>
      //requires (!std::is_empty_v<nodes_type>)
    static void test(CheckType flavour, test_logger<Mode>& logger, const type& graph, connectivity_equivalent_type<E> connPrediction, NodesEquivalentType&& nodesPrediction)
    {
      check(flavour, "", logger, static_cast<const connectivity_type&>(graph), connPrediction);
      check(flavour, "", logger, static_cast<const nodes_type&>(graph), std::forward<NodesEquivalentType>(nodesPrediction));
    }

    template<class CheckType, test_mode Mode, class E>
     // requires std::is_empty_v<nodes_type>
    static void test(CheckType flavour, test_logger<Mode>& logger, const type& graph, connectivity_equivalent_type<E> connPrediction)
    {
      check(flavour, "", logger, static_cast<const connectivity_type&>(graph), connPrediction);
    }

    template<test_mode Mode, class NodeWeight>
      requires maths::dynamic_tree<type>
    static void test(equivalence_check_t, test_logger<Mode>& logger, const type& actual, const maths::tree_initializer<NodeWeight>& prediction)
    {
      check_node(logger, 0, type::npos, actual, prediction);
    }
  private:
    using edge_iterator = typename type::const_edge_iterator;
    using size_type     = typename type::size_type;

    template<test_mode Mode, class NodeWeight>
      requires maths::dynamic_tree<type>
    static size_type check_node(test_logger<Mode>& logger, size_type node, size_type parent, const type& actual, const maths::tree_initializer<NodeWeight>& prediction)
    {
      constexpr auto TreeLinkDir{type::link_dir};

      if(node == type::npos) return node;

      if(testing::check("Insufficient nodes detected while checking node " + std::to_string(node), logger, node < actual.order()))
      {
        if constexpr(TreeLinkDir != maths::tree_link_direction::backward)
        {
          check(equality, "Node weight for node " + std::to_string(node), logger, actual.cbegin_node_weights()[node], prediction.node);

          if(auto optIter{check_num_edges(logger, node, parent, actual, prediction)})
          {
            for(const auto& child : prediction.children)
            {
              if(!check_edge(logger, (*optIter)++, ++node, actual))
                return type::npos;

              node = check_node(logger, node, optIter->partition_index(), actual, child);

              if(node == type::npos) return type::npos;
            }

            return node;
          }
        }
        else
        {
          if(auto optIter{check_num_edges(logger, node, parent, actual, prediction)})
          {
            check(equality, "Node weight for node " + std::to_string(node), logger, actual.cbegin_node_weights()[node], prediction.node);

            for(const auto& child : prediction.children)
            {
              node = check_node(logger, ++node, optIter->partition_index(), actual, child);

              if(node == type::npos) return type::npos;
            }

            if(const auto next{node + 1}; next < actual.order())
            {
              if(auto begin{actual.cbegin_edges(next)}; begin != actual.cend_edges(next))
              {
                if(!testing::check("Extraneous nodes joined to node " + std::to_string(node), logger, begin->target_node() != optIter->partition_index()))
                {
                  return type::npos;
                }
              }
            }

            return node;
          }
        }
      }

      return type::npos;
    }

    template<test_mode Mode, class NodeWeight>
      requires maths::dynamic_tree<type>
    static std::optional<edge_iterator> check_num_edges(test_logger<Mode>& logger, size_type node, [[maybe_unused]] size_type parent, const type& actual, const maths::tree_initializer<NodeWeight>& prediction)
    {
      using std::distance;

      constexpr auto TreeLinkDir{type::link_dir};

      if constexpr(TreeLinkDir == maths::tree_link_direction::forward)
      {
        if(check(equality, "Number of children for node " + std::to_string(node), logger, static_cast<std::size_t>(distance(actual.cbegin_edges(node), actual.cend_edges(node))), prediction.children.size()))
          return actual.cbegin_edges(node);
      }
      else
      {
        const auto begin{actual.cbegin_edges(node)};
        const auto dist{static_cast<std::size_t>(distance(begin, actual.cend_edges(node)))};
        using dist_t = decltype(dist);

        const auto num{
          [&logger,parent,dist,begin]() -> std::optional<dist_t> {
            if(parent == type::npos) return dist;

            if(testing::check("Return edge detected", logger, dist > 0)
               && check(equality, "Return edge target", logger, begin->target_node(), parent))
              return dist - 1;

            return std::nullopt;
          }()
        };

        if(num.has_value())
        {
          if constexpr(TreeLinkDir == maths::tree_link_direction::symmetric)
          {
            if(check(equality, "Number of children for node " + std::to_string(node), logger, num.value(), prediction.children.size()))
              return num < dist ? std::next(begin) : begin;
          }
          else
          {
            if(check(equality, "No reachable children for node " + std::to_string(node), logger, num.value(), size_type{}))
              return num < dist ? std::next(begin) : begin;
          }
        }
      }

      return std::nullopt;
    }

    template<test_mode Mode, std::input_or_output_iterator EdgeIter>
      requires maths::dynamic_tree<type>
    static bool check_edge(test_logger<Mode>& logger, EdgeIter iter, size_type nodeCounter, const type& actual)
    {
      using std::distance;

      constexpr auto TreeLinkDir{type::link_dir};

      if constexpr(TreeLinkDir == maths::tree_link_direction::forward)
      {
        const auto parent{iter.partition_index()};
        const auto dist{distance(actual.cbegin_edges(parent), iter)};
        const auto mess{std::string{"Index for child "}.append(std::to_string(dist)).append(" of node ").append(std::to_string(parent))};

        return check(equality, mess, logger, iter->target_node(), nodeCounter);
      }
      else if constexpr(TreeLinkDir == maths::tree_link_direction::backward)
      {
        static_assert(dependent_false<type>::value, "Not yet implemented");
      }
      else if constexpr(TreeLinkDir == maths::tree_link_direction::symmetric)
      {
        const auto parent{iter.partition_index()};
        const auto dist{distance(actual.cbegin_edges(parent), iter)};
        const auto mess{std::string{"Index for child "}.append(std::to_string(dist)).append(" of node ").append(std::to_string(parent))};

        return check(equality, mess, logger, iter->target_node(), nodeCounter);
      }
      else
      {
        static_assert(dependent_false<type>::value);
      }
    }
  };

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

    template<class G, class... NodeWeights, class E=typename G::edge_init_type>
    void check_graph(std::string_view description, const G& graph, std::initializer_list<std::initializer_list<E>> edges, const std::tuple<NodeWeights...>& nodeWeights)
    {
      checker_t::check(weak_equivalence, description, graph, std::move(edges), nodeWeights);
    }

    template
    <
      class G,
      class E=typename G::edge_init_type
    >
      requires (!std::is_empty_v<typename G::node_weight_type>)
    void check_graph(std::string_view description, const G& graph, std::initializer_list<std::initializer_list<E>> edges, std::initializer_list<typename G::node_weight_type> nodeWeights)
    {
      checker_t::check(weak_equivalence, description, graph, edges, nodeWeights);
    }

    template
    <
      class G,
      class E=typename G::edge_init_type
    >
      requires std::is_empty_v<typename G::node_weight_type>
    void check_graph(std::string_view description, const G& graph, std::initializer_list<std::initializer_list<E>> edges)
    {
      checker_t::check(weak_equivalence, description, graph, edges);
    }
  protected:
    graph_checker(graph_checker&&)            noexcept = default;
    graph_checker& operator=(graph_checker&&) noexcept = default;

    ~graph_checker() = default;
  };

  template<test_mode Mode, class... Extenders>
  class graph_basic_test : public basic_test<graph_checker<Mode, Extenders...>>
  {
  public:
    using base_t = basic_test<graph_checker<Mode, Extenders...>>;

    using base_t::base_t;
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
