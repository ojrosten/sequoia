////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Traits and classes for static graphs with homogeneous node weights.

 */

#include "sequoia/Core/DataStructures/PartitionedData.hpp"
#include "sequoia/Maths/Graph/GraphImpl.hpp"
#include "sequoia/Maths/Graph/StaticGraphImpl.hpp"
#include "sequoia/Maths/Graph/StaticNodeStorage.hpp"

namespace sequoia::maths
{
  template<std::size_t Size, std::size_t Order, class EdgeWeight, class NodeWeight>
  struct static_graph_traits
  {
    using edge_index_type = typename graph_impl::static_edge_index_type_generator<Size, Order, false>::index_type;
  };

  template
  <
    std::size_t Size,
    std::size_t Order,
    class EdgeWeight,
    class NodeWeight,
    class Traits = static_graph_traits<Size, Order, EdgeWeight, NodeWeight>
  >
  class static_directed_graph final : public
    graph_primitive
    <
      connectivity<graph_impl::static_edge_traits<graph_flavour::directed, Order, Size, EdgeWeight, null_meta_data, typename Traits::edge_index_type>>,
      static_node_storage<NodeWeight, Order>
    >
  {
  private:
    using primitive_type =
      graph_primitive
      <
        connectivity<graph_impl::static_edge_traits<graph_flavour::directed, Order, Size, EdgeWeight, null_meta_data, typename Traits::edge_index_type>>,
        static_node_storage<NodeWeight, Order>
      >;

  public:
    constexpr static graph_flavour flavour{graph_flavour::directed};

    [[nodiscard]]
    constexpr static std::size_t order() noexcept { return Order; }

    [[nodiscard]]
    constexpr static std::size_t size() noexcept { return Size; }

    using node_weight_type = NodeWeight;
    using edge_index_type = typename Traits::edge_index_type;

    using
      graph_primitive
      <
        connectivity<graph_impl::static_edge_traits<graph_flavour::directed, Order, Size, EdgeWeight, null_meta_data, typename Traits::edge_index_type>>,
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
    class EdgeMetaData = null_meta_data,
    class Traits       = static_graph_traits<Size, Order, EdgeWeight, NodeWeight>
  >
  class static_undirected_graph final : public
    graph_primitive
    <
      connectivity<graph_impl::static_edge_traits<graph_flavour::undirected, Order, Size, EdgeWeight, EdgeMetaData, typename Traits::edge_index_type>>,
      static_node_storage<NodeWeight, Order>
    >
  {
  private:
    using primitive_type =
      graph_primitive
      <
        connectivity<graph_impl::static_edge_traits<graph_flavour::undirected, Order, Size, EdgeWeight, EdgeMetaData, typename Traits::edge_index_type>>,
        static_node_storage<NodeWeight, Order>
      >;

  public:
    constexpr static graph_flavour flavour{graph_flavour::undirected};

    [[nodiscard]]
    constexpr static std::size_t order() noexcept { return Order; }

    [[nodiscard]]
    constexpr static std::size_t size() noexcept { return Size; }

    using node_weight_type = NodeWeight;
    using edge_index_type = typename Traits::edge_index_type;

    using
      graph_primitive
      <
        connectivity<graph_impl::static_edge_traits<graph_flavour::undirected, Order, Size, EdgeWeight, EdgeMetaData, typename Traits::edge_index_type>>,
        static_node_storage<NodeWeight, Order>
      >::graph_primitive;

    using primitive_type::swap_nodes;
    using primitive_type::sort_edges;
    using primitive_type::swap_edges;
  };

  template<std::size_t Size, std::size_t Order, class EdgeWeight, class NodeWeight>
  struct static_embedded_graph_traits
  {
    using edge_index_type = typename graph_impl::static_edge_index_type_generator<Size, Order, true>::index_type;
  };

  template
  <
    std::size_t Size,
    std::size_t Order,
    class EdgeWeight,
    class NodeWeight,
    class EdgeMetaData = null_meta_data,
    class Traits       = static_embedded_graph_traits<Size, Order, EdgeWeight, NodeWeight>
  >
  class static_embedded_graph final : public
    graph_primitive
    <
      connectivity<graph_impl::static_edge_traits<graph_flavour::undirected_embedded, Order, Size, EdgeWeight, EdgeMetaData, typename Traits::edge_index_type>>,
      static_node_storage<NodeWeight, Order>
    >
  {
  private:
    using primitive =
      graph_primitive
      <
        connectivity<graph_impl::static_edge_traits<graph_flavour::undirected_embedded, Order, Size, EdgeWeight, EdgeMetaData, typename Traits::edge_index_type>>,
        static_node_storage<NodeWeight, Order>
      >;

  public:
    constexpr static graph_flavour flavour{graph_flavour::undirected_embedded};

    [[nodiscard]]
    constexpr static std::size_t order() noexcept { return Order; }

    [[nodiscard]]
    constexpr static std::size_t size() noexcept { return Size; }

    using node_weight_type =  NodeWeight;
    using edge_index_type = typename Traits::edge_index_type;

    using
      graph_primitive
      <
        connectivity<graph_impl::static_edge_traits<graph_flavour::undirected_embedded, Order, Size, EdgeWeight, EdgeMetaData, typename Traits::edge_index_type>>,
        static_node_storage<NodeWeight, Order>
      >::graph_primitive;

    using primitive::swap_nodes;
  };
}
