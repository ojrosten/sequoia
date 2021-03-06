////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "DynamicGraphTestingUtilities.hpp"

namespace sequoia::testing
{
  class test_graph_false_positives final : public graph_false_positive_test
  {
  public:
    using graph_false_positive_test::graph_false_positive_test;

    [[nodiscard]]
    std::string_view source_file() const noexcept final;
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
  class dynamic_graph_false_positives
    : public false_positive_graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>
  {
  public:
      
  private:
    using base_t = false_positive_graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>;
      
    using graph_t = typename base_t::graph_type;
    using checker_t = typename base_t::checker_type;

    using checker_t::check_equality;      
    using checker_t::check_exception_thrown;
    using checker_t::check_graph;

    void execute_operations() override;
  };
}
