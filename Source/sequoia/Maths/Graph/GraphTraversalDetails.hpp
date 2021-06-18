////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Meta-programming urilities and underlying function for graph traversals.

 */

#include "sequoia/Maths/Graph/GraphTraits.hpp"
#include "sequoia/Maths/Graph/GraphDetails.hpp"

#include <vector>

namespace sequoia::maths
{
  struct null_func_obj
  {
    template<class T>
    void operator()(T&&);
  };

  enum class disconnected_discovery_mode { on, off };

  class traversal_conditions_base
  {
  public:
    constexpr traversal_conditions_base() = default;

    constexpr explicit traversal_conditions_base(std::size_t startingIndex)
      : m_Index{startingIndex}
    {}

    [[nodiscard]]
    constexpr std::size_t starting_index() const noexcept
    {
      return m_Index;
    }
  protected:
    constexpr traversal_conditions_base(const traversal_conditions_base&) = default;

    constexpr traversal_conditions_base& operator=(const traversal_conditions_base&) = default;

    ~traversal_conditions_base() = default;
  private:
    std::size_t m_Index{};
  };

  template<disconnected_discovery_mode FindDisconnected>
  class traversal_conditions : public traversal_conditions_base
  {
  public:
    constexpr static disconnected_discovery_mode mode{FindDisconnected};

    using traversal_conditions_base::traversal_conditions_base;

    constexpr traversal_conditions(const traversal_conditions&) = default;
    constexpr traversal_conditions& operator=(const traversal_conditions&) = default;

    template<class Bitset>
    constexpr std::size_t compute_restart_index(const Bitset& b)
    {
      if(m_NumDiscovered)
      {
        while(b.size() && b[m_Restart]) ++m_Restart;

        return m_Restart;
      }

      return starting_index();
    }

    template<class Bitset>
    constexpr void register_discovered(Bitset& b, const std::size_t index)
    {
      b[index] = true;
      ++m_NumDiscovered;
    }

    [[nodiscard]]
    constexpr bool terminate(const std::size_t order) const noexcept
    {
      return m_NumDiscovered == order;
    }
  private:
    std::size_t m_NumDiscovered{}, m_Restart{};
  };

  template<>
  class traversal_conditions<disconnected_discovery_mode::off> : public traversal_conditions_base
  {
  public:
    constexpr static disconnected_discovery_mode mode{disconnected_discovery_mode::off};

    using traversal_conditions_base::traversal_conditions_base;

    constexpr traversal_conditions(const traversal_conditions&) = default;
    constexpr traversal_conditions& operator=(const traversal_conditions&) = default;

    template<class Bitset>
    constexpr std::size_t compute_restart_index(const Bitset&) const noexcept
    {
      return starting_index();
    }

    [[nodiscard]]
    constexpr bool terminate(std::size_t) const noexcept
    {
      return true;
    }

    template<class Bitset>
    constexpr void register_discovered(Bitset& b, const std::size_t index)
    {
      b[index] = true;
    }
  };

  using find_disconnected_t = traversal_conditions<disconnected_discovery_mode::on>;

  using ignore_disconnected_t = traversal_conditions<disconnected_discovery_mode::off>;
}

namespace sequoia::maths::graph_impl
{
  enum class edge_processing_mode { immediate, deferred };

  template<bool Forward> struct iterator_getter
  {
    template<network G>
    [[nodiscard]]
    constexpr static auto begin(const G& graph, const typename G::edge_index_type nodeIndex) { return graph.cbegin_edges(nodeIndex); }

    template<network G>
    [[nodiscard]]
    constexpr static auto end(const G& graph, const typename G::edge_index_type nodeIndex) { return graph.cend_edges(nodeIndex); }
  };

  template<> struct iterator_getter<false>
  {
    template<network G>
    [[nodiscard]]
    constexpr static auto begin(const G& graph, const typename G::edge_index_type nodeIndex) { return graph.crbegin_edges(nodeIndex); }

    template<network G>
    [[nodiscard]]
    constexpr static auto end(const G& graph, const typename G::edge_index_type nodeIndex) { return graph.crend_edges(nodeIndex); }
  };

