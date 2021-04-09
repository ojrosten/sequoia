////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "GraphTestingUtilities.hpp"

namespace sequoia:: testing
{    
  class test_static_graph final : public graph_unit_test
  {
  public:
    using graph_unit_test::graph_unit_test;

    [[nodiscard]]
    std::string_view source_file() const noexcept final;
  private:
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

    template<class NodeWeight, class EdgeWeight> void test_generic_undirected();
    template<class NodeWeight, class EdgeWeight> void test_generic_embedded_undirected();
    template<class NodeWeight, class EdgeWeight> void test_generic_directed();
    template<class NodeWeight, class EdgeWeight> void test_generic_embedded_directed();
  };
}
