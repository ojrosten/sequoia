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
#include "sequoia/Core/Ownership/DataPool.hpp"
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
    directed_flavour Directedness,
    std::size_t Size,
    std::size_t Order,
    class EdgeWeight,
    class NodeWeight,
    class Traits = static_graph_traits<Size, Order, EdgeWeight, NodeWeight>
  >
  class static_graph final : public
    graph_primitive
    <
      connectivity
      <
        Directedness,
        graph_impl::static_edge_traits<(Directedness == directed_flavour::directed) ? graph_flavour::directed : graph_flavour::undirected, Order, Size, EdgeWeight, typename Traits::edge_index_type>,
        ownership::spawner<EdgeWeight>
      >,
      graph_impl::static_node_storage<ownership::spawner<NodeWeight>, Order>
    >
  {
  private:
    using primitive_type =
      graph_primitive
      <
        connectivity
        <
          Directedness,
          graph_impl::static_edge_traits<(Directedness == directed_flavour::directed) ? graph_flavour::directed : graph_flavour::undirected, Order, Size, EdgeWeight, typename Traits::edge_index_type>,
          ownership::spawner<EdgeWeight>
        >,
        graph_impl::static_node_storage<ownership::spawner<NodeWeight>, Order>
      >;

  public:
    constexpr static graph_flavour flavour{(Directedness == directed_flavour::directed) ? graph_flavour::directed : graph_flavour::undirected};

    [[nodiscard]]
    constexpr static std::size_t order() noexcept { return Order; }

    [[nodiscard]]
    constexpr static std::size_t size() noexcept { return Size; }

    using node_weight_type = NodeWeight;
    using edge_index_type = typename Traits::edge_index_type;

    using
      graph_primitive
      <
        connectivity
        <
          Directedness,
          graph_impl::static_edge_traits<(Directedness == directed_flavour::directed) ? graph_flavour::directed : graph_flavour::undirected, Order, Size, EdgeWeight, typename Traits::edge_index_type>,
          ownership::spawner<EdgeWeight>
        >,
        graph_impl::static_node_storage<ownership::spawner<NodeWeight>, Order>
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
    directed_flavour Directedness,
    std::size_t Size,
    std::size_t Order,
    class EdgeWeight,
    class NodeWeight,
    class Traits = static_embedded_graph_traits<Size, Order, EdgeWeight, NodeWeight>
  >
  class static_embedded_graph final : public
    graph_primitive
    <
      connectivity
      <
        Directedness,
        graph_impl::static_edge_traits<(Directedness == directed_flavour::directed) ? graph_flavour::directed_embedded : graph_flavour::undirected_embedded, Order, Size, EdgeWeight, typename Traits::edge_index_type>,
        ownership::spawner<EdgeWeight>
      >,
      graph_impl::static_node_storage<ownership::spawner<NodeWeight>, Order>
    >
  {
  private:
    using primitive =
      graph_primitive
      <
        connectivity
        <
          Directedness,
          graph_impl::static_edge_traits<(Directedness == directed_flavour::directed) ? graph_flavour::directed_embedded : graph_flavour::undirected_embedded, Order, Size, EdgeWeight, typename Traits::edge_index_type>,
          ownership::spawner<EdgeWeight>
        >,
        graph_impl::static_node_storage<ownership::spawner<NodeWeight>, Order>
      >;

  public:
    constexpr static graph_flavour flavour{(Directedness == directed_flavour::directed) ? graph_flavour::directed_embedded : graph_flavour::undirected_embedded};

    [[nodiscard]]
    constexpr static std::size_t order() noexcept { return Order; }

    [[nodiscard]]
    constexpr static std::size_t size() noexcept { return Size; }

    using node_weight_type =  NodeWeight;
    using edge_index_type = typename Traits::edge_index_type;

    using
      graph_primitive
      <
        connectivity
        <
          Directedness,
          graph_impl::static_edge_traits<(Directedness == directed_flavour::directed) ? graph_flavour::directed_embedded : graph_flavour::undirected_embedded, Order, Size, EdgeWeight, typename Traits::edge_index_type>,
          ownership::spawner<EdgeWeight>
        >,
        graph_impl::static_node_storage<ownership::spawner<NodeWeight>, Order>
      >::graph_primitive;

    using primitive::swap_nodes;
  };
}
