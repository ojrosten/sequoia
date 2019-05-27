////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "GraphTestingUtilities.hpp"

namespace sequoia::unit_testing
{    
  class test_static_fixed_topology : public graph_unit_test
  {
  public:
    using graph_unit_test::graph_unit_test;

  private:
    void run_tests() override;

    template<class EdgeWeight, class NodeWeight> void test_undirected();
    template<class EdgeWeight, class NodeWeight> void test_embedded_undirected();
    template<class EdgeWeight, class NodeWeight> void test_directed();
    template<class EdgeWeight, class NodeWeight> void test_embedded_directed();
  };
}
