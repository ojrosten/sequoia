////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "StaticDirectedGraphFundamentalWeightTest.hpp"
#include "Maths/Graph/GraphTestingUtilities.hpp"

#include "sequoia/TestFramework/StateTransitionUtilities.hpp"

#ifdef _MSC_VER
#define DODGY_MSVC_CONSTEXPR const
#else
#define DODGY_MSVC_CONSTEXPR constexpr
#endif


namespace sequoia::testing
{
  using namespace maths;

  namespace
  {
    class compare
    {
      static_directed_graph_fundamental_weight_test& m_Test;
    public:
      explicit compare(static_directed_graph_fundamental_weight_test& test)
        : m_Test{test}
      {}

      template<static_network G>
      void operator()(std::string_view description, const G& obtained, const G& prediction, const G& parent, std::size_t host, std::size_t target) const {
        m_Test.check(equality, description, obtained, prediction);
        if(host != target) m_Test.check_semantics(description, prediction, parent);
      }
    };
  }

  [[nodiscard]]
  std::filesystem::path static_directed_graph_fundamental_weight_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void static_directed_graph_fundamental_weight_test::run_tests()
  {
    test_empty();
    test_node();
    test_node_0();
    test_node_0_0();
    test_node_node();
    test_node_1_node();
    test_node_1_node_0();
  }

  void static_directed_graph_fundamental_weight_test::test_empty()
  {
    using graph_t = static_directed_graph<0, 0, float, double>;
    using edge_t = typename graph_t::edge_init_type;
    using edges_init_t = std::initializer_list<std::initializer_list<edge_t>>;

    check_exception_thrown<std::logic_error>(report_line(""), [](){ graph_t{{}}; });

    constexpr graph_t g{};
    check(equivalence, report_line(""), g, edges_init_t{});
    check(equality, report_line(""), g, graph_t{});
  }

  void static_directed_graph_fundamental_weight_test::test_node()
  {
    enum graph_description { node=0, nodew};

    using graph_t = static_directed_graph<0, 1, float, double>;
    using edge_t = typename graph_t::edge_init_type;
    using edges_init_t = std::initializer_list<std::initializer_list<edge_t>>;
    using nodes_init_t = std::initializer_list<double>;
    using transition_graph = typename transition_checker<graph_t>::transition_graph;

    check_exception_thrown<std::logic_error>(report_line(""), [](){ graph_t{{edge_t{0}}}; });

    transition_graph trg{
      {
        { // begin 'graph_description::node'
          {
            graph_description::nodew,
            report_line(""),
            [this](graph_t g) -> graph_t {
              check(equality, report_line(""), g.mutate_node_weight(g.cbegin_node_weights(), [](auto& w) { w += 2.1; return 42; }), 42);
              return g;
            }
          },
          {
            graph_description::nodew,
            report_line(""),
            [this](graph_t g) -> graph_t {
              g.node_weight(g.cbegin_node_weights(), 2.1);
              return g;
            }
          }
        }, // end 'graph_description::node'
        {  // begin 'graph_description::nodew'
        }  // end 'graph_description::nodew'
      },
      {
        // 'graph_description::node'
        [this]() -> graph_t {
          constexpr graph_t g{{}};
          check(equivalence, report_line(""), g, edges_init_t{{}});
          check(equality, report_line(""), g, graph_t{{}});

          return g;
        },

        // 'graph_description::nodew'
        [this]() -> graph_t {
          DODGY_MSVC_CONSTEXPR graph_t g{edges_init_t{{}},  nodes_init_t{2.1}};

          check(equivalence, report_line(""), g, edges_init_t{{}}, nodes_init_t{2.1});
          check(equality, report_line(""), g, graph_t{edges_init_t{{}},  nodes_init_t{2.1}});

          return g;
        }
      }
    };

    transition_checker<graph_t>::check(report_line(""), trg, compare{*this});
  }

