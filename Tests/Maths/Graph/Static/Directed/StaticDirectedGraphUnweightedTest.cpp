////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "StaticDirectedGraphUnweightedTest.hpp"
#include "Maths/Graph/GraphTestingUtilities.hpp"

#include "sequoia/TestFramework/StateTransitionUtilities.hpp"

namespace sequoia::testing
{
  using namespace maths;

  [[nodiscard]]
  std::filesystem::path static_directed_graph_unweighted_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void static_directed_graph_unweighted_test::run_tests()
  {
    test_empty();
    test_0_1();
    test_1_1();
  }

  void static_directed_graph_unweighted_test::test_empty()
  {
    using graph_t = static_directed_graph<0, 0, null_weight, null_weight>;
    using edge_t = typename graph_t::edge_init_type;
    using edges_equivalent_t = std::initializer_list<std::initializer_list<edge_t>>;
    
    constexpr graph_t g{};
    check(equivalence, report_line(""), g, edges_equivalent_t{});
    check(equality, report_line(""), g, graph_t{});
  }

  void static_directed_graph_unweighted_test::test_0_1()
  {
    using graph_t = static_directed_graph<0, 1, null_weight, null_weight>;
    using edge_t = typename graph_t::edge_init_type;
    using edges_equivalent_t = std::initializer_list<std::initializer_list<edge_t>>;

    constexpr graph_t g{{}};
    check(equivalence, report_line(""), g, edges_equivalent_t{{}});
    check(equality, report_line(""), g, graph_t{{}});
  }

  void static_directed_graph_unweighted_test::test_1_1()
  {
    using graph_t = static_directed_graph<1, 1, null_weight, null_weight>;
    using edge_t = typename graph_t::edge_init_type;
    using edges_equivalent_t = std::initializer_list<std::initializer_list<edge_t>>;

    constexpr graph_t g{{edge_t{0}}};
    check(equivalence, report_line(""), g, edges_equivalent_t{{edge_t{0}}});
    check(equality, report_line(""), g, graph_t{{edge_t{0}}});
  }
}