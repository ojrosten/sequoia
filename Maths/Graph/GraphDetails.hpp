////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file GraphDetails.hpp
    \brief Meta-programming elements for graph implementation.

 */

#include "Edge.hpp"

namespace sequoia
{
  namespace data_sharing
  {
    template <class> struct shared;
    template <class> struct independent;
    template <class> class unpooled;
  }

  namespace data_structures
  {
    template <class, std::size_t, std::size_t, class> class static_contiguous_storage;
  }
  
  namespace maths
  {
    enum class directed_flavour { undirected, directed };

    constexpr bool directed(const directed_flavour directedness) noexcept
    {
      return directedness == directed_flavour::directed;
    }

    enum class graph_flavour {undirected, undirected_embedded, directed, directed_embedded};

    constexpr bool undirected(const graph_flavour flavour) noexcept
    {
      return (flavour == graph_flavour::undirected) || ( flavour == graph_flavour::undirected_embedded);
    }

    constexpr directed_flavour to_directedness(const graph_flavour gf) noexcept
    {
      return ((gf == graph_flavour::directed) || (gf == graph_flavour::directed_embedded)) ? directed_flavour::directed : directed_flavour::undirected;
    }
    
    enum class edge_sharing_preference {agnostic, shared_edge, shared_weight, independent};

    namespace graph_impl
    {      
      constexpr std::size_t num_static_edges(const graph_flavour flavour, const std::size_t size) noexcept
      {
        return (flavour != graph_flavour::directed) ? 2*size : size;
      }

      // Sharing

      template<bool Sharing>
      struct sharing_v_to_type
      {
        template<class W> using policy = data_sharing::shared<W>;
      };

      template<>
      struct sharing_v_to_type<false>
      {
        template<class W> using policy = data_sharing::independent<W>;
      };

      // Edge Weight Wrapper

      template<class EdgeWeight, template <class, template<class> class...> class EdgeWeightPooling, bool=std::is_empty_v<EdgeWeight>>
      struct edge_weight_wrapper
      {
        using proxy = typename EdgeWeightPooling<EdgeWeight>::proxy;
      };


      template<class EdgeWeight, template <class, template<class> class...> class EdgeWeightPooling>
      struct edge_weight_wrapper<EdgeWeight, EdgeWeightPooling, true>
      {
        using proxy = typename utilities::protective_wrapper<EdgeWeight>;
      };
      
      // Edge (Weight) Sharing      
       
      template<class EdgeWeight, template <class, template<class> class...> class EdgeWeightPooling>
      constexpr bool big_proxy() noexcept
      {        
        // 2 * sizeof(proxy) > 2 * sizeof(proxy*) + sizeof(proxy)
        return sizeof(typename edge_weight_wrapper<EdgeWeight, EdgeWeightPooling>::proxy)
          > 2*sizeof(typename edge_weight_wrapper<EdgeWeight,EdgeWeightPooling>::proxy*);
      }

      template<class EdgeWeight, template <class, template<class> class...> class EdgeWeightPooling>
      constexpr bool copy_constructible_proxy() noexcept
      {
        using proxy = typename edge_weight_wrapper<EdgeWeight, EdgeWeightPooling>::proxy;
        return std::is_copy_constructible_v<proxy>;
      }

      template
      <
        graph_flavour GraphFlavour,
        edge_sharing_preference SharingPreference,
        class EdgeWeight,
        template <class, template<class> class...> class EdgeWeightPooling
      >
      struct sharing_traits
      {
      private:
        constexpr static bool default_weight_sharing{
              undirected(GraphFlavour)
           && (big_proxy<EdgeWeight, EdgeWeightPooling>() || !copy_constructible_proxy<EdgeWeight, EdgeWeightPooling>())
        };
      public:
        static_assert((GraphFlavour != graph_flavour::directed) || (SharingPreference != edge_sharing_preference::shared_weight), "A directed graph without embedding cannot have shared weights");

        static_assert((SharingPreference != edge_sharing_preference::shared_edge) || (GraphFlavour == graph_flavour::directed_embedded), "Edges may only be shared for directed, embedded graphs");
        
        constexpr static bool shared_edge_v{
          (GraphFlavour == graph_flavour::directed_embedded)
          && ((SharingPreference == edge_sharing_preference::shared_edge) || (SharingPreference == edge_sharing_preference::agnostic))
        };

        constexpr static bool shared_weight_v{
               SharingPreference == edge_sharing_preference::shared_weight
          || ((SharingPreference == edge_sharing_preference::agnostic) && default_weight_sharing)
        };
      };
      
          
      // Edge Init Type

      template<class Edge, graph_flavour GraphFlavour, edge_flavour=Edge::flavour>
      struct edge_init_type_generator
      {
        using weight_type = typename Edge::weight_type;        
        using edge_init_type = embedded_edge<weight_type, sharing_v_to_type<false>::template policy, utilities::protective_wrapper<weight_type>, typename Edge::index_type>;

