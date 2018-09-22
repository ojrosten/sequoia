#include "GraphTesterDiagnostics.hpp"

namespace sequoia::unit_testing
{
  void test_graph_false_positives::run_tests()
  {    
    graph_test_helper<null_weight, null_weight> helper;
    helper.run_tests<dynamic_graph_false_positives>(*this);
  }

  template
  <
    maths::graph_flavour GraphFlavour,    
    class EdgeWeight,
    class NodeWeight,    
    template <class> class EdgeWeightPooling,
    template <class> class NodeWeightPooling,
    template <maths::graph_flavour, class, template<class> class> class EdgeStorageTraits,
    template <class, template<class> class, bool> class NodeWeightStorageTraits
  >
  void dynamic_graph_false_positives<
    GraphFlavour,
    EdgeWeight,
    NodeWeight,
    EdgeWeightPooling,
    NodeWeightPooling,
    EdgeStorageTraits,
    NodeWeightStorageTraits
  >::execute_operations()
  {
    using namespace maths;

    using Edge = edge<EdgeWeight, utilities::protective_wrapper<EdgeWeight>>;     
    using E_Edge = embedded_edge<EdgeWeight, data_sharing::independent, utilities::protective_wrapper<EdgeWeight>>;

    graph_t network{};

    if constexpr (mutual_info(GraphFlavour))
    {
      check_graph(network, {{}}, {}, LINE("Check false positive: empty graph versus single node"));
      check_graph(network, {{E_Edge{0,0,0}, E_Edge{0,0,1}}}, {},
                  LINE("Check false positive: empty graph versus single node with loop"));
    }
    else
    {
      check_graph(network, {{}}, {}, LINE("Check false positive: empty graph versus single node"));
      check_graph(network, {{Edge{0,0}}}, {},
                  LINE("Check false positive: empty graph versus single node with loop"));        
    }
  }
}
