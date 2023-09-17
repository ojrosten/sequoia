////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Classes for static graphs with heterogeneous node weights.

 */

#include "sequoia/Maths/Graph/GraphPrimitive.hpp"
#include "sequoia/Maths/Graph/StaticGraphConfig.hpp"
#include "sequoia/Maths/Graph/HeterogeneousNodeStorage.hpp"

#include <tuple>

namespace sequoia::maths
{
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
      connectivity<graph_flavour::directed, graph_impl::edge_storage_generator_t<graph_flavour::directed, EdgeWeight, null_meta_data, typename static_edge_storage_config<graph_flavour::directed, Size, Order>::index_type, static_edge_storage_config<graph_flavour::directed, Size, Order>>>,
      heterogeneous_node_storage<NodeWeights...>
    >
  {
  private:
    using primitive_type =
      graph_primitive
      <
        connectivity<graph_flavour::directed, graph_impl::edge_storage_generator_t<graph_flavour::directed, EdgeWeight, null_meta_data, typename static_edge_storage_config<graph_flavour::directed, Size, Order>::index_type, static_edge_storage_config<graph_flavour::directed, Size, Order>>>,
        heterogeneous_node_storage<NodeWeights...>
      >;

  public:
    static_assert(sizeof...(NodeWeights) == Order);

    constexpr static graph_flavour flavour{graph_flavour::directed};

    [[nodiscard]]
    constexpr static std::size_t order() noexcept { return Order; }

    [[nodiscard]]
    constexpr static std::size_t size() noexcept { return Size; }

    using
      graph_primitive
      <
        connectivity<graph_flavour::directed, graph_impl::edge_storage_generator_t<graph_flavour::directed, EdgeWeight, null_meta_data, typename static_edge_storage_config<graph_flavour::directed, Size, Order>::index_type, static_edge_storage_config<graph_flavour::directed, Size, Order>>>,
        heterogeneous_node_storage<NodeWeights...>
      >::graph_primitive;

    using primitive_type::set_edge_weight;
    using primitive_type::mutate_edge_weight;
    using primitive_type::sort_edges;
    using primitive_type::stable_sort_edges;
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
      connectivity<graph_flavour::undirected, graph_impl::edge_storage_generator_t<graph_flavour::undirected, EdgeWeight, EdgeMetaData, typename static_edge_storage_config<graph_flavour::undirected, Size, Order>::index_type, static_edge_storage_config<graph_flavour::undirected, Size, Order>>>,
      heterogeneous_node_storage<NodeWeights...>
    >
  {
  private:
    using primitive_type =
      graph_primitive
      <
        connectivity<graph_flavour::undirected, graph_impl::edge_storage_generator_t<graph_flavour::undirected, EdgeWeight, EdgeMetaData, typename static_edge_storage_config<graph_flavour::undirected, Size, Order>::index_type, static_edge_storage_config<graph_flavour::undirected, Size, Order>>>,
        heterogeneous_node_storage<NodeWeights...>
      >;

  public:
    static_assert(sizeof...(NodeWeights) == Order);

    constexpr static graph_flavour flavour{graph_flavour::undirected};

    [[nodiscard]]
    constexpr static std::size_t order() noexcept { return Order; }

    [[nodiscard]]
    constexpr static std::size_t size() noexcept { return Size; }

    using
      graph_primitive
      <
        connectivity<graph_flavour::undirected, graph_impl::edge_storage_generator_t<graph_flavour::undirected, EdgeWeight, EdgeMetaData, typename static_edge_storage_config<graph_flavour::undirected, Size, Order>::index_type, static_edge_storage_config<graph_flavour::undirected, Size, Order>>>,
        heterogeneous_node_storage<NodeWeights...>
      >::graph_primitive;

    using primitive_type::set_edge_weight;
    using primitive_type::mutate_edge_weight;
    using primitive_type::sort_edges;
    using primitive_type::stable_sort_edges;
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
  class heterogeneous_embedded_graph final : public
    graph_primitive
    <
      connectivity<graph_flavour::undirected_embedded, graph_impl::edge_storage_generator_t<graph_flavour::undirected_embedded, EdgeWeight, EdgeMetaData, typename static_edge_storage_config<graph_flavour::undirected_embedded, Size, Order>::index_type, static_edge_storage_config<graph_flavour::undirected_embedded, Size, Order>>>,
      heterogeneous_node_storage<NodeWeights...>
    >
  {
  private:
    using primitive_type =
      graph_primitive
      <
        connectivity<graph_flavour::undirected_embedded, graph_impl::edge_storage_generator_t<graph_flavour::undirected_embedded, EdgeWeight, EdgeMetaData, typename static_edge_storage_config<graph_flavour::undirected_embedded, Size, Order>::index_type, static_edge_storage_config<graph_flavour::undirected_embedded, Size, Order>>>,
        heterogeneous_node_storage<NodeWeights...>
      >;

  public:
    static_assert(sizeof...(NodeWeights) == Order);

    constexpr static graph_flavour flavour{graph_flavour::undirected_embedded};

    constexpr static std::size_t order() noexcept { return Order; }

    constexpr static std::size_t size() noexcept { return Size; }

    using
      graph_primitive
      <
        connectivity<graph_flavour::undirected_embedded, graph_impl::edge_storage_generator_t<graph_flavour::undirected_embedded, EdgeWeight, EdgeMetaData, typename static_edge_storage_config<graph_flavour::undirected_embedded, Size, Order>::index_type, static_edge_storage_config<graph_flavour::undirected_embedded, Size, Order>>>,
        heterogeneous_node_storage<NodeWeights...>
      >::graph_primitive;

    using primitive_type::set_edge_weight;
    using primitive_type::mutate_edge_weight;
  };
}
