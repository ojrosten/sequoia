////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "FixedTopologyTestingUtilities.hpp"
#include "DynamicGraphTestingUtilities.hpp"

namespace sequoia
{
  namespace testing
  {    
    class test_fixed_topology final : public graph_unit_test
    {
    public:
      using graph_unit_test::graph_unit_test;

      [[nodiscard]]
      std::string_view source_file() const noexcept final;
    private:
      void run_tests() final;
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
      class EdgeWeight,
      class NodeWeight,      
      class EdgeWeightPooling,
      class NodeWeightPooling,
      class EdgeStorageTraits,
      class NodeWeightStorageTraits
    >
    class generic_fixed_topology_tests
      : public graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>
    {
    public:
      using base_t = graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>;

      using checker_t = typename base_t::checker_type;
      
      using checker_t::check_equality;      
      using checker_t::check_graph;
      using checker_t::check_semantics;

    private:
      using GGraph = typename
        graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>::graph_type;

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
