////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Meta-programming elements for graph implementation.

 */

#include "sequoia/Maths/Graph/Edge.hpp"
#include "sequoia/Core/Ownership/DataPoolTraits.hpp"
#include "sequoia/Core/Utilities/UniformWrapper.hpp"

namespace sequoia
{
  namespace ownership
  {
    template <class> struct shared;
    template <class> struct independent;
  }

  namespace maths
  {
    enum class directed_flavour { undirected, directed };

    [[nodiscard]]
    constexpr bool directed(const directed_flavour directedness) noexcept
    {
      return directedness == directed_flavour::directed;
    }

    enum class graph_flavour {undirected, undirected_embedded, directed, directed_embedded};

    [[nodiscard]]
    constexpr bool undirected(const graph_flavour flavour) noexcept
    {
      return (flavour == graph_flavour::undirected) || ( flavour == graph_flavour::undirected_embedded);
    }

    [[nodiscard]]
    constexpr directed_flavour to_directedness(const graph_flavour gf) noexcept
    {
      return ((gf == graph_flavour::directed) || (gf == graph_flavour::directed_embedded)) ? directed_flavour::directed : directed_flavour::undirected;
    }

    enum class edge_sharing_preference {agnostic, shared_edge, shared_weight, independent};

    namespace graph_impl
    {
      [[nodiscard]]
      constexpr std::size_t num_static_edges(const graph_flavour flavour, const std::size_t size) noexcept
      {
        return (flavour != graph_flavour::directed) ? 2*size : size;
      }

      // Sharing

      template<bool Shared, class Proxy>
      struct shared_to_handler : std::conditional<Shared, ownership::shared<Proxy>, ownership::independent<Proxy>>
      {};

      template<bool Shared, class Proxy>
      using shared_to_handler_t = typename shared_to_handler<Shared, Proxy>::type;

      // Edge Weight Proxy

      template<class EdgeWeightCreator>
        requires ownership::creator<EdgeWeightCreator>
      using edge_weight_proxy_t = typename EdgeWeightCreator::proxy;

      // Edge (Weight) Sharing

      template<class EdgeWeightCreator>
        requires ownership::creator<EdgeWeightCreator>
      [[nodiscard]]
      constexpr bool big_proxy() noexcept
      {
        using proxy = edge_weight_proxy_t<EdgeWeightCreator>;

        return sizeof(proxy) > 2*sizeof(proxy*);
      }

      template<class EdgeWeightCreator>
        requires ownership::creator<EdgeWeightCreator>
      [[nodiscard]]
      constexpr bool copy_constructible_proxy() noexcept
      {
        using proxy = edge_weight_proxy_t<EdgeWeightCreator>;
        return std::is_copy_constructible_v<proxy>;
      }

      template
      <
        graph_flavour GraphFlavour,
        edge_sharing_preference SharingPreference,
        class EdgeWeightCreator
      >
        requires ownership::creator<EdgeWeightCreator>
      struct sharing_traits
      {
      private:
        constexpr static bool default_weight_sharing{
              undirected(GraphFlavour)
           && (big_proxy<EdgeWeightCreator>() || !copy_constructible_proxy<EdgeWeightCreator>())
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
        using weight_type    = typename Edge::weight_type;
        using proxy_type     = utilities::uniform_wrapper<weight_type>;
        using handler_type   = shared_to_handler_t<false, proxy_type>;
        using edge_init_type = embedded_edge<handler_type, typename Edge::index_type>;

        constexpr static bool complementary_data_v{true};
      };

      template<class Edge>
      struct edge_init_type_generator<Edge, graph_flavour::undirected, edge_flavour::partial>
      {
        using weight_type    = typename Edge::weight_type;
        using proxy_type     = utilities::uniform_wrapper<weight_type>;
        using handler_type   = shared_to_handler_t<false, proxy_type>;
        using edge_init_type = partial_edge<handler_type, typename Edge::index_type>;

        constexpr static bool complementary_data_v{false};
      };

      template<class Edge>
      struct edge_init_type_generator<Edge, graph_flavour::undirected_embedded, edge_flavour::partial>
      {
        using weight_type    = typename Edge::weight_type;
        using proxy_type     = utilities::uniform_wrapper<weight_type>;
        using handler_type   = shared_to_handler_t<false, proxy_type>;
        using edge_init_type = embedded_partial_edge<handler_type, typename Edge::index_type>;

        constexpr static bool complementary_data_v{true};
      };

      template<class Edge>
      struct edge_init_type_generator<Edge, graph_flavour::directed, edge_flavour::partial>
      {
        using weight_type    = typename Edge::weight_type;
        using proxy_type     = utilities::uniform_wrapper<weight_type>;
        using handler_type   = shared_to_handler_t<false, proxy_type>;
        using edge_init_type = partial_edge<handler_type, typename Edge::index_type>;

        constexpr static bool complementary_data_v{false};
      };

      template<class Edge, graph_flavour GraphFlavour>
      struct edge_init_type_generator<Edge, GraphFlavour, edge_flavour::partial_embedded>
      {
        using weight_type    = typename Edge::weight_type;
        using proxy_type     = utilities::uniform_wrapper<weight_type>;
        using handler_type   = shared_to_handler_t<false, proxy_type>;
        using edge_init_type = embedded_partial_edge<handler_type, typename Edge::index_type>;

        constexpr static bool complementary_data_v{true};
      };

      // Edge Type

