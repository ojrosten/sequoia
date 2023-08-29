////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Implementation details for edge traits.

 */

#include "sequoia/Maths/Graph/GraphDetails.hpp"

namespace sequoia::maths::graph_impl
{
  template<class NodeStorage>
  struct node_allocator_generator
  {
    using allocator_type = typename NodeStorage::node_weight_container_type::allocator_type;
  };

  template<class NodeStorage>
    requires std::is_empty_v<typename NodeStorage::weight_type>
  struct node_allocator_generator<NodeStorage>
  {};

  template
  <
    graph_flavour GraphFlavour,
    class EdgeWeight,
    class EdgeMetaData,
    class EdgeStorageConfig,
    std::integral IndexType
  >
  struct dynamic_edge_traits : public
    edge_type_generator
    <
      GraphFlavour,
      EdgeWeight,
      EdgeMetaData,
      IndexType,
      EdgeStorageConfig::edge_sharing
    >
  {
    using edge_type_gen =
      edge_type_generator
      <
        GraphFlavour,
        EdgeWeight,
        EdgeMetaData,
        IndexType,
        EdgeStorageConfig::edge_sharing
      >;

    using edge_type = typename edge_type_gen::edge_type;

    using edge_storage_type
      = typename EdgeStorageConfig::template storage_type<edge_type>;

    using edge_allocator_type = typename edge_storage_type::allocator_type;
  };
}
