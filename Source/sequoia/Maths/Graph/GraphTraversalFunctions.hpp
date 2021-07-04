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
#include "sequoia/Maths/Graph/DynamicGraphTraversalDetails.hpp"
#include "sequoia/Maths/Graph/StaticGraphTraversalDetails.hpp"
#include "sequoia/Core/Concurrency/ConcurrencyModels.hpp"

namespace sequoia::maths
{
  template
  <
    class TaskProcessingModel = concurrency::serial<void>,
    network G,
    disconnected_discovery_mode Mode,
    class NBEF = null_func_obj,
    class NAEF = null_func_obj,
    class EFTF = null_func_obj,
    class ESTF = null_func_obj
  >
    requires (G::directedness != directed_flavour::directed)
          && (invocable<NBEF, typename G::edge_index_type>)
          && (invocable<NAEF, typename G::edge_index_type>)
          && (invocable<EFTF, typename G::const_edge_iterator>)
          && (invocable<ESTF, typename G::const_edge_iterator>)
  constexpr auto breadth_first_search(const G& graph,
                                      const traversal_conditions<Mode> conditions,
                                      NBEF&& nodeBeforeEdgesFn     = null_func_obj{},
                                      NAEF&& nodeAfterEdgesFn      = null_func_obj{},
                                      EFTF&& edgeFirstTraversalFn  = null_func_obj{},
                                      ESTF&& edgeSecondTraversalFn = null_func_obj{},
                                      TaskProcessingModel&& taskProcessingModel = TaskProcessingModel{})
  {
    using queue_type = typename graph_impl::queue_selector<G>::queue_type;
    return graph_impl::traversal_helper<G, queue_type>{}.traverse(
             graph,
             conditions,
             std::forward<NBEF>(nodeBeforeEdgesFn),
             std::forward<NAEF>(nodeAfterEdgesFn),
             std::forward<EFTF>(edgeFirstTraversalFn),
             std::forward<ESTF>(edgeSecondTraversalFn),
             std::forward<TaskProcessingModel>(taskProcessingModel)
           );
  }

  template
  <
    class TaskProcessingModel = concurrency::serial<void>,
    network G,
    disconnected_discovery_mode Mode,
    class NBEF = null_func_obj,
    class NAEF = null_func_obj,
    class EFTF = null_func_obj
  >
    requires (G::directedness == directed_flavour::directed)
          && (invocable<NBEF, typename G::edge_index_type>)
          && (invocable<NAEF, typename G::edge_index_type>)
          && (invocable<EFTF, typename G::const_edge_iterator>)
  constexpr auto breadth_first_search(const G& graph,
                                      const traversal_conditions<Mode> conditions,
                                      NBEF&& nodeBeforeEdgesFn     = null_func_obj{},
                                      NAEF&& nodeAfterEdgesFn      = null_func_obj{},
                                      EFTF&& edgeFirstTraversalFn  = null_func_obj{},
                                      TaskProcessingModel&& taskProcessingModel = TaskProcessingModel{})
  {
    using queue_type = typename graph_impl::queue_selector<G>::queue_type;
    return graph_impl::traversal_helper<G, queue_type>{}.traverse(
             graph,
             conditions,
             std::forward<NBEF>(nodeBeforeEdgesFn),
             std::forward<NAEF>(nodeAfterEdgesFn),
             std::forward<EFTF>(edgeFirstTraversalFn),
             null_func_obj{},
             std::forward<TaskProcessingModel>(taskProcessingModel)
           );
  }

  template
  <
    class TaskProcessingModel = concurrency::serial<void>,
    network G,
    disconnected_discovery_mode Mode,
    class NBEF = null_func_obj,
    class NAEF = null_func_obj,
    class EFTF = null_func_obj,
    class ESTF = null_func_obj
  >
    requires (G::directedness != directed_flavour::directed)
          && (invocable<NBEF, typename G::edge_index_type>)
          && (invocable<NAEF, typename G::edge_index_type>)
          && (invocable<EFTF, typename G::const_edge_iterator>)
          && (invocable<ESTF, typename G::const_edge_iterator>)
  constexpr auto pseudo_depth_first_search(const G& graph,
                                           const traversal_conditions<Mode> conditions,
                                           NBEF&& nodeBeforeEdgesFn     = null_func_obj{},
                                           NAEF&& nodeAfterEdgesFn      = null_func_obj{},
                                           EFTF&& edgeFirstTraversalFn  = null_func_obj{},
                                           ESTF&& edgeSecondTraversalFn = null_func_obj{},
                                           TaskProcessingModel&& taskProcessingModel = TaskProcessingModel{})
  {
    using stack_type = typename graph_impl::stack_selector<G>::stack_type;
    return graph_impl::traversal_helper<G, stack_type>{}.traverse(
             graph,
             conditions,
             std::forward<NBEF>(nodeBeforeEdgesFn),
             std::forward<NAEF>(nodeAfterEdgesFn),
             std::forward<EFTF>(edgeFirstTraversalFn),
             std::forward<ESTF>(edgeSecondTraversalFn),
             std::forward<TaskProcessingModel>(taskProcessingModel)
           );
  }

