////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "DynamicGraphAllocationTestingUtilities.hpp"

namespace sequoia::testing
{
  class weighted_graph_allocation_test final : public graph_unit_test
  {
  public:
    using graph_unit_test::graph_unit_test;

    [[nodiscard]]
    std::string_view source_file_name() const noexcept final;
  private:
    void run_tests() final;
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
  class graph_contiguous_memory
    : public graph_allocation_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>
  {
  public:
      
  private:
    using base_t = graph_allocation_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>;
      
    using graph_t = typename base_t::graph_type;
    using checker_t = typename base_t::checker_type;
      
    using checker_t::check_equality;
    using checker_t::check_semantics;
    using graph_checker<test_mode::standard, regular_allocation_extender<test_mode::standard>>::check_exception_thrown;


    void execute_operations() override;
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
  class graph_bucketed_memory
    : public graph_allocation_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>
  {
  private:
    using base_t = graph_allocation_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>;
      
    using graph_t = typename base_t::graph_type;
    using checker_t = typename base_t::checker_type;

    using checker_t::check_equality;
    using checker_t::check_semantics;
    using graph_checker<test_mode::standard, regular_allocation_extender<test_mode::standard>>::check_exception_thrown;

    void execute_operations() override;
  };
}
