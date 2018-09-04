#pragma once

#include "TestGraphHelper.hpp"

#include <complex>

namespace sequoia
{
  namespace unit_testing
  {
    class test_edge_insertion : public graph_unit_test
    {
    public:
      using graph_unit_test::graph_unit_test;

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
    class generic_edge_insertions
      : public graph_operations<GraphFlavour, NodeWeight, EdgeWeight, ThrowOnError, NodeWeightStorage, EdgeWeightStorage, EdgeStoragePolicy>
    {
    public:
      
    private:
      using base_t = graph_operations<GraphFlavour, NodeWeight, EdgeWeight, ThrowOnError, NodeWeightStorage, EdgeWeightStorage, EdgeStoragePolicy>;
      
      using graph_t = typename base_t::graph_type;

      using graph_checker<unit_test_logger<test_mode::standard>>::check_equality;      
      using graph_checker<unit_test_logger<test_mode::standard>>::check_exception_thrown;
      using graph_checker<unit_test_logger<test_mode::standard>>::check_graph;

      void execute_operations();
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
    class generic_weighted_edge_insertions
      : public graph_operations<GraphFlavour, NodeWeight, EdgeWeight, ThrowOnError, NodeWeightStorage, EdgeWeightStorage, EdgeStoragePolicy>
    {
    public:
      
    private:
      using base_t = graph_operations<GraphFlavour, NodeWeight, EdgeWeight, ThrowOnError, NodeWeightStorage, EdgeWeightStorage, EdgeStoragePolicy>;
      
      using graph_t = typename base_t::graph_type;

      using graph_checker<unit_test_logger<test_mode::standard>>::check_equality;      
      using graph_checker<unit_test_logger<test_mode::standard>>::check_exception_thrown;
      using graph_checker<unit_test_logger<test_mode::standard>>::check_graph;

      void execute_operations();
    };
  }
}
