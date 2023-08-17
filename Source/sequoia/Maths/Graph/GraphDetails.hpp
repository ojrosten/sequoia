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

    enum class graph_flavour { undirected, undirected_embedded, directed };

    [[nodiscard]]
    constexpr directed_flavour to_directedness(const graph_flavour gf) noexcept
    {
      return (gf == graph_flavour::directed) ? directed_flavour::directed : directed_flavour::undirected;
    }

    [[nodiscard]]
    constexpr bool is_embedded(const graph_flavour gf) noexcept
    {
      return gf == graph_flavour::undirected_embedded;
    }

    [[nodiscard]]
    constexpr bool is_directed(const graph_flavour gf) noexcept
    {
      return gf == graph_flavour::directed;
    }

    enum class edge_sharing_preference {agnostic, shared_weight, independent};

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

      // Flavour to Edge
      template<graph_flavour GraphFlavour, class Handler, std::integral IndexType>
        requires object::handler<Handler>
      struct flavour_to_edge
      {
        using edge_type  = partial_edge<Handler, IndexType>;
      };

      template<class Handler, std::integral IndexType>
        requires object::handler<Handler>
      struct flavour_to_edge<graph_flavour::undirected_embedded, Handler, IndexType>
      {
        using edge_type = embedded_partial_edge<Handler, IndexType>;
      };

      template<graph_flavour GraphFlavour, class Handler, std::integral IndexType>
        requires object::handler<Handler>
      using flavour_to_edge_t = typename flavour_to_edge<GraphFlavour, Handler, IndexType>::edge_type;

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
        constexpr static graph_flavour graph_species{GraphFlavour};

        constexpr static bool default_weight_sharing{
              !is_directed(GraphFlavour)
           && (big_weight<EdgeWeight>() || !std::is_copy_constructible_v<EdgeWeight>)
        };

        constexpr static bool shared_weight_v{
              (SharingPreference == edge_sharing_preference::shared_weight)
          || ((SharingPreference == edge_sharing_preference::agnostic) && default_weight_sharing)
        };

        static_assert(!shared_weight_v || (GraphFlavour != graph_flavour::directed),
          "A directed graph without embedding cannot have shared weights");

        using handler_type     = shared_to_handler_t<shared_weight_v, EdgeWeight>;
        using edge_type        = flavour_to_edge_t<GraphFlavour, handler_type, IndexType>;
        using index_type       = typename edge_type::index_type;
        using edge_init_type   = std::conditional_t<is_embedded(GraphFlavour),
                                                    embedded_partial_edge<object::by_value<EdgeWeight>, index_type>,
                                                    partial_edge<object::by_value<EdgeWeight>, index_type>>;
      };

      template<class T>
      inline constexpr bool has_reservable_partitions = requires(T& t) {
        t.reserve_partition(0, 0);
      };
    }
  }
}
