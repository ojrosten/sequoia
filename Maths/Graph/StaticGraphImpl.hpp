#pragma once

#include "GraphDetails.hpp"

namespace sequoia::maths::graph_impl
{
  template
  <
    graph_flavour GraphFlavour,
    std::size_t Order,
    std::size_t Size,
    class EdgeWeight,
    class IndexType
  >
  struct static_edge_traits : public edge_type_generator<GraphFlavour, EdgeWeight, data_sharing::unpooled, IndexType, sharing_preference::independent>
  {
    using edge_type = typename edge_type_generator<GraphFlavour, EdgeWeight, data_sharing::unpooled, IndexType, sharing_preference::independent>::edge_type;
    using edge_storage_type = data_structures::static_contiguous_storage<edge_type, Order, num_static_edges(GraphFlavour, Size), IndexType>;

    constexpr static bool shared_edge_v{};       
    constexpr static bool mutual_info_v{GraphFlavour != graph_flavour::directed};
  };
}
