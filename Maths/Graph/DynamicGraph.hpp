#pragma once

#include "GraphImpl.hpp"
#include "DynamicGraphImpl.hpp"

#include "NodeStorage.hpp"

namespace sequoia::maths
{
  template<graph_flavour GraphFlavour, class EdgeWeight, template <class> class EdgeWeightPooling>
  struct contiguous_edge_storage_traits
  {
    template <class T, class Sharing, class Traits> using storage_type = data_structures::contiguous_storage<T, Sharing, Traits>;
    template <class T, class Sharing> using traits_type = data_structures::contiguous_storage_traits<T, Sharing>;

    constexpr static edge_sharing_preference edge_sharing{edge_sharing_preference::agnostic};
  };

  template<graph_flavour GraphFlavour, class EdgeWeight, template <class> class EdgeWeightPooling>
  struct bucketed_edge_storage_traits
  {
    template <class T, class Sharing, class Traits> using storage_type = data_structures::bucketed_storage<T, Sharing, Traits>;
    template <class T, class Sharing> using traits_type = data_structures::bucketed_storage_traits<T, Sharing>;

    constexpr static edge_sharing_preference edge_sharing{edge_sharing_preference::agnostic};
  };

  template<class NodeWeight, template <class> class NodeWeightPooling, bool=std::is_empty_v<NodeWeight>>
  struct node_weight_storage_traits
  {
    constexpr static bool throw_on_range_error{true};
    template<class S> using underlying_storage_type = std::vector<S, std::allocator<S>>;
  };

  template<class NodeWeight, template <class> class NodeWeightPooling>
  struct node_weight_storage_traits<NodeWeight, NodeWeightPooling, true>
  {
  };
  
  template
  <
    graph_flavour GraphFlavour,      
    class EdgeWeight,
    class NodeWeight,      
    template <class> class EdgeWeightPooling,
    template <class> class NodeWeightPooling,
    template<graph_flavour, class, template<class> class> class EdgeStorageTraits,
    template<class, template<class> class, bool> class NodeWeightStorageTraits
  >
  class graph_base : public
    graph_primitive
    <
      to_directedness(GraphFlavour),      
      typename graph_impl::dynamic_edge_traits<GraphFlavour, EdgeWeight, EdgeWeightPooling, EdgeStorageTraits, std::size_t>,
      graph_impl::node_storage
      <
        typename NodeWeightPooling<NodeWeight>::proxy,
        NodeWeightStorageTraits<NodeWeight, NodeWeightPooling, std::is_empty_v<NodeWeight>>
      >,
      typename graph_impl::weight_maker<NodeWeightPooling<NodeWeight>, EdgeWeightPooling<EdgeWeight>>
    >
  {
  private:
    using primitive =
      graph_primitive
      <
        to_directedness(GraphFlavour),      
        typename graph_impl::dynamic_edge_traits<GraphFlavour, EdgeWeight, EdgeWeightPooling, EdgeStorageTraits, std::size_t>,
        graph_impl::node_storage
        <
          typename NodeWeightPooling<NodeWeight>::proxy,
          NodeWeightStorageTraits<NodeWeight, NodeWeightPooling, std::is_empty_v<NodeWeight>>
        >,
        typename graph_impl::weight_maker<NodeWeightPooling<NodeWeight>, EdgeWeightPooling<EdgeWeight>>
      >;
    
  public:
    using node_weight_type = NodeWeight;
    
    using
      graph_primitive
      <
        to_directedness(GraphFlavour),     
        typename graph_impl::dynamic_edge_traits<GraphFlavour, EdgeWeight, EdgeWeightPooling, EdgeStorageTraits, std::size_t>,
        graph_impl::node_storage
        <
          typename NodeWeightPooling<NodeWeight>::proxy,
          NodeWeightStorageTraits<NodeWeight, NodeWeightPooling, std::is_empty_v<NodeWeight>>
        >,
        typename graph_impl::weight_maker<NodeWeightPooling<NodeWeight>, EdgeWeightPooling<EdgeWeight>>
      >::graph_primitive;

    graph_base(const graph_base&)            = default;
    graph_base(graph_base&&)                 = default;
    graph_base& operator=(const graph_base&) = default;
    graph_base& operator=(graph_base&&)      = default;

    using primitive::add_node;
    using primitive::insert_node;
    using primitive::erase_node;

    using primitive::set_edge_weight;
    using primitive::mutate_edge_weight;

    using primitive::join;      
    using primitive::erase_edge;

    using primitive::clear;

    using primitive::reserve_edges;
    using primitive::edges_capacity;
    using primitive::reserve_nodes;
    using primitive::node_capacity;
    using primitive::shrink_to_fit;
      
    constexpr static graph_flavour flavour{GraphFlavour};

    constexpr typename primitive::size_type order() const noexcept { return primitive::order_impl(); }

    constexpr typename primitive::size_type size() const noexcept { return primitive::size_impl(); }
  protected:
    ~graph_base() = default;
  };

