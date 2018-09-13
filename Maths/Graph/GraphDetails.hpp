#pragma once

#include "Edge.hpp"

#include <map>
#include <queue>
#include <stack>

namespace sequoia
{
  namespace data_sharing
  {
    template <class> class shared;
    template <class> class independent;
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

    namespace graph_impl
    {      
      constexpr std::size_t num_static_edges(const graph_flavour flavour, const std::size_t size) noexcept
      {
        return (flavour != graph_flavour::directed) ? 2*size : size;
      }

      // Sharing

      template<bool Sharing>
      struct sharing
      {
        template<class W> using policy = data_sharing::shared<W>;
      };

      template<>
      struct sharing<false>
      {
        template<class W> using policy = data_sharing::independent<W>;
      };


      // Edge Weight Wrapper

      template<class EdgeWeight, template <class> class EdgeWeightPooling, bool=std::is_empty_v<EdgeWeight>>
      struct edge_weight_wrapper
      {
        using proxy = typename EdgeWeightPooling<EdgeWeight>::proxy;
      };


      template<class EdgeWeight, template <class> class EdgeWeightPooling>
      struct edge_weight_wrapper<EdgeWeight, EdgeWeightPooling, true>
      {
        using proxy = typename utilities::protective_wrapper<EdgeWeight>;
      };
      
      // Edge Weight Sharing

      template<class EdgeWeight, template <class> class EdgeWeightPooling>
      constexpr bool big_proxy() noexcept
      {        
        // 2 * sizeof(proxy) > 2 * sizeof(proxy*) + sizeof(proxy)
        return sizeof(typename edge_weight_wrapper<EdgeWeight, EdgeWeightPooling>::proxy)
          > 2*sizeof(typename edge_weight_wrapper<EdgeWeight,EdgeWeightPooling>::proxy*);
      }
      
      template<bool Static, graph_flavour GraphFlavour, class EdgeWeight, template <class> class EdgeWeightPooling>
      struct shared_edge_weight
        : public std::bool_constant<!Static && undirected(GraphFlavour) && big_proxy<EdgeWeight, EdgeWeightPooling>()>
      {
      };

      template<bool Static, graph_flavour GraphFlavour, class EdgeWeight, template <class> class EdgeWeightPooling>
      constexpr bool shared_edge_weight_v{shared_edge_weight<Static, GraphFlavour, EdgeWeight, EdgeWeightPooling>::value};

      // Edge Sharing
      
      template<bool Static, graph_flavour GraphFlavour>
      struct shared_edge : public std::bool_constant<!Static && (GraphFlavour == graph_flavour::directed_embedded)>
      {
      };

      template<bool Static, graph_flavour GraphFlavour>
      constexpr bool shared_edge_v{shared_edge<Static, GraphFlavour>::value};
      
      // Edge Init Type

      template<class Edge, graph_flavour GraphFlavour, edge_flavour=Edge::flavour>
      struct edge_init_type_generator
      {
        using weight_type = typename Edge::weight_type;        
        using edge_init_type = embedded_edge<weight_type, sharing<false>::template policy, utilities::protective_wrapper<weight_type>, typename Edge::index_type>;

        constexpr static bool complementary_data_v{true};        
      };
      
      template<class Edge> struct edge_init_type_generator<Edge, graph_flavour::undirected, edge_flavour::partial>
      {
        using weight_type = typename Edge::weight_type;       
        using edge_init_type = partial_edge<weight_type, sharing<false>::template policy, utilities::protective_wrapper<weight_type>, typename Edge::index_type>;

        constexpr static bool complementary_data_v{false};
      };

      template<class Edge> struct edge_init_type_generator<Edge, graph_flavour::undirected_embedded, edge_flavour::partial>
      {
        using weight_type = typename Edge::weight_type;       
        using edge_init_type = embedded_partial_edge<weight_type, sharing<false>::template policy, utilities::protective_wrapper<weight_type>, typename Edge::index_type>;

        constexpr static bool complementary_data_v{true};
      };
      
