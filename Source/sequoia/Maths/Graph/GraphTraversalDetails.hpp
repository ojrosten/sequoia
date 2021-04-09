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
  enum class graph_flavour;
  struct null_functor{};
}

namespace sequoia::maths::graph_impl
{
  template<bool Forward> struct iterator_getter
  {
    template<class G>
    [[nodiscard]]
    constexpr static auto begin(const G& graph, const std::size_t nodeIndex) { return graph.cbegin_edges(nodeIndex); }

    template<class G>
    [[nodiscard]]
    constexpr static auto end(const G& graph, const std::size_t nodeIndex) { return graph.cend_edges(nodeIndex); }
  };

  template<> struct iterator_getter<false>
  {
    template<class G>
    [[nodiscard]]
    constexpr static auto begin(const G& graph, const std::size_t nodeIndex) { return graph.crbegin_edges(nodeIndex); }

    template<class G>
    [[nodiscard]]
    constexpr static auto end(const G& graph, const std::size_t nodeIndex) { return graph.crend_edges(nodeIndex); }
  };

  template<class Q> struct traversal_traits_base;

  template<network G, class Q>
  struct traversal_traits : public traversal_traits_base<Q>
  {
    [[nodiscard]]
    constexpr static auto begin(const G& graph, const std::size_t nodeIndex)
    {
      return iterator_getter<traversal_traits_base<Q>::uses_forward_iterator()>::begin(graph, nodeIndex);
    }

    [[nodiscard]]
    constexpr static auto end(const G& graph, const std::size_t nodeIndex)
    {
      return iterator_getter<traversal_traits_base<Q>::uses_forward_iterator()>::end(graph, nodeIndex);
    }

    using bitset = std::array<bool, G::order()>;

    [[nodiscard]]
    constexpr static bitset make_bitset(const G&)
    {
      return bitset{};
    }
  };

  template<dynamic_network G, class Q>
  struct traversal_traits<G, Q> : public traversal_traits_base<Q>
  {
    [[nodiscard]]
    static auto begin(const G& graph, const std::size_t nodeIndex)
    {
      return iterator_getter<traversal_traits_base<Q>::uses_forward_iterator()>::begin(graph, nodeIndex);
    }

    [[nodiscard]]
    static auto end(const G& graph, const std::size_t nodeIndex)
    {
      return iterator_getter<traversal_traits_base<Q>::uses_forward_iterator()>::end(graph, nodeIndex);
    }

    using bitset = std::vector<bool>;

    [[nodiscard]]
    static bitset make_bitset(const G& g)
    {
      return bitset(g.order(), false);
    }
  };


  template<class G, class Compare>
  class node_comparer
  {
  public:
    using compare_type = Compare;

    constexpr node_comparer(const G& g) : m_Graph(g) {}
    constexpr node_comparer(const node_comparer&) = default;

    [[nodiscard]]
    constexpr bool operator()(const std::size_t index1, const std::size_t index2)
    {
      Compare comparer{};
      return comparer(*(m_Graph.cbegin_node_weights() + index1), *(m_Graph.cbegin_node_weights() + index2));
    }
  private:
    const G& m_Graph;
  };

  template<class G, class Q> struct queue_constructor;

  template<class NodeFunctor>
  struct node_functor_processor
  {
    template<class ProcessingModel, class... Args>
    constexpr static void process(ProcessingModel& model, NodeFunctor&& nodeFunctor, Args... args)
    {
      model.push(std::forward<NodeFunctor>(nodeFunctor), args...);
    }
  };

  template<>
  struct node_functor_processor<null_functor>
  {
    template<class ProcessingModel, class... Args>
    constexpr static void process(ProcessingModel&, null_functor, Args...) {}
  };

  template<>
  struct node_functor_processor<null_functor&>
  {
    template<class ProcessingModel, class... Args>
    constexpr static void process(ProcessingModel&, null_functor, Args...) {}
  };

  template<>
  struct node_functor_processor<const null_functor&>
  {
    template<class ProcessingModel, class... Args>
    constexpr static void process(ProcessingModel&, null_functor, Args...) {}
  };

  template<class EdgeFunctor>
  struct edge_functor_processor
  {
    template<class ProcessingModel, class... Args>
    constexpr static void process(ProcessingModel& model, EdgeFunctor&& edgeFunctor, Args... args)
    {
      model.push(std::forward<EdgeFunctor>(edgeFunctor), args...);
    }
  };

  template<>
  struct edge_functor_processor<null_functor>
  {
    template<class ProcessingModel, class... Args>
    constexpr static void process(ProcessingModel&, null_functor, Args...) {}
  };

  template<>
  struct edge_functor_processor<null_functor&>
  {
    template<class ProcessingModel, class... Args>
    constexpr static void process(ProcessingModel&, null_functor, Args...) {}
  };

