////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Classes for static graphs with homogeneous node weights.

 */

#include "sequoia/Core/DataStructures/PartitionedData.hpp"
#include "sequoia/Maths/Graph/GraphImpl.hpp"
#include "sequoia/Maths/Graph/StaticGraphImpl.hpp"
#include "sequoia/Maths/Graph/StaticNodeStorage.hpp"

namespace sequoia::maths
{
  template
  <
    std::size_t Size,
    std::size_t Order,
    class EdgeWeight,
    class NodeWeight,
    class EdgeStorageConfig = static_edge_storage_config<graph_flavour::directed, Size, Order>
  >
  class static_directed_graph final : public
    graph_primitive
    <
      connectivity<graph_flavour::directed, graph_impl::edge_storage_generator_t<graph_flavour::directed, EdgeWeight, null_meta_data, typename EdgeStorageConfig::index_type, EdgeStorageConfig>>,
      static_node_storage<NodeWeight, Order>
    >
  {
  private:
    using primitive_type =
      graph_primitive
      <
        connectivity<graph_flavour::directed, graph_impl::edge_storage_generator_t<graph_flavour::directed, EdgeWeight, null_meta_data, typename EdgeStorageConfig::index_type, EdgeStorageConfig>>,
        static_node_storage<NodeWeight, Order>
      >;

  public:
    constexpr static graph_flavour flavour{graph_flavour::directed};

    [[nodiscard]]
    constexpr static std::size_t order() noexcept { return Order; }

    [[nodiscard]]
    constexpr static std::size_t size() noexcept { return Size; }

    using node_weight_type = NodeWeight;

    using
      graph_primitive
      <
        connectivity<graph_flavour::directed, graph_impl::edge_storage_generator_t<graph_flavour::directed, EdgeWeight, null_meta_data, typename EdgeStorageConfig::index_type, EdgeStorageConfig>>,
        static_node_storage<NodeWeight, Order>
      >::graph_primitive;

    using primitive_type::swap_nodes;
    using primitive_type::sort_edges;
    using primitive_type::swap_edges;
  };

  template
  <
    std::size_t Size,
    std::size_t Order,
    class EdgeWeight,
    class NodeWeight,
    class EdgeMetaData      = null_meta_data,
    class EdgeStorageConfig = static_edge_storage_config<graph_flavour::undirected, Size, Order>
  >
  class static_undirected_graph final : public
    graph_primitive
    <
      connectivity<graph_flavour::undirected, graph_impl::edge_storage_generator_t<graph_flavour::undirected, EdgeWeight, EdgeMetaData, typename EdgeStorageConfig::index_type, EdgeStorageConfig>>,
      static_node_storage<NodeWeight, Order>
    >
  {
  private:
    using primitive_type =
      graph_primitive
      <
        connectivity<graph_flavour::undirected, graph_impl::edge_storage_generator_t<graph_flavour::undirected, EdgeWeight, EdgeMetaData, typename EdgeStorageConfig::index_type, EdgeStorageConfig>>,
        static_node_storage<NodeWeight, Order>
      >;

  public:
    constexpr static graph_flavour flavour{graph_flavour::undirected};

    [[nodiscard]]
    constexpr static std::size_t order() noexcept { return Order; }

    [[nodiscard]]
    constexpr static std::size_t size() noexcept { return Size; }

    using node_weight_type = NodeWeight;

    using
      graph_primitive
      <
        connectivity<graph_flavour::undirected, graph_impl::edge_storage_generator_t<graph_flavour::undirected, EdgeWeight, EdgeMetaData, typename EdgeStorageConfig::index_type, EdgeStorageConfig>>,
        static_node_storage<NodeWeight, Order>
      >::graph_primitive;

    using primitive_type::swap_nodes;
    using primitive_type::sort_edges;
    using primitive_type::swap_edges;
  };

  template
  <
    std::size_t Size,
    std::size_t Order,
    class EdgeWeight,
    class NodeWeight,
    class EdgeMetaData      = null_meta_data,
    class EdgeStorageConfig = static_edge_storage_config<graph_flavour::undirected_embedded, Size, Order>
  >
  class static_embedded_graph final : public
    graph_primitive
    <
      connectivity<graph_flavour::undirected_embedded, graph_impl::edge_storage_generator_t<graph_flavour::undirected_embedded, EdgeWeight, EdgeMetaData, typename EdgeStorageConfig::index_type, EdgeStorageConfig>>,
      static_node_storage<NodeWeight, Order>
    >
  {
  private:
    using primitive =
      graph_primitive
      <
        connectivity<graph_flavour::undirected_embedded, graph_impl::edge_storage_generator_t<graph_flavour::undirected_embedded, EdgeWeight, EdgeMetaData, typename EdgeStorageConfig::index_type, EdgeStorageConfig>>,
        static_node_storage<NodeWeight, Order>
      >;

  public:
    constexpr static graph_flavour flavour{graph_flavour::undirected_embedded};

    [[nodiscard]]
    constexpr static std::size_t order() noexcept { return Order; }

    [[nodiscard]]
    constexpr static std::size_t size() noexcept { return Size; }

    using node_weight_type =  NodeWeight;

    using
      graph_primitive
      <
        connectivity<graph_flavour::undirected_embedded, graph_impl::edge_storage_generator_t<graph_flavour::undirected_embedded, EdgeWeight, EdgeMetaData, typename EdgeStorageConfig::index_type, EdgeStorageConfig>>,
        static_node_storage<NodeWeight, Order>
      >::graph_primitive;

    using primitive::swap_nodes;
  };
}