      template<class Edge> struct edge_init_type_generator<Edge, graph_flavour::directed, edge_flavour::partial>
      {
        using weight_type = typename Edge::weight_type;       
        using edge_init_type = partial_edge<weight_type, sharing<false>::template policy, utilities::protective_wrapper<weight_type>, typename Edge::index_type>;

        constexpr static bool complementary_data_v{false};
      };
      
      template<class Edge, graph_flavour GraphFlavour>
      struct edge_init_type_generator<Edge, GraphFlavour, edge_flavour::partial_embedded>
      {
        using weight_type = typename Edge::weight_type;        
        using edge_init_type = embedded_partial_edge<weight_type, sharing<false>::template policy, utilities::protective_wrapper<weight_type>, typename Edge::index_type>;

        constexpr static bool complementary_data_v{true};
      };      
      
      // Edge Type

      // undirected, directed: partial_edge
      template
      <
        graph_flavour GraphFlavour,
        class EdgeWeight,
        template <class> class EdgeWeightPooling,
        class IndexType,
        bool Static
      >
      struct edge_type_generator
      {        
        constexpr static bool shared_weight_v{shared_edge_weight<Static, GraphFlavour, EdgeWeight, EdgeWeightPooling>::value};
        
        using edge_weight_proxy = typename edge_weight_wrapper<EdgeWeight, EdgeWeightPooling>::proxy;
        using edge_type = partial_edge<EdgeWeight, sharing<shared_weight_v>::template policy, edge_weight_proxy, IndexType>;
        using edge_init_type = typename edge_init_type_generator<edge_type, GraphFlavour>::edge_init_type;
        
        constexpr static bool init_complementary_data_v{edge_init_type_generator<edge_type, GraphFlavour>::complementary_data_v};
      };

      // undirected_embedded: embedded_partial_edge
      template
      <
        class EdgeWeight,
        template <class> class EdgeWeightPooling,
        class IndexType,
        bool Static
      >
      struct edge_type_generator<
        graph_flavour::undirected_embedded,
        EdgeWeight,
        EdgeWeightPooling,
        IndexType,
        Static
      >
      {        
        constexpr static bool shared_weight_v{shared_edge_weight<Static, graph_flavour::undirected_embedded, EdgeWeight, EdgeWeightPooling>::value};
        
        using edge_weight_proxy = typename edge_weight_wrapper<EdgeWeight, EdgeWeightPooling>::proxy;
        using edge_type = embedded_partial_edge<EdgeWeight, sharing<shared_weight_v>::template policy, edge_weight_proxy, IndexType>;
        using edge_init_type = typename edge_init_type_generator<edge_type, graph_flavour::undirected_embedded>::edge_init_type;

        constexpr static bool init_complementary_data_v{edge_init_type_generator<edge_type, graph_flavour::undirected_embedded>::complementary_data_v};
      };

      // directed_embedded, dynamic
      template
      <
        class EdgeWeight,
        template <class> class EdgeWeightPooling,
        class IndexType
      >
      struct edge_type_generator<graph_flavour::directed_embedded, EdgeWeight, EdgeWeightPooling, IndexType, false>
      {
        constexpr static bool shared_weight_v{};

        using edge_weight_proxy = typename edge_weight_wrapper<EdgeWeight, EdgeWeightPooling>::proxy;
        using edge_type = edge<EdgeWeight, edge_weight_proxy, IndexType>;
        using edge_init_type = typename edge_init_type_generator<edge_type, graph_flavour::directed_embedded>::edge_init_type;
        
        constexpr static bool init_complementary_data_v{edge_init_type_generator<edge_type, graph_flavour::directed_embedded>::complementary_data_v};
      };

      // directed_embedded, static
      template
      <
        class EdgeWeight,        
        template <class> class EdgeWeightPooling,
        class IndexType
      >
      struct edge_type_generator<graph_flavour::directed_embedded, EdgeWeight, EdgeWeightPooling, IndexType, true>
      {
        constexpr static bool shared_weight_v{};
        