      // undirected, directed: partial_edge
      template
      <
        graph_flavour GraphFlavour,
        class EdgeWeightCreator,
        integral IndexType,
        edge_sharing_preference SharingPreference,
        bool SharedEdge=sharing_traits<GraphFlavour, SharingPreference, EdgeWeightCreator>::shared_edge_v
      >
        requires ownership::creator<EdgeWeightCreator>
      struct edge_type_generator
      {
        using sharing = sharing_traits<GraphFlavour, SharingPreference, EdgeWeightCreator>;
        constexpr static bool shared_weight_v{sharing::shared_weight_v};
        constexpr static bool shared_edge_v{SharedEdge};

        using edge_weight_proxy = edge_weight_proxy_t<EdgeWeightCreator>;
        using handler_type      = shared_to_handler_t<shared_weight_v, edge_weight_proxy>;
        using edge_type         = partial_edge<handler_type, IndexType>;
        using edge_init_type    = typename edge_init_type_generator<edge_type, GraphFlavour>::edge_init_type;

        constexpr static bool init_complementary_data_v{edge_init_type_generator<edge_type, GraphFlavour>::complementary_data_v};
      };

      // undirected_embedded: embedded_partial_edge
      template
      <
        class EdgeWeightCreator,
        integral IndexType,
        edge_sharing_preference SharingPreference,
        bool SharedEdge
      >
        requires ownership::creator<EdgeWeightCreator>
      struct edge_type_generator
      <
        graph_flavour::undirected_embedded,
        EdgeWeightCreator,
        IndexType,
        SharingPreference,
        SharedEdge
      >
      {
        using sharing = sharing_traits<graph_flavour::undirected_embedded, SharingPreference, EdgeWeightCreator>;
        constexpr static bool shared_weight_v{sharing::shared_weight_v};
        constexpr static bool shared_edge_v{sharing::shared_edge_v};

        using edge_weight_proxy = edge_weight_proxy_t<EdgeWeightCreator>;
        using handler_type      = shared_to_handler_t<shared_weight_v, edge_weight_proxy>;
        using edge_type         = embedded_partial_edge<handler_type, IndexType>;
        using edge_init_type    = typename edge_init_type_generator<edge_type, graph_flavour::undirected_embedded>::edge_init_type;

        constexpr static bool init_complementary_data_v{edge_init_type_generator<edge_type, graph_flavour::undirected_embedded>::complementary_data_v};
      };

      // directed_embedded: (shared) edge
      template
      <
        class EdgeWeightCreator,
        integral IndexType,
        edge_sharing_preference SharingPreference
      >
        requires ownership::creator<EdgeWeightCreator>
      struct edge_type_generator<graph_flavour::directed_embedded, EdgeWeightCreator, IndexType, SharingPreference, true>
      {
        using sharing = sharing_traits<graph_flavour::directed_embedded, SharingPreference, EdgeWeightCreator>;
        constexpr static bool shared_weight_v{sharing::shared_weight_v};
        constexpr static bool shared_edge_v{sharing::shared_edge_v};
        static_assert(shared_edge_v && !shared_weight_v);

        using edge_weight_proxy = edge_weight_proxy_t<EdgeWeightCreator>;
        using handler_type      = shared_to_handler_t<shared_weight_v, edge_weight_proxy>;
        using edge_type         = edge<handler_type, IndexType>;
        using edge_init_type    = typename edge_init_type_generator<edge_type, graph_flavour::directed_embedded>::edge_init_type;

        constexpr static bool init_complementary_data_v{edge_init_type_generator<edge_type, graph_flavour::directed_embedded>::complementary_data_v};
      };

      // directed_embedded: embedded edge
      template
      <
        class EdgeWeightCreator,
        integral IndexType,
        edge_sharing_preference SharingPreference
      >
        requires ownership::creator<EdgeWeightCreator>
      struct edge_type_generator<graph_flavour::directed_embedded, EdgeWeightCreator, IndexType, SharingPreference, false>
      {
        using sharing = sharing_traits<graph_flavour::directed_embedded, SharingPreference, EdgeWeightCreator>;
        constexpr static bool shared_weight_v{sharing::shared_weight_v};
        constexpr static bool shared_edge_v{sharing::shared_edge_v};
        static_assert(!sharing::shared_edge_v);

        using edge_weight_proxy = edge_weight_proxy_t<EdgeWeightCreator>;
        using handler_type      = shared_to_handler_t<shared_weight_v, edge_weight_proxy>;
        using edge_type         = embedded_edge<handler_type, IndexType>;
        using edge_init_type    = typename edge_init_type_generator<edge_type, graph_flavour::directed_embedded>::edge_init_type;

        constexpr static bool init_complementary_data_v{edge_init_type_generator<edge_type, graph_flavour::directed_embedded>::complementary_data_v};
      };

      // Dynamic Edge Traits

      template<bool>
      struct shared_edge_v_to_policy
      {
        template<class EdgeType>
        using edge_storage_handler = ownership::independent<EdgeType>;
      };

      template<>
      struct shared_edge_v_to_policy<true>
      {
        template<class EdgeType>
        using edge_storage_handler = ownership::shared<EdgeType>;
      };

      // Determine dynamic reservartion type etc

      // TO DO: replace with contexpr bool once MSVC can cope with this
      template<class T>
      concept has_reservable_partitions = requires(T& t) {
        t.reserve_partition(0, 0);
      };

      // IndexType for Static Graphs

      enum class index_type { u_char, u_short, u_int, u_long};

      template<std::size_t Size, std::size_t Order, bool Embedded>
      [[nodiscard]]
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
