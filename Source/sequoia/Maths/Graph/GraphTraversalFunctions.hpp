////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Breadth first, depth first and priority searches.

 */

#include "sequoia/Maths/Graph/GraphTraversalDetails.hpp"
#include "sequoia/Core/Concurrency/ConcurrencyModels.hpp"

namespace sequoia::maths
{
  template
  <
    class TaskProcessingModel = concurrency::serial<void>,
    class G = void,
    class NFBE = null_functor,
    class NFAE = null_functor,
    class EFTF = null_functor,
    class ESTF = null_functor
  >
    requires (G::directedness != directed_flavour::directed)
  constexpr auto breadth_first_search(const G& graph, const bool findDisconnectedPieces = true,
                 const std::size_t start = 0,
                 NFBE&& nodeFunctorBeforeEdges     = null_functor{},
                 NFAE&& nodeFunctorAfterEdges      = null_functor{},
                 EFTF&& edgeFirstTraversalFunctor  = null_functor{},
                 ESTF&& edgeSecondTraversalFunctor = null_functor{},
                 TaskProcessingModel&& taskProcessingModel = TaskProcessingModel{})
  {
    using queue_type = typename graph_impl::queue_selector<G>::queue_type;
    return graph_impl::traversal_helper<G>{}.template traverse<queue_type>(
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
    class EFTF = null_functor
  >
    requires (G::directedness == directed_flavour::directed)
  constexpr auto breadth_first_search(const G& graph, const bool findDisconnectedPieces = true,
                 const std::size_t start = 0,
                 NFBE&& nodeFunctorBeforeEdges     = null_functor{},
                 NFAE&& nodeFunctorAfterEdges      = null_functor{},
                 EFTF&& edgeFirstTraversalFunctor  = null_functor{},
                 TaskProcessingModel&& taskProcessingModel = TaskProcessingModel{})
  {
    using queue_type = typename graph_impl::queue_selector<G>::queue_type;
    return graph_impl::traversal_helper<G>{}.template traverse<queue_type>(
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
    class ESTF = null_functor
  >
    requires (G::directedness != directed_flavour::directed)
  constexpr auto pseudo_depth_first_search(const G& graph, const bool findDisconnectedPieces = true,
                 const std::size_t start = 0,
                 NFBE&& nodeFunctorBeforeEdges     = null_functor{},
                 NFAE&& nodeFunctorAfterEdges      = null_functor{},
                 EFTF&& edgeFirstTraversalFunctor  = null_functor{},
                 ESTF&& edgeSecondTraversalFunctor = null_functor{},
                 TaskProcessingModel&& taskProcessingModel = TaskProcessingModel{})
  {
    using stack_type = typename graph_impl::stack_selector<G>::stack_type;
    return graph_impl::traversal_helper<G>{}.template traverse<stack_type>(
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
    class EFTF = null_functor
  >
    requires (G::directedness == directed_flavour::directed)
  constexpr auto pseudo_depth_first_search(const G& graph, const bool findDisconnectedPieces = true,
                 const std::size_t start = 0,
                 NFBE&& nodeFunctorBeforeEdges     = null_functor{},
                 NFAE&& nodeFunctorAfterEdges      = null_functor{},
                 EFTF&& edgeFirstTraversalFunctor  = null_functor{},
                 TaskProcessingModel&& taskProcessingModel = TaskProcessingModel{})
  {
    using stack_type = typename graph_impl::stack_selector<G>::stack_type;
    return graph_impl::traversal_helper<G>{}.template traverse<stack_type>(
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
    class QCompare = graph_impl::node_comparer<G, std::less<typename G::node_weight_type>>
  >
    requires (G::directedness != directed_flavour::directed)
  constexpr auto priority_search(const G& graph, const bool findDisconnectedPieces = true,
                 const std::size_t start = 0,
                 NFBE&& nodeFunctorBeforeEdges     = null_functor{},
                 NFAE&& nodeFunctorAfterEdges      = null_functor{},
                 EFTF&& edgeFirstTraversalFunctor  = null_functor{},
                 ESTF&& edgeSecondTraversalFunctor = null_functor{},
                 TaskProcessingModel&& taskProcessingModel = TaskProcessingModel{})
  {
    using queue_type = typename graph_impl::priority_queue_selector<G, QCompare>::queue_type;
    return graph_impl::traversal_helper<G>{}.template traverse<queue_type>(
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
    class QCompare = graph_impl::node_comparer<G, std::less<typename G::node_weight_type>>
  >
    requires (G::directedness == directed_flavour::directed)
  constexpr auto priority_search(const G& graph, const bool findDisconnectedPieces = true,
                 const std::size_t start = 0,
                 NFBE&& nodeFunctorBeforeEdges     = null_functor{},
                 NFAE&& nodeFunctorAfterEdges      = null_functor{},
                 EFTF&& edgeFirstTraversalFunctor  = null_functor{},
                 TaskProcessingModel&& taskProcessingModel = TaskProcessingModel{})
  {
    using queue_type = typename graph_impl::priority_queue_selector<G, QCompare>::queue_type;
    return graph_impl::traversal_helper<G>{}.template traverse<queue_type>(
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
}
