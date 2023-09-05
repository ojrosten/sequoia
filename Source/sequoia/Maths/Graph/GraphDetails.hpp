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
    enum class graph_flavour { undirected, undirected_embedded, directed };

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

    template<class T>
    struct edge_init_type_generator;

    template<class WeightHandler, class MetaData, std::integral IndexType>
      requires object::handler<WeightHandler>
    struct edge_init_type_generator<partial_edge<WeightHandler, MetaData, IndexType>>
    {
      using type = partial_edge<object::by_value<typename WeightHandler::value_type>, MetaData, IndexType>;
    };

    template<class WeightHandler, class MetaData, std::integral IndexType>
      requires object::handler<WeightHandler>
    struct edge_init_type_generator<embedded_partial_edge<WeightHandler, MetaData, IndexType>>
    {
      using type = embedded_partial_edge<object::by_value<typename WeightHandler::value_type>, MetaData, IndexType>;
    };

    template<class T>
    using edge_init_type_generator_t = typename edge_init_type_generator<T>::type;

    namespace graph_impl
    {
      template<class Edge>
      inline constexpr bool has_shared_weight_v{
        requires{
          typename Edge::weight_handler_type;
          typename Edge::weight_type;

          requires std::is_same_v<object::shared<typename Edge::weight_type>, typename Edge::weight_handler_type>;
        }
      };

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
      template<graph_flavour GraphFlavour, class Handler, class MetaData, std::integral IndexType>
        requires object::handler<Handler>
      struct flavour_to_edge
      {
        using edge_type  = partial_edge<Handler, MetaData, IndexType>;
      };

      template<class Handler, class MetaData, std::integral IndexType>
        requires object::handler<Handler>
      struct flavour_to_edge<graph_flavour::undirected_embedded, Handler, MetaData, IndexType>
      {
        using edge_type = embedded_partial_edge<Handler, MetaData, IndexType>;
      };

      template<graph_flavour GraphFlavour, class Handler, class MetaData, std::integral IndexType>
        requires object::handler<Handler>
      using flavour_to_edge_t = typename flavour_to_edge<GraphFlavour, Handler, MetaData, IndexType>::edge_type;

      template
      <
        graph_flavour GraphFlavour,
        class EdgeWeight,
        class EdgeMetaData,
        std::integral IndexType,
        class EdgeStorageConfig
      >
      struct edge_storage_generator
      {
        constexpr static graph_flavour flavour{GraphFlavour};
        constexpr static edge_sharing_preference sharing_preference{EdgeStorageConfig::edge_sharing};

        constexpr static bool default_weight_sharing{
              !is_directed(GraphFlavour)
           && (big_weight<EdgeWeight>() || !std::is_copy_constructible_v<EdgeWeight>)
        };

        constexpr static bool shared_weight_v{
              (sharing_preference == edge_sharing_preference::shared_weight)
          || ((sharing_preference == edge_sharing_preference::agnostic) && default_weight_sharing)
        };

        static_assert(!shared_weight_v || (GraphFlavour != graph_flavour::directed));
        static_assert((GraphFlavour == graph_flavour::directed) || std::is_empty_v<EdgeMetaData> || std::is_empty_v<EdgeWeight> || shared_weight_v);

        using handler_type = shared_to_handler_t<shared_weight_v, EdgeWeight>;
        using edge_type    = flavour_to_edge_t<GraphFlavour, handler_type, EdgeMetaData, IndexType>;
        using storage_type = EdgeStorageConfig::template storage_type<edge_type>;
      };

      template
      <
        graph_flavour GraphFlavour,
        class EdgeWeight,
        class EdgeMetaData,
        std::integral IndexType,
        class EdgeStorageConfig
      >
      using edge_storage_generator_t = typename edge_storage_generator<GraphFlavour, EdgeWeight, EdgeMetaData, IndexType, EdgeStorageConfig>::storage_type;

      template<class T>
      inline constexpr bool has_reservable_partitions = requires(T& t) {
        t.reserve_partition(0, 0);
      };
    }
  }
}
