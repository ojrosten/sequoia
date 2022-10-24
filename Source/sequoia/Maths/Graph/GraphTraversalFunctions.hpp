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
    traversal_flavour F,
    network G,
    disconnected_discovery_mode Mode,
    class NBEF = null_func_obj,
    class NAEF = null_func_obj,
    class EFTF = null_func_obj,
    class ESTF = null_func_obj
  >
    requires (G::directedness != directed_flavour::directed)
          && (std::invocable<NBEF, typename G::edge_index_type>)
          && (std::invocable<NAEF, typename G::edge_index_type>)
          && (std::invocable<EFTF, typename G::const_edge_iterator>)
          && (std::invocable<ESTF, typename G::const_edge_iterator>)
  constexpr auto traverse(traversal_constant<F> tc,
                          const G& graph,
                          const traversal_conditions<Mode> conditions,
                          NBEF&& nodeBeforeEdgesFn                  = {},
                          NAEF&& nodeAfterEdgesFn                   = {},
                          EFTF&& edgeFirstTraversalFn               = {},
                          ESTF&& edgeSecondTraversalFn              = {},
                          TaskProcessingModel&& taskProcessingModel = {})
  {
    return graph_impl::traversal_helper<G>{}.traverse(
             tc,
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
    traversal_flavour F,
    network G,
    disconnected_discovery_mode Mode,
    class NBEF = null_func_obj,
    class NAEF = null_func_obj,
    class EFTF = null_func_obj
  >
    requires (G::directedness == directed_flavour::directed)
          && (std::invocable<NBEF, typename G::edge_index_type>)
          && (std::invocable<NAEF, typename G::edge_index_type>)
          && (std::invocable<EFTF, typename G::const_edge_iterator>)
  constexpr auto traverse(traversal_constant<F> tc,
                          const G& graph,
                          const traversal_conditions<Mode> conditions,
                          NBEF&& nodeBeforeEdgesFn                  = {},
                          NAEF&& nodeAfterEdgesFn                   = {},
                          EFTF&& edgeFirstTraversalFn               = {},
                          TaskProcessingModel&& taskProcessingModel = {})
  {
    return graph_impl::traversal_helper<G>{}.traverse(
             tc,
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
    requires (std::invocable<NBEF, typename G::edge_index_type>)
          && (std::invocable<NAEF, typename G::edge_index_type>)
          && (std::invocable<ETUN, typename G::const_edge_iterator>)
    constexpr auto depth_first_search(const G& graph,
                                      const traversal_conditions<Mode> conditions,
                                      NBEF&& nodeBeforeEdgesFn                  = {},
                                      NAEF&& nodeAfterEdgesFn                   = {},
                                      ETUN&& edgeToUndiscoveredNodeFn           = {},
                                      TaskProcessingModel&& taskProcessingModel = {})
  {
    return graph_impl::traversal_helper<G>{}.recursive_dfs(
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
    class NBEF     = null_func_obj,
    class NAEF     = null_func_obj,
    class EFTF     = null_func_obj,
    class ESTF     = null_func_obj,
    class QCompare = graph_impl::node_comparer<G, std::less<typename G::node_weight_type>>
  >
    requires (G::directedness != directed_flavour::directed)
          && (std::invocable<NBEF, typename G::edge_index_type>)
          && (std::invocable<NAEF, typename G::edge_index_type>)
          && (std::invocable<EFTF, typename G::const_edge_iterator>)
          && (std::invocable<ESTF, typename G::const_edge_iterator>)
  constexpr auto traverse(priority_first_search_type,
                          const G& graph,
                          const traversal_conditions<Mode> conditions,
                          NBEF&& nodeBeforeEdgesFn                  = {},
                          NAEF&& nodeAfterEdgesFn                   = {},
                          EFTF&& edgeFirstTraversalFn               = {},
                          ESTF&& edgeSecondTraversalFn              = {},
                          TaskProcessingModel&& taskProcessingModel = {})
  {
    return graph_impl::traversal_helper<G>{}.traverse(
             priority_first,
             graph,
             conditions,
             std::forward<NBEF>(nodeBeforeEdgesFn),
             std::forward<NAEF>(nodeAfterEdgesFn),
             std::forward<EFTF>(edgeFirstTraversalFn),
             std::forward<ESTF>(edgeSecondTraversalFn),
             std::forward<TaskProcessingModel>(taskProcessingModel),
             QCompare{graph}
           );
  }

  template
  <
    class TaskProcessingModel = concurrency::serial<void>,
    network G,
    disconnected_discovery_mode Mode,
    class NBEF     = null_func_obj,
    class NAEF     = null_func_obj,
    class EFTF     = null_func_obj,
    class QCompare = graph_impl::node_comparer<G, std::less<typename G::node_weight_type>>
  >
    requires (G::directedness == directed_flavour::directed)
          && (std::invocable<NBEF, typename G::edge_index_type>)
          && (std::invocable<NAEF, typename G::edge_index_type>)
          && (std::invocable<EFTF, typename G::const_edge_iterator>)
  constexpr auto traverse(priority_first_search_type,
                          const G& graph,
                          const traversal_conditions<Mode> conditions,
                          NBEF&& nodeBeforeEdgesFn                  = {},
                          NAEF&& nodeAfterEdgesFn                   = {},
                          EFTF&& edgeFirstTraversalFn               = {},
                          TaskProcessingModel&& taskProcessingModel = {})
  {
    return graph_impl::traversal_helper<G>{}.traverse(
             priority_first,
             graph,
             conditions,
             std::forward<NBEF>(nodeBeforeEdgesFn),
             std::forward<NAEF>(nodeAfterEdgesFn),
             std::forward<EFTF>(edgeFirstTraversalFn),
             null_func_obj{},
             std::forward<TaskProcessingModel>(taskProcessingModel),
             QCompare{graph}
           );
  }
}
