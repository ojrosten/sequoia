////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file DynamicGraph.hpp
    \brief Edge & Node storage traits, base class and final classes for dynamic graphs.
  
*/

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
    constexpr static bool static_storage_v{};
    template<class S> using container_type = std::vector<S, std::allocator<S>>;
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
      connectivity
      <
        to_directedness(GraphFlavour),
        graph_impl::dynamic_edge_traits<GraphFlavour, EdgeWeight, EdgeWeightPooling, EdgeStorageTraits, std::size_t>,
        graph_impl::weight_maker<EdgeWeightPooling<EdgeWeight>>
      >,
      graph_impl::node_storage
      <     
        graph_impl::weight_maker<NodeWeightPooling<NodeWeight>>,
        NodeWeightStorageTraits<NodeWeight, NodeWeightPooling, std::is_empty_v<NodeWeight>>
      >
    >
  {       
  public:
    using primitive_type =
      graph_primitive
      <
        connectivity<
          to_directedness(GraphFlavour),
          graph_impl::dynamic_edge_traits<GraphFlavour, EdgeWeight, EdgeWeightPooling, EdgeStorageTraits, std::size_t>,
          graph_impl::weight_maker<EdgeWeightPooling<EdgeWeight>>
        >,      
        graph_impl::node_storage
        <
          graph_impl::weight_maker<NodeWeightPooling<NodeWeight>>,
          NodeWeightStorageTraits<NodeWeight, NodeWeightPooling, std::is_empty_v<NodeWeight>>
        >
      >;
    
    using node_weight_type = NodeWeight;
    
    using
      graph_primitive
      <
        connectivity<
          to_directedness(GraphFlavour),
          graph_impl::dynamic_edge_traits<GraphFlavour, EdgeWeight, EdgeWeightPooling, EdgeStorageTraits, std::size_t>,
          graph_impl::weight_maker<EdgeWeightPooling<EdgeWeight>>
        >,
        graph_impl::node_storage
        <
          graph_impl::weight_maker<NodeWeightPooling<NodeWeight>>,
          NodeWeightStorageTraits<NodeWeight, NodeWeightPooling, std::is_empty_v<NodeWeight>>
        >
      >::graph_primitive;

    graph_base(const graph_base&)            = default;
    graph_base(graph_base&&)                 = default;
    graph_base& operator=(const graph_base&) = default;
    graph_base& operator=(graph_base&&)      = default;

    using primitive_type::swap_nodes;
    
    using primitive_type::add_node;
    using primitive_type::insert_node;
    using primitive_type::erase_node;

    using primitive_type::join;      
    using primitive_type::erase_edge;

    using primitive_type::clear;

    using primitive_type::reserve_edges;
    using primitive_type::edges_capacity;
    using primitive_type::reserve_nodes;
    using primitive_type::node_capacity;
    using primitive_type::shrink_to_fit;
      
    constexpr static graph_flavour flavour{GraphFlavour};
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
  class graph final : public
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
    using base::swap_edges;
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
  class embedded_graph final : public
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
      [[nodiscard]]
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
      
      using base_type =
        graph_base
        <
          (Directedness == directed_flavour::directed) ? graph_flavour::directed_embedded : graph_flavour::undirected_embedded,      
          EdgeWeight,
          NodeWeight,     
          EdgeWeightPooling,
          NodeWeightPooling,
          EdgeStorageTraits,
          NodeWeightStorageTraits
        >;

      using base_type::primitive_type::insert_join;
    };  
}
