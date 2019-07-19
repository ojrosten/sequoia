////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "DynamicGraphTestingUtilities.hpp"

namespace sequoia
{
  namespace unit_testing
  {
    class test_graph : public graph_unit_test
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
    class generic_graph_operations
      : public graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>
    {
    public:
      
    private:
      using base_t = graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>;
      
      using graph_t = typename base_t::graph_type;

      using base_t::check_equality;
      using base_t::check_regular_semantics;
      using graph_checker<unit_test_logger<test_mode::standard>>::check_exception_thrown;
      
      void execute_operations() override;
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
    class generic_weighted_graph_tests
      : public graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>
    {
    public:
    private:
      using base_t = graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>;
      
      using graph_t = typename base_t::graph_type;

      using base_t::check_equality;
      using base_t::check_regular_semantics;
      using graph_checker<unit_test_logger<test_mode::standard>>::check_exception_thrown;
      
      void execute_operations() override
      {
        test_basic_operations();
      }

      void test_basic_operations();
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
    class graph_contiguous_memory
      : public graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>
    {
    public:
      
    private:
      using base_t = graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>;
      
      using graph_t = typename base_t::graph_type;
      
      using base_t::check_equality;
      using base_t::check_regular_semantics;
      using graph_checker<unit_test_logger<test_mode::standard>>::check_exception_thrown;

      void execute_operations() override;
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
    class graph_bucketed_memory
      : public graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>
    {
    public:
      
    private:
      using base_t = graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>;
      
      using graph_t = typename base_t::graph_type;

      using base_t::check_equality;
      using base_t::check_regular_semantics;
      using graph_checker<unit_test_logger<test_mode::standard>>::check_exception_thrown;

      void execute_operations() override;
    };
  }
}