        using edge_weight_proxy = utilities::protective_wrapper<EdgeWeight>;
        using edge_type = embedded_edge<EdgeWeight, sharing<false>::template policy, edge_weight_proxy, IndexType>;
        using edge_init_type = typename edge_init_type_generator<edge_type, graph_flavour::directed_embedded>::edge_init_type;
        
        constexpr static bool init_complementary_data_v{edge_init_type_generator<edge_type, graph_flavour::directed_embedded>::complementary_data_v};
      };
      

      // Edge Traits

      template
      <        
        graph_flavour GraphFlavour,
        class EdgeWeight,
        template <class> class EdgeWeightPooling,
        template<class, template<class> class> class EdgeStorageTraits,
        class IndexType
      >
      struct edge_traits : public edge_type_generator<GraphFlavour, EdgeWeight, EdgeWeightPooling, IndexType, false>
      {
        using edge_type = typename edge_type_generator<GraphFlavour, EdgeWeight, EdgeWeightPooling, IndexType, false>::edge_type;
        using edge_storage_sharing_policy =  data_sharing::independent<edge_type>;
        using edge_storage_traits = typename EdgeStorageTraits<EdgeWeight, EdgeWeightPooling>::template traits_type<edge_type, edge_storage_sharing_policy>;
        using edge_storage_type = typename EdgeStorageTraits<EdgeWeight, EdgeWeightPooling>::template storage_type<edge_type, edge_storage_sharing_policy, edge_storage_traits>;

        constexpr static bool shared_edge_v{};
        constexpr static bool mutual_info_v{true};
      };
      
      template
      <
        class EdgeWeight,
        template <class> class EdgeWeightPooling,
        template<class, template<class> class> class EdgeStorageTraits,
        class IndexType
      >
      struct edge_traits
      <        
        graph_flavour::directed,
        EdgeWeight,
        EdgeWeightPooling,
        EdgeStorageTraits,
        IndexType 
        > : public edge_type_generator<graph_flavour::directed, EdgeWeight, EdgeWeightPooling, IndexType, false>
      {
        using edge_type = typename edge_type_generator<graph_flavour::directed, EdgeWeight, EdgeWeightPooling, IndexType, false>::edge_type;        
        using edge_storage_sharing_policy =  data_sharing::independent<edge_type>;
        using edge_storage_traits = typename EdgeStorageTraits<EdgeWeight, EdgeWeightPooling>::template traits_type<edge_type, edge_storage_sharing_policy>;
        using edge_storage_type = typename EdgeStorageTraits<EdgeWeight, EdgeWeightPooling>::template storage_type<edge_type, edge_storage_sharing_policy, edge_storage_traits>;

        constexpr static bool shared_edge_v{};
        constexpr static bool mutual_info_v{};
      };

      template
      <
        class EdgeWeight,
        template <class> class EdgeWeightPooling,
        template<class, template<class> class> class EdgeStorageTraits,
        class IndexType
      >
      struct edge_traits
      <        
        graph_flavour::directed_embedded,
        EdgeWeight,
        EdgeWeightPooling,
        EdgeStorageTraits,
        IndexType
        > : public edge_type_generator<graph_flavour::directed_embedded, EdgeWeight, EdgeWeightPooling, IndexType, false>
      {
        using edge_type = typename edge_type_generator<graph_flavour::directed_embedded, EdgeWeight, EdgeWeightPooling, IndexType, false>::edge_type;        
        using edge_storage_sharing_policy =  data_sharing::shared<edge_type>;
        using edge_storage_traits = typename EdgeStorageTraits<EdgeWeight, EdgeWeightPooling>::template traits_type<edge_type, edge_storage_sharing_policy>;
        using edge_storage_type = typename EdgeStorageTraits<EdgeWeight, EdgeWeightPooling>::template storage_type<edge_type, edge_storage_sharing_policy, edge_storage_traits>;

        constexpr static bool shared_edge_v{true};
        constexpr static bool mutual_info_v{true};
      };


      // Static Edge Traits