        constexpr static bool complementary_data_v{true};        
      };
      
      template<class Edge> struct edge_init_type_generator<Edge, graph_flavour::undirected, edge_flavour::partial>
      {
        using weight_type = typename Edge::weight_type;       
        using edge_init_type = partial_edge<weight_type, sharing_v_to_type<false>::template policy, utilities::protective_wrapper<weight_type>, typename Edge::index_type>;

        constexpr static bool complementary_data_v{false};
      };

      template<class Edge> struct edge_init_type_generator<Edge, graph_flavour::undirected_embedded, edge_flavour::partial>
      {
        using weight_type = typename Edge::weight_type;       
        using edge_init_type = embedded_partial_edge<weight_type, sharing_v_to_type<false>::template policy, utilities::protective_wrapper<weight_type>, typename Edge::index_type>;

        constexpr static bool complementary_data_v{true};
      };
      
      template<class Edge> struct edge_init_type_generator<Edge, graph_flavour::directed, edge_flavour::partial>
      {
        using weight_type = typename Edge::weight_type;       
        using edge_init_type = partial_edge<weight_type, sharing_v_to_type<false>::template policy, utilities::protective_wrapper<weight_type>, typename Edge::index_type>;

        constexpr static bool complementary_data_v{false};
      };
      
      template<class Edge, graph_flavour GraphFlavour>
      struct edge_init_type_generator<Edge, GraphFlavour, edge_flavour::partial_embedded>
      {
        using weight_type = typename Edge::weight_type;        
        using edge_init_type = embedded_partial_edge<weight_type, sharing_v_to_type<false>::template policy, utilities::protective_wrapper<weight_type>, typename Edge::index_type>;

        constexpr static bool complementary_data_v{true};
      };      
      
      // Edge Type

      // undirected, directed: partial_edge
      template
      <
        graph_flavour GraphFlavour,
        class EdgeWeight,
        template <class, template<class> class...> class EdgeWeightPooling,
        class IndexType,
        edge_sharing_preference SharingPreference,
        bool SharedEdge=sharing_traits<GraphFlavour, SharingPreference, EdgeWeight, EdgeWeightPooling>::shared_edge_v
      >
      struct edge_type_generator
      {
        using sharing = sharing_traits<GraphFlavour, SharingPreference, EdgeWeight, EdgeWeightPooling>;
        constexpr static bool shared_weight_v{sharing::shared_weight_v};
        constexpr static bool shared_edge_v{SharedEdge};
        
        using edge_weight_proxy = typename edge_weight_wrapper<EdgeWeight, EdgeWeightPooling>::proxy;
        using edge_type = partial_edge<EdgeWeight, sharing_v_to_type<shared_weight_v>::template policy, edge_weight_proxy, IndexType>;
        using edge_init_type = typename edge_init_type_generator<edge_type, GraphFlavour>::edge_init_type;
        
        constexpr static bool init_complementary_data_v{edge_init_type_generator<edge_type, GraphFlavour>::complementary_data_v};
      };

      // undirected_embedded: embedded_partial_edge
      template
      <
        class EdgeWeight,
        template <class, template<class> class...> class EdgeWeightPooling,
        class IndexType,
        edge_sharing_preference SharingPreference,
        bool SharedEdge
      >
      struct edge_type_generator<
        graph_flavour::undirected_embedded,
        EdgeWeight,
        EdgeWeightPooling,
        IndexType,
        SharingPreference,
        SharedEdge
      >
      {
        using sharing = sharing_traits<graph_flavour::undirected_embedded, SharingPreference, EdgeWeight, EdgeWeightPooling>;
        constexpr static bool shared_weight_v{sharing::shared_weight_v};
        constexpr static bool shared_edge_v{sharing::shared_edge_v};
        
        using edge_weight_proxy = typename edge_weight_wrapper<EdgeWeight, EdgeWeightPooling>::proxy;
        using edge_type = embedded_partial_edge<EdgeWeight, sharing_v_to_type<shared_weight_v>::template policy, edge_weight_proxy, IndexType>;
        using edge_init_type = typename edge_init_type_generator<edge_type, graph_flavour::undirected_embedded>::edge_init_type;

        constexpr static bool init_complementary_data_v{edge_init_type_generator<edge_type, graph_flavour::undirected_embedded>::complementary_data_v};
      };

      // directed_embedded: (shared) edge
      template
      <
        class EdgeWeight,
        template <class, template<class> class...> class EdgeWeightPooling,
        class IndexType,
        edge_sharing_preference SharingPreference
      >
      struct edge_type_generator<graph_flavour::directed_embedded, EdgeWeight, EdgeWeightPooling, IndexType, SharingPreference, true>
      {
        using sharing = sharing_traits<graph_flavour::directed_embedded, SharingPreference, EdgeWeight, EdgeWeightPooling>;
        constexpr static bool shared_weight_v{sharing::shared_weight_v};
        constexpr static bool shared_edge_v{sharing::shared_edge_v};
        static_assert(shared_edge_v && !shared_weight_v);

