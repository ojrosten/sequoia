////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file HeterogeneousStaticGraph.hpp
    \brief Traits and classes for static graphs with heterogeneous node weights.

 */

#include "GraphImpl.hpp"
#include "StaticGraphImpl.hpp"
#include "HeterogeneousNodeStorage.hpp"

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
    directed_flavour Directedness,      
    std::size_t Size,
    std::size_t Order,      
    class EdgeWeight,
    class... NodeWeights
  >
  class heterogeneous_static_graph final : public
    graph_primitive
    <
      connectivity
      <
        Directedness,
        graph_impl::static_edge_traits
        <
          (Directedness == directed_flavour::directed) ? graph_flavour::directed : graph_flavour::undirected,
          Order,
          Size,
          EdgeWeight,
          typename heterogeneous_graph_traits<Size, Order, EdgeWeight>::edge_index_type
        >,
        graph_impl::weight_maker<data_sharing::unpooled<EdgeWeight>>
      >,
      graph_impl::heterogeneous_node_storage<NodeWeights...>
    >
  {
  private:
    using primitive_type =
      graph_primitive
      <
        connectivity
        <
          Directedness,
          graph_impl::static_edge_traits
          <
            (Directedness == directed_flavour::directed) ? graph_flavour::directed : graph_flavour::undirected,
            Order,
            Size,
            EdgeWeight,
            typename heterogeneous_graph_traits<Size, Order, EdgeWeight>::edge_index_type
          >,
          graph_impl::weight_maker<data_sharing::unpooled<EdgeWeight>>
        >,
        graph_impl::heterogeneous_node_storage<NodeWeights...>
      >;

  public:
    static_assert(sizeof...(NodeWeights) == Order);
    
    constexpr static graph_flavour flavour{(Directedness == directed_flavour::directed) ? graph_flavour::directed : graph_flavour::undirected};

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
          Directedness,
          graph_impl::static_edge_traits
          <
            (Directedness == directed_flavour::directed) ? graph_flavour::directed : graph_flavour::undirected,
            Order,
            Size,
            EdgeWeight,
            typename heterogeneous_graph_traits<Size, Order, EdgeWeight>::edge_index_type
          >,
          graph_impl::weight_maker<data_sharing::unpooled<EdgeWeight>>
        >,
        graph_impl::heterogeneous_node_storage<NodeWeights...>
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
    directed_flavour Directedness,      
    std::size_t Size,
    std::size_t Order,      
    class EdgeWeight,
    class... NodeWeights
  >
  class heterogeneous_embedded_static_graph final : public
    graph_primitive
    <
      connectivity
      <
        Directedness,
        graph_impl::static_edge_traits
        <
          (Directedness == directed_flavour::directed) ? graph_flavour::directed_embedded : graph_flavour::undirected_embedded,
          Order,
          Size,
          EdgeWeight,
          typename heterogeneous_embedded_graph_traits<Size, Order, EdgeWeight>::edge_index_type
        >,
        graph_impl::weight_maker<data_sharing::unpooled<EdgeWeight>>
      >,
      graph_impl::heterogeneous_node_storage<NodeWeights...>
    >
  {
  private:
    using primitive_type =
      graph_primitive
      <
        connectivity
        <
          Directedness,
          graph_impl::static_edge_traits
          <
            (Directedness == directed_flavour::directed) ? graph_flavour::directed_embedded : graph_flavour::undirected_embedded,
            Order,
            Size,
            EdgeWeight,
            typename heterogeneous_embedded_graph_traits<Size, Order, EdgeWeight>::edge_index_type
          >,
          graph_impl::weight_maker<data_sharing::unpooled<EdgeWeight>>
        >,
        graph_impl::heterogeneous_node_storage<NodeWeights...>
      >;

  public:
    static_assert(sizeof...(NodeWeights) == Order);
    
    constexpr static graph_flavour flavour{(Directedness == directed_flavour::directed) ? graph_flavour::directed_embedded : graph_flavour::undirected_embedded};

    constexpr static std::size_t order() noexcept { return Order; }

    constexpr static std::size_t size() noexcept { return Size; }

    using edge_index_type = typename heterogeneous_embedded_graph_traits<Size, Order, EdgeWeight>::edge_index_type;
      
    using
      graph_primitive
      <
        connectivity
        <
          Directedness,
          graph_impl::static_edge_traits
          <
            (Directedness == directed_flavour::directed) ? graph_flavour::directed_embedded : graph_flavour::undirected_embedded,
            Order,
            Size,
            EdgeWeight,
            typename heterogeneous_embedded_graph_traits<Size, Order, EdgeWeight>::edge_index_type
          >,
          graph_impl::weight_maker<data_sharing::unpooled<EdgeWeight>>
        >,
        graph_impl::heterogeneous_node_storage<NodeWeights...>
      >::graph_primitive;

    using primitive_type::set_edge_weight;
    using primitive_type::mutate_edge_weight;   
  };
}