      template
      <
        graph_flavour GraphFlavour,
        std::size_t Order,
        std::size_t Size,
        class EdgeWeight,
        class IndexType
      >
      struct static_edge_traits : public edge_type_generator<GraphFlavour, EdgeWeight, data_sharing::unpooled, IndexType, true>
      {
        using edge_type = typename edge_type_generator<GraphFlavour, EdgeWeight, data_sharing::unpooled, IndexType, true>::edge_type;
        using edge_storage_type = data_structures::static_contiguous_storage<edge_type, Order, num_static_edges(GraphFlavour, Size), IndexType>;

        constexpr static bool shared_edge_v{};       
        constexpr static bool mutual_info_v{GraphFlavour != graph_flavour::directed};
      };

      // Weight Makers
      
      template
      <
        class NodeWeightPooling,
        class EdgeWeightPooling,
        bool=std::is_empty_v<NodeWeightPooling>,
        bool=std::is_empty_v<EdgeWeightPooling>,
        bool=std::is_same_v<NodeWeightPooling, EdgeWeightPooling>
      >
      class weight_maker;
      

      template
      <
        class NodeWeightPooling,
        class EdgeWeightPooling,
        bool Same
      >
      class weight_maker<NodeWeightPooling, EdgeWeightPooling, true, true, Same>
      {
      public:
        using node_weight_proxy = typename NodeWeightPooling::proxy;
        using edge_weight_proxy = typename EdgeWeightPooling::proxy;

        template<class... Args>
        constexpr static node_weight_proxy make_node_weight(Args&&... args) { return NodeWeightPooling::make(std::forward<Args>(args)...); }

        template<class... Args>
        constexpr static edge_weight_proxy make_edge_weight(Args&&... args) { return EdgeWeightPooling::make(std::forward<Args>(args)...); }
      };

      template<class NodeWeightPooling, class EdgeWeightPooling>
      class weight_maker<NodeWeightPooling, EdgeWeightPooling, true, false, false>
      {
      public:
        using node_weight_proxy = typename NodeWeightPooling::proxy;
        using edge_weight_proxy = typename EdgeWeightPooling::proxy;

        template<class... Args>
        constexpr static node_weight_proxy make_node_weight(Args&&... args) { return NodeWeightPooling::make(std::forward<Args>(args)...); }

        template<class... Args>
        edge_weight_proxy make_edge_weight(Args&&... args) { return m_EdgeWeightPooling.make(std::forward<Args>(args)...); }
      private: 
        EdgeWeightPooling m_EdgeWeightPooling;
      };

      template<class NodeWeightPooling, class EdgeWeightPooling>
      class weight_maker<NodeWeightPooling, EdgeWeightPooling, false, true, false>
      {
      public:
        using node_weight_proxy = typename NodeWeightPooling::proxy;
        using edge_weight_proxy = typename EdgeWeightPooling::proxy;

        template<class... Args>
        node_weight_proxy make_node_weight(Args&&... args) { return m_NodeWeightPooling.make(std::forward<Args>(args)...); }
       
        template<class... Args>
        constexpr static edge_weight_proxy make_edge_weight(Args&&... args) { return EdgeWeightPooling::make(std::forward<Args>(args)...); }
        
      private: 
        NodeWeightPooling m_NodeWeightPooling;
      };

      template<class NodeWeightPooling, class EdgeWeightPooling>
      class weight_maker<NodeWeightPooling, EdgeWeightPooling, false, false, false>
      {
      public:
        using node_weight_proxy = typename NodeWeightPooling::proxy;
        using edge_weight_proxy = typename EdgeWeightPooling::proxy;

        template<class... Args>
        node_weight_proxy make_node_weight(Args&&... args) { return m_NodeWeightPooling.make(std::forward<Args>(args)...); }

        template<class... Args>
        edge_weight_proxy make_edge_weight(Args&&... args) { return m_EdgeWeightPooling.make(std::forward<Args>(args)...); }
      private:
        NodeWeightPooling m_NodeWeightPooling;
        EdgeWeightPooling m_EdgeWeightPooling;
      };

      template<class NodeWeightPooling, class EdgeWeightPooling>
      class weight_maker<NodeWeightPooling, EdgeWeightPooling, false, false, true>
      {
      public:
        using node_weight_proxy = typename NodeWeightPooling::proxy;
        using edge_weight_proxy = typename EdgeWeightPooling::proxy;

