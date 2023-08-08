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
#include "sequoia/Core/Object/Handlers.hpp"

namespace sequoia
{
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

      template<bool Shared, class Weight>
      using shared_to_handler_t = std::conditional_t<Shared, object::shared<Weight>, object::by_value<Weight>>;

      template<class Weight>
      [[nodiscard]]
      constexpr bool big_weight() noexcept
      {
        return sizeof(Weight) > 2*sizeof(Weight*);
      }

      template
      <
        graph_flavour GraphFlavour,
        edge_sharing_preference SharingPreference,
        class EdgeWeight
      >
      struct sharing_traits
      {
      private:
        constexpr static bool default_weight_sharing{
              undirected(GraphFlavour)
           && (big_weight<EdgeWeight>() || !std::is_copy_constructible_v<EdgeWeight>)
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
              (SharingPreference == edge_sharing_preference::shared_weight)
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

      // Edge Type Generator
      template
      <
        graph_flavour GraphFlavour,
        class EdgeWeight,
        std::integral IndexType,
        edge_sharing_preference SharingPreference
      >
      struct edge_type_generator
      {
        using sharing = sharing_traits<GraphFlavour, SharingPreference, EdgeWeight>;
        constexpr static bool shared_weight_v{sharing::shared_weight_v};
        constexpr static bool shared_edge_v{sharing::shared_edge_v};
        constexpr static bool is_embedded_v{(GraphFlavour == graph_flavour::undirected_embedded) || (GraphFlavour == graph_flavour::directed_embedded)};
        constexpr static bool is_directed_v{(GraphFlavour == graph_flavour::directed)            || (GraphFlavour == graph_flavour::directed_embedded)};

        static_assert((GraphFlavour != graph_flavour::directed_embedded) || !sharing::shared_edge_v || !shared_weight_v);

        using handler_type     = shared_to_handler_t<shared_weight_v, EdgeWeight>;
        using edge_type        = flavour_to_edge_t<GraphFlavour, handler_type, IndexType, shared_edge_v>;
        using index_type       = typename edge_type::index_type;
        using edge_init_type   = std::conditional_t<is_embedded_v,
                                                   std::conditional_t<edge_type::flavour == edge_flavour::partial_embedded,
                                                                      embedded_partial_edge<object::by_value<EdgeWeight>, index_type>,
                                                                      embedded_edge<object::by_value<EdgeWeight>, index_type>>,
                                                   partial_edge<object::by_value<EdgeWeight>, index_type>>;
      };

      template<class T>
      inline constexpr bool has_reservable_partitions = requires(T& t) {
        t.reserve_partition(0, 0);
      };
    }
  }
}
