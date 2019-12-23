////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file StaticGraphImpl.hpp
    \brief Edge traits for static homogeneous graphs.
 */

#include "GraphDetails.hpp"

namespace sequoia::maths::graph_impl
{
  template
  <
    graph_flavour GraphFlavour,
    std::size_t Order,
    std::size_t Size,
    class EdgeWeight,
    class IndexType
  >
  struct static_edge_traits
    : public edge_type_generator<GraphFlavour, EdgeWeight, data_sharing::unpooled<EdgeWeight>, IndexType, edge_sharing_preference::independent>
  {
    using edge_type = typename edge_type_generator<GraphFlavour, EdgeWeight, data_sharing::unpooled<EdgeWeight>, IndexType, edge_sharing_preference::independent>::edge_type;
    using edge_storage_type = data_structures::static_partitioned_sequence<edge_type, Order, num_static_edges(GraphFlavour, Size), IndexType>;

    constexpr static bool shared_edge_v{};       
    constexpr static bool mutual_info_v{GraphFlavour != graph_flavour::directed};
    constexpr static bool weight_setting_exception_guarantee_v{};
  };
}
