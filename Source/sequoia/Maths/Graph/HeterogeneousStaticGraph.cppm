////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

module;

#include "sequoia/PlatformSpecific/Macros.hpp"

#include <algorithm>
#include <array>
#include <concepts>
#include <execution>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <numeric>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

export module sequoia.maths.graph:HeterogeneousStaticGraph;

import :Connectivity;
import :Edge;
import :EdgesAndNodesUtilities;
import :GraphDetails;
import :GraphErrors;
import :GraphPrimitive;
import :GraphTraits;
import :HeterogeneousNodeStorage;
import :StaticGraphConfig;
import :StaticGraphDetails;
export import sequoia.algorithms;
export import sequoia.core.container_utilities;
export import sequoia.core.data_structures;
export import sequoia.core.meta;
export import sequoia.core.object;
export import sequoia.maths.sequences;
export import sequoia.platform_specific;

/** \file
    \brief Classes for static graphs with heterogeneous node weights.

 */



export namespace sequoia::maths
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
