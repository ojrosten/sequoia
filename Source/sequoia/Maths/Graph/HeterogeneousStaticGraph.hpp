////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Traits and classes for static graphs with heterogeneous node weights.

 */

#include "sequoia/Maths/Graph/GraphImpl.hpp"
#include "sequoia/Maths/Graph/StaticGraphImpl.hpp"
#include "sequoia/Maths/Graph/HeterogeneousNodeStorage.hpp"

#include <tuple>

namespace sequoia::maths
{
  template<std::size_t Size, std::size_t Order, class EdgeWeight>
  struct heterogeneous_graph_traits
  {
    using edge_index_type = typename graph_impl::static_edge_index_type_generator<Size, Order, false>::index_type;
  };

  template
  <
    std::size_t Size,
    std::size_t Order,
    class EdgeWeight,
    class... NodeWeights
  >
  class heterogeneous_directed_graph final : public
    graph_primitive
    <
      connectivity
      <
        graph_impl::static_edge_traits
        <
          graph_flavour::directed,
          Order,
          Size,
          EdgeWeight,
          null_meta_data,
          typename heterogeneous_graph_traits<Size, Order, EdgeWeight>::edge_index_type
        >
      >,
      heterogeneous_node_storage<NodeWeights...>
    >
  {
  private:
    using primitive_type =
      graph_primitive
      <
        connectivity
        <
          graph_impl::static_edge_traits
          <
            graph_flavour::directed,
            Order,
            Size,
            EdgeWeight,
            null_meta_data,
            typename heterogeneous_graph_traits<Size, Order, EdgeWeight>::edge_index_type
          >
        >,
        heterogeneous_node_storage<NodeWeights...>
      >;

  public:
    static_assert(sizeof...(NodeWeights) == Order);

    constexpr static graph_flavour flavour{graph_flavour::directed};

    [[nodiscard]]
    constexpr static std::size_t order() noexcept { return Order; }

    [[nodiscard]]
    constexpr static std::size_t size() noexcept { return Size; }

    using edge_index_type = typename heterogeneous_graph_traits<Size, Order, EdgeWeight>::edge_index_type;

    using
      graph_primitive
      <
        connectivity
        <
          graph_impl::static_edge_traits
          <
            graph_flavour::directed,
            Order,
            Size,
            EdgeWeight,
            null_meta_data,
            typename heterogeneous_graph_traits<Size, Order, EdgeWeight>::edge_index_type
          >
        >,
        heterogeneous_node_storage<NodeWeights...>
      >::graph_primitive;

    using primitive_type::set_edge_weight;
    using primitive_type::mutate_edge_weight;
    using primitive_type::sort_edges;
    using primitive_type::swap_edges;
  };

  template
  <
    std::size_t Size,
    std::size_t Order,
    class EdgeWeight,
    class EdgeMetaData,
    class... NodeWeights
  >
  class heterogeneous_undirected_graph final : public
    graph_primitive
    <
      connectivity
      <
        graph_impl::static_edge_traits
        <
          graph_flavour::undirected,
          Order,
          Size,
          EdgeWeight,
          EdgeMetaData,
          typename heterogeneous_graph_traits<Size, Order, EdgeWeight>::edge_index_type
        >
      >,
      heterogeneous_node_storage<NodeWeights...>
    >
  {
  private:
    using primitive_type =
      graph_primitive
      <
        connectivity
        <
          graph_impl::static_edge_traits
          <
            graph_flavour::undirected,
            Order,
            Size,
            EdgeWeight,
            EdgeMetaData,
            typename heterogeneous_graph_traits<Size, Order, EdgeWeight>::edge_index_type
          >
        >,
        heterogeneous_node_storage<NodeWeights...>
      >;

  public:
    static_assert(sizeof...(NodeWeights) == Order);

    constexpr static graph_flavour flavour{graph_flavour::undirected};

    [[nodiscard]]
    constexpr static std::size_t order() noexcept { return Order; }

    [[nodiscard]]
    constexpr static std::size_t size() noexcept { return Size; }

    using edge_index_type = typename heterogeneous_graph_traits<Size, Order, EdgeWeight>::edge_index_type;

    using
      graph_primitive
      <
        connectivity
        <
          graph_impl::static_edge_traits
          <
            graph_flavour::undirected,
            Order,
            Size,
            EdgeWeight,
            null_meta_data,
            typename heterogeneous_graph_traits<Size, Order, EdgeWeight>::edge_index_type
          >
        >,
        heterogeneous_node_storage<NodeWeights...>
      >::graph_primitive;

    using primitive_type::set_edge_weight;
    using primitive_type::mutate_edge_weight;
    using primitive_type::sort_edges;
    using primitive_type::swap_edges;
  };

  template<std::size_t Size, std::size_t Order, class EdgeWeight>
  struct heterogeneous_embedded_graph_traits
  {
    using edge_index_type = typename graph_impl::static_edge_index_type_generator<Size, Order, true>::index_type;
  };

  template
  <
    std::size_t Size,
    std::size_t Order,
    class EdgeWeight,
    class EdgeMetaData,
    class... NodeWeights
  >
  class heterogeneous_embedded_graph final : public
    graph_primitive
    <
      connectivity
      <
        graph_impl::static_edge_traits
        <
          graph_flavour::undirected_embedded,
          Order,
          Size,
          EdgeWeight,
          EdgeMetaData,
          typename heterogeneous_embedded_graph_traits<Size, Order, EdgeWeight>::edge_index_type
        >
      >,
      heterogeneous_node_storage<NodeWeights...>
    >
  {
  private:
    using primitive_type =
      graph_primitive
      <
        connectivity
        <
          graph_impl::static_edge_traits
          <
            graph_flavour::undirected_embedded,
            Order,
            Size,
            EdgeWeight,
            EdgeMetaData,
            typename heterogeneous_embedded_graph_traits<Size, Order, EdgeWeight>::edge_index_type
          >
        >,
        heterogeneous_node_storage<NodeWeights...>
      >;

  public:
    static_assert(sizeof...(NodeWeights) == Order);

    constexpr static graph_flavour flavour{graph_flavour::undirected_embedded};

    constexpr static std::size_t order() noexcept { return Order; }

    constexpr static std::size_t size() noexcept { return Size; }

    using edge_index_type = typename heterogeneous_embedded_graph_traits<Size, Order, EdgeWeight>::edge_index_type;

    using
      graph_primitive
      <
        connectivity
        <
          graph_impl::static_edge_traits
          <
            graph_flavour::undirected_embedded,
            Order,
            Size,
            EdgeWeight,
            EdgeMetaData,
            typename heterogeneous_embedded_graph_traits<Size, Order, EdgeWeight>::edge_index_type
          >
        >,
        heterogeneous_node_storage<NodeWeights...>
      >::graph_primitive;

    using primitive_type::set_edge_weight;
    using primitive_type::mutate_edge_weight;
  };
}
