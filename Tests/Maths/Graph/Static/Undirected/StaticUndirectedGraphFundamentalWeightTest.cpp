////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "StaticUndirectedGraphFundamentalWeightTest.hpp"
#include "Maths/Graph/GraphTestingUtilities.hpp"

#include "sequoia/Maths/Graph/StaticGraph.hpp"
#include "sequoia/TestFramework/StateTransitionUtilities.hpp"

#include "../MSVC_Workarounds.hpp"

namespace sequoia::testing
{
  using namespace maths;

  namespace
  {
    class compare
    {
      static_undirected_graph_fundamental_weight_test& m_Test;
    public:
      explicit compare(static_undirected_graph_fundamental_weight_test& test)
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
  std::filesystem::path static_undirected_graph_fundamental_weight_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void static_undirected_graph_fundamental_weight_test::run_tests()
  {
    test_empty<float, double>();
    test_empty<null_weight, double>();
    test_empty<float, null_weight>();

    test_node<float, double>();
    test_node<null_weight, double>();

    test_node_0<float, double>();
    test_node_0<float, null_weight>();

    test_node_0_0();
    test_node_node();
    test_node_1_node_0();
    test_node_1_1_node_0_0();
    test_node_1_2_node_0_2_node_0_1();
  }

  template<class EdgeWeight, class NodeWeight>
  void static_undirected_graph_fundamental_weight_test::test_empty()
  {
    using graph_t = static_undirected_graph<0, 0, EdgeWeight, NodeWeight>;
    using edge_t = typename graph_t::edge_init_type;
    using edges_init_t = std::initializer_list<std::initializer_list<edge_t>>;

    check_exception_thrown<std::logic_error>(report_line(""), [](){ graph_t{{}}; });

    constexpr graph_t g{};
    check(equivalence, report_line(""), g, edges_init_t{});
    check(equality, report_line(""), g, graph_t{});
  }

