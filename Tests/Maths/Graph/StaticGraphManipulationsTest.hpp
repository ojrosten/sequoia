////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "GraphInitializationTestingUtilities.hpp"

namespace sequoia::testing
{
  class test_static_fixed_topology final : public graph_init_test
  {
  public:
    using graph_init_test::graph_init_test;

    [[nodiscard]]
    std::filesystem::path source_file() const;

    void run_tests();
  private:
    template<concrete_test>
    friend class undirected_fixed_topology_checker;

    template<concrete_test>
    friend class directed_fixed_topology_checker;

    template<concrete_test>
    friend class e_undirected_fixed_topology_checker;

    template<concrete_test>
    friend class e_directed_fixed_topology_checker;

    template<class EdgeWeight, class NodeWeight> void test_undirected();
    template<class EdgeWeight, class NodeWeight> void test_embedded_undirected();
    template<class EdgeWeight, class NodeWeight> void test_directed();
  };
}