  template
  <
    class TaskProcessingModel = concurrency::serial<void>,
    network G,
    disconnected_discovery_mode Mode,
    class NBEF = null_func_obj,
    class NAEF = null_func_obj,
    class EFTF = null_func_obj
  >
    requires (G::directedness == directed_flavour::directed)
          && (invocable<NBEF, typename G::edge_index_type>)
          && (invocable<NAEF, typename G::edge_index_type>)
          && (invocable<EFTF, typename G::const_edge_iterator>)
  constexpr auto pseudo_depth_first_search(const G& graph,
                                           const traversal_conditions<Mode> conditions,
                                           NBEF&& nodeBeforeEdgesFn     = null_func_obj{},
                                           NAEF&& nodeAfterEdgesFn      = null_func_obj{},
                                           EFTF&& edgeFirstTraversalFn  = null_func_obj{},
                                           TaskProcessingModel&& taskProcessingModel = TaskProcessingModel{})
  {
    using stack_type = typename graph_impl::stack_selector<G>::stack_type;
    return graph_impl::traversal_helper<G, stack_type>{}.traverse(
             graph,
             conditions,
             std::forward<NBEF>(nodeBeforeEdgesFn),
             std::forward<NAEF>(nodeAfterEdgesFn),
             std::forward<EFTF>(edgeFirstTraversalFn),
             null_func_obj{},
             std::forward<TaskProcessingModel>(taskProcessingModel)
           );
  }

  template
  <
    class TaskProcessingModel = concurrency::serial<void>,
    network G,
    disconnected_discovery_mode Mode,
    class NBEF = null_func_obj,
    class NAEF = null_func_obj,
    class ETUN = null_func_obj
  >
    requires (invocable<NBEF, typename G::edge_index_type>)
          && (invocable<NAEF, typename G::edge_index_type>)
          && (invocable<ETUN, typename G::const_edge_iterator>)
    constexpr auto depth_first_search(const G& graph,
                                      const traversal_conditions<Mode> conditions,
                                      NBEF&& nodeBeforeEdgesFn        = null_func_obj{},
                                      NAEF&& nodeAfterEdgesFn         = null_func_obj{},
                                      ETUN&& edgeToUndiscoveredNodeFn = null_func_obj{},
                                      TaskProcessingModel&& taskProcessingModel = TaskProcessingModel{})
  {
    return graph_impl::traversal_helper<G, void>{}.recursive_dfs(
      graph,
      conditions,
      std::forward<NBEF>(nodeBeforeEdgesFn),
      std::forward<NAEF>(nodeAfterEdgesFn),
      std::forward<ETUN>(edgeToUndiscoveredNodeFn),
      std::forward<TaskProcessingModel>(taskProcessingModel)
    );
  }

  template
  <
    class TaskProcessingModel = concurrency::serial<void>,
    network G,
    disconnected_discovery_mode Mode,
    class NBEF = null_func_obj,
    class NAEF = null_func_obj,
    class EFTF = null_func_obj,
    class ESTF = null_func_obj,
    class QCompare = graph_impl::node_comparer<G, std::less<typename G::node_weight_type>>
  >
    requires (G::directedness != directed_flavour::directed)
          && (invocable<NBEF, typename G::edge_index_type>)
          && (invocable<NAEF, typename G::edge_index_type>)
          && (invocable<EFTF, typename G::const_edge_iterator>)
          && (invocable<ESTF, typename G::const_edge_iterator>)
  constexpr auto priority_search(const G& graph,
                                 const traversal_conditions<Mode> conditions,
                                 NBEF&& nodeBeforeEdgesFn     = null_func_obj{},
                                 NAEF&& nodeAfterEdgesFn      = null_func_obj{},
                                 EFTF&& edgeFirstTraversalFn  = null_func_obj{},
                                 ESTF&& edgeSecondTraversalFn = null_func_obj{},
                                 TaskProcessingModel&& taskProcessingModel = TaskProcessingModel{})
  {
    using queue_type = typename graph_impl::priority_queue_selector<G, QCompare>::queue_type;
    return graph_impl::traversal_helper<G, queue_type>{}.traverse(
             graph,
             conditions,
             std::forward<NBEF>(nodeBeforeEdgesFn),
             std::forward<NAEF>(nodeAfterEdgesFn),
             std::forward<EFTF>(edgeFirstTraversalFn),
             std::forward<ESTF>(edgeSecondTraversalFn),
             std::forward<TaskProcessingModel>(taskProcessingModel)
           );
  }

  template
  <
    class TaskProcessingModel = concurrency::serial<void>,
    network G,
    disconnected_discovery_mode Mode,
    class NBEF = null_func_obj,
    class NAEF = null_func_obj,
    class EFTF = null_func_obj,
    class QCompare = graph_impl::node_comparer<G, std::less<typename G::node_weight_type>>
  >
    requires (G::directedness == directed_flavour::directed)
          && (invocable<NBEF, typename G::edge_index_type>)
          && (invocable<NAEF, typename G::edge_index_type>)
          && (invocable<EFTF, typename G::const_edge_iterator>)
  constexpr auto priority_search(const G& graph,
                                 const traversal_conditions<Mode> conditions,
                                 NBEF&& nodeBeforeEdgesFn     = null_func_obj{},
                                 NAEF&& nodeAfterEdgesFn      = null_func_obj{},
                                 EFTF&& edgeFirstTraversalFn  = null_func_obj{},
                                 TaskProcessingModel&& taskProcessingModel = TaskProcessingModel{})
  {
    using queue_type = typename graph_impl::priority_queue_selector<G, QCompare>::queue_type;
    return graph_impl::traversal_helper<G, queue_type>{}.traverse(
             graph,
             conditions,
             std::forward<NBEF>(nodeBeforeEdgesFn),
             std::forward<NAEF>(nodeAfterEdgesFn),
             std::forward<EFTF>(edgeFirstTraversalFn),
             null_func_obj{},
             std::forward<TaskProcessingModel>(taskProcessingModel)
           );
  }
}