  template<class Q> struct traversal_traits_base;
  template<network G, class Q> struct traversal_traits;

  template<network G> struct traversal_tracking_traits;

  template<network G, class Compare>
  class node_comparer
  {
  public:
    using compare_type = Compare;

    constexpr node_comparer(const G& g) : m_Graph(g) {}
    constexpr node_comparer(const node_comparer&) = default;

    [[nodiscard]]
    constexpr bool operator()(const std::size_t index1, const std::size_t index2)
    {
      return Compare{}(*(m_Graph.cbegin_node_weights() + index1), *(m_Graph.cbegin_node_weights() + index2));
    }
  private:
    const G& m_Graph;
  };

  template<network G, class Q> struct queue_constructor;

  template<network G, graph_flavour GraphFlavour=G::flavour>
  class loop_processor
  {
  public:
    template<class Iter>
    [[nodiscard]]
    constexpr static bool loop_matched(Iter begin, Iter current)
    {
      for(auto i{begin}; i != current; ++i)
      {
        if(&(*i) == &(*current)) return true;
      }

      return false;
    }
  };

  template<network G, graph_flavour GraphFlavour>
    requires requires() {
      std::declval<typename G::edge_type>().complementary_index();
    }
  class loop_processor<G, GraphFlavour>
  {
  public:
    template<class Iter>
    [[nodiscard]]
    constexpr static bool loop_matched(Iter begin, Iter current)
    {
      using index_type = typename G::edge_index_type;
      const auto dist{static_cast<index_type>(distance(begin, current))};
      return (current->complementary_index() < dist);
    }
  };

  template<network G>
  class loop_processor<G, graph_flavour::undirected>
  {
  public:
    template<class Iter>
    [[nodiscard]]
    constexpr bool loop_matched(Iter, Iter) noexcept
    {
      m_Matched = !m_Matched;
      return m_Matched;
    }
  private:
    bool m_Matched{true};
  };

  template<network G> struct stack_selector;

  template<network G> struct queue_selector;

  template<network G, class Compare> struct priority_queue_selector;

  template<network G, class Q>
  class traversal_helper
  {
  public:
    using edge_index_type     = typename G::edge_index_type;
    using const_edge_iterator = typename G::const_edge_iterator;

    template
    <
      disconnected_discovery_mode FindDisconnected,
      class NBEF,
      class NAEF,
      class EFTF,
      class ESTF,
      class TaskProcessingModel
    >
      requires (invocable<NBEF, edge_index_type>    )
            && (invocable<NAEF, edge_index_type>    )
            && (invocable<EFTF, const_edge_iterator>)
            && (invocable<ESTF, const_edge_iterator>)
    constexpr auto traverse(const G& graph,
                            traversal_conditions<FindDisconnected> conditions,
                            NBEF&& nodeBeforeEdgesFn,
                            NAEF&& nodeAfterEdgesFn,
                            EFTF&& edgeFirstTraversalFn,
                            ESTF&& edgeSecondTraversalFn,
                            TaskProcessingModel&& taskProcessingModel)
    {
      // Note: do not forward any of the Fns as they could in principle end up repeatedly moved from.
      // However, the Fns should not be captured by value as they may have mutable state with
      // external visibility.

      constexpr bool hasEdgeSecondFn{!same_as<std::remove_cvref_t<ESTF>, null_func_obj>};
      static_assert(!directed(G::directedness) || !hasEdgeSecondFn,
                    "For a directed graph, edges are traversed only once: the edgeSecondTraversalFn is ignored and so should be the null_func_obj");

      if(conditions.starting_index() < graph.order())
      {
        auto discovered{traversal_tracking_traits<G>::make_bitset(graph)};
        auto processed{traversal_tracking_traits<G>::make_bitset(graph)};

        auto nodeIndexQueue{queue_constructor<G, Q>::make(graph)};

        do
        {
          const auto restartNode{static_cast<edge_index_type>(conditions.compute_restart_index(discovered))};

          nodeIndexQueue.push(restartNode);
          conditions.register_discovered(discovered, restartNode);

          while(!nodeIndexQueue.empty())
          {
            const auto nodeIndex{traversal_traits<G, Q>::get_container_element(nodeIndexQueue)};
            nodeIndexQueue.pop();

            auto onDiscovery{[&nodeIndexQueue](const edge_index_type nextNode) { nodeIndexQueue.push(nextNode); }};

            inner_loop(graph,
                       nodeIndex,
                       conditions,
                       traversal_traits<G, Q>::begin(graph, nodeIndex),
                       traversal_traits<G, Q>::end(graph, nodeIndex),
                       discovered,
                       processed,
                       onDiscovery,
                       nodeBeforeEdgesFn,
                       nodeAfterEdgesFn,
                       edgeFirstTraversalFn,
                       edgeSecondTraversalFn,
                       taskProcessingModel);
          }
        } while(!conditions.terminate(graph.order()));
      }

      return taskProcessingModel.get();
    }

