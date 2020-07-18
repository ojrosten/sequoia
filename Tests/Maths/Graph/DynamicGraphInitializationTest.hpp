////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "GraphInitializationTestingUtilities.hpp"
#include "DynamicGraphTestingUtilities.hpp"

namespace sequoia
{
  namespace testing
  {    
    class test_graph_init final : public graph_unit_test
    {
    public:
      using graph_unit_test::graph_unit_test;

      [[nodiscard]]
      std::string_view source_file() const noexcept final;
    private:
      void run_tests() final;
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
      class EdgeWeightPooling,
      class NodeWeightPooling,
      class EdgeStorageTraits,
      class NodeWeightStorageTraits
    >
    class test_initialization
      : public graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>
    {    
    private:
      using base_t = graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>;
      
      using graph_t = typename base_t::graph_type;
      
      void execute_operations() override
      {
        typename checker_selector<GraphFlavour>::template init_checker<test_initialization> checker{*this};
        checker.template check_all<graph_t>();
      }
    public:
      using base_t::check_exception_thrown;
      using base_t::check_equality;      
      using base_t::check_graph;
      using base_t::check_equivalence;
      using base_t::check_semantics;
    };
  }
}
