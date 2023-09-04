////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "sequoia/TestFramework/RegularTestCore.hpp"

namespace sequoia::testing
{
  class static_undirected_graph_unweighted_test final : public regular_test
  {
  public:
    using regular_test::regular_test;

    [[nodiscard]]
    std::filesystem::path source_file() const;

    void run_tests();

    void test_empty();

    void test_node();

    void test_node_0();

    void test_node_0_0();

    void test_node_node();

    void test_node_1_node_0();

    void test_node_1_1_node_0_0();
  };
}