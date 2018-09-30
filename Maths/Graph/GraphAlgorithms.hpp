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
  
  template
  <    
    class Q,
    class G,
    class NFBE,
    class NFAE,
    class EFTF,
    class ESTF,    
    class TaskProcessingModel
  >
  auto basic_traversal(const G& graph, const bool findDisconnectedPieces,
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
        auto nodeIndexQueue = queue_constructor<G, Q>::make(graph);
        using container_type = decltype(nodeIndexQueue);
        nodeIndexQueue.push(restart);
        discovered[restart] = true;
        ++numDiscovered;

        while(!nodeIndexQueue.empty())
        {
          const std::size_t nodeIndex{TraversalTraits<container_type>::get_container_element(nodeIndexQueue)};

          nodeIndexQueue.pop();

          node_functor_processor<NFBE>::process(taskProcessingModel, std::forward<NFBE>(nodeFunctorBeforeEdges), nodeIndex);
             
          bool loopMatched{true};
          for(auto iter = TraversalTraits<container_type>::begin(graph, nodeIndex); iter != TraversalTraits<container_type>::end(graph, nodeIndex); ++iter)
          {               
            // Needs fixing for embedded
               
            const auto loop = [iter](const std::size_t currentNodeIndex){
              if constexpr (directed(G::directedness) && G::store_incident_edges_v)
                return iter->target_node() == iter->host_node();
              else
                return iter->target_node() == currentNodeIndex;
            }(nodeIndex);
               
            if(loop)
            {
              loopMatched = !loopMatched;
              if constexpr (directed(G::directedness) && G::store_incident_edges_v)
              {
                if(loopMatched) continue;
              }
            }
            else if constexpr (directed(G::directedness) && G::store_incident_edges_v)
            {
              if(iter->host_node() != nodeIndex) continue;
            }
               
            const auto nextNode = iter->target_node();               
            const bool secondTraversal{!directed(G::directedness) && ((discovered[nextNode] && processed[nextNode]) || (loop && loopMatched))};
            if(secondTraversal) edge_functor_processor<ESTF>::process(taskProcessingModel, std::forward<ESTF>(edgeSecondTraversalFunctor), iter);
            else                edge_functor_processor<EFTF>::process(taskProcessingModel, std::forward<EFTF>(edgeFirstTraversalFunctor), iter);

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
    return graph_impl::basic_traversal<std::queue<std::size_t>>(
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
    return graph_impl::basic_traversal<std::queue<std::size_t>>(
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
    return graph_impl::basic_traversal<std::stack<std::size_t>>(
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
    return graph_impl::basic_traversal<std::stack<std::size_t>>(
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
    return graph_impl::basic_traversal<std::priority_queue<std::size_t, std::vector<std::size_t>, QCompare>>(
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
    return graph_impl::basic_traversal<std::priority_queue<std::size_t, std::vector<std::size_t>, QCompare>>(
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
        subGraph.delete_node(pos);
      }
      else
      {
        ++pos;
      }
    }

    return subGraph;
  }
}
