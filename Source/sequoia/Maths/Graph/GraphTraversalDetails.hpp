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
  private:
  };

  template<>
  class traversal_conditions<disconnected_discovery_mode::off> : public traversal_conditions_base
  {
  public:
    constexpr static disconnected_discovery_mode mode{disconnected_discovery_mode::off};

    using traversal_conditions_base::traversal_conditions_base;

    constexpr traversal_conditions(const traversal_conditions&) = default;
    constexpr traversal_conditions& operator=(const traversal_conditions&) = default;
  };

  using find_disconnected_t = traversal_conditions<disconnected_discovery_mode::on>;

  using ignore_disconnected_t = traversal_conditions<disconnected_discovery_mode::off>;
}

namespace sequoia::maths::graph_impl
{
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

    constexpr void reset() noexcept {}
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

    constexpr void reset() noexcept {}
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

    constexpr void reset() noexcept
    {
      m_Matched = true;
    }
  private:
    bool m_Matched{true};
  };

  template<network G> struct stack_selector;

  template<network G> struct queue_selector;

  template<network G, class Compare> struct priority_queue_selector;

  template<network G>
  class traversal_helper
  {
  public:
    using edge_index_type     = typename G::edge_index_type;
    using const_edge_iterator = typename G::const_edge_iterator;

    template
    <
      class Q,
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
        auto discovered{traversal_traits<G, Q>::make_bitset(graph)};
        auto processed{traversal_traits<G, Q>::make_bitset(graph)};

        edge_index_type numDiscovered{}, restart{};

        using namespace graph_impl;
        auto nodeIndexQueue{queue_constructor<G, Q>::make(graph)};
        using container_type = decltype(nodeIndexQueue);

        do
        {
          while(discovered[restart]) ++restart;

          const std::size_t startNode{numDiscovered ? restart : conditions.starting_index()};
          nodeIndexQueue.push(startNode);
          discovered[startNode] = true;
          ++numDiscovered;

          while(!nodeIndexQueue.empty())
          {
            const auto nodeIndex{traversal_traits<G, container_type>::get_container_element(nodeIndexQueue)};

            nodeIndexQueue.pop();

            constexpr bool hasNodeBeforeFn{!same_as<std::remove_cvref_t<NBEF>, null_func_obj>};
            if constexpr(hasNodeBeforeFn)
            {
              taskProcessingModel.push(nodeBeforeEdgesFn, nodeIndex);
            }

            m_Loops.reset();
            for(auto iter{traversal_traits<G, container_type>::begin(graph, nodeIndex)}; iter != traversal_traits<G, container_type>::end(graph, nodeIndex); ++iter)
            {
              const auto nextNode{iter->target_node()};
              constexpr bool hasEdgeFirstFn{!same_as<std::remove_cvref_t<EFTF>, null_func_obj>};

              if constexpr(G::flavour != graph_flavour::directed)
              {
                if constexpr(G::flavour == graph_flavour::directed_embedded)
                {
                  if(is_loop(iter, nodeIndex))
                  {
                    const bool loopMatched{m_Loops.loop_matched(traversal_traits<G, container_type>::begin(graph, nodeIndex), iter)};
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
                else
                {
                  if constexpr(hasEdgeFirstFn || hasEdgeSecondFn)
                  {
                    const bool loopMatched{is_loop(iter, nodeIndex) && m_Loops.loop_matched(traversal_traits<G, container_type>::begin(graph, nodeIndex), iter)};
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
              }
              else if constexpr(hasEdgeFirstFn)
              {
                taskProcessingModel.push(edgeFirstTraversalFn, iter);
              }

              if(!discovered[nextNode])
              {
                nodeIndexQueue.push(nextNode);
                discovered[nextNode] = true;
                ++numDiscovered;
              }
            }

            if constexpr(!same_as<std::remove_cvref_t<NAEF>, null_func_obj>)
            {
              taskProcessingModel.push(nodeAfterEdgesFn, nodeIndex);
            }

            processed[nodeIndex] = true;
          }
        } while((conditions.mode == disconnected_discovery_mode::on) && (numDiscovered != graph.order()));
      }

      return taskProcessingModel.get();
    }
    private:
      [[no_unique_address]]
      loop_processor<G> m_Loops;

      template<class Iter>
      [[nodiscard]]
      constexpr static bool is_loop(Iter iter, [[maybe_unused]] const edge_index_type currentNodeIndex) {
        if constexpr(G::flavour == graph_flavour::directed_embedded)
          return iter->target_node() == iter->source_node();
        else
          return iter->target_node() == currentNodeIndex;
      }
  };
}
