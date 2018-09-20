#pragma once

#include "GraphImpl.hpp"
#include "StaticGraphImpl.hpp"

#include "StaticNodeStorage.hpp"

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
  class static_graph : public
    graph_primitive
    <
      Directedness,
      typename graph_impl::static_edge_traits<(Directedness == directed_flavour::directed) ? graph_flavour::directed : graph_flavour::undirected, Order, Size, EdgeWeight, typename Traits::edge_index_type>,  
      graph_impl::static_node_storage<utilities::protective_wrapper<NodeWeight>, Order>,
      typename graph_impl::weight_maker<data_sharing::unpooled<NodeWeight>, data_sharing::unpooled<EdgeWeight>>
    >
  {
  private:
    using primitive =
      graph_primitive
      <
        Directedness,
        typename graph_impl::static_edge_traits<(Directedness == directed_flavour::directed) ? graph_flavour::directed : graph_flavour::undirected, Order, Size, EdgeWeight, typename Traits::edge_index_type>,      
        graph_impl::static_node_storage<utilities::protective_wrapper<NodeWeight>, Order>,
        typename graph_impl::weight_maker<data_sharing::unpooled<NodeWeight>, data_sharing::unpooled<EdgeWeight>>
      >;
      
  public:
    constexpr static graph_flavour flavour{(Directedness == directed_flavour::directed) ? graph_flavour::directed : graph_flavour::undirected};

    using node_weight_type = NodeWeight;
    using edge_index_type = typename Traits::edge_index_type;
      
    using
      graph_primitive
      <
        Directedness,
        typename graph_impl::static_edge_traits<flavour, Order, Size, EdgeWeight, edge_index_type>,        
        graph_impl::static_node_storage<utilities::protective_wrapper<NodeWeight>, Order>,
        typename graph_impl::weight_maker<data_sharing::unpooled<NodeWeight>, data_sharing::unpooled<EdgeWeight>>
      >::graph_primitive;

    using primitive::set_edge_weight;
    using primitive::mutate_edge_weight;
    using primitive::sort_edges;      
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
  class static_embedded_graph : public
    graph_primitive
    <
      Directedness,
      typename graph_impl::static_edge_traits<(Directedness == directed_flavour::directed) ? graph_flavour::directed_embedded : graph_flavour::undirected_embedded, Order, Size, EdgeWeight, typename Traits::edge_index_type>,      
      graph_impl::static_node_storage<utilities::protective_wrapper<NodeWeight>, Order>,
      typename graph_impl::weight_maker<data_sharing::unpooled<NodeWeight>, data_sharing::unpooled<EdgeWeight>>
    >
  {
  private:
    using primitive =
      graph_primitive
      <
        Directedness,
        typename graph_impl::static_edge_traits<(Directedness == directed_flavour::directed) ? graph_flavour::directed_embedded : graph_flavour::undirected_embedded, Order, Size, EdgeWeight, typename Traits::edge_index_type>, 
        graph_impl::static_node_storage<utilities::protective_wrapper<NodeWeight>, Order>,
        typename graph_impl::weight_maker<data_sharing::unpooled<NodeWeight>, data_sharing::unpooled<EdgeWeight>>
      >;
      
  public:
    constexpr static graph_flavour flavour{(Directedness == directed_flavour::directed) ? graph_flavour::directed_embedded : graph_flavour::undirected_embedded};

    using node_weight_type =  NodeWeight;
    using edge_index_type = typename Traits::edge_index_type;
      
    using graph_primitive
    <
      Directedness,
      typename graph_impl::static_edge_traits<flavour, Order, Size, EdgeWeight, edge_index_type>,        
      graph_impl::static_node_storage<utilities::protective_wrapper<NodeWeight>, Order>,
      typename graph_impl::weight_maker<data_sharing::unpooled<NodeWeight>, data_sharing::unpooled<EdgeWeight>>
    >::graph_primitive;

    using primitive::set_edge_weight;
    using primitive::mutate_edge_weight;
  };
}
