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

#include"sequoia/Core/DataStructures/PartitionedData.hpp"
#include "sequoia/Maths/Graph/Edge.hpp"
#include "sequoia/Maths/Graph/GraphErrors.hpp"
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
        constexpr static bool is_embedded_v{(GraphFlavour == graph_flavour::undirected_embedded) || (GraphFlavour == graph_flavour::directed_embedded)};
        constexpr static bool is_directed_v{(GraphFlavour == graph_flavour::directed)            || (GraphFlavour == graph_flavour::directed_embedded)};

        static_assert((GraphFlavour != graph_flavour::directed_embedded) || !sharing::shared_edge_v || !shared_weight_v);

        using edge_weight_proxy = edge_weight_proxy_t<EdgeWeightCreator>;
        using handler_type      = shared_to_handler_t<shared_weight_v, edge_weight_proxy>;
        using edge_type         = flavour_to_edge_t<GraphFlavour, handler_type, IndexType, shared_edge_v>;
        using edge_init_type    = edge_to_init_type_t<edge_type, is_embedded_v>;
        using edge_weight_type = typename edge_type::weight_type;
        using edges_initializer = std::initializer_list<std::initializer_list<edge_init_type>>;

        constexpr static edges_initializer validate(edges_initializer edges)
          requires is_embedded_v
        {
          for(auto nodeEdgesIter{edges.begin()}; nodeEdgesIter != edges.end(); ++nodeEdgesIter)
          {
            const auto nodeIndex{static_cast<std::size_t>(std::distance(edges.begin(), nodeEdgesIter))};
            const auto& nodeEdges{*nodeEdgesIter};
            for(auto edgeIter{nodeEdges.begin()}; edgeIter != nodeEdges.end(); ++edgeIter)
            {
              const auto edgeIndex{static_cast<std::size_t>(std::distance(nodeEdges.begin(), edgeIter))};
              const auto& edge{*edgeIter};
              const auto target{edge.target_node()};

              graph_errors::check_edge_index_range("process_complementary_edges", {nodeIndex, edgeIndex}, "complementary", edges.size(), target);
              const auto compIndex{edge.complementary_index()};

              const bool doValidate{
                [&]() {
                  if constexpr(!is_directed_v) return true;
                  else return (edge.target_node() != nodeIndex) || (edge.source_node() == edge.target_node());
                }()
              };

              if(doValidate)
              {
                auto targetEdgesIter{edges.begin() + target};
                graph_errors::check_edge_index_range("process_complementary_edges", {nodeIndex, edgeIndex}, "complementary", targetEdgesIter->size(), compIndex);

                if((target == nodeIndex) && (compIndex == edgeIndex))
                {
                  throw std::logic_error{graph_errors::self_referential_error({nodeIndex, edgeIndex}, target, compIndex)};
                }
                else if(const auto& targetEdge{*(targetEdgesIter->begin() + compIndex)}; targetEdge.complementary_index() != edgeIndex)
                {
                  throw std::logic_error{graph_errors::reciprocated_error_message({nodeIndex, edgeIndex}, "complementary", targetEdge.complementary_index(), edgeIndex)};
                }
                else
                {
                  if constexpr(!is_directed_v)
                  {
                    graph_errors::check_reciprocated_index({nodeIndex, edgeIndex}, "target", targetEdge.target_node(), nodeIndex);
                  }
                  else
                  {
                    graph_errors::check_reciprocated_index({nodeIndex, edgeIndex}, "target", targetEdge.target_node(), target);
                    graph_errors::check_reciprocated_index({nodeIndex, edgeIndex}, "source", targetEdge.source_node(), edge.source_node());
                  }

                  if constexpr(!std::is_empty_v<edge_weight_type>)
                  {
                    if(edge.weight() != targetEdge.weight())
                      throw std::logic_error{ graph_errors::error_prefix("process_complementary_edges", {nodeIndex, edgeIndex}).append("Mismatch between weights") };
                  }
                }
              }

              if constexpr(is_directed_v)
              {
                const auto source{edge.source_node()};
                graph_errors::check_edge_index_range("process_complementary_edges", {nodeIndex, edgeIndex}, "source", edges.size(), source);
                graph_errors::check_embedded_edge(nodeIndex, source, target);

                if((edge.target_node() == nodeIndex) && (edge.source_node() != edge.target_node()))
                {
                  auto sourceEdgesIter{edges.begin() + source};
                  graph_errors::check_edge_index_range("process_complementary_edges", {nodeIndex, edgeIndex}, "source", sourceEdgesIter->size(), compIndex);

                  const auto& sourceEdge{*(sourceEdgesIter->begin() + compIndex)};
                  graph_errors::check_reciprocated_index({nodeIndex, edgeIndex}, "target", sourceEdge.target_node(), nodeIndex);
                }
              }
            }
          }

          return edges;
        }

        constexpr static edges_initializer validate(edges_initializer edges)
          requires (!is_embedded_v && is_directed_v)
        {
          for(auto nodeEdgesIter{edges.begin()}; nodeEdgesIter != edges.end(); ++nodeEdgesIter)
          {
            const auto nodeIndex{static_cast<std::size_t>(std::distance(edges.begin(), nodeEdgesIter))};
            const auto& nodeEdges{*nodeEdgesIter};
            for(auto edgeIter{nodeEdges.begin()}; edgeIter != nodeEdges.end(); ++edgeIter)
            {
              const auto edgeIndex{static_cast<std::size_t>(std::distance(nodeEdges.begin(), edgeIter))};
              const auto& edge{*edgeIter};

              const auto target{edge.target_node()};
              graph_errors::check_edge_index_range("process_edges", {nodeIndex, edgeIndex}, "target", edges.size(), target);
            }
          }

          return edges;
        }

        template<class FwdIter> constexpr static FwdIter find_cluster_end(FwdIter begin, FwdIter end)
        {
          if(distance(begin, end) > 1)
          {
            auto testIter{end - 1};
            while(*testIter != *begin) --testIter;

            end = testIter + 1;
          }

          return end;
        }

        constexpr static auto validate(edges_initializer edges)
          requires (!is_embedded_v && !is_directed_v)
        {
          using namespace data_structures;
          using traits_t = partitioned_sequence_traits<edge_init_type, object::by_value<edge_init_type>>;
          partitioned_sequence<edge_init_type, object::by_value<edge_init_type>, traits_t> orderedEdges{edges};

          constexpr bool sortWeights{!std::is_empty_v<edge_weight_type> && std::totally_ordered<edge_weight_type>};
          constexpr bool clusterEdges{!std::is_empty_v<edge_weight_type> && !std::totally_ordered<edge_weight_type>};

          auto edgeComparer{
            [](const auto& e1, const auto& e2) {
              if constexpr(!sortWeights)
              {
                return e1.target_node() < e2.target_node();
              }
              else
              {
                return (e1.target_node() == e2.target_node()) ? e1.weight() < e2.weight() : e1.target_node() < e2.target_node();
              }
            }
          };

          for(std::size_t i{}; i < orderedEdges.num_partitions(); ++i)
          {
            sequoia::sort(orderedEdges.begin_partition(i), orderedEdges.end_partition(i), edgeComparer);

            if constexpr(clusterEdges)
            {
              auto lowerIter{orderedEdges.begin_partition(i)}, upperIter{orderedEdges.begin_partition(i)};
              while(lowerIter != orderedEdges.end_partition(i))
              {
                while((upperIter != orderedEdges.end_partition(i)) && (lowerIter->target_node() == upperIter->target_node()))
                  ++upperIter;

                sequoia::cluster(lowerIter, upperIter, [](const auto& e1, const auto& e2) {
                  return e1.weight() == e2.weight();
                  });

                lowerIter = upperIter;
              }
            }
          }

          for(std::size_t i{}; i < orderedEdges.num_partitions(); ++i)
          {
            auto lowerIter{orderedEdges.cbegin_partition(i)}, upperIter{orderedEdges.cbegin_partition(i)};
            while(lowerIter != orderedEdges.cend_partition(i))
            {
              const auto target{lowerIter->target_node()};
              graph_errors::check_node_index_range("process_edges", orderedEdges.num_partitions(), target);

              upperIter = std::upper_bound(lowerIter, orderedEdges.cend_partition(i), *lowerIter, edgeComparer);
              if constexpr(clusterEdges)
              {
                upperIter = find_cluster_end(lowerIter, upperIter);
              }

              const auto count{distance(lowerIter, upperIter)};

              if(target == i)
              {
                if(count % 2) throw std::logic_error{graph_errors::odd_num_loops_error("process_edges")};
              }
              else
              {
                const auto comparisonEdge{
                  [lowerIter,i]() {
                    auto compEdge{*lowerIter};
                    compEdge.target_node(i);
                    return compEdge;
                  }()
                };

                auto eqrange{
                  std::equal_range(orderedEdges.cbegin_partition(target), orderedEdges.cend_partition(target), comparisonEdge, edgeComparer)
                };

                if(eqrange.first == eqrange.second)
                  throw std::logic_error{graph_errors::error_prefix("process_edges").append("Reciprocated partial edge does not exist")};

                if constexpr(clusterEdges)
                {
                  while((eqrange.first != eqrange.second) && (eqrange.first->weight() != lowerIter->weight())) ++eqrange.first;

                  eqrange.second = find_cluster_end(eqrange.first, eqrange.second);
                }

                if(distance(eqrange.first, eqrange.second) != count)
                  throw std::logic_error{graph_errors::error_prefix("process_edges").append("Reciprocated target indices do not match")};
              }

              lowerIter = upperIter;
            }
          }

          return orderedEdges;
        }
      };

      template<class T>
      inline constexpr bool has_reservable_partitions = requires(T& t) {
        t.reserve_partition(0, 0);
      };
    }
  }
}
