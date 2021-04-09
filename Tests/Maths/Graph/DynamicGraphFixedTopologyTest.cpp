////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "DynamicGraphFixedTopologyTest.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view test_fixed_topology::source_file() const noexcept
  {
    return __FILE__;
  }
  
  void test_fixed_topology::run_tests()
  {
    using namespace maths;

    {
      graph_test_helper<int, null_weight, test_fixed_topology> helper{*this};
      helper.run_tests();
    }

    {
      graph_test_helper<int, int, test_fixed_topology> helper{*this};
      helper.run_tests();
    }
  }

  template
  <
    maths::graph_flavour GraphFlavour,      
    class EdgeWeight,
    class NodeWeight,      
    class EdgeWeightCreator,
    class NodeWeightCreator,
    class EdgeStorageTraits,
    class NodeWeightStorageTraits
  >
  void test_fixed_topology::execute_operations()
  {
    using ESTraits = EdgeStorageTraits;
    using NSTraits = NodeWeightStorageTraits;
    
    using graph_type = graph_type_generator_t<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightCreator, NodeWeightCreator, ESTraits, NSTraits>;
    
    typename ft_checker_selector<GraphFlavour>::template ft_checker<test_fixed_topology> checker{*this};
    checker.template check_all<graph_type>();
  }
}
