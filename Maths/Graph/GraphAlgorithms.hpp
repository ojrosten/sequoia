#pragma once

#include "DynamicGraph.hpp"
// #include "Graph.hpp" 
#include "GraphAlgorithmDetails.hpp"
#include "ThreadingModels.hpp"


namespace sequoia::maths
{
  struct null_functor{};
}

namespace sequoia::maths::graph_impl
{
  template<class NodeFunctor>
  struct node_functor_processor
  {
    template<class ProcessingModel, class... Args>
    static void process(ProcessingModel& model, NodeFunctor&& nodeFunctor, Args... args)
    {
      model.push(std::forward<NodeFunctor>(nodeFunctor), args...);
    }
  };

  template<>
  struct node_functor_processor<null_functor>
  {
    template<class ProcessingModel, class... Args>
    static void process(ProcessingModel& model, null_functor nodeFunctor, Args... args) {}
  };

  template<>
  struct node_functor_processor<null_functor&>
  {
    template<class ProcessingModel, class... Args>
    static void process(ProcessingModel& model, null_functor nodeFunctor, Args... args) {}
  };

  template<>
  struct node_functor_processor<const null_functor&>
  {
    template<class ProcessingModel, class... Args>
    static void process(ProcessingModel& model, null_functor nodeFunctor, Args... args) {}
  };

  template<class EdgeFunctor>
  struct edge_functor_processor
  {
    template<class ProcessingModel, class... Args>
    static void process(ProcessingModel& model, EdgeFunctor&& edgeFunctor, Args... args)
    {
      model.push(std::forward<EdgeFunctor>(edgeFunctor), args...);
    }
  };

  template<>
  struct edge_functor_processor<null_functor>
  {
    template<class ProcessingModel, class... Args>
    static void process(ProcessingModel& model, null_functor edgeFunctor, Args... args) {}
  };

  template<>
  struct edge_functor_processor<null_functor&>
  {
    template<class ProcessingModel, class... Args>
    static void process(ProcessingModel& model, null_functor edgeFunctor, Args... args) {}
  };

  template<>
  struct edge_functor_processor<const null_functor&>
  {
    template<class ProcessingModel, class... Args>
    static void process(ProcessingModel& model, null_functor edgeFunctor, Args... args) {}
  };

  template<class G, class = void> struct comp_index_detector : std::false_type
  {
  };

  template<class G> struct comp_index_detector<G, std::void_t<decltype(std::declval<typename G::edge_type>().complementary_index())>> : std::true_type
  {
  };

  template<class G> constexpr bool comp_index_detector_v{comp_index_detector<G>::value};

  template<class G, maths::graph_flavour GraphFlavour=G::flavour> class loop_processor
  {
  public:
    template<class Iter>
    constexpr static bool loop_matched(Iter begin, Iter current)
    {
      if constexpr (comp_index_detector_v<G>)
      {
        return (current->complementary_index() < distance(begin, current));
      }
      else
      {
        for(auto i{begin}; i != current; ++i)
        {
          if(&(*i) == &(*current)) return true;
        }

        return false;
      }
    }

    constexpr void reset() noexcept {}
  };

