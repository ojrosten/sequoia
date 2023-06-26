////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "GraphInitializationTestingUtilities.hpp"
#include "DynamicGraphTestingUtilities.hpp"
#include "GraphInitializationGenericTests.hpp"

namespace sequoia
{
  namespace testing
  {
    class test_graph_init final : public graph_init_test
    {
    public:
      using graph_init_test::graph_init_test;

      [[nodiscard]]
      std::filesystem::path source_file() const;

      void run_tests();
    private:
      template<class, class, concrete_test>
      friend class graph_test_helper;

      template<concrete_test>
      friend class init_checker;

      template<concrete_test>
      friend class undirected_init_checker;

      template<concrete_test>
      friend class undirected_embedded_init_checker;

      template<concrete_test>
      friend class directed_init_checker;

      template<concrete_test>
      friend class directed_embedded_init_checker;

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
      void execute_operations();
    };

    template<maths::graph_flavour GraphFlavour>
    struct checker_selector;

    template<>
    struct checker_selector<maths::graph_flavour::undirected>
    {
      template<concrete_test Test> using init_checker = undirected_init_checker<Test>;
    };

    template<>
    struct checker_selector<maths::graph_flavour::undirected_embedded>
    {
      template<concrete_test Test> using init_checker = undirected_embedded_init_checker<Test>;
    };

    template<>
    struct checker_selector<maths::graph_flavour::directed>
    {
      template<concrete_test Test> using init_checker = directed_init_checker<Test>;
    };

    template<>
    struct checker_selector<maths::graph_flavour::directed_embedded>
    {
      template<concrete_test Test> using init_checker = directed_embedded_init_checker<Test>;
    };

    template<maths::graph_flavour GraphFlavour, concrete_test Test>
    using checker_selector_checker_t = typename checker_selector<GraphFlavour>::template init_checker<Test>;
  }
}
