#pragma once

#include "DynamicGraphTestingUtils.hpp"

namespace sequoia::unit_testing
{
  class test_subgraph : public graph_unit_test
  {
  public:
    using graph_unit_test::graph_unit_test;

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
    template <maths::graph_flavour, class, template<class> class> class EdgeStorageTraits,
    template <class, template<class> class, bool> class NodeWeightStorageTraits
  >
  class generic_subgraph_tests
    : public graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>
  {
  public:
  private:
    using GGraph = typename
      graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>::graph_type;

    using graph_checker<unit_test_logger<test_mode::standard>>::check_equality;      
    using graph_checker<unit_test_logger<test_mode::standard>>::check_exception_thrown;
    using graph_checker<unit_test_logger<test_mode::standard>>::check_graph;
      
    void execute_operations() override
    {
      test_sub_graph();
    }

    void test_sub_graph();
  };
}
