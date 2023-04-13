////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "DynamicGraphTestingUtilities.hpp"
#include "FixedTopologyGenericTests.hpp"

namespace sequoia
{
  namespace testing
  {
    class test_fixed_topology final : public graph_init_test
    {
    public:
      using graph_init_test::graph_init_test;

      [[nodiscard]]
      std::filesystem::path source_file() const noexcept final;
    private:
      template<class, class, class>
      friend class graph_test_helper;

      template<class>
      friend class undirected_fixed_topology_checker;

      template<class>
      friend class directed_fixed_topology_checker;

      template<class>
      friend class e_undirected_fixed_topology_checker;

      template<class>
      friend class e_directed_fixed_topology_checker;

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
  }
}
