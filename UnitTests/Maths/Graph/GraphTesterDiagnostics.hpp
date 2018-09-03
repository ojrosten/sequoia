#pragma once

#include "TestGraphHelper.hpp"

namespace sequoia::unit_testing
{
  class test_graph_false_positives : public graph_false_positive_test
  {
  public:
    using  graph_false_positive_test::graph_false_positive_test;

  private:
    void run_tests() override;
  };

  template
  <
    maths::graph_flavour GraphFlavour,
    class NodeWeight,
    class EdgeWeight,
    bool ThrowOnError,
    template <class> class NodeWeightStorage,
    template <class> class EdgeWeightStorage,
    template <class, class, bool, template<class...> class> class EdgeStoragePolicy
  >
  class generic_graph_false_positives
    : public graph_operations<GraphFlavour, NodeWeight, EdgeWeight, ThrowOnError, NodeWeightStorage, EdgeWeightStorage, EdgeStoragePolicy, unit_test_logger<test_mode::false_positive>>
  {
  public:
      
  private:
    using base_t = graph_operations<GraphFlavour, NodeWeight, EdgeWeight, ThrowOnError, NodeWeightStorage, EdgeWeightStorage, EdgeStoragePolicy>;
      
    using GGraph = typename base_t::graph_type;

    using graph_checker<unit_test_logger<test_mode::false_positive>>::check_equality;      
    using graph_checker<unit_test_logger<test_mode::false_positive>>::check_exception_thrown;
    using graph_checker<unit_test_logger<test_mode::false_positive>>::check_graph;

    void execute_operations() override
    {
      using namespace maths;

      using Edge = edge<EdgeWeight, utilities::protective_wrapper<EdgeWeight>>;
      using Edges = std::vector<std::vector<Edge>>;
        
      using NP_Edge = embedded_edge<EdgeWeight, data_sharing::independent, utilities::protective_wrapper<EdgeWeight>>;
      using NP_Edges = std::vector<std::vector<NP_Edge>>;
      using NodeWeights = std::vector<typename GGraph::node_weight_type>;

      GGraph network{};

      if constexpr (mutual_info(GraphFlavour))
      {
        check_graph(network, NP_Edges{{}}, NodeWeights{}, LINE("Check false positive: empty graph versus single node"));
        check_graph(network, NP_Edges{{NP_Edge{0,0,0}, NP_Edge{0,0,1}}}, NodeWeights{}, LINE("Check false positive: empty graph versus single node with loop"));
      }
      else
      {
        check_graph(network, Edges{{}}, NodeWeights{}, LINE("Check false positive: empty graph versus single node"));
        check_graph(network, Edges{{Edge{0,0}}}, NodeWeights{}, LINE("Check false positive: empty graph versus single node with loop"));        
      }
    }
  };
}
