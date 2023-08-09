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
  template<directed_flavour F>
  [[nodiscard]]
  constexpr graph_flavour to_graph_flavour() noexcept
  {
    return F == directed_flavour::directed ? graph_flavour::directed : graph_flavour::undirected;
  }

  template<directed_flavour F>
  [[nodiscard]]
  constexpr graph_flavour to_embedded_graph_flavour() noexcept
  {
    return F == directed_flavour::directed ? graph_flavour::directed_embedded : graph_flavour::undirected_embedded;
  }

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
    class EdgeStorageTraits,
    std::integral IndexType
  >
  struct dynamic_edge_traits : public
    edge_type_generator
    <
      GraphFlavour,
      EdgeWeight,
      IndexType,
      EdgeStorageTraits::edge_sharing
    >
  {
    using edge_type_gen =
      edge_type_generator
      <
        GraphFlavour,
        EdgeWeight,
        IndexType,
        EdgeStorageTraits::edge_sharing
      >;

    using edge_type = typename edge_type_gen::edge_type;

    using edge_storage_handler = std::conditional_t<edge_type_gen::shared_edge_v,
                                                    object::shared<edge_type>,
                                                    object::by_value<edge_type>>;

    using edge_storage_traits
      = typename EdgeStorageTraits::template traits_type<edge_type, edge_storage_handler>;

    using edge_storage_type
      = typename EdgeStorageTraits::template storage_type<edge_type, edge_storage_handler, edge_storage_traits>;

    using edge_allocator_type = typename edge_storage_type::allocator_type;
  };
}
