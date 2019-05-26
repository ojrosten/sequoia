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
  class test_static_graph_traversals : public graph_unit_test
  {
  public:
    using graph_unit_test::graph_unit_test;

  private:
    void run_tests() override;

    constexpr static auto topological_sort();

    constexpr static auto bfs();

    constexpr static auto priority_search();
  };
}
