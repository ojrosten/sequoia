#pragma once

#include "Edge.hpp"

namespace sequoia
{
  namespace unit_testing
  {
    //===================================Edge Conversions===================================//

    template<class, class=std::void_t<>>
    struct is_partial_edge : public std::true_type {};
    
    template<class Edge>
    struct is_partial_edge<Edge, std::void_t<decltype(std::declval<Edge>().host_node())>> : public std::false_type {};

    template<class Edge>
    constexpr bool is_partial_edge_v{is_partial_edge<Edge>::value};

    
    template<class, class=std::void_t<>>
    struct is_nonplanar_edge : public std::true_type {};
    
    template<class Edge>
    struct is_nonplanar_edge<Edge, std::void_t<decltype(std::declval<Edge>().complementary_index_position())>> : public std::false_type {};

    template<class Edge>
    constexpr bool is_nonplanar_edge_v{is_nonplanar_edge<Edge>::value};

    
    template<class Edge, class NewEdge>
    class edge_converter
    {
    public:
      static_assert(std::is_same_v<typename Edge::index_type, typename NewEdge::index_type>, "Edge Index Types must be the same");

      using index_type = typename NewEdge::index_type;
      
      static constexpr NewEdge convert(const Edge& edge, const index_type node)
      {
        constexpr bool requiresConversion{
             (is_partial_edge_v<Edge> && !is_partial_edge_v<NewEdge>)
          || (is_partial_edge_v<NewEdge> && !is_partial_edge_v<Edge>)};

        return convert(edge, node, std::integral_constant<bool, requiresConversion>{});
      }
    private:      
      static constexpr NewEdge convert(const Edge& edge, const index_type node, std::true_type)
      {
        constexpr bool toPartial{is_partial_edge_v<NewEdge>};
        constexpr auto toPartial_t = std::integral_constant<bool, toPartial>{};
        
        return discriminator(edge, node, toPartial_t);
      }

      static constexpr NewEdge convert(const Edge& edge, const index_type node, std::false_type)
      {
        return edge;
      }

      static constexpr NewEdge discriminator(const Edge& edge, const index_type node, std::true_type)
      {
        constexpr auto nullWeight_t = typename std::is_empty<typename Edge::weight_type>::type{};
        return convert_to_partial(edge, node, nullWeight_t);
      }

      static constexpr NewEdge discriminator(const Edge& edge, const index_type node, std::false_type)
      {
        constexpr auto nullWeight_t = typename std::is_empty<typename Edge::weight_type>::type{};
        return convert_to_full(edge, node, nullWeight_t);
      }
      

      static constexpr NewEdge convert_to_partial(const Edge& edge, const index_type node, std::false_type)
      {
        return NewEdge{get_target_for_partial(edge, node), edge.weight()};
      }

      static constexpr NewEdge convert_to_partial(const Edge& edge, const index_type node, std::true_type)
      {
        return NewEdge{get_target_for_partial(edge, node)};
      }

      static constexpr NewEdge convert_to_full(const Edge& edge, const index_type node, std::false_type)
      {
        const index_type target{edge.target_node()};
        return {node, target, edge.weight()};
      }

      static constexpr NewEdge convert_to_full(const Edge& edge, const index_type node, std::true_type)
      {
        const index_type target{edge.target_node()};
        return {node, target};
      }

      static constexpr index_type get_target_for_partial(const Edge& edge, const index_type node)
      {
        const bool isNode{(edge.host_node() != node) && (edge.target_node() != node)};
        const index_type target{isNode ? node : (edge.host_node() == node ? edge.target_node() : edge.host_node())};
        return target;
      }
    };
  }
}