  template<class G>
  class loop_processor<G, maths::graph_flavour::undirected>
  {
  public:
    template<class Iter>
    constexpr bool loop_matched(Iter begin, Iter current) noexcept
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
    auto traverse(const G& graph, const bool findDisconnectedPieces,
                 const std::size_t start,
                 NFBE&& nodeFunctorBeforeEdges,
                 NFAE&& nodeFunctorAfterEdges,
                 EFTF&& edgeFirstTraversalFunctor,
                 ESTF&& edgeSecondTraversalFunctor,
                 TaskProcessingModel&& taskProcessingModel)
    {
      static_assert(!directed(G::directedness) || std::is_same<std::decay_t<ESTF>, null_functor>::value, "For a directed graph, edges are traversed only once: the edgeSecondTraversalFunctor is ignored and so should be the null_functor");

      if(start < graph.order())
      {
        std::vector<bool> discovered(graph.order(), false),
                          processed(graph.order(), false);
        std::size_t numDiscovered{};

        do
        {
          // Probably want to replace
          // findDisconnectedPieces and start
          // with a seeding policy.
          std::size_t restart{start};
          if(discovered[start])
          {
            restart = 0;
            for(; restart < graph.order(); ++restart)
            {
              if(!discovered[restart]) break;
            }
          }

          using namespace graph_impl;
          auto nodeIndexQueue{queue_constructor<G, Q>::make(graph)};
          using container_type = decltype(nodeIndexQueue);
          nodeIndexQueue.push(restart);
          discovered[restart] = true;
          ++numDiscovered;

          while(!nodeIndexQueue.empty())
          {
            const std::size_t nodeIndex{TraversalTraits<container_type>::get_container_element(nodeIndexQueue)};

            nodeIndexQueue.pop();

            node_functor_processor<NFBE>::process(taskProcessingModel, std::forward<NFBE>(nodeFunctorBeforeEdges), nodeIndex);

            constexpr bool embedded{(G::flavour == graph_flavour::directed_embedded) || (G::flavour == graph_flavour::undirected_embedded)};            
            this->reset();
            for(auto iter{TraversalTraits<container_type>::begin(graph, nodeIndex)}; iter != TraversalTraits<container_type>::end(graph, nodeIndex); ++iter)
            {
              const auto nextNode{iter->target_node()}; 
              
              if constexpr(G::flavour != maths::graph_flavour::directed)
              {
                const bool loop{[iter](const std::size_t currentNodeIndex){
                    if constexpr (directed(G::directedness) && embedded)
                      return iter->target_node() == iter->host_node();
                    else
                      return iter->target_node() == currentNodeIndex;
                  }(nodeIndex)
                };

                if constexpr(G::flavour == graph_flavour::directed_embedded)
                {
                  if(loop)
                  {
                    const bool loopMatched{this->loop_matched(TraversalTraits<container_type>::begin(graph, nodeIndex), iter)};
                    if((iter->inverted() && !loopMatched) || (!iter->inverted() && loopMatched)) continue;
                  }
                  else
                  {
                    if(iter->host_node() != nodeIndex) continue;
                  }

                  edge_functor_processor<EFTF>::process(taskProcessingModel, std::forward<EFTF>(edgeFirstTraversalFunctor), iter);
                }
                else
                {
                  const bool loopMatched{loop && this->loop_matched(TraversalTraits<container_type>::begin(graph, nodeIndex), iter)};
                  const bool secondTraversal{((discovered[nextNode] && processed[nextNode]) || loopMatched)};
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

namespace sequoia::maths
{  
  template
  <    
    class TaskProcessingModel = concurrency::serial<void>,
    class G = void,
    class NFBE = null_functor,
    class NFAE = null_functor,
    class EFTF = null_functor,
    class ESTF = null_functor,
    class = std::enable_if_t<G::directedness != directed_flavour::directed>
  >
  auto breadth_first_search(const G& graph, const bool findDisconnectedPieces = true,
                 const std::size_t start = 0,
                 NFBE&& nodeFunctorBeforeEdges     = null_functor{},
                 NFAE&& nodeFunctorAfterEdges      = null_functor{},
                 EFTF&& edgeFirstTraversalFunctor  = null_functor{},
                 ESTF&& edgeSecondTraversalFunctor = null_functor{},
                 TaskProcessingModel&& taskProcessingModel = TaskProcessingModel{})
  {
    return graph_impl::traversal_helper<G>{}.template traverse<std::queue<std::size_t>>(
             graph,
             findDisconnectedPieces,
             start,
             std::forward<NFBE>(nodeFunctorBeforeEdges),
             std::forward<NFAE>(nodeFunctorAfterEdges),
             std::forward<EFTF>(edgeFirstTraversalFunctor),
             std::forward<ESTF>(edgeSecondTraversalFunctor),
             std::forward<TaskProcessingModel>(taskProcessingModel)
           );
  }

  template
  <    
    class TaskProcessingModel = concurrency::serial<void>,
    class G = void,
    class NFBE = null_functor,
    class NFAE = null_functor,
    class EFTF = null_functor,
    class = std::enable_if_t<G::directedness == directed_flavour::directed>
  >
  auto breadth_first_search(const G& graph, const bool findDisconnectedPieces = true,
                 const std::size_t start = 0,
                 NFBE&& nodeFunctorBeforeEdges     = null_functor{},
                 NFAE&& nodeFunctorAfterEdges      = null_functor{},
                 EFTF&& edgeFirstTraversalFunctor  = null_functor{},
                 TaskProcessingModel&& taskProcessingModel = TaskProcessingModel{})
  {
    return graph_impl::traversal_helper<G>{}.template traverse<std::queue<std::size_t>>(
             graph,
             findDisconnectedPieces,
             start,
             std::forward<NFBE>(nodeFunctorBeforeEdges),
             std::forward<NFAE>(nodeFunctorAfterEdges),
             std::forward<EFTF>(edgeFirstTraversalFunctor),
             null_functor{},
             std::forward<TaskProcessingModel>(taskProcessingModel)
           );
  }

  template
  <    
    class TaskProcessingModel = concurrency::serial<void>,
    class G = void,
    class NFBE = null_functor,
    class NFAE = null_functor,
    class EFTF = null_functor,
    class ESTF = null_functor,
    class = std::enable_if_t<G::directedness != directed_flavour::directed>
  >
  auto depth_first_search(const G& graph, const bool findDisconnectedPieces = true,
                 const std::size_t start = 0,
                 NFBE&& nodeFunctorBeforeEdges     = null_functor{},
                 NFAE&& nodeFunctorAfterEdges      = null_functor{},
                 EFTF&& edgeFirstTraversalFunctor  = null_functor{},
                 ESTF&& edgeSecondTraversalFunctor = null_functor{},
                 TaskProcessingModel&& taskProcessingModel = TaskProcessingModel{})
  {
    return graph_impl::traversal_helper<G>{}.template traverse<std::stack<std::size_t>>(
             graph,
             findDisconnectedPieces,
             start,
             std::forward<NFBE>(nodeFunctorBeforeEdges),
             std::forward<NFAE>(nodeFunctorAfterEdges),
             std::forward<EFTF>(edgeFirstTraversalFunctor),
             std::forward<ESTF>(edgeSecondTraversalFunctor),
             std::forward<TaskProcessingModel>(taskProcessingModel)
           );
  }

  template
  <    
    class TaskProcessingModel = concurrency::serial<void>,
    class G = void,
    class NFBE = null_functor,
    class NFAE = null_functor,
    class EFTF = null_functor,
    class = std::enable_if_t<G::directedness == directed_flavour::directed>
  >
  auto depth_first_search(const G& graph, const bool findDisconnectedPieces = true,
                 const std::size_t start = 0,
                 NFBE&& nodeFunctorBeforeEdges     = null_functor{},
                 NFAE&& nodeFunctorAfterEdges      = null_functor{},
                 EFTF&& edgeFirstTraversalFunctor  = null_functor{},
                 TaskProcessingModel&& taskProcessingModel = TaskProcessingModel{})
  {
    return graph_impl::traversal_helper<G>{}.template traverse<std::stack<std::size_t>>(
             graph,
             findDisconnectedPieces,
             start,
             std::forward<NFBE>(nodeFunctorBeforeEdges),
             std::forward<NFAE>(nodeFunctorAfterEdges),
             std::forward<EFTF>(edgeFirstTraversalFunctor),
             null_functor{},
             std::forward<TaskProcessingModel>(taskProcessingModel)
           );
  }

  template
  <    
    class TaskProcessingModel = concurrency::serial<void>,
    class G = void,
    class NFBE = null_functor,
    class NFAE = null_functor,
    class EFTF = null_functor,
    class ESTF = null_functor,
    class QCompare = graph_impl::node_comparer<G, std::less<typename G::node_weight_type>>,
    class = std::enable_if_t<G::directedness != directed_flavour::directed>
  >
  auto priority_search(const G& graph, const bool findDisconnectedPieces = true,
                 const std::size_t start = 0,
                 NFBE&& nodeFunctorBeforeEdges     = null_functor{},
                 NFAE&& nodeFunctorAfterEdges      = null_functor{},
                 EFTF&& edgeFirstTraversalFunctor  = null_functor{},
                 ESTF&& edgeSecondTraversalFunctor = null_functor{},
                 TaskProcessingModel&& taskProcessingModel = TaskProcessingModel{})
  {
    return graph_impl::traversal_helper<G>{}.template traverse<std::priority_queue<std::size_t, std::vector<std::size_t>, QCompare>>(
             graph,
             findDisconnectedPieces,
             start,
             std::forward<NFBE>(nodeFunctorBeforeEdges),
             std::forward<NFAE>(nodeFunctorAfterEdges),
             std::forward<EFTF>(edgeFirstTraversalFunctor),
             std::forward<ESTF>(edgeSecondTraversalFunctor),
             std::forward<TaskProcessingModel>(taskProcessingModel)
           );
  }

  template
  <    
    class TaskProcessingModel = concurrency::serial<void>,
    class G = void,
    class NFBE = null_functor,
    class NFAE = null_functor,
    class EFTF = null_functor,
    class QCompare = graph_impl::node_comparer<G, std::less<typename G::node_weight_type>>,
    class = std::enable_if_t<G::directedness == directed_flavour::directed>
  >
  auto priority_search(const G& graph, const bool findDisconnectedPieces = true,
                 const std::size_t start = 0,
                 NFBE&& nodeFunctorBeforeEdges     = null_functor{},
                 NFAE&& nodeFunctorAfterEdges      = null_functor{},
                 EFTF&& edgeFirstTraversalFunctor  = null_functor{},
                 TaskProcessingModel&& taskProcessingModel = TaskProcessingModel{})
  {
    return graph_impl::traversal_helper<G>{}.template traverse<std::priority_queue<std::size_t, std::vector<std::size_t>, QCompare>>(
             graph,
             findDisconnectedPieces,
             start,
             std::forward<NFBE>(nodeFunctorBeforeEdges),
             std::forward<NFAE>(nodeFunctorAfterEdges),
             std::forward<EFTF>(edgeFirstTraversalFunctor),
             null_functor{},
             std::forward<TaskProcessingModel>(taskProcessingModel)
           );
  }

  template<class G, class Pred>
  G sub_graph(const G& g, Pred nodePred)
  {
    G subGraph{g};

    std::size_t pos{};
    while(pos < subGraph.order())
    {
      if(!nodePred(*(subGraph.cbegin_node_weights() + pos)))
      {
        subGraph.erase_node(pos);
      }
      else
      {
        ++pos;
      }
    }

    return subGraph;
  }
}
