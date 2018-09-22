#pragma once

#include "GraphDetails.hpp"

namespace sequoia::maths::graph_impl
{
  template
  <        
    graph_flavour GraphFlavour,
    class EdgeWeight,
    template <class> class EdgeWeightPooling,
    template<class, template<class> class> class EdgeStorageTraits,
    class IndexType
  >
  struct dynamic_edge_traits : public edge_type_generator<GraphFlavour, EdgeWeight, EdgeWeightPooling, IndexType, edge_sharing_preference::agnostic>
  {
    using edge_type_gen = edge_type_generator<GraphFlavour, EdgeWeight, EdgeWeightPooling, IndexType, edge_sharing_preference::agnostic>;        
    using edge_type = typename edge_type_gen::edge_type;
    constexpr static bool shared_edge_v{edge_type_gen::shared_edge_v};
        
    using edge_storage_sharing_policy = typename shared_edge_v_to_policy<shared_edge_v>::template edge_storage_sharing_policy<edge_type>;
    using edge_storage_traits = typename EdgeStorageTraits<EdgeWeight, EdgeWeightPooling>::template traits_type<edge_type, edge_storage_sharing_policy>;
    using edge_storage_type = typename EdgeStorageTraits<EdgeWeight, EdgeWeightPooling>::template storage_type<edge_type, edge_storage_sharing_policy, edge_storage_traits>;

    constexpr static bool mutual_info_v{GraphFlavour != graph_flavour::directed};
  };     
}
