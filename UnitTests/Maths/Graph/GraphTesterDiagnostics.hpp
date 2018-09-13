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
    class EdgeWeight,
    class NodeWeight,    
    template <class> class EdgeWeightPooling,
    template <class> class NodeWeightPooling,
    template <class, template<class> class> class EdgeStorageTraits,
    template <class, template<class> class, bool> class NodeWeightStorageTraits
  >
  class generic_graph_false_positives
    : public graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits, unit_test_logger<test_mode::false_positive>>
  {
  public:
      
  private:
    using base_t = graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>;
      
    using GGraph = typename base_t::graph_type;

    using graph_checker<unit_test_logger<test_mode::false_positive>>::check_equality;      
    using graph_checker<unit_test_logger<test_mode::false_positive>>::check_exception_thrown;
    using graph_checker<unit_test_logger<test_mode::false_positive>>::check_graph;

    void execute_operations() override
    {
      using namespace maths;

      using Edge = edge<EdgeWeight, utilities::protective_wrapper<EdgeWeight>>;     
      using E_Edge = embedded_edge<EdgeWeight, data_sharing::independent, utilities::protective_wrapper<EdgeWeight>>;

      GGraph network{};

      if constexpr (mutual_info(GraphFlavour))
      {
        check_graph(network, {{}}, {}, LINE("Check false positive: empty graph versus single node"));
        check_graph(network, {{E_Edge{0,0,0}, E_Edge{0,0,1}}}, {}, LINE("Check false positive: empty graph versus single node with loop"));
      }
      else
      {
        check_graph(network, {{}}, {}, LINE("Check false positive: empty graph versus single node"));
        check_graph(network, {{Edge{0,0}}}, {}, LINE("Check false positive: empty graph versus single node with loop"));        
      }
    }
  };
}
