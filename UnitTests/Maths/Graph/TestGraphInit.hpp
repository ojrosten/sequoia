#pragma once

#include "TestGraphInitHelper.hpp"
#include "DynamicGraphTestingUtils.hpp"

namespace sequoia
{
  namespace unit_testing
  {    
    class test_graph_init : public graph_unit_test
    {
    public:
      using graph_unit_test::graph_unit_test;

    private:
      void run_tests() override;
    };

    template<maths::graph_flavour GraphFlavour>
    struct checker_selector;

    template<>
    struct checker_selector<maths::graph_flavour::undirected>
    {
      template<class Checker> using init_checker = undirected_init_checker<Checker>;
    };

    template<>
    struct checker_selector<maths::graph_flavour::undirected_embedded>
    {
      template<class Checker> using init_checker = undirected_embedded_init_checker<Checker>;
    };

    template<>
    struct checker_selector<maths::graph_flavour::directed>
    {
      template<class Checker> using init_checker = directed_init_checker<Checker>;
    };

    template<>
    struct checker_selector<maths::graph_flavour::directed_embedded>
    {
      template<class Checker> using init_checker = directed_embedded_init_checker<Checker>;
    };

    template
    <
      maths::graph_flavour GraphFlavour,      
      class EdgeWeight,
      class NodeWeight,      
      template <class> class EdgeWeightPooling,
      template <class> class NodeWeightPooling,
      template<class, template<class> class> class EdgeStorageTraits,
      template<class, template<class> class, bool> class NodeWeightStorageTraits
    >
    class test_initialization
      : public graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>
    {
    public:      
      using graph_checker<unit_test_logger<test_mode::standard>>::check_exception_thrown;
      using graph_checker<unit_test_logger<test_mode::standard>>::check_equality;      
      using graph_checker<unit_test_logger<test_mode::standard>>::check_graph;

    private:
      using GGraph = typename
        graph_operations<GraphFlavour, NodeWeight, EdgeWeight, NodeWeightPooling, EdgeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>::graph_type;

      using node_weight_type = typename GGraph::node_weight_type;
      using NodeWeights = std::vector<node_weight_type>;
      using edge = typename GGraph::edge_init_type;
      
      void execute_operations() override
      {
        typename checker_selector<GraphFlavour>::template init_checker<test_initialization> checker{*this};
        checker.template check_all<GGraph>();
      }
    };
  }
}