  template<class EdgeWeight, class NodeWeight>
  void static_undirected_graph_fundamental_weight_test::test_node()
  {
    enum graph_description { node=0, nodew};

    using graph_t = static_undirected_graph<0, 1, EdgeWeight, NodeWeight>;
    using edge_t = typename graph_t::edge_init_type;
    using edges_init_t = std::initializer_list<std::initializer_list<edge_t>>;
    using nodes_init_t = std::initializer_list<NodeWeight>;
    using transition_graph = typename transition_checker<graph_t>::transition_graph;

    check_exception_thrown<std::logic_error>(report_line(""), [](){ graph_t{{edge_t{0}}}; });

    transition_graph trg{
      {
        { // begin 'graph_description::node'
          {
            graph_description::node,
            report_line(""),
            [this](graph_t g) -> graph_t {
              g.swap_nodes(0, 0);
              return g;
            }
          },
          {
            graph_description::node,
            report_line(""),
            [this](graph_t g) -> graph_t {
              g.sort_nodes(0, 0, [](auto i, auto j){ return i < j; });
              return g;
            }
          },
          {
            graph_description::node,
            report_line(""),
            [this](graph_t g) -> graph_t {
              g.sort_nodes(0, 1, [](auto i, auto j){ return i < j; });
              return g;
            }
          },
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
              g.set_node_weight(g.cbegin_node_weights(), 2.1);
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

  template<class EdgeWeight, class NodeWeight>
  void static_undirected_graph_fundamental_weight_test::test_node_0()
  {
    enum graph_description { node_0=0, node_0x, nodew_0x, nodew_0 };

    using graph_t = static_undirected_graph<1, 1, EdgeWeight, NodeWeight>;
    using edge_t = typename graph_t::edge_init_type;
    using edges_init_t = std::initializer_list<std::initializer_list<edge_t>>;
    using transition_graph = typename transition_checker<graph_t>::transition_graph;

    check_exception_thrown<std::logic_error>(report_line(""), [](){ graph_t{{edge_t{0}}}; });
    check_exception_thrown<std::logic_error>(report_line(""), [](){ graph_t{{edge_t{0, 0.3f}, edge_t{0, 0.31f}}}; });

    transition_graph trg{
      {
        { // begin 'graph_description::node_0'
          {
            graph_description::node_0x,
            report_line(""),
            [this](graph_t g) -> graph_t {
              check(equality, report_line(""), g.mutate_edge_weight(g.cbegin_edges(0), [](auto& w) { w -= 0.2f; return 42; }), 42);
              return g;
            }
          },
           {
            graph_description::node_0x,
            report_line(""),
            [this](graph_t g) -> graph_t {
              check(equality, report_line(""), g.mutate_edge_weight(g.cbegin_edges(0)+1, [](auto& w) { w -= 0.2f; return 42; }), 42);
              return g;
            }
          },
          {
            graph_description::node_0x,
            report_line(""),
            [this](graph_t g) -> graph_t {
              g.set_edge_weight(g.cbegin_edges(0), -0.2f);
              return g;
            }
          },
          {
            graph_description::node_0x,
            report_line(""),
            [this](graph_t g) -> graph_t {
              g.set_edge_weight(g.cbegin_edges(0) + 1, -0.2f);
              return g;
            }
          },
        }, // end 'graph_description::node_0'
        {  // begin 'graph_description::node_0x'
        }  // end 'graph_description::node_0x'
      },
      {
        // 'graph_description::node_0'
        [this]() -> graph_t {
          constexpr graph_t g{{edge_t{0}, edge_t{0}}};
          check(equivalence, report_line(""), g, edges_init_t{{edge_t{0}, edge_t{0}}});
          check(equality, report_line(""), g, graph_t{{edge_t{0}, edge_t{0}}});

          return g;
        },

        // 'graph_description::node_0x'
        [this]() -> graph_t {
          constexpr graph_t g{{edge_t{0, -0.2f}, edge_t{0, -0.2f}}};
          check(equivalence, report_line(""), g, edges_init_t{{edge_t{0, -0.2f}, edge_t{0, -0.2f}}});
          check(equality, report_line(""), g, graph_t{{edge_t{0, -0.2f}, edge_t{0, -0.2f}}});

          return g;
        }
      }
    };

    if constexpr(!std::is_empty_v<NodeWeight>)
    {
      using nodes_init_t = std::initializer_list<NodeWeight>;

      // 'graph_description::nodew_0x'
      trg.add_node(
        [this]() -> graph_t {
          constexpr graph_t g{edges_init_t{{edge_t{0, -0.2f}, edge_t{0, -0.2f}}},  nodes_init_t{2.1}};

          check(equivalence, report_line(""), g, edges_init_t{{edge_t{0, -0.2f}, edge_t{0, -0.2f}}}, nodes_init_t{2.1});
          check(equality, report_line(""), g, graph_t{edges_init_t{{edge_t{0, -0.2f}, edge_t{0, -0.2f}}},  nodes_init_t{2.1}});

          return g;
        }
      );

      // 'graph_description::nodew_0'
      trg.add_node(
        [this]() -> graph_t {
          DODGY_MSVC_CONSTEXPR graph_t g{edges_init_t{{edge_t{0}, edge_t{0}}},  nodes_init_t{2.1}};

          check(equivalence, report_line(""), g, edges_init_t{{edge_t{0}, edge_t{0}}}, nodes_init_t{2.1});
          check(equality, report_line(""), g, graph_t{edges_init_t{{edge_t{0}, edge_t{0}}},  nodes_init_t{2.1}});

          return g;
        }
      );

      trg.join(
        graph_description::node_0,
        graph_description::nodew_0,
        report_line(""),
        [this](graph_t g) -> graph_t {
          check(equality, report_line(""), g.mutate_node_weight(g.cbegin_node_weights(), [](auto& w) { w += 2.1; return 42; }), 42);
          return g;
        }
      );

      trg.join(
        graph_description::node_0,
        graph_description::nodew_0,
        report_line(""),
        [this](graph_t g) -> graph_t {
          g.set_node_weight(g.cbegin_node_weights(), 2.1);
          return g;
        }
      );
    }

    transition_checker<graph_t>::check(report_line(""), trg, compare{*this});
  }

  void static_undirected_graph_fundamental_weight_test::test_node_0_0()
  {
    enum graph_description { node_0_0=0, nodew_0_0, node_0x_0, node_0x_0y, node_0y_0x};

    using graph_t = static_undirected_graph<2, 1, float, double>;
    using edge_t = typename graph_t::edge_init_type;
    using edges_init_t = std::initializer_list<std::initializer_list<edge_t>>;
    using nodes_init_t = std::initializer_list<double>;
    using transition_graph = typename transition_checker<graph_t>::transition_graph;

    transition_graph trg{
      {
        { // begin 'graph_description::node_0_0'
          {
            graph_description::nodew_0_0,
            report_line(""),
            [this](graph_t g) -> graph_t {
              check(equality, report_line(""), g.mutate_node_weight(g.cbegin_node_weights(), [](auto& w) { w += 2.1; return 42; }), 42);
              return g;
            }
          },
          {
            graph_description::node_0x_0,
            report_line(""),
            [this](graph_t g) -> graph_t {
              check(equality, report_line(""), g.mutate_edge_weight(g.cbegin_edges(0), [](auto& w) { w -= 0.2f; return 42; }), 42);
              return g;
            }
          },
          {
            graph_description::node_0x_0,
            report_line(""),
            [this](graph_t g) -> graph_t {
              check(equality, report_line(""), g.mutate_edge_weight(++g.cbegin_edges(0), [](auto& w) { w -= 0.2f; return 42; }), 42);
              return g;
            }
          }
        }, // end 'graph_description::node_0_0'
        {  // begin 'graph_description::nodew_0_0'
        }, // end 'graph_description::nodew_0_0'
        {  // begin 'graph_description::node_0x_0'
        }, // end 'graph_description::node_0x_0'
        {  // begin 'graph_description::nodew_0x_0y'
          {
            graph_description::node_0y_0x,
            report_line(""),
            [this](graph_t g) -> graph_t {
              g.sort_edges(g.cbegin_edges(0), g.cend_edges(0), [](const auto& lhs, const auto& rhs){ return lhs.weight() > rhs.weight(); });
              return g;
            }
          }
        }, // end 'graph_description::nodew_0x_0y'
        {  // begin 'graph_description::nodew_0y_0x'
          {
            graph_description::node_0x_0y,
            report_line(""),
            [this](graph_t g) -> graph_t {
              g.sort_edges(g.cbegin_edges(0), g.cend_edges(0), [](const auto& lhs, const auto& rhs){ return lhs.weight() < rhs.weight(); });
              return g;
            }
          },
          {
            graph_description::node_0x_0y,
            report_line(""),
            [this](graph_t g) -> graph_t {
              g.stable_sort_edges(g.cbegin_edges(0), g.cend_edges(0), [](const auto& lhs, const auto& rhs){ return lhs.weight() < rhs.weight(); });
              return g;
            }
          }
        }  // end 'graph_description::nodew_0y_0x'
      },
      {
        // 'graph_description::node_0_0'
        [this]() -> graph_t {
          constexpr graph_t g{{edge_t{0}, edge_t{0}, edge_t{0}, edge_t{0}}};
          check(equivalence, report_line(""), g, edges_init_t{{edge_t{0}, edge_t{0}, edge_t{0}, edge_t{0}}});
          check(equality, report_line(""), g, graph_t{{edge_t{0}, edge_t{0}, edge_t{0}, edge_t{0}}});

          return g;
        },

        // 'graph_description::nodew_0_0'
        [this]() -> graph_t {
          DODGY_MSVC_CONSTEXPR graph_t g{edges_init_t{{edge_t{0}, edge_t{0}, edge_t{0}, edge_t{0}}}, nodes_init_t{2.1}};

          check(equivalence, report_line(""), g, edges_init_t{{edge_t{0}, edge_t{0}, edge_t{0}, edge_t{0}}}, nodes_init_t{2.1});
          check(equality, report_line(""), g, graph_t{edges_init_t{{edge_t{0}, edge_t{0}, edge_t{0}, edge_t{0}}}, nodes_init_t{2.1}});

          return g;
        },

        // 'graph_description::node_0x_0'
        [this]() -> graph_t {
          constexpr graph_t g{{edge_t{0, -0.2f}, edge_t{0, -0.2f}, edge_t{0}, edge_t{0}}};
          check(equivalence, report_line(""), g, edges_init_t{{edge_t{0, -0.2f},  edge_t{0, -0.2f}, edge_t{0}, edge_t{0}}});
          check(equality, report_line(""), g, graph_t{{edge_t{0, -0.2f},  edge_t{0, -0.2f}, edge_t{0}, edge_t{0}}});
          check(equality, report_line(""), g, graph_t{{edge_t{0}, edge_t{0}, edge_t{0, -0.2f},  edge_t{0, -0.2f}}});

          return g;
        },

        // 'graph_description::node_0x_0y'
        [this]() -> graph_t {
          constexpr graph_t g{{edge_t{0, -0.2f}, edge_t{0, -0.2f}, edge_t{0, 0.3f}, edge_t{0, 0.3f}}};
          check(equivalence, report_line(""), g, edges_init_t{{edge_t{0, -0.2f},  edge_t{0, -0.2f}, edge_t{0, 0.3f}, edge_t{0, 0.3f}}});
          check(equality, report_line(""), g, graph_t{{edge_t{0, -0.2f}, edge_t{0, -0.2f}, edge_t{0, 0.3f}, edge_t{0, 0.3f}}});
          check(equality, report_line(""), g, graph_t{{edge_t{0, 0.3f}, edge_t{0, 0.3f}, edge_t{0, -0.2f}, edge_t{0, -0.2f}}});

          return g;
        },

        // 'graph_description::node_0y_0x'
        [this]() -> graph_t {
          constexpr auto g{[](){
              graph_t g{{edge_t{0, -0.2f}, edge_t{0, -0.2f}, edge_t{0, 0.3f}, edge_t{0, 0.3f}}};
              g.sort_edges(g.cbegin_edges(0), g.cend_edges(0), [](const auto& lhs, const auto& rhs){ return lhs.weight() > rhs.weight(); });
              return g;
            }()
          };

          check(equivalence, report_line(""), g, edges_init_t{{edge_t{0, 0.3f}, edge_t{0, 0.3f}, edge_t{0, -0.2f},  edge_t{0, -0.2f}}}, nodes_init_t{0.0});

          return g;
        }
      }
    };

    transition_checker<graph_t>::check(report_line(""), trg, compare{*this});
  }

  void static_undirected_graph_fundamental_weight_test::test_node_node()
  {
    enum graph_description { node_node=0, nodew_node, nodew_nodex, nodex_nodew};

    using graph_t = static_undirected_graph<0, 2, float, double>;
    using edge_t = typename graph_t::edge_init_type;
    using edges_init_t = std::initializer_list<std::initializer_list<edge_t>>;
    using nodes_init_t = std::initializer_list<double>;
    using transition_graph = typename transition_checker<graph_t>::transition_graph;

    check_exception_thrown<std::logic_error>(report_line(""), [](){ graph_t{{}}; });

    transition_graph trg{
      {
        { // begin 'graph_description::node_node'
          {
            graph_description::nodew_node,
            report_line(""),
            [this](graph_t g) -> graph_t {
              check(equality, report_line(""), g.mutate_node_weight(g.cbegin_node_weights(), [](auto& w) { w += 2.1; return 42; }), 42);
              return g;
            }
          },
          {
            graph_description::nodew_node,
            report_line(""),
            [this](graph_t g) -> graph_t {
              g.set_node_weight(g.cbegin_node_weights(), 2.1);
              return g;
            }
          }
        }, // end 'graph_description::node_node'
        {  // begin 'graph_description::nodew_node'
        }, // end 'graph_description::nodew_node'
        {  // begin 'graph_description::nodew_nodex'
          {
            graph_description::nodex_nodew,
            report_line(""),
            [this](graph_t g) -> graph_t {
              g.swap_nodes(0, 1);
              return g;
            }
          },
          {
            graph_description::nodex_nodew,
            report_line(""),
            [this](graph_t g) -> graph_t {
              g.sort_nodes(0, 2, [&g](auto i, auto j){ return g.begin_node_weights()[i] < g.begin_node_weights()[j]; });
              return g;
            }
          },
        }, // end 'graph_description::nodew_nodex'
        {  // begin 'graph_description::nodex_nodew'
        }  // end 'graph_description::nodex_nodew'
      },
      {
        // 'graph_description::node_node'
        [this]() -> graph_t {
          constexpr graph_t g{{}, {}};
          check(equivalence, report_line(""), g, edges_init_t{{}, {}});
          check(equality, report_line(""), g, graph_t{{}, {}});

          return g;
        },

        // 'graph_description::nodew_node'
        [this]() -> graph_t {
          DODGY_MSVC_CONSTEXPR graph_t g{edges_init_t{{}, {}},  nodes_init_t{2.1, 0.0}};

          check(equivalence, report_line(""), g, edges_init_t{{}, {}}, nodes_init_t{2.1, 0.0});
          check(equality, report_line(""), g, graph_t{edges_init_t{{}, {}},  nodes_init_t{2.1, 0.0}});

          return g;
        },

        // 'graph_description::nodew_nodex'
        [this]() -> graph_t {
          DODGY_MSVC_CONSTEXPR graph_t g{edges_init_t{{}, {}},  nodes_init_t{2.1, -0.7}};

          check(equivalence, report_line(""), g, edges_init_t{{}, {}}, nodes_init_t{2.1, -0.7});
          check(equality, report_line(""), g, graph_t{edges_init_t{{}, {}},  nodes_init_t{2.1, -0.7}});

          return g;
        },

        // 'graph_description::nodex_nodew'
        [this]() -> graph_t {
          DODGY_MSVC_CONSTEXPR graph_t g{edges_init_t{{}, {}},  nodes_init_t{-0.7, 2.1}};

          check(equivalence, report_line(""), g, edges_init_t{{}, {}}, nodes_init_t{-0.7, 2.1});
          check(equality, report_line(""), g, graph_t{edges_init_t{{}, {}},  nodes_init_t{ -0.7, 2.1}});

          return g;
        }
      }
    };

    transition_checker<graph_t>::check(report_line(""), trg, compare{*this});
  }

  void static_undirected_graph_fundamental_weight_test::test_node_1_node_0()
  {
    enum graph_description { node_1_node_0 = 0, node_1u_node_0u, nodew_1u_node_0u, node_1u_nodew_0u };

    using graph_t = static_undirected_graph<1, 2, float, double>;
    using edge_t = typename graph_t::edge_init_type;
    using edges_init_t = std::initializer_list<std::initializer_list<edge_t>>;
    using nodes_init_t = std::initializer_list<double>;
    using transition_graph = typename transition_checker<graph_t>::transition_graph;

    transition_graph trg{
      {
        { // begin 'graph_description::node_1_node_0'
          {
            graph_description::node_1u_node_0u,
            report_line(""),
            [this](graph_t g) -> graph_t {
              g.set_edge_weight(g.cbegin_edges(0), -0.2f);
              return g;
            }
          },
          {
            graph_description::node_1u_node_0u,
            report_line(""),
            [this](graph_t g) -> graph_t {
              g.set_edge_weight(g.cbegin_edges(1), -0.2f);
              return g;
            }
          }
        }, // end 'graph_description::node_1_node_0'
        {  // begin 'graph_description::node_1u_node_0u'
          {
            graph_description::node_1u_node_0u,
            report_line(""),
            [this](graph_t g) -> graph_t {
              g.swap_nodes(1, 0);
              return g;
            }
          },
          {
            graph_description::node_1u_node_0u,
            report_line(""),
            [this](graph_t g) -> graph_t {
              g.sort_nodes(0, 2, [&g](auto i, auto j){ return i > j; });
              return g;
            }
          },
        }, // end 'graph_description::node_1u_node_0u'
        {  // begin 'graph_description::nodew_1u_node_0u'
           {
            graph_description::node_1u_nodew_0u,
            report_line(""),
            [this](graph_t g) -> graph_t {
              g.swap_nodes(1, 0);
              return g;
            }
          }
        }, // end 'graph_description::nodew_1u_node_0u'
        {  // begin 'graph_description::node_1u_nodew_0u'
          {
            graph_description::nodew_1u_node_0u,
            report_line(""),
            [this](graph_t g) -> graph_t {
              g.swap_nodes(0, 1);
              return g;
            }
          }
        }  // end 'graph_description::node_1u_nodew_0u'
      },
      {
        // 'graph_description::node_1_node_0'
        [this]() -> graph_t {
          constexpr graph_t g{{edge_t{1}}, {edge_t{0}}};
          check(equivalence, report_line(""), g, edges_init_t{{edge_t{1}}, {edge_t{0}}});
          check(equality, report_line(""), g, graph_t{{edge_t{1}}, {edge_t{0}}});

          return g;
        },

        // 'graph_description::node_1u_node_0u'
        [this]() -> graph_t {
          constexpr graph_t g{{edge_t{1, -0.2f}}, {edge_t{0, -0.2f}}};
          check(equivalence, report_line(""), g, edges_init_t{{edge_t{1, -0.2f}}, {edge_t{0, -0.2f}}});
          check(equality, report_line(""), g, graph_t{{edge_t{1, -0.2f}}, {edge_t{0, -0.2f}}});

          return g;
        },

        // 'graph_description::nodew_1u_node_0u'
        [this]() -> graph_t {
          DODGY_MSVC_CONSTEXPR graph_t g{edges_init_t{{edge_t{1, -0.2f}}, {edge_t{0, -0.2f}}}, nodes_init_t{2.1, 0.0}};
          check(equivalence, report_line(""), g, edges_init_t{{edge_t{1, -0.2f}}, {edge_t{0, -0.2f}}}, nodes_init_t{2.1, 0.0});
          check(equality, report_line(""), g, graph_t{edges_init_t{{edge_t{1, -0.2f}}, {edge_t{0, -0.2f}}}, nodes_init_t{2.1, 0.0}});

          return g;
        },

        // 'graph_description::node_1u_nodew_0'
        [this]() -> graph_t {
          DODGY_MSVC_CONSTEXPR graph_t g{edges_init_t{{edge_t{1, -0.2f}}, {edge_t{0, -0.2f}}}, nodes_init_t{0.0, 2.1}};
          check(equivalence, report_line(""), g, edges_init_t{{edge_t{1, -0.2f}}, {edge_t{0, -0.2f}}}, nodes_init_t{0.0, 2.1});
          check(equality, report_line(""), g, graph_t{edges_init_t{{edge_t{1, -0.2f}}, {edge_t{0, -0.2f}}}, nodes_init_t{0.0, 2.1}});

          return g;
        }
      }
    };

    transition_checker<graph_t>::check(report_line(""), trg, compare{*this});
  }

  void static_undirected_graph_fundamental_weight_test::test_node_1_1_node_0_0()
  {
    enum graph_description { node_1_1_node_0_0 = 0, node_1u_1_node_0u_0, node_1u_1_node_0_0u, node_1_1u_node_0u_0};

    using graph_t = static_undirected_graph<2, 2, float, double>;
    using edge_t = typename graph_t::edge_init_type;
    using edges_init_t = std::initializer_list<std::initializer_list<edge_t>>;
    using transition_graph = typename transition_checker<graph_t>::transition_graph;

    transition_graph trg{
     {
       {  // begin 'graph_description::node_1_1_node_0_0'
         {
           graph_description::node_1u_1_node_0u_0,
           report_line(""),
           [this](graph_t g) -> graph_t {
             g.set_edge_weight(g.cbegin_edges(0), -0.2f);
             return g;
           }
         },
         {
            graph_description::node_1u_1_node_0u_0,
            report_line(""),
            [this](graph_t g) -> graph_t {
              check(equality, report_line(""), g.mutate_edge_weight(g.cbegin_edges(0), [](auto& w) { w -= 0.2f; return 42; }), 42);
              return g;
            }
         },
         {
           graph_description::node_1u_1_node_0u_0,
           report_line(""),
           [this](graph_t g) -> graph_t {
             g.set_edge_weight(g.cbegin_edges(1), -0.2f);
             return g;
           }
         },
         {
            graph_description::node_1u_1_node_0u_0,
            report_line(""),
            [this](graph_t g) -> graph_t {
              check(equality, report_line(""), g.mutate_edge_weight(g.cbegin_edges(1), [](auto& w) { w -= 0.2f; return 42; }), 42);
              return g;
            }
         },
         {
           graph_description::node_1u_1_node_0_0u,
           report_line(""),
           [this](graph_t g) -> graph_t {
             g.set_edge_weight(g.cbegin_edges(1)+1, -0.2f);
             return g;
           }
         },
         {
            graph_description::node_1u_1_node_0_0u,
            report_line(""),
            [this](graph_t g) -> graph_t {
              check(equality, report_line(""), g.mutate_edge_weight(g.cbegin_edges(1)+1, [](auto& w) { w -= 0.2f; return 42; }), 42);
              return g;
            }
         }
       }, // end 'graph_description::node_1_1_node_0_0'
       {  // begin 'graph_description::node_1u_1_node_0u_0'
         {
            graph_description::node_1u_1_node_0u_0,
            report_line(""),
            [this](graph_t g) -> graph_t {
              g.swap_nodes(0, 1);
              return g;
            }
         }
       }, // end 'graph_description::node_1u_1_node_0u_0'
       {  // begin 'graph_description::node_1u_1_node_0_0u'
         {
            graph_description::node_1_1u_node_0u_0,
            report_line(""),
            [this](graph_t g) -> graph_t {
              g.swap_nodes(0, 1);
              return g;
            }
         }
       }, // end 'graph_description::node_1u_1_node_0_0u'
       {  // begin 'graph_description::node_1_1u_node_0u_0'
         {
            graph_description::node_1u_1_node_0_0u,
            report_line(""),
            [this](graph_t g) -> graph_t {
              g.swap_nodes(1, 0);
              return g;
            }
         }
       }, // end 'graph_description::node_1_1u_node_0u_0'
     },
     {
       // 'graph_description::node_1_1_node_0_0'
       [this]() -> graph_t {
         constexpr graph_t g{{edge_t{1}, edge_t{1}}, {edge_t{0}, edge_t{0}}};
         check(equivalence, report_line(""), g, edges_init_t{{edge_t{1}, edge_t{1}}, {edge_t{0}, edge_t{0}}});
         check(equality, report_line(""), g, graph_t{{edge_t{1}, edge_t{1}}, {edge_t{0}, edge_t{0}}});

         return g;
       },

       // 'graph_description::node_1u_1_node_0u_0'
       [this]() -> graph_t {
         constexpr graph_t g{{edge_t{1, -0.2f}, edge_t{1}}, {edge_t{0, -0.2f}, edge_t{0}}};
         check(equivalence, report_line(""), g, edges_init_t{{edge_t{1, -0.2f}, edge_t{1}}, {edge_t{0, -0.2f}, edge_t{0}}});
         check(equality, report_line(""), g, graph_t{{edge_t{1, -0.2f}, edge_t{1}}, {edge_t{0, -0.2f}, edge_t{0}}});

         return g;
       },

      // 'graph_description::node_1u_1_node_0_0u'
      [this]() -> graph_t {
         constexpr auto g{[]{
            graph_t g{{edge_t{1, -0.2f}, edge_t{1}}, {edge_t{0, -0.2f}, edge_t{0}}};
            g.swap_edges(1, 0, 1);
            return g;
           }()
         };

         return g;
       },

      // 'graph_description::node_1_1u_node_0u_0'
      [this]() -> graph_t {
       constexpr auto g{[]{
          graph_t g{{edge_t{1, -0.2f}, edge_t{1}}, {edge_t{0, -0.2f}, edge_t{0}}};
          g.swap_edges(0, 0, 1);
          return g;
         }()
       };

       return g;
     }
     }
    };

    transition_checker<graph_t>::check(report_line(""), trg, compare{*this});
  }

  void static_undirected_graph_fundamental_weight_test::test_node_1_2_node_0_2_node_0_1()
  {
    enum graph_description { nodew_1_2v_nodex_0_2u_node_0v_1u = 0, node_2v_1u_nodex_2_0u_nodew_1_0v };

    using graph_t = static_undirected_graph<3, 3, float, double>;
    using edge_t = typename graph_t::edge_init_type;
    using index_type = typename graph_t::edge_index_type;
    using edges_init_t = std::initializer_list<std::initializer_list<edge_t>>;
    using nodes_init_t = std::initializer_list<double>;
    using transition_graph = typename transition_checker<graph_t>::transition_graph;

    transition_graph trg{
      {
        {  // begin 'graph_description::nodew_1_2v_nodex_0_2u_node_0v_1u'
          {
            graph_description::node_2v_1u_nodex_2_0u_nodew_1_0v,
            report_line(""),
            [this](graph_t g) -> graph_t {
              g.sort_nodes(0, 3, [&g](auto i, auto j){ return g.cbegin_node_weights()[i] < g.cbegin_node_weights()[j]; });
              return g;
            }
          }
        }, // end 'graph_description::nodew_1_2v_nodex_0_2u_node_0v_1u'
        {  // begin 'graph_description::node_2v_1u_nodex_2_0u_nodew_1_0v'
          {
            graph_description::nodew_1_2v_nodex_0_2u_node_0v_1u,
            report_line(""),
            [this](graph_t g) -> graph_t {
              g.sort_nodes(0, 3, [&g](auto i, auto j){ return g.cbegin_node_weights()[i] > g.cbegin_node_weights()[j]; });
              return g;
            }
          }
        }  // end 'graph_description::node_2v_1u_nodex_2_0u_nodew_1_0v'
      },
      {
         // 'graph_description::nodew_1_2v_nodex_0_2u_node_0v_1u'
         [this]() -> graph_t {
           DODGY_MSVC_CONSTEXPR graph_t g{edges_init_t{{edge_t{1}, edge_t{2, 0.5f}}, {edge_t{0}, edge_t{2, -0.2f}}, {edge_t{0, 0.5f}, edge_t{1, -0.2f}}}, nodes_init_t{2.1, 0.7, 0.0}};
           check(equivalence, report_line(""), g, edges_init_t{{edge_t{1}, edge_t{2, 0.5f}}, {edge_t{0}, edge_t{2, -0.2f}}, {edge_t{0, 0.5f}, edge_t{1, -0.2f}}}, nodes_init_t{2.1, 0.7, 0.0});
           check(equality, report_line(""), g, graph_t{edges_init_t{{edge_t{1}, edge_t{2, 0.5f}}, {edge_t{0}, edge_t{2, -0.2f}}, {edge_t{0, 0.5f}, edge_t{1, -0.2f}}}, nodes_init_t{2.1, 0.7, 0.0}});

           return g;
         },

         // 'graph_description::node_2v_1u_nodex_2_0u_nodew_1_0v'
         [this]() -> graph_t {
           DODGY_MSVC_CONSTEXPR auto g{[](){
              graph_t g{edges_init_t{{edge_t{1, -0.2f}, edge_t{2, 0.5f}}, {edge_t{0, -0.2f}, edge_t{2}}, {edge_t{0, 0.5f}, edge_t{1}}}, nodes_init_t{0.0, 0.7, 2.1}};
              for(auto i : std::views::iota(0_sz, g.order())) g.swap_edges(static_cast<index_type>(i), 0, 1);
              return g;
            }()
           };

           check(equivalence, report_line(""), g, edges_init_t{{edge_t{2, 0.5f}, edge_t{1, -0.2f}}, {edge_t{2}, edge_t{0, -0.2f}}, {edge_t{1}, edge_t{0, 0.5f}}}, nodes_init_t{0.0, 0.7, 2.1});

           return g;
         }
      }
    };

    transition_checker<graph_t>::check(report_line(""), trg, compare{*this});
  }
}