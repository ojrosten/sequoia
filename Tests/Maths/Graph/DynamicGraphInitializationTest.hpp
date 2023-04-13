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
      std::filesystem::path source_file() const noexcept final;
    private:
      template<class, class, class>
      friend class graph_test_helper;

      template<class>
      friend class init_checker;

      template<class>
      friend class undirected_init_checker;

      template<class>
      friend class undirected_embedded_init_checker;

      template<class>
      friend class directed_init_checker;

      template<class>
      friend class directed_embedded_init_checker;

      void run_tests() final;

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
  }
}
