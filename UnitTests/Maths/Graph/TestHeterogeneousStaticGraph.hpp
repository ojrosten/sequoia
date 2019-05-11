////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "HeterogeneousGraphTestingUtilities.hpp"

namespace sequoia::unit_testing
{    
  class test_heterogeneous_static_graph : public graph_unit_test
  {
  public:
    using graph_unit_test::graph_unit_test;

  private:
    void run_tests() override;
    
    constexpr static auto make_undirected_graph();
    constexpr static auto make_directed_graph();
    constexpr static auto make_undirected_embedded_graph();
    constexpr static auto make_directed_embedded_graph();

    void test_generic_undirected();
    void test_generic_embedded_undirected();
    void test_generic_directed();
    void test_generic_embedded_directed();

    struct function_object
    {
      int operator()(int i) const noexcept { return i*=2;}
    };
  };
}
