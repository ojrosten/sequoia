////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "StaticDirectedGraphUnweightedTest.hpp"
#include "Maths/Graph/GraphTestingUtilities.hpp"

#include "sequoia/Maths/Graph/StaticGraph.hpp"
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
    test_node();
    test_node_0();
    test_node_0_0();
    test_node_node();
    test_node_1_node();
    test_node_1_node_0();
  }

  void static_directed_graph_unweighted_test::test_empty()
  {
    using graph_t = static_directed_graph<0, 0, null_weight, null_weight>;
    using edge_t = typename graph_t::edge_init_type;
    using edges_equivalent_t = std::initializer_list<std::initializer_list<edge_t>>;

    check_exception_thrown<std::logic_error>(report(""), [](){ graph_t{{}}; });

    constexpr graph_t g{};
    check(equivalence, report(""), g, edges_equivalent_t{});
    check(equality, report(""), g, graph_t{});
  }

  void static_directed_graph_unweighted_test::test_node()
  {
    using graph_t = static_directed_graph<0, 1, null_weight, null_weight>;
    using edge_t = typename graph_t::edge_init_type;
    using edges_equivalent_t = std::initializer_list<std::initializer_list<edge_t>>;

    check_exception_thrown<std::logic_error>(report(""), [](){ graph_t{{edge_t{0}}}; });

    constexpr graph_t g{{}};
    check(equivalence, report(""), g, edges_equivalent_t{{}});
    check(equality, report(""), g, graph_t{{}});
  }

  void static_directed_graph_unweighted_test::test_node_0()
  {
    using graph_t = static_directed_graph<1, 1, null_weight, null_weight>;
    using edge_t = typename graph_t::edge_init_type;
    using edges_equivalent_t = std::initializer_list<std::initializer_list<edge_t>>;

    check_exception_thrown<std::logic_error>(report(""), [](){ graph_t{{edge_t{0}, edge_t{0}}}; });

    constexpr graph_t g{{edge_t{0}}};
    check(equivalence, report(""), g, edges_equivalent_t{{edge_t{0}}});
    check(equality, report(""), g, graph_t{{edge_t{0}}});
  }

  void static_directed_graph_unweighted_test::test_node_0_0()
  {
    using graph_t = static_directed_graph<2, 1, null_weight, null_weight>;
    using edge_t = typename graph_t::edge_init_type;
    using edges_equivalent_t = std::initializer_list<std::initializer_list<edge_t>>;

    constexpr graph_t g{{edge_t{0}, edge_t{0}}};
    check(equivalence, report(""), g, edges_equivalent_t{{edge_t{0}, edge_t{0}}});
    check(equality, report(""), g, graph_t{{edge_t{0}, edge_t{0}}});
  }

  void static_directed_graph_unweighted_test::test_node_node()
  {
    using graph_t = static_directed_graph<0, 2, null_weight, null_weight>;
    using edge_t = typename graph_t::edge_init_type;
    using edges_equivalent_t = std::initializer_list<std::initializer_list<edge_t>>;

    check_exception_thrown<std::logic_error>(report(""), [](){ graph_t{{}}; });

    constexpr graph_t g{{}, {}};
    check(equivalence, report(""), g, edges_equivalent_t{{}, {}});
    check(equality, report(""), g, graph_t{{}, {}});
  }

  void static_directed_graph_unweighted_test::test_node_1_node()
  {
    using graph_t = static_directed_graph<1, 2, null_weight, null_weight>;
    using edge_t = typename graph_t::edge_init_type;
    using edges_equivalent_t = std::initializer_list<std::initializer_list<edge_t>>;

    check_exception_thrown<std::logic_error>(report(""), [](){ graph_t{{}}; });
    check_exception_thrown<std::logic_error>(report(""), [](){ graph_t{{edge_t{1}}}; });
    check_exception_thrown<std::logic_error>(report(""), [](){ graph_t{{edge_t{1}}, {}, {}}; });

    constexpr graph_t g{{edge_t{1}}, {}};
    check(equivalence, report(""), g, edges_equivalent_t{{edge_t{1}}, {}});
    check(equality, report(""), g, graph_t{{edge_t{1}}, {}});
  }

  void static_directed_graph_unweighted_test::test_node_1_node_0()
  {
    using graph_t = static_directed_graph<2, 2, null_weight, null_weight>;
    using edge_t = typename graph_t::edge_init_type;
    using edges_equivalent_t = std::initializer_list<std::initializer_list<edge_t>>;

    constexpr graph_t g{{edge_t{1}}, {edge_t{0}}};
    check(equivalence, report(""), g, edges_equivalent_t{{edge_t{1}}, {edge_t{0}}});
    check(equality, report(""), g, graph_t{{edge_t{1}}, {edge_t{0}}});
  }
}