    template
    <
      disconnected_discovery_mode FindDisconnected,
      class NBEF,
      class NAEF,
      class EFTF,
      class ESTF,
      class TaskProcessingModel
    >
      requires (invocable<NBEF, edge_index_type>)
            && (invocable<NAEF, edge_index_type>)
            && (invocable<EFTF, const_edge_iterator>)
            && (invocable<ESTF, const_edge_iterator>)
    constexpr auto recursive_dfs(const G& graph,
                                 traversal_conditions<FindDisconnected> conditions,
                                 NBEF&& nodeBeforeEdgesFn,
                                 NAEF&& nodeAfterEdgesFn,
                                 EFTF&& edgeFirstTraversalFn,
                                 ESTF&& edgeSecondTraversalFn,
                                 TaskProcessingModel&& taskProcessingModel)
    {
      // Note: do not forward any of the Fns as they could in principle end up repeatedly moved from.
      // However, the Fns should not be captured by value as they may have mutable state with
      // external visibility.

      constexpr bool hasEdgeSecondFn{!same_as<std::remove_cvref_t<ESTF>, null_func_obj>};
      static_assert(!directed(G::directedness) || !hasEdgeSecondFn,
        "For a directed graph, edges are traversed only once: the edgeSecondTraversalFn is ignored and so should be the null_func_obj");

      if(conditions.starting_index() < graph.order())
      {
        auto discovered{traversal_tracking_traits<G>::make_bitset(graph)};
        auto processed{traversal_tracking_traits<G>::make_bitset(graph)};

        do
        {
          const auto restartNode{static_cast<edge_index_type>(conditions.compute_restart_index(discovered))};
          conditions.register_discovered(discovered, restartNode);

          inner_loop(graph,
                     restartNode,
                     conditions,
                     graph.cbegin_edges(restartNode),
                     graph.cend_edges(restartNode),
                     discovered,
                     processed,
                     recurse{},
                     nodeBeforeEdgesFn,
                     nodeAfterEdgesFn,
                     edgeFirstTraversalFn,
                     edgeSecondTraversalFn,
                     taskProcessingModel);

        } while(!conditions.terminate(graph.order()));
      }

      return taskProcessingModel.get();
    }
  private:
    struct recurse {};

    template<class Iter>
    [[nodiscard]]
    constexpr static bool is_loop(Iter iter, [[maybe_unused]] const edge_index_type currentNodeIndex)
    {
      if constexpr(G::flavour == graph_flavour::directed_embedded)
        return iter->target_node() == iter->source_node();
      else
        return iter->target_node() == currentNodeIndex;
    }