        template<class... Args>
        node_weight_proxy make_node_weight(Args&&... args) { return m_NodeWeightPooling.make(std::forward<Args>(args)...); }

        template<class... Args>
        edge_weight_proxy make_edge_weight(Args&&... args) { return m_NodeWeightPooling.make(std::forward<Args>(args)...); }
      private:
        NodeWeightPooling m_NodeWeightPooling;
      };

      // size_type generator

      template<class EdgeStorage, class NodeStorage, bool=std::is_empty_v<typename NodeStorage::weight_type>>
      struct size_type_generator
      {
        using size_type = typename EdgeStorage::size_type;
      };

      template<class EdgeStorage, class NodeStorage>
      struct size_type_generator<EdgeStorage, NodeStorage, false>
      {
        using size_type = std::common_type_t<typename EdgeStorage::size_type, typename NodeStorage::size_type>;
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

      // Move this to a different impl file!
      
      template<bool Forward> struct iterator_getter
      {
        template<class G> static auto begin(const G& graph, const std::size_t nodeIndex) { return graph.cbegin_edges(nodeIndex); }
        template<class G> static auto end(const G& graph, const std::size_t nodeIndex) { return graph.cend_edges(nodeIndex); }
      };

      template<> struct iterator_getter<false>
      {
        template<class G> static auto begin(const G& graph, const std::size_t nodeIndex) { return graph.crbegin_edges(nodeIndex); }
        template<class G> static auto end(const G& graph, const std::size_t nodeIndex) { return graph.crend_edges(nodeIndex); }
      };

      
      template<class Q> struct traversal_traits_base
      {
        static constexpr bool uses_forward_iterator() { return true; }
        static auto get_container_element(const Q& q) { return q.top(); }
      };

      template<> struct traversal_traits_base<std::stack<std::size_t>>
      {
        static constexpr bool uses_forward_iterator() { return false; }
        static auto get_container_element(const std::stack<std::size_t>& s) { return s.top(); }
      };


      template<> struct traversal_traits_base<std::queue<std::size_t>>
      {
        static constexpr bool uses_forward_iterator() { return true; }
        static auto get_container_element(const std::queue<std::size_t>& q) { return q.front(); }
      };

      template<class Q> struct TraversalTraits : public traversal_traits_base<Q>
      {
        template<class G> static auto begin(const G& graph, const std::size_t nodeIndex)
        {
          return iterator_getter<traversal_traits_base<Q>::uses_forward_iterator()>::begin(graph, nodeIndex);
        }
        template<class G> static auto end(const G& graph, const std::size_t nodeIndex)
        {
          return iterator_getter<traversal_traits_base<Q>::uses_forward_iterator()>::end(graph, nodeIndex);
        }
      };
      
      template<class G, class Compare>
      class node_comparer
      {
      public:
        using compare_type = Compare;

        node_comparer(const G& g) : m_Graph(g) {}
        node_comparer(const node_comparer& in) : m_Graph(in.m_Graph) {};

        bool operator()(const std::size_t index1, const std::size_t index2)
        {
          Compare comparer;
          return comparer(*(m_Graph.cbegin_node_weights() + index1), *(m_Graph.cbegin_node_weights() + index2));
        }
      private:
        const G& m_Graph;
      };

      template<class G, class Q> struct queue_constructor;
      
      template<class G, class Container, class Comparer>
      struct queue_constructor<G, std::priority_queue<std::size_t, Container, Comparer>>
      {
        static auto make(const G& g)
        {
          return std::priority_queue<std::size_t, Container, Comparer>{Comparer{g}};
        }
      };

      template <class G, class Container>
      struct queue_constructor<G, std::stack<std::size_t, Container>>
      {
        static auto make(const G& g) { return std::stack<std::size_t, Container>{}; }
      };

      template <class G, class Container>
      struct queue_constructor<G, std::queue<std::size_t, Container>>
      {
        static auto make(const G& g) { return std::queue<std::size_t, Container>{}; }
      };
    }
  }
}