  template
  <
    directed_flavour Directedness,      
    class EdgeWeight,
    class NodeWeight,      
    template <class> class EdgeWeightPooling=data_sharing::unpooled,
    template <class> class NodeWeightPooling=data_sharing::unpooled,
    template<graph_flavour, class, template<class> class> class EdgeStorageTraits = bucketed_edge_storage_traits,
    template<class, template<class> class, bool> class NodeWeightStorageTraits = node_weight_storage_traits
  >
  class graph : public
    graph_base
    <
      (Directedness == directed_flavour::directed) ? graph_flavour::directed : graph_flavour::undirected,      
      EdgeWeight,
      NodeWeight,      
      EdgeWeightPooling,
      NodeWeightPooling,
      EdgeStorageTraits,
      NodeWeightStorageTraits    
    >
  {
  private:
    using base =
      graph_base
      <
        (Directedness == directed_flavour::directed) ? graph_flavour::directed : graph_flavour::undirected,      
         EdgeWeight,
         NodeWeight,     
         EdgeWeightPooling,
         NodeWeightPooling,
         EdgeStorageTraits,
         NodeWeightStorageTraits
      >;
  public:
    using node_weight_type = NodeWeight;
    
    using
      graph_base
      <
        (Directedness == directed_flavour::directed) ? graph_flavour::directed : graph_flavour::undirected,      
         EdgeWeight,
         NodeWeight, 
         EdgeWeightPooling,
         NodeWeightPooling,
         EdgeStorageTraits,
         NodeWeightStorageTraits
      >::graph_base;

    using base::sort_edges;
  };

  template
  <
    directed_flavour Directedness,      
    class EdgeWeight,
    class NodeWeight,      
    template <class> class EdgeWeightPooling=data_sharing::unpooled,
    template <class> class NodeWeightPooling=data_sharing::unpooled,
    template<graph_flavour, class, template<class> class> class EdgeStorageTraits=bucketed_edge_storage_traits,
    template<class, template<class> class, bool> class NodeWeightStorageTraits=node_weight_storage_traits
  >
  class embedded_graph : public
    graph_base
    <
      (Directedness == directed_flavour::directed) ? graph_flavour::directed_embedded : graph_flavour::undirected_embedded,      
      EdgeWeight,
      NodeWeight,     
      EdgeWeightPooling,
      NodeWeightPooling,
      EdgeStorageTraits,
      NodeWeightStorageTraits
    >
    {
    private:
      static constexpr graph_flavour to_graph_flavour() noexcept
      {
        return (Directedness == directed_flavour::directed) ? graph_flavour::directed_embedded : graph_flavour::undirected_embedded;
      }
    public:
      using node_weight_type = NodeWeight;
      
      using
        graph_base
        <
         (Directedness == directed_flavour::directed) ? graph_flavour::directed_embedded : graph_flavour::undirected_embedded,      
          EdgeWeight,
          NodeWeight,      
          EdgeWeightPooling,
          NodeWeightPooling,
          EdgeStorageTraits,
          NodeWeightStorageTraits
        >::graph_base;
      
      using
        graph_primitive
        <
          Directedness,        
          typename graph_impl::dynamic_edge_traits<to_graph_flavour(), EdgeWeight, EdgeWeightPooling, EdgeStorageTraits, std::size_t>,
          graph_impl::node_storage
          <
            typename NodeWeightPooling<NodeWeight>::proxy,
            NodeWeightStorageTraits<NodeWeight, NodeWeightPooling, std::is_empty_v<NodeWeight>>
          >,
          typename graph_impl::weight_maker<NodeWeightPooling<NodeWeight>, EdgeWeightPooling<EdgeWeight>>
        >::insert_join;
    };  
}
