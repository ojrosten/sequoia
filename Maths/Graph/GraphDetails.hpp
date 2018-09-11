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

      template<class EdgeWeight, template <class> class EdgeWeightStorage, bool=std::is_empty_v<EdgeWeight>>
      struct edge_weight_wrapper
      {
        using proxy = typename EdgeWeightStorage<EdgeWeight>::proxy;
      };


      template<class EdgeWeight, template <class> class EdgeWeightStorage>
      struct edge_weight_wrapper<EdgeWeight, EdgeWeightStorage, true>
      {
        using proxy = typename utilities::protective_wrapper<EdgeWeight>;
      };
      
      // Edge Weight Sharing

      template<class EdgeWeight, template <class> class EdgeWeightStorage>
      constexpr bool big_proxy() noexcept
      {        
        // 2 * sizeof(proxy) > 2 * sizeof(proxy*) + sizeof(proxy)
        return sizeof(typename edge_weight_wrapper<EdgeWeight, EdgeWeightStorage>::proxy)
          > 2*sizeof(typename edge_weight_wrapper<EdgeWeight,EdgeWeightStorage>::proxy*);
      }
      
      template<bool Static, graph_flavour GraphFlavour, class EdgeWeight, template <class> class EdgeWeightStorage>
      struct shared_edge_weight
        : public std::bool_constant<!Static && undirected(GraphFlavour) && big_proxy<EdgeWeight, EdgeWeightStorage>()>
      {
      };

      template<bool Static, graph_flavour GraphFlavour, class EdgeWeight, template <class> class EdgeWeightStorage>
      constexpr bool shared_edge_weight_v{shared_edge_weight<Static, GraphFlavour, EdgeWeight, EdgeWeightStorage>::value};

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
        template <class> class EdgeWeightStorage,
        class IndexType,
        bool Static
      >
      struct edge_type_generator
      {        
        constexpr static bool shared_weight_v{shared_edge_weight<Static, GraphFlavour, EdgeWeight, EdgeWeightStorage>::value};
        
        using edge_weight_proxy = typename edge_weight_wrapper<EdgeWeight, EdgeWeightStorage>::proxy;
        using edge_type = partial_edge<EdgeWeight, sharing<shared_weight_v>::template policy, edge_weight_proxy, IndexType>;
        using edge_init_type = typename edge_init_type_generator<edge_type, GraphFlavour>::edge_init_type;
        
        constexpr static bool init_complementary_data_v{edge_init_type_generator<edge_type, GraphFlavour>::complementary_data_v};
      };

      // undirected_embedded: embedded_partial_edge
      template
      <
        class EdgeWeight,
        template <class> class EdgeWeightStorage,
        class IndexType,
        bool Static
      >
      struct edge_type_generator<
        graph_flavour::undirected_embedded,
        EdgeWeight,
        EdgeWeightStorage,
        IndexType,
        Static
      >
      {        
        constexpr static bool shared_weight_v{shared_edge_weight<Static, graph_flavour::undirected_embedded, EdgeWeight, EdgeWeightStorage>::value};
        
        using edge_weight_proxy = typename edge_weight_wrapper<EdgeWeight, EdgeWeightStorage>::proxy;
        using edge_type = embedded_partial_edge<EdgeWeight, sharing<shared_weight_v>::template policy, edge_weight_proxy, IndexType>;
        using edge_init_type = typename edge_init_type_generator<edge_type, graph_flavour::undirected_embedded>::edge_init_type;

        constexpr static bool init_complementary_data_v{edge_init_type_generator<edge_type, graph_flavour::undirected_embedded>::complementary_data_v};
      };

      // directed_embedded, dynamic
      template
      <
        class EdgeWeight,
        template <class> class EdgeWeightStorage,
        class IndexType
      >
      struct edge_type_generator<graph_flavour::directed_embedded, EdgeWeight, EdgeWeightStorage, IndexType, false>
      {
        constexpr static bool shared_weight_v{};

        using edge_weight_proxy = typename edge_weight_wrapper<EdgeWeight, EdgeWeightStorage>::proxy;
        using edge_type = edge<EdgeWeight, edge_weight_proxy, IndexType>;
        using edge_init_type = typename edge_init_type_generator<edge_type, graph_flavour::directed_embedded>::edge_init_type;
        
        constexpr static bool init_complementary_data_v{edge_init_type_generator<edge_type, graph_flavour::directed_embedded>::complementary_data_v};
      };

      // directed_embedded, static
      template
      <
        class EdgeWeight,        
        template <class> class EdgeWeightStorage,
        class IndexType
      >
      struct edge_type_generator<graph_flavour::directed_embedded, EdgeWeight, EdgeWeightStorage, IndexType, true>
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
        template <class> class EdgeWeightStorage,
        template<class, template<class> class> class EdgeStorageTraits,
        class IndexType
      >
      struct edge_traits : public edge_type_generator<GraphFlavour, EdgeWeight, EdgeWeightStorage, IndexType, false>
      {
        using edge_type = typename edge_type_generator<GraphFlavour, EdgeWeight, EdgeWeightStorage, IndexType, false>::edge_type;
        using edge_storage_sharing_policy =  data_sharing::independent<edge_type>;
        using edge_storage_traits = typename EdgeStorageTraits<EdgeWeight, EdgeWeightStorage>::template traits_type<edge_type, edge_storage_sharing_policy>;
        using edge_storage_type = typename EdgeStorageTraits<EdgeWeight, EdgeWeightStorage>::template storage_type<edge_type, edge_storage_sharing_policy, edge_storage_traits>;

        constexpr static bool shared_edge_v{};
        constexpr static bool mutual_info_v{true};
      };
      
      template
      <
        class EdgeWeight,
        template <class> class EdgeWeightStorage,
        template<class, template<class> class> class EdgeStorageTraits,
        class IndexType
      >
      struct edge_traits
      <        
        graph_flavour::directed,
        EdgeWeight,
        EdgeWeightStorage,
        EdgeStorageTraits,
        IndexType 
        > : public edge_type_generator<graph_flavour::directed, EdgeWeight, EdgeWeightStorage, IndexType, false>
      {
        using edge_type = typename edge_type_generator<graph_flavour::directed, EdgeWeight, EdgeWeightStorage, IndexType, false>::edge_type;        
        using edge_storage_sharing_policy =  data_sharing::independent<edge_type>;
        using edge_storage_traits = typename EdgeStorageTraits<EdgeWeight, EdgeWeightStorage>::template traits_type<edge_type, edge_storage_sharing_policy>;
        using edge_storage_type = typename EdgeStorageTraits<EdgeWeight, EdgeWeightStorage>::template storage_type<edge_type, edge_storage_sharing_policy, edge_storage_traits>;

        constexpr static bool shared_edge_v{};
        constexpr static bool mutual_info_v{};
      };

      template
      <
        class EdgeWeight,
        template <class> class EdgeWeightStorage,
        template<class, template<class> class> class EdgeStorageTraits,
        class IndexType
      >
      struct edge_traits
      <        
        graph_flavour::directed_embedded,
        EdgeWeight,
        EdgeWeightStorage,
        EdgeStorageTraits,
        IndexType
        > : public edge_type_generator<graph_flavour::directed_embedded, EdgeWeight, EdgeWeightStorage, IndexType, false>
      {
        using edge_type = typename edge_type_generator<graph_flavour::directed_embedded, EdgeWeight, EdgeWeightStorage, IndexType, false>::edge_type;        
        using edge_storage_sharing_policy =  data_sharing::shared<edge_type>;
        using edge_storage_traits = typename EdgeStorageTraits<EdgeWeight, EdgeWeightStorage>::template traits_type<edge_type, edge_storage_sharing_policy>;
        using edge_storage_type = typename EdgeStorageTraits<EdgeWeight, EdgeWeightStorage>::template storage_type<edge_type, edge_storage_sharing_policy, edge_storage_traits>;

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
        class NodeWeightStorage,
        class EdgeWeightStorage,
        bool=std::is_empty_v<NodeWeightStorage>,
        bool=std::is_empty_v<EdgeWeightStorage>,
        bool=std::is_same_v<NodeWeightStorage, EdgeWeightStorage>
      >
      class weight_maker;
      

      template
      <
        class NodeWeightStorage,
        class EdgeWeightStorage,
        bool Same
      >
      class weight_maker<NodeWeightStorage, EdgeWeightStorage, true, true, Same>
      {
      public:
        using node_weight_proxy = typename NodeWeightStorage::proxy;
        using edge_weight_proxy = typename EdgeWeightStorage::proxy;

        template<class... Args>
        constexpr static node_weight_proxy make_node_weight(Args&&... args) { return NodeWeightStorage::make(std::forward<Args>(args)...); }

        template<class... Args>
        constexpr static edge_weight_proxy make_edge_weight(Args&&... args) { return EdgeWeightStorage::make(std::forward<Args>(args)...); }
      };

      template<class NodeWeightStorage, class EdgeWeightStorage>
      class weight_maker<NodeWeightStorage, EdgeWeightStorage, true, false, false>
      {
      public:
        using node_weight_proxy = typename NodeWeightStorage::proxy;
        using edge_weight_proxy = typename EdgeWeightStorage::proxy;

        template<class... Args>
        constexpr static node_weight_proxy make_node_weight(Args&&... args) { return NodeWeightStorage::make(std::forward<Args>(args)...); }

        template<class... Args>
        edge_weight_proxy make_edge_weight(Args&&... args) { return m_EdgeWeightStorage.make(std::forward<Args>(args)...); }
      private: 
        EdgeWeightStorage m_EdgeWeightStorage;
      };

      template<class NodeWeightStorage, class EdgeWeightStorage>
      class weight_maker<NodeWeightStorage, EdgeWeightStorage, false, true, false>
      {
      public:
        using node_weight_proxy = typename NodeWeightStorage::proxy;
        using edge_weight_proxy = typename EdgeWeightStorage::proxy;

        template<class... Args>
        node_weight_proxy make_node_weight(Args&&... args) { return m_NodeWeightStorage.make(std::forward<Args>(args)...); }
       
        template<class... Args>
        constexpr static edge_weight_proxy make_edge_weight(Args&&... args) { return EdgeWeightStorage::make(std::forward<Args>(args)...); }
        
      private: 
        NodeWeightStorage m_NodeWeightStorage;
      };

      template<class NodeWeightStorage, class EdgeWeightStorage>
      class weight_maker<NodeWeightStorage, EdgeWeightStorage, false, false, false>
      {
      public:
        using node_weight_proxy = typename NodeWeightStorage::proxy;
        using edge_weight_proxy = typename EdgeWeightStorage::proxy;

        template<class... Args>
        node_weight_proxy make_node_weight(Args&&... args) { return m_NodeWeightStorage.make(std::forward<Args>(args)...); }

        template<class... Args>
        edge_weight_proxy make_edge_weight(Args&&... args) { return m_EdgeWeightStorage.make(std::forward<Args>(args)...); }
      private:
        NodeWeightStorage m_NodeWeightStorage;
        EdgeWeightStorage m_EdgeWeightStorage;
      };

      template<class NodeWeightStorage, class EdgeWeightStorage>
      class weight_maker<NodeWeightStorage, EdgeWeightStorage, false, false, true>
      {
      public:
        using node_weight_proxy = typename NodeWeightStorage::proxy;
        using edge_weight_proxy = typename EdgeWeightStorage::proxy;

        template<class... Args>
        node_weight_proxy make_node_weight(Args&&... args) { return m_NodeWeightStorage.make(std::forward<Args>(args)...); }

        template<class... Args>
        edge_weight_proxy make_edge_weight(Args&&... args) { return m_NodeWeightStorage.make(std::forward<Args>(args)...); }
      private:
        NodeWeightStorage m_NodeWeightStorage;
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
