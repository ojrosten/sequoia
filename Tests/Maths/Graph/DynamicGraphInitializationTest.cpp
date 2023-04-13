////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "DynamicGraphInitializationTest.hpp"

#include <complex>

namespace sequoia:: testing
{
  [[nodiscard]]
  std::filesystem::path test_graph_init::source_file() const noexcept
  {
    return std::source_location::current().file_name();
  }

  void test_graph_init::run_tests()
  {
    using namespace maths;

    {
      graph_test_helper<null_weight, null_weight, test_graph_init> helper{*this};
      helper.run_tests();
    }

    {
      graph_test_helper<int, null_weight, test_graph_init> helper{*this};
      helper.run_tests();
    }

    {
      graph_test_helper<unsortable, null_weight, test_graph_init> helper{*this};
      helper.run_tests();
    }

    {
      graph_test_helper<big_unsortable, null_weight, test_graph_init> helper{*this};
      helper.run_tests();
    }

    {
      graph_test_helper<null_weight, int, test_graph_init> helper{*this};
      helper.run_tests();
    }

    {
      graph_test_helper<int, int, test_graph_init> helper{*this};
      helper.run_tests();
    }

    {
      graph_test_helper<unsortable, int, test_graph_init> helper{*this};
      helper.run_tests();
    }

    {
      graph_test_helper<big_unsortable, int, test_graph_init> helper{*this};
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
  void test_graph_init::execute_operations()
  {
    using ESTraits = EdgeStorageTraits;
    using NSTraits = NodeWeightStorageTraits;

    using graph_type = graph_type_generator_t<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightCreator, NodeWeightCreator, ESTraits, NSTraits>;

    typename checker_selector<GraphFlavour>::template init_checker<test_graph_init> checker{*this};
    checker.template check_all<graph_type>();
  }
}