        using edge_weight_proxy = typename edge_weight_wrapper<EdgeWeight, EdgeWeightPooling>::proxy;
        using edge_type = edge<EdgeWeight, edge_weight_proxy, IndexType>;
        using edge_init_type = typename edge_init_type_generator<edge_type, graph_flavour::directed_embedded>::edge_init_type;
        
        constexpr static bool init_complementary_data_v{edge_init_type_generator<edge_type, graph_flavour::directed_embedded>::complementary_data_v};
      };

      // directed_embedded: embedded edge
      template
      <
        class EdgeWeight,        
        template <class, template<class> class...> class EdgeWeightPooling,
        class IndexType,
        edge_sharing_preference SharingPreference
      >
      struct edge_type_generator<graph_flavour::directed_embedded, EdgeWeight, EdgeWeightPooling, IndexType, SharingPreference, false>
      {
        using sharing = sharing_traits<graph_flavour::directed_embedded, SharingPreference, EdgeWeight, EdgeWeightPooling>;
        constexpr static bool shared_weight_v{sharing::shared_weight_v};
        constexpr static bool shared_edge_v{sharing::shared_edge_v};      
        static_assert(!sharing::shared_edge_v);
        
        using edge_weight_proxy = typename edge_weight_wrapper<EdgeWeight, EdgeWeightPooling>::proxy;
        using edge_type = embedded_edge<EdgeWeight, sharing_v_to_type<shared_weight_v>::template policy, edge_weight_proxy, IndexType>;
        using edge_init_type = typename edge_init_type_generator<edge_type, graph_flavour::directed_embedded>::edge_init_type;
        
        constexpr static bool init_complementary_data_v{edge_init_type_generator<edge_type, graph_flavour::directed_embedded>::complementary_data_v};
      };
            
      // Dynamic Edge Traits

      template<bool>
      struct shared_edge_v_to_policy
      {
        template<class EdgeType>
        using edge_storage_sharing_policy = data_sharing::independent<EdgeType>;
      };

      template<>
      struct shared_edge_v_to_policy<true>
      {
        template<class EdgeType>
        using edge_storage_sharing_policy = data_sharing::shared<EdgeType>;
      };
      
      // Weight Makers
      
      template<class WeightPooling, bool=std::is_empty_v<WeightPooling>>
      class weight_maker
      {
      public:
        using weight_proxy = typename WeightPooling::proxy;

        template<class... Args>
        [[nodiscard]]
        constexpr static weight_proxy make(Args&&... args)
        {
          return WeightPooling::make(std::forward<Args>(args)...);
        }
      private:
      };

      template<class WeightPooling>
      class weight_maker<WeightPooling, false>
      {
      public:
        using weight_proxy = typename WeightPooling::proxy;

        template<class... Args>
        [[nodiscard]]
        weight_proxy make(Args&&... args)
        {
          return m_Pool.make(std::forward<Args>(args)...);
        }
      private:
        WeightPooling m_Pool;
      };
                           
      // Determine dynamic reservartion type etc

      template<class T, class = std::void_t<>>
      struct has_reservable_partitions : std::false_type
      {};

      template<class T>
      struct has_reservable_partitions<T, std::void_t<decltype(std::declval<T>().reserve_partition(0, 0))>> : std::true_type
      {};

      template<class T> constexpr bool has_reservable_partitions_v{has_reservable_partitions<T>::value};

      // IndexType for Static Graphs

      enum class index_type { u_char, u_short, u_int, u_long};

      template
      <
        std::size_t Size,
        std::size_t Order,
        bool Embedded
      >
      constexpr index_type to_index_max() noexcept
      {
        if constexpr((Order < 255) && (!Embedded || (Size < 255))) return index_type::u_char;
        else if constexpr((Order < 65535) && (!Embedded || (Size < 65535))) return index_type::u_short;
        else return index_type::u_long;        
      }

      template
      <                          
        std::size_t Size,
        std::size_t Order,
        bool Embedded,
        index_type=to_index_max<Size, Order, Embedded>()
      >
      struct static_edge_index_type_generator
      {
        using index_type = std::size_t;
      };      

      template
      <                          
        std::size_t Size,
        std::size_t Order,
        bool Embedded             
      >
      struct static_edge_index_type_generator<Size, Order, Embedded, index_type::u_char>
      {
        using index_type = unsigned char;
      };

      template
      <                          
        std::size_t Size,
        std::size_t Order,
        bool Embedded             
      >
      struct static_edge_index_type_generator<Size, Order, Embedded, index_type::u_short>
      {
        using index_type = unsigned short;
      };            
    }
  }
}