  void static_directed_graph_fundamental_weight_test::test_node_0()
  {
    enum graph_description { node_0=0, nodew_0, node_0x, nodew_0x};

    using graph_t = static_directed_graph<1, 1, float, double>;
    using edge_t = typename graph_t::edge_init_type;
    using edges_init_t = std::initializer_list<std::initializer_list<edge_t>>;
    using nodes_init_t = std::initializer_list<double>;
    using transition_graph = typename transition_checker<graph_t>::transition_graph;

    check_exception_thrown<std::logic_error>(report_line(""), [](){ graph_t{{edge_t{0}, edge_t{0}}}; });

    transition_graph trg{
      {
        { // begin 'graph_description::node_0'
          {
            graph_description::nodew_0,
            report_line(""),
            [this](graph_t g) -> graph_t {
              check(equality, report_line(""), g.mutate_node_weight(g.cbegin_node_weights(), [](auto& w) { w += 2.1; return 42; }), 42);
              return g;
            }
          },
          {
            graph_description::nodew_0,
            report_line(""),
            [this](graph_t g) -> graph_t {
              g.node_weight(g.cbegin_node_weights(), 2.1);
              return g;
            }
          },
          {
            graph_description::node_0x,
            report_line(""),
            [this](graph_t g) -> graph_t {
              check(equality, report_line(""), g.mutate_edge_weight(g.cbegin_edges(0), [](auto& w) { w += 0.2f; return 42; }), 42);
              return g;
            }
          },
          {
            graph_description::node_0x,
            report_line(""),
            [this](graph_t g) -> graph_t {
              g.set_edge_weight(g.cbegin_edges(0), 0.2f);
              return g;
            }
          },
        }, // end 'graph_description::node_0'
        {  // begin 'graph_description::nodew_0'
        }, // end 'graph_description::nodew_0'
        {  // begin 'graph_description::node_0x'
        }, // end 'graph_description::node_0x'
        {  // begin 'graph_description::nodew_0x'
        }  // end 'graph_description::nodew_0x'
      },
      {
        // 'graph_description::node_0'
        [this]() -> graph_t {
          constexpr graph_t g{{edge_t{0}}};
          check(equivalence, report_line(""), g, edges_init_t{{edge_t{0}}});
          check(equality, report_line(""), g, graph_t{{edge_t{0}}});

          return g;
        },

        // 'graph_description::nodew_0'
        [this]() -> graph_t {
          DODGY_MSVC_CONSTEXPR graph_t g{edges_init_t{{edge_t{0}}},  nodes_init_t{2.1}};

          check(equivalence, report_line(""), g, edges_init_t{{edge_t{0}}}, nodes_init_t{2.1});
          check(equality, report_line(""), g, graph_t{edges_init_t{{edge_t{0}}},  nodes_init_t{2.1}});

          return g;
        },

        // 'graph_description::node_0x'
        [this]() -> graph_t {
          constexpr graph_t g{{edge_t{0, 0.2f}}};
          check(equivalence, report_line(""), g, edges_init_t{{edge_t{0, 0.2f}}});
          check(equality, report_line(""), g, graph_t{{edge_t{0, 0.2f}}});

          return g;
        },

        // 'graph_description::nodew_0x'
        [this]() -> graph_t {
          constexpr graph_t g{edges_init_t{{edge_t{0, 0.2f}}},  nodes_init_t{2.1}};

          check(equivalence, report_line(""), g, edges_init_t{{edge_t{0, 0.2f}}}, nodes_init_t{2.1});
          check(equality, report_line(""), g, graph_t{edges_init_t{{edge_t{0, 0.2f}}},  nodes_init_t{2.1}});

          return g;
        }
      }
    };

    transition_checker<graph_t>::check(report_line(""), trg, compare{*this});
  }

  void static_directed_graph_fundamental_weight_test::test_node_0_0()
  {
    using graph_t = static_directed_graph<2, 1, float, double>;
    using edge_t = typename graph_t::edge_init_type;
    using edges_init_t = std::initializer_list<std::initializer_list<edge_t>>;

    constexpr graph_t g{{edge_t{0}, edge_t{0}}};
    check(equivalence, report_line(""), g, edges_init_t{{edge_t{0}, edge_t{0}}});
    check(equality, report_line(""), g, graph_t{{edge_t{0}, edge_t{0}}});
  }

  void static_directed_graph_fundamental_weight_test::test_node_node()
  {
    using graph_t = static_directed_graph<0, 2, float, double>;
    using edge_t = typename graph_t::edge_init_type;
    using edges_init_t = std::initializer_list<std::initializer_list<edge_t>>;

    check_exception_thrown<std::logic_error>(report_line(""), [](){ graph_t{{}}; });

    constexpr graph_t g{{}, {}};
    check(equivalence, report_line(""), g, edges_init_t{{}, {}});
    check(equality, report_line(""), g, graph_t{{}, {}});
  }

  void static_directed_graph_fundamental_weight_test::test_node_1_node()
  {
    using graph_t = static_directed_graph<1, 2, float, double>;
    using edge_t = typename graph_t::edge_init_type;
    using edges_init_t = std::initializer_list<std::initializer_list<edge_t>>;

    check_exception_thrown<std::logic_error>(report_line(""), [](){ graph_t{{}}; });
    check_exception_thrown<std::logic_error>(report_line(""), [](){ graph_t{{edge_t{1}}}; });
    check_exception_thrown<std::logic_error>(report_line(""), [](){ graph_t{{edge_t{1}}, {}, {}}; });

    constexpr graph_t g{{edge_t{1}}, {}};
    check(equivalence, report_line(""), g, edges_init_t{{edge_t{1}}, {}});
    check(equality, report_line(""), g, graph_t{{edge_t{1}}, {}});
  }

  void static_directed_graph_fundamental_weight_test::test_node_1_node_0()
  {
    using graph_t = static_directed_graph<2, 2, float, double>;
    using edge_t = typename graph_t::edge_init_type;
    using edges_init_t = std::initializer_list<std::initializer_list<edge_t>>;

    constexpr graph_t g{{edge_t{1}}, {edge_t{0}}};
    check(equivalence, report_line(""), g, edges_init_t{{edge_t{1}}, {edge_t{0}}});
    check(equality, report_line(""), g, graph_t{{edge_t{1}}, {edge_t{0}}});
  }
}