////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "DynamicGraphTestingUtilities.hpp"

namespace sequoia::testing
{
  class weighted_graph_test final : public graph_unit_test
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
  class generic_weighted_graph_tests
    : public graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>
  {
  public:
  private:
    using base_t = graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>;
      
    using graph_t = typename base_t::graph_type;

    using base_t::check_equality;
    using base_t::check_semantics;
      
    void execute_operations() override
    {
      test_basic_operations();
    }

    void test_basic_operations();

    template<class E, class Fn>
    bool check_exception_thrown(std::string_view description, Fn&& function)
    {
      using checker = graph_checker<test_mode::standard, regular_extender<test_mode::standard>>;
      return checker::check_exception_thrown<E>(description, std::forward<Fn>(function)); 
    }
  };
}
