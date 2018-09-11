#pragma once

#include "TestFixedTopologyHelper.hpp"

namespace sequoia
{
  namespace unit_testing
  {    
    class test_fixed_topology : public graph_unit_test
    {
    public:
      using graph_unit_test::graph_unit_test;

    private:
      void run_tests() override;
    };

    template<maths::graph_flavour GraphFlavour>
    struct ft_checker_selector;

    template<>
    struct ft_checker_selector<maths::graph_flavour::undirected>
    {
      template<class Checker> using ft_checker = undirected_fixed_topology_checker<Checker>;
    };

    template<>
    struct ft_checker_selector<maths::graph_flavour::undirected_embedded>
    {
      template<class Checker> using ft_checker = e_undirected_fixed_topology_checker<Checker>;
    };

    template<>
    struct ft_checker_selector<maths::graph_flavour::directed>
    {
      template<class Checker> using ft_checker = directed_fixed_topology_checker<Checker>;
    };

    template<>
    struct ft_checker_selector<maths::graph_flavour::directed_embedded>
    {
      template<class Checker> using ft_checker = e_directed_fixed_topology_checker<Checker>;
    };

    template
    <
      maths::graph_flavour GraphFlavour,
      class NodeWeight,
      class EdgeWeight,
      template <class> class NodeWeightStorage,
      template <class> class EdgeWeightStorage,
      template <class, template<class> class> class EdgeStorageTraits
    >
    class generic_fixed_topology_tests
      : public graph_operations<GraphFlavour, NodeWeight, EdgeWeight, NodeWeightStorage, EdgeWeightStorage, EdgeStorageTraits>
    {
    public:
      using graph_checker<unit_test_logger<test_mode::standard>>::check_equality;      
      using graph_checker<unit_test_logger<test_mode::standard>>::check_graph;

    private:
      using GGraph = typename
        graph_operations<GraphFlavour, NodeWeight, EdgeWeight, NodeWeightStorage, EdgeWeightStorage, EdgeStorageTraits>::graph_type;

      using node_weight_type = typename GGraph::node_weight_type;
      using NodeWeights = std::vector<node_weight_type>;
      using edge = typename GGraph::edge_init_type;
      
      void execute_operations() override
      {
        typename ft_checker_selector<GraphFlavour>::template ft_checker<decltype(*this)> checker{*this};
        checker.template check_all<GGraph>();
      }
    };
  }
}