  template<>
  struct edge_functor_processor<const null_functor&>
  {
    template<class ProcessingModel, class... Args>
    constexpr static void process(ProcessingModel&, null_functor, Args...) {}
  };

  template<class G, graph_flavour GraphFlavour=G::flavour>
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

  template<class G, graph_flavour GraphFlavour>
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

  template<class G>
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

  template<class G> struct stack_selector;

  template<class G> struct queue_selector;

  template<class G, class Compare> struct priority_queue_selector;


  template<class G>
  class traversal_helper : private loop_processor<G>
  {
  public:
    template<
      class Q,
      class NFBE,
      class NFAE,
      class EFTF,
      class ESTF,
      class TaskProcessingModel
    >
    constexpr auto traverse(const G& graph,
                            const bool findDisconnectedPieces,
                            std::size_t start,
                            NFBE&& nodeFunctorBeforeEdges,
                            NFAE&& nodeFunctorAfterEdges,
                            EFTF&& edgeFirstTraversalFunctor,
                            ESTF&& edgeSecondTraversalFunctor,
                            TaskProcessingModel&& taskProcessingModel)
    {
      static_assert(!directed(G::directedness) || std::is_same<std::decay_t<ESTF>, null_functor>::value, "For a directed graph, edges are traversed only once: the edgeSecondTraversalFunctor is ignored and so should be the null_functor");

      if(start < graph.order())
      {
        auto discovered{traversal_traits<G, Q>::make_bitset(graph)};
        auto processed{traversal_traits<G, Q>::make_bitset(graph)};

        std::size_t numDiscovered{}, restart{};

        using namespace graph_impl;
        auto nodeIndexQueue{queue_constructor<G, Q>::make(graph)};
        using container_type = decltype(nodeIndexQueue);

        do
        {
          while(discovered[restart]) ++restart;

          const std::size_t startNode{numDiscovered ? restart : start};
          nodeIndexQueue.push(startNode);
          discovered[startNode] = true;
          ++numDiscovered;

          while(!nodeIndexQueue.empty())
          {
            const std::size_t nodeIndex{traversal_traits<G, container_type>::get_container_element(nodeIndexQueue)};

            nodeIndexQueue.pop();

            node_functor_processor<NFBE>::process(taskProcessingModel, std::forward<NFBE>(nodeFunctorBeforeEdges), nodeIndex);

            this->reset();
            for(auto iter{traversal_traits<G, container_type>::begin(graph, nodeIndex)}; iter != traversal_traits<G, container_type>::end(graph, nodeIndex); ++iter)
            {
              const auto nextNode{iter->target_node()};

              if constexpr(G::flavour != graph_flavour::directed)
              {
                const bool loop{[iter]([[maybe_unused]] const std::size_t currentNodeIndex){
                    if constexpr (G::flavour == graph_flavour::directed_embedded)
                      return iter->target_node() == iter->source_node();
                    else
                      return iter->target_node() == currentNodeIndex;
                  }(nodeIndex)
                };

                if constexpr(G::flavour == graph_flavour::directed_embedded)
                {
                  if(loop)
                  {
                    const bool loopMatched{this->loop_matched(traversal_traits<G, container_type>::begin(graph, nodeIndex), iter)};
                    if((iter->inverted() && !loopMatched) || (!iter->inverted() && loopMatched)) continue;
                  }
                  else
                  {
                    if(iter->source_node() != nodeIndex) continue;
                  }

                  edge_functor_processor<EFTF>::process(taskProcessingModel, std::forward<EFTF>(edgeFirstTraversalFunctor), iter);
                }
                else
                {
                  const bool loopMatched{loop && this->loop_matched(traversal_traits<G, container_type>::begin(graph, nodeIndex), iter)};
                  const bool secondTraversal{processed[nextNode] || loopMatched};
                  if(secondTraversal) edge_functor_processor<ESTF>::process(taskProcessingModel, std::forward<ESTF>(edgeSecondTraversalFunctor), iter);
                  else                edge_functor_processor<EFTF>::process(taskProcessingModel, std::forward<EFTF>(edgeFirstTraversalFunctor), iter);
                }
              }
              else
              {
                edge_functor_processor<EFTF>::process(taskProcessingModel, std::forward<EFTF>(edgeFirstTraversalFunctor), iter);
              }

              if(!discovered[nextNode])
              {
                nodeIndexQueue.push(nextNode);
                discovered[nextNode] = true;
                ++numDiscovered;
              }
            }

            node_functor_processor<NFAE>::process(taskProcessingModel, std::forward<NFAE>(nodeFunctorAfterEdges), nodeIndex);
            processed[nodeIndex] = true;
          }
        } while(findDisconnectedPieces && (numDiscovered != graph.order()));
      }

      return taskProcessingModel.get();
    }
  };
}
