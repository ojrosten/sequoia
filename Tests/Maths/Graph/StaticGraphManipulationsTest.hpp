////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "GraphTestingUtilities.hpp"

namespace sequoia::testing
{    
  class test_static_fixed_topology final : public graph_unit_test
  {
  public:
    using graph_unit_test::graph_unit_test;

    [[nodiscard]]
    std::string_view source_file_name() const noexcept final;
  private:
    void run_tests() final;

    template<class EdgeWeight, class NodeWeight> void test_undirected();
    template<class EdgeWeight, class NodeWeight> void test_embedded_undirected();
    template<class EdgeWeight, class NodeWeight> void test_directed();
    template<class EdgeWeight, class NodeWeight> void test_embedded_directed();
  };
}
