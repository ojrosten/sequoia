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
#include "sequoia/Core/Object/Creator.hpp"
#include "sequoia/Core/Object/FaithfulWrapper.hpp"

namespace sequoia
{
  namespace object
  {
    template <class> struct shared;
    template <class> struct by_value;
  }

  namespace maths
  {
    enum class directed_flavour { undirected, directed };

    template<directed_flavour Directedness>
    struct directed_flavour_constant : std::integral_constant<directed_flavour, Directedness>
    {};

    using undirected_type = directed_flavour_constant<directed_flavour::undirected>;
    using directed_type   = directed_flavour_constant<directed_flavour::directed>;

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

      template<bool Shared, class Proxy>
      using shared_to_handler_t = std::conditional_t<Shared, object::shared<Proxy>, object::by_value<Proxy>>;

      template<class EdgeWeightCreator>
        requires object::creator<EdgeWeightCreator>
      using edge_weight_proxy_t = typename EdgeWeightCreator::product_type;

      template<class EdgeWeightCreator>
        requires object::creator<EdgeWeightCreator>
      [[nodiscard]]
      constexpr bool big_proxy() noexcept
      {
        using proxy = edge_weight_proxy_t<EdgeWeightCreator>;

        return sizeof(proxy) > 2*sizeof(proxy*);
      }

      template<class EdgeWeightCreator>
        requires object::creator<EdgeWeightCreator>
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
        requires object::creator<EdgeWeightCreator>
      struct sharing_traits
      {
      private:
        constexpr static bool default_weight_sharing{
              undirected(GraphFlavour)
           && (big_proxy<EdgeWeightCreator>() || !copy_constructible_proxy<EdgeWeightCreator>())
        };
      public:
        static_assert((GraphFlavour != graph_flavour::directed) || (SharingPreference != edge_sharing_preference::shared_weight),
                      "A directed graph without embedding cannot have shared weights");

        static_assert((SharingPreference != edge_sharing_preference::shared_edge) || (GraphFlavour == graph_flavour::directed_embedded),
                      "Edges may only be shared for directed, embedded graphs");

        constexpr static bool shared_edge_v{
          (GraphFlavour == graph_flavour::directed_embedded)
          && ((SharingPreference == edge_sharing_preference::shared_edge) || (SharingPreference == edge_sharing_preference::agnostic))
        };

        constexpr static bool shared_weight_v{
               SharingPreference == edge_sharing_preference::shared_weight
          || ((SharingPreference == edge_sharing_preference::agnostic) && default_weight_sharing)
        };
      };

      // Flavour to Edge
      template<graph_flavour GraphFlavour, class Handler, std::integral IndexType, bool SharedEdge>
        requires object::handler<Handler>
      struct flavour_to_edge
      {
        using edge_type  = partial_edge<Handler, IndexType>;
      };

      template<class Handler, std::integral IndexType, bool SharedEdge>
        requires object::handler<Handler>
      struct flavour_to_edge<graph_flavour::undirected_embedded, Handler, IndexType, SharedEdge>
      {
        using edge_type = embedded_partial_edge<Handler, IndexType>;
      };

      template<class Handler, std::integral IndexType, bool SharedEdge>
        requires object::handler<Handler>
      struct flavour_to_edge<graph_flavour::directed_embedded, Handler, IndexType, SharedEdge>
      {
        using edge_type = std::conditional_t<SharedEdge, edge<Handler, IndexType>, embedded_edge<Handler, IndexType>>;
      };

      template<graph_flavour GraphFlavour, class Handler, std::integral IndexType, bool SharedEdge>
        requires object::handler<Handler>
      using flavour_to_edge_t = typename flavour_to_edge<GraphFlavour, Handler, IndexType, SharedEdge>::edge_type;

      // Edge Init Type
      template<class Edge, bool Embedded>
      struct edge_to_init_type
      {
        using weight_type    = typename Edge::weight_type;
        using proxy_type     = object::faithful_wrapper<weight_type>;
        using handler_type   = object::by_value<proxy_type>;
        using index_type     = typename Edge::index_type;
        using edge_init_type = std::conditional_t<Embedded,
                                                  std::conditional_t<Edge::flavour == edge_flavour::partial_embedded,
                                                                     embedded_partial_edge<handler_type, index_type>,
                                                                     embedded_edge<handler_type, index_type>>,
                                                  partial_edge<handler_type, index_type>>;
      };

      template<class Edge, bool Embedded>
      using edge_to_init_type_t = typename edge_to_init_type<Edge, Embedded>::edge_init_type;

      // Edge Type Generator
      template
      <
        graph_flavour GraphFlavour,
        class EdgeWeightCreator,
        std::integral IndexType,
        edge_sharing_preference SharingPreference
      >
        requires object::creator<EdgeWeightCreator>
      struct edge_type_generator
      {
        using sharing = sharing_traits<GraphFlavour, SharingPreference, EdgeWeightCreator>;
        constexpr static bool shared_weight_v{sharing::shared_weight_v};
        constexpr static bool shared_edge_v{sharing::shared_edge_v};
        constexpr static bool is_embedded_v{
             GraphFlavour == graph_flavour::undirected_embedded
          || GraphFlavour == graph_flavour::directed_embedded};

        static_assert(  (GraphFlavour != graph_flavour::directed_embedded)
                      || !sharing::shared_edge_v || !shared_weight_v);

        using edge_weight_proxy = edge_weight_proxy_t<EdgeWeightCreator>;
        using handler_type      = shared_to_handler_t<shared_weight_v, edge_weight_proxy>;
        using edge_type         = flavour_to_edge_t<GraphFlavour, handler_type, IndexType, shared_edge_v>;
        using edge_init_type    = edge_to_init_type_t<edge_type, is_embedded_v>;
      };

      template<class T>
      inline constexpr bool has_reservable_partitions = requires(T& t) {
        t.reserve_partition(0, 0);
      };
    }
  }
}