    template
    <
      disconnected_discovery_mode FindDisconnected,
      class Iter,
      class Bitset,
      class OnDiscovery,
      class NBEF,
      class NAEF,
      class EFTF,
      class ESTF,
      class TaskProcessingModel
    >
      requires (invocable<NBEF, edge_index_type>)
            && (invocable<NAEF, edge_index_type>)
            && (invocable<EFTF, const_edge_iterator>)
            && (invocable<ESTF, const_edge_iterator>)
            && (invocable<OnDiscovery, edge_index_type> || same_as<OnDiscovery, recurse>)
      constexpr void inner_loop([[maybe_unused]] const G& graph,
                                const edge_index_type nodeIndex,
                                traversal_conditions<FindDisconnected>& conditions,
                                Iter begin,
                                Iter end,
                                Bitset& discovered,
                                Bitset& processed,
                                OnDiscovery onDiscovery,
                                NBEF&& nodeBeforeEdgesFn,
                                NAEF&& nodeAfterEdgesFn,
                                EFTF&& edgeFirstTraversalFn,
                                ESTF&& edgeSecondTraversalFn,
                                TaskProcessingModel&& taskProcessingModel)
    {
      constexpr bool hasNodeBeforeFn{!same_as<std::remove_cvref_t<NBEF>, null_func_obj>};
      if constexpr(hasNodeBeforeFn)
      {
        taskProcessingModel.push(nodeBeforeEdgesFn, nodeIndex);
      }

      [[maybe_unused]] loop_processor<G> loops{};
      for(auto iter{begin}; iter != end; ++iter)
      {
        const auto nextNode{iter->target_node()};
        constexpr bool hasEdgeFirstFn{!same_as<std::remove_cvref_t<EFTF>, null_func_obj>};

        if constexpr(G::flavour != graph_flavour::directed)
        {
          constexpr bool hasEdgeSecondFn{!same_as<std::remove_cvref_t<ESTF>, null_func_obj>};
          if constexpr(G::flavour == graph_flavour::directed_embedded)
          {
            if(is_loop(iter, nodeIndex))
            {
              const bool loopMatched{loops.loop_matched(begin, iter)};
              if((iter->inverted() && !loopMatched) || (!iter->inverted() && loopMatched)) continue;
            }
            else
            {
              if(iter->source_node() != nodeIndex) continue;
            }

            if constexpr(hasEdgeFirstFn)
            {
              taskProcessingModel.push(edgeFirstTraversalFn, iter);
            }
          }
          else if constexpr(hasEdgeFirstFn || hasEdgeSecondFn)
          {
            const bool loopMatched{is_loop(iter, nodeIndex) && loops.loop_matched(begin, iter)};
            const bool secondTraversal{processed[nextNode] || loopMatched};
            if(secondTraversal)
            {
              if constexpr(hasEdgeSecondFn)
              {
                taskProcessingModel.push(edgeSecondTraversalFn, iter);
              }
            }
            else
            {
              if constexpr(hasEdgeFirstFn)
              {
                taskProcessingModel.push(edgeFirstTraversalFn, iter);
              }
            }
          }
        }
        else if constexpr(hasEdgeFirstFn)
        {
          taskProcessingModel.push(edgeFirstTraversalFn, iter);
        }

        if(!discovered[nextNode])
        {
          conditions.register_discovered(discovered, nextNode);
          if constexpr(same_as<OnDiscovery, recurse>)
          {
            inner_loop(graph,
                       nextNode,
                       conditions,
                       graph.cbegin_edges(nextNode),
                       graph.cend_edges(nextNode),
                       discovered,
                       processed,
                       recurse{},
                       nodeBeforeEdgesFn,
                       nodeAfterEdgesFn,
                       edgeFirstTraversalFn,
                       edgeSecondTraversalFn,
                       taskProcessingModel);
          }
          else
          {
            onDiscovery(nextNode);
          }
        }
      }

      if constexpr(!same_as<std::remove_cvref_t<NAEF>, null_func_obj>)
      {
        taskProcessingModel.push(nodeAfterEdgesFn, nodeIndex);
      }

      processed[nodeIndex] = true;
    }
  };
}
