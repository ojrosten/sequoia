#pragma once

#include "DynamicGraphTestingUtils.hpp"

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
  class dynamic_graph_false_positives
    : public graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits, unit_test_logger<test_mode::false_positive>>
  {
  public:
      
  private:
    using base_t = graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>;
      
    using graph_t = typename base_t::graph_type;

    using graph_checker<unit_test_logger<test_mode::false_positive>>::check_equality;      
    using graph_checker<unit_test_logger<test_mode::false_positive>>::check_exception_thrown;
    using graph_checker<unit_test_logger<test_mode::false_positive>>::check_graph;

    void execute_operations() override;
  };
}
