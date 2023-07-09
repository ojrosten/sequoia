////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "DynamicEmbeddedUndirectedGraphUnweightedTest.hpp"

#include "sequoia/TestFramework/StateTransitionUtilities.hpp"

namespace sequoia::testing
{
  namespace
  {
    /// Convention: the indices following 'node' - separated by underscores - give the target node of the associated edges
    enum graph_description : std::size_t {
      empty = 0,

      // x
      node,

      //  /-\
      //  \ /
      //   x
      node_0,

      //  /-\ /-\
      //  \ / \ /
      //     x
      node_0_0,

      //     /-\/-\
      //    /  /\  \
      //    \ /  \ /
      //        x
      node_0_0_interleaved,

      //     /-\/-\
      //    /  /\  \
      //    \ /  \ /
      //       x
      //      / \
      //      \-/
      //
      node_0_0_0_interleaved,

      //  x    x
      node_node,

      //  x ---- x
      node_1_node,

      //   /-\
      //   \ /
      //    x ---- x
      node_0_1_node,

      //  /-\
      //  \ /
      //   x      x
      node_0_node,

      //      /-\
      //      \ /
      //  x    x
      node_node_1,

      //      /-\/-\
      //     /  /\  \
      //     \ /  \ /
      //  x      x
      node_node_1_1_interleaved,

      //      /-\/-\
      //     /  /\  \
      //     \ /  \ /
      //  x     x
      //       / \
      //       \-/
      //
      node_node_1_1_1_interleaved,

      //  x ==== x
      node_1_1_node,

      // x ---- x
      //   ----
      node_1pos1_node_0pos0,

      //  x    x    x
      node_node_node,

      //  x ---- x    x
      node_1_node_node,

      //  x    x ---- x
      node_node_node_1,

      //  x ---- x ---- x
      node_1_node_2_node,

      // -- x ---- x ---- x --
      node_1_node_2_node_0,

      //      /-\
      //      \ /
      //  x    x    x
      node_node_1_node,

      //        /-\
      //        \ /
      //  x ---- x    x
      node_1_node_1_node,


      //        /-\
      //        \ /
      //  x ---- x    x
      //    ----
      node_1_node_1_0_node,

      //        /-\
      //        \ /
      //  x ---- x ---- x
      //    ----
      node_1_node_1_0_2_node,

      //        /-\
      //        \ /
      //  x ---- x ---- x
      node_1_node_1_2_node,

      //             /-\
      //             \ /
      // -- x    x    x --
      node_2_node_node_2,

      //     [2]
      //      x
      //    //  \
      //   //    \
      //  //      \
      //  x  ====  x
      // [0]      [1]
      node_1_1_2_2_node_2_node,

      //      [2]
      //       x
      //    //  \\
      //   //    \\
      //  //      \\
      //  x  ====  x
      // [0]      [1]
      node_1_1_2_2_node_2_node_1,

      //      [2]
      //       x
      //    //  \\
      //   //    \\
      //  //      \\
      //  x  ====  x
      // [0]      [1]
      node_2_2_1_1_node_2pos3_node_1,

      //  x [3]
      //  |
      //  |
      //  |
      //  x ---- x ---- x
      // [0]    [1]    [2]
      node_3_1_node_2_node_node,

      // x ---- x    x ---- x
      node_1_node_node_node_2,

      // x --   x --   -- x  -- x
      node_2_node_node_node_1
    };
  }

  [[nodiscard]]
  std::filesystem::path dynamic_embedded_undirected_graph_unweighted_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void dynamic_embedded_undirected_graph_unweighted_test::run_tests()
  {
    using namespace maths;
    graph_test_helper<null_weight, null_weight, dynamic_embedded_undirected_graph_unweighted_test> helper{*this};

    helper.run_tests<graph_flavour::undirected_embedded>();
  }

  template
  <
    maths::graph_flavour GraphFlavour,
    class EdgeWeight,
    class NodeWeight,
    class EdgeWeightCreator,
    class NodeWeightCreator,
    class EdgeStorageTraits,
    class NodeWeightStorageTraits
  >
  void dynamic_embedded_undirected_graph_unweighted_test::execute_operations()
  {
    using graph_to_test      = graph_type_generator_t<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightCreator, NodeWeightCreator, EdgeStorageTraits, NodeWeightStorageTraits>;
    using edge_t             = typename graph_to_test::edge_init_type;
    using edges_equivalent_t = std::initializer_list<std::initializer_list<edge_t>>;
    using transition_graph   = transition_checker<graph_to_test>::transition_graph;

    auto make_and_check{
      [this](std::string_view description, edges_equivalent_t init){
        return graph_initialization_checker<graph_to_test>::make_and_check(*this, description, init);
      }
    };

    transition_graph trg{
      {
        { // begin, 'empty'
          {
            graph_description::empty,
            report_line(""),
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("cbegin_edges throws for empty graph"), [&g]() { return g.cbegin_edges(0); });
              return g;
            }
          },
          {
            graph_description::empty,
            report_line(report_line("")),
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("cend_edges throws for empty graph"), [&g]() { return g.cend_edges(0); });
              return g;
            }
          },
          {
            graph_description::empty,
            report_line(""),
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("crbegin_edges throws for empty graph"), [&g]() { return g.crbegin_edges(0); });
              return g;
            }
          },
          {
            graph_description::empty,
            report_line(""),
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("crend_edges throws for empty graph"), [&g]() { return g.crend_edges(0); });
              return g;
            }
          },
          {
            graph_description::empty,
            report_line(""),
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("cedges throws for empty graph"), [&g]() { return g.cedges(0); });
              return g;
            }
          },
          {
            graph_description::empty,
            report_line(""),
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("swapping nodes throws for empty graph"), [g{g}]() mutable { g.swap_nodes(0, 0); });
              return g;
            }
          },
          {
            graph_description::empty,
            report_line(""),
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("joining nodes throws for empty graph"), [g{g}]() mutable { g.join(0, 0); });
              return g;
            }
          },
          {
            graph_description::empty,
            report_line("Clear empty graph"),
            [](graph_to_test g) -> graph_to_test {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node,
            report_line("Add node to empty graph"),
            [this](graph_to_test g) -> graph_to_test {
              check(equality, report_line("Index of added node is 0"), g.add_node(), 0_sz);
              return g;
            }
          },
          {
            graph_description::node,
            report_line("insert node into empty graph"),
            [this](graph_to_test g) -> graph_to_test {
              check(equality, report_line("Index of added node is 0"), g.insert_node(0), 0_sz);
              return g;
            }
          }
        }, // end 'empty'
        {  // begin 'node'
          {
            graph_description::node,
            report_line(""),
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("cbegin_edges throws when index is out of range"), [&g]() { return g.cbegin_edges(1); });
              return g;
            }
          },
          {
            graph_description::node,
            report_line(""),
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("cend_edges throws when index is out of range"), [&g]() { return g.cend_edges(1); });
              return g;
            }
          },
          {
            graph_description::node,
            report_line(""),
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("crbegin_edges throws when index is out of range"), [&g]() { return g.crbegin_edges(1); });
              return g;
            }
          },
          {
            graph_description::node,
            report_line(""),
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("crend_edges throws when index is out of range"), [&g]() { return g.crend_edges(1); });
              return g;
            }
          },
          {
            graph_description::node,
            report_line(""),
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("cedges throws when index is out of range"), [&g]() { return g.cedges(1); });
              return g;
            }
          },
          {
            graph_description::node,
            report_line(""),
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("swapping nodes throws if first index out of range"), [g{g}]() mutable { g.swap_nodes(1, 0); });
              return g;
            }
          },
          {
            graph_description::node,
            report_line(""),
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("swapping nodes throws if second index out of range"), [g{g}]() mutable { g.swap_nodes(0, 1); });
              return g;
            }
          },
          {
            graph_description::node,
            report_line(""),
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("joining nodes throws if first index out of range"), [g{g}]() mutable { g.join(1, 0); });
              return g;
            }
          },
          {
            graph_description::node,
            report_line(""),
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("joining nodes throws if second index out of range"), [g{g}]() mutable { g.join(0, 1); });
              return g;
            }
          },
          {
            graph_description::node,
            report_line(""),
            [this](const graph_to_test& g) -> const graph_to_test& {
              check_exception_thrown<std::out_of_range>(report_line("inserting join throws if second index out of range"), [g{g}]() mutable { g.insert_join(g.cbegin_edges(0), 2); });
              return g;
            }
          },
          {
            graph_description::empty,
            report_line("Clear graph"),
            [](graph_to_test g) -> graph_to_test {
              g.clear();
              return g;
            }
          },
          {
            graph_description::empty,
            report_line("Erase node to give empty graph"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node,
            report_line("Attempt to erase edge past the end"),
            [](graph_to_test g) -> const graph_to_test {
              g.erase_edge(g.cend_edges(0));
              return g;
            }
          },
          {
            graph_description::node_0,
            report_line("Add loop"),
            [](graph_to_test g) -> graph_to_test {
              g.join(0, 0);
              return g;
            }
          },
          {
            graph_description::node_node,
            report_line("Add second node"),
            [this](graph_to_test g) -> graph_to_test {
              check(equality, report_line("Index of added node is 1"), g.add_node(), 1_sz);
              return g;
            }
          },
          {
            graph_description::node_node,
            report_line("Insert second node"),
            [this](graph_to_test g) -> graph_to_test {
              check(equality, report_line("Index of added node is 0"), g.insert_node(0), 0_sz);
              return g;
            }
          },
          {
            graph_description::node_node,
            report_line("Insert second node at end"),
            [this](graph_to_test g) -> graph_to_test {
              check(equality, report_line("Index of added node is 1"), g.insert_node(1), 1_sz);
              return g;
            }
          },
          {
            graph_description::node,
            report_line("Swap node with self"),
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(0,0);
              return g;
            }
          }
        }, // end 'node'
        {  // begin 'node_0'
          {
            graph_description::empty,
            report_line("Clear graph"),
            [](graph_to_test g) -> graph_to_test {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node,
            report_line("Remove loop"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(0));
              return g;
            }
          },
          {
            graph_description::node_0_0,
            report_line("Add a second loop"),
            [](graph_to_test g) -> graph_to_test {
              g.join(0, 0);
              return g;
            }
          },
          {
            graph_description::node_0_0_interleaved,
            report_line("Interleave a second loop"),
            [](graph_to_test g) -> graph_to_test {
              g.insert_join(g.cbegin_edges(0)+1, 3);
              return g;
            }
          },
          {
            graph_description::node_node_1,
            report_line("Insert node"),
            [this](graph_to_test g) -> graph_to_test {
              check(equality, report_line("Index of added node is 0"), g.insert_node(0), 0_sz);
              return g;
            }
          },
          {
            graph_description::node_0_node,
            report_line("Insert node at end"),
            [this](graph_to_test g) -> graph_to_test {
              check(equality, report_line("Index of added node is 1"), g.insert_node(1), 1_sz);
              return g;
            }
          },
          {
            graph_description::node_0,
            report_line("Swap node with self"),
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(0,0);
              return g;
            }
          }
        }, // end 'node_0'
        {  // begin 'node_0_0'
          {
            graph_description::empty,
            report_line("Clear graph"),
            [](graph_to_test g) -> graph_to_test {
              g.clear();
              return g;
            }
          },
          {
            graph_description::empty,
            report_line("Erase node 0"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_0,
            report_line("Remove first loop"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(0));
              return g;
            }
          },
          {
            graph_description::node_0,
            report_line("Remove first loop via second insertion"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(0) + 1);
              return g;
            }
          },
          {
            graph_description::node_0,
            report_line("Remove second loop"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(std::ranges::next(g.cbegin_edges(0), 2));
              return g;
            }
          },
          {
            graph_description::node_0,
            report_line("Remove second loop via second insertion"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(std::ranges::next(g.cbegin_edges(0), 3));
              return g;
            }
          }
        }, // end 'node_0_0'
        {  // begin 'node_0_0_interleaved'
          {
            graph_description::empty,
            report_line("Clear graph"),
            [](graph_to_test g) -> graph_to_test {
              g.clear();
              return g;
            }
          },
          {
            graph_description::empty,
            report_line("Erase node 0"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_0,
            report_line("Remove first loop via first insertion"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(0));
              return g;
            }
          },
          {
            graph_description::node_0_0_0_interleaved,
            report_line("Insert a third loop"),
            [](graph_to_test g) -> graph_to_test {
              g.insert_join(g.cbegin_edges(0), ++g.cbegin_edges(0));
              return g;
            }
          }
          ,
          {
            graph_description::node_0_0_0_interleaved,
            report_line("Insert a third loop, inverted"),
            [](graph_to_test g) -> graph_to_test {
              g.insert_join(g.cbegin_edges(0), g.cbegin_edges(0));
              return g;
            }
          },
          {
            graph_description::node_node_1_1_interleaved,
            report_line("Insert node"),
            [](graph_to_test g) -> graph_to_test {
              g.insert_node(0);
              return g;
            }
          }
        }, // end 'node_0_0_interleaved'
        {  // begin 'node_0_0_0_interleaved'
          {
            graph_description::node_0_0_interleaved,
            report_line("Remove first loop via first insertion"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(0));
              return g;
            }
          },
          {
            graph_description::node_0_0_interleaved,
            report_line("Remove first loop via second insertion"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(0)+1);
              return g;
            }
          },
          {
            graph_description::node_0_0,
            report_line("Remove second loop via first insertion"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(0)+2);
              return g;
            }
          },
          {
            graph_description::node_0_0,
            report_line("Remove second loop via second insertion"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(0)+4);
              return g;
            }
          },
          {
            graph_description::node_0_0,
            report_line("Remove third loop via first insertion"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(0) + 3);
              return g;
            }
          },
          {
            graph_description::node_0_0,
            report_line("Remove third loop via second insertion"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(0) + 5);
              return g;
            }
          },
          {
            graph_description::node_node_1_1_1_interleaved,
            report_line("Insert node"),
            [](graph_to_test g) -> graph_to_test {
              g.insert_node(0);
              return g;
            }
          }
        }, // end 'node_0_0_0_interleaved'
        {  // begin 'node_node'
          {
            graph_description::empty,
            report_line("Clear graph"),
            [](graph_to_test g) -> graph_to_test {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node,
            report_line("Erase node 0"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node,
            report_line("Erase node 1"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_1_node,
            report_line("Join nodes 0,1"),
            [](graph_to_test g) -> graph_to_test {
              g.join(0, 1);
              return g;
            }
          }
        }, // end 'node_node'
        {  // begin 'node_1_node'
          {
            graph_description::empty,
            report_line("Clear graph"),
            [](graph_to_test g) -> graph_to_test {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node,
            report_line("Erase node 0"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node,
            report_line("Erase node 1"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_1_node,
            report_line("Swap nodes {0,1}"),
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(0,1);
              return g;
            }
          },
          {
            graph_description::node_1_node,
            report_line("Swap nodes {1,0}"),
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(1,0);
              return g;
            }
          },
          {
            graph_description::node_1_1_node,
            report_line("Join {0,1}"),
            [](graph_to_test g) {
              g.join(0, 1);
              return g;
            }
          }
        }, // end 'node_1_node'
        {  // begin 'node_0_1_node'
          {
            graph_description::empty,
            report_line("Clear graph"),
            [](graph_to_test g) -> graph_to_test {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node,
            report_line("Erase node 0"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_0,
            report_line("Erase node 1"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_1_node,
            report_line("Remove loop"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(0));
              return g;
            }
          },
          {
            graph_description::node_0_node,
            report_line("Remove link"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(std::ranges::next(g.cbegin_edges(0), 2));
              return g;
            }
          }
        }, // end 'node_0_1_node'
        {  // begin 'node_0_node'
          {
            graph_description::empty,
            report_line("Clear graph"),
            [](graph_to_test g) -> graph_to_test {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node_node,
            report_line("Remove link"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(0));
              return g;
            }
          },
          {
            graph_description::node_node_1_node,
            report_line("Insert node"),
            [this](graph_to_test g) -> graph_to_test {
              check(equality, report_line("Index of added node is 0"), g.insert_node(0), 0_sz);
              return g;
            }
          },
          {
            graph_description::node_node_1,
            report_line("Swap nodes"),
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(0,1);
              return g;
            }
          }
        }, // end 'node_0_node'
        {  // begin 'node_node_1'
          {
            graph_description::empty,
            report_line("Clear graph"),
            [](graph_to_test g) -> graph_to_test {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node_0,
            report_line("Erase node 0"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node,
            report_line("Erase node 1"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_node,
            report_line("Remove link"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(1));
              return g;
            }
          },
          {
            graph_description::node_0_node,
            report_line("swap nodes"),
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(0,1);
              return g;
            }
          }
        }, // end 'node_node_1'
        {  // begin 'node_node_1_1_interleaved'
          {
            graph_description::node_0_0_interleaved,
            report_line("Insert node"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(0);
              return g;
            }
          }
        }, // end 'node_node_1_1_interleaved'
        {  // begin 'node_node_1_1_1_interleaved'
          {
            graph_description::node_0_0_0_interleaved,
            report_line("Insert node"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(0);
              return g;
            }
          }
        }, // end 'node_node_1_1_1_interleaved'
        {  // begin 'node_1_1_node'
          {
            graph_description::empty,
            report_line("Clear graph"),
            [](graph_to_test g) -> graph_to_test {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node,
            report_line("Erase node 0"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node,
            report_line("Erase node 1"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_1_node,
            report_line("Erase node 0 zeroth link"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(0));
              return g;
            }
          },
          {
            graph_description::node_1_node,
            report_line("Erase node 0 first link"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(0)+1);
              return g;
            }
          }
        }, // end 'node_1_1_node'
        {  // begin 'node_1pos1_node_0pos0'
          {
            graph_description::empty,
            report_line("Clear graph"),
            [](graph_to_test g) -> graph_to_test {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node,
            report_line("Erase node 0"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node,
            report_line("Erease node 1"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_1_node,
            report_line("Erase node 0 link"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(0));
              return g;
            }
          },
          {
            graph_description::node_1_node,
            report_line("Erase node 1 link"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(1));
              return g;
            }
          },
        }, // end 'node_1pos1_node_0pos0'
        {  // begin 'node_node_node'
          {
            graph_description::empty,
            report_line("Clear graph"),
            [](graph_to_test g) -> graph_to_test {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node_node,
            report_line("Erase node 0"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_node,
            report_line("Erase node 1"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_node,
            report_line("Erase node 2"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(2);
              return g;
            }
          },
          {
            graph_description::node_1_node_node,
            report_line("Join {0,1}"),
            [](graph_to_test g) -> graph_to_test {
              g.join(0,1);
              return g;
            }
          },
          {
            graph_description::node_node_node_1,
            report_line("Join {2,1}"),
            [](graph_to_test g) -> graph_to_test {
              g.join(2,1);
              return g;
            }
          }
        }, // end 'node_node_node'
        {  // begin 'node_1_node_node'
          {
            graph_description::empty,
            report_line("Clear graph"),
            [](graph_to_test g) -> graph_to_test {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node_node,
            report_line("Erase node 0"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_node,
            report_line("Erase node 1"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_1_node,
            report_line("Erase node 2"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(2);
              return g;
            }
          },
          {
            graph_description::node_node_node,
            report_line("Remove link {0,1}"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(0));
              return g;
            }
          },
          {
            graph_description::node_1_node_2_node,
            report_line("Join {2,1}"),
            [](graph_to_test g) -> graph_to_test {
              g.join(1,2);
              return g;
            }
          },
          {
            graph_description::node_1_node_2_node,
            report_line("Join {2,1}"),
            [](graph_to_test g) -> graph_to_test {
              g.join(2,1);
              return g;
            }
          },
          {
            graph_description::node_node_node_1,
            report_line("Swap nodes {0,2}"),
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(0,2);
              return g;
            }
          },
          {
            graph_description::node_node_node_1,
            report_line("Swap nodes {2,0}"),
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(2,0);
              return g;
            }
          }
        }, // end 'node_1_node_node'
        {  // begin 'node_node_node_1'
          {
            graph_description::empty,
            report_line("Clear graph"),
            [](graph_to_test g) -> graph_to_test {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node_1_node,
            report_line("Erase node 0"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_node,
            report_line("Erase node 1"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_node,
            report_line("Erase node 2"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(2);
              return g;
            }
          },
          {
            graph_description::node_node_node,
            report_line("Remove link {2,1}"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(2));
              return g;
            }
          },
          {
            graph_description::node_1_node_2_node,
            report_line("Join {0,1}"),
            [](graph_to_test g) -> graph_to_test {
              g.insert_join(g.cbegin_edges(0), g.cbegin_edges(1));
              return g;
            }
          },
          {
            graph_description::node_1_node_node,
            report_line("Swap nodes {0,2}"),
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(0,2);
              return g;
            }
          },
          {
            graph_description::node_1_node_node,
            report_line("Swap nodes {2,0}"),
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(2,0);
              return g;
            }
          }
        }, // end 'node_node_node_1'
        {  // begin 'node_1_node_2_node'
          {
            graph_description::empty,
            report_line("Clear graph"),
            [](graph_to_test g) -> graph_to_test {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node_1_node,
            report_line("Erase node 0"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_node,
            report_line("Erase node 1"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_1_node,
            report_line("Erase node 2"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(2);
              return g;
            }
          },
          {
            graph_description::node_1_node_node,
            report_line("Remove link {1,2}"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(++g.cbegin_edges(1));
              return g;
            }
          }
        }, // end 'node_1_node_2_node'
        {  // begin 'node_1_node_2_node_0'
          {
            graph_description::empty,
            report_line("Clear graph"),
            [](graph_to_test g) -> graph_to_test {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node_1_node,
            report_line("Erase node 0"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_1_node,
            report_line("Erase node 1"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_1_node,
            report_line("Erase node 2"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(2);
              return g;
            }
          },
          {
            graph_description::node_1_node_2_node,
            report_line("Remove {2,0}"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(++g.cbegin_edges(2));
              return g;
            }
          }
        }, // end 'node_1_node_2_node_0'
        {  // begin 'node_node_1_node'
          {
            graph_description::empty,
            report_line("Clear graph"),
            [](graph_to_test g) -> graph_to_test {
              g.clear();
              return g;
            }
          },
          {
            graph_description::node_0_node,
            report_line("Erase node 0"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_node,
            report_line("Erase node 1"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_node_1,
            report_line("Erase node 2"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(2);
              return g;
            }
          },
        }, // end 'node_node_1_node'
        {  // begin 'node_1_node_1_node'
          {
            graph_description::node_0_node,
            report_line("Erase node 0"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_node,
            report_line("Erase node 1"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_node_1_node,
            report_line("Remove {0,1}"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(0));
              return g;
            }
          },
          {
            graph_description::node_1_node_1_0_node,
            report_line("Join {1,0}"),
            [](graph_to_test g) -> graph_to_test {
              g.join(1, 0);
              return g;
            }
          }
        }, // end 'node_1_node_1_node'
        {  // begin 'node_1_node_1_0_node'
          {
            graph_description::node_0_node,
            report_line("Erase node 0"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_node,
            report_line("Erase node 1"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_1_node_1_node,
            report_line("Remove {1,0}"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(1)+3);
              return g;
            }
          },
          {
            graph_description::node_1_node_1_0_2_node,
            report_line("Join {1,2}"),
            [](graph_to_test g) -> graph_to_test {
              g.join(1, 2);
              return g;
            }
          }
        }, // end 'node_1_node_1_0_node'
        {  // begin 'node_1_node_1_0_2_node'
          {
            graph_description::node_0_1_node,
            report_line("Erase node 0"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_node,
            report_line("Erase node 1"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_1_node_1_2_node,
            report_line("Remove {1,0}"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(1)+3);
              return g;
            }
          },
          {
            graph_description::node_1_node_1_0_node,
            report_line("Remove {1,2}"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(1)+4);
              return g;
            }
          }
        }, // end 'node_1_node_1_0_2_node'
        {  // begin 'node_1_node_1_2_node'
          {
            graph_description::node_1_node_2_node,
            report_line("Remove {1,1}"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_edge(g.cbegin_edges(1)+2);
              return g;
            }
          }
        }, // end 'node_1_node_1_2_node'
        {  // begin 'node_2_node_node_2'
          {
            graph_description::node_1_node_1_node,
            report_line("Swap {1,2}"),
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(1,2);
              return g;
            }
          }
        }, // end 'node_2_node_node_2'
        {  // begin 'node_1_1_2_2_node_2_node'
          {
            graph_description::node_1_node,
            report_line("Erase node 0"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_1_1_node,
            report_line("Erase node 1"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_1_1_node,
            report_line("Erase node 2"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(2);
              return g;
            }
          },
          {
            graph_description::node_1_1_2_2_node_2_node_1,
            report_line("Insert Join {2, 1}"),
            [](graph_to_test g) -> graph_to_test {
              g.insert_join(g.cbegin_edges(2)+1, g.cbegin_edges(1)+2);
              return g;
            }
          }
        }, // end 'node_1_1_2_2_node_2_node'
        {  // begin 'node_1_1_2_2_node_2_node_1'
          {
            graph_description::node_1pos1_node_0pos0,
            report_line("Erase node 0"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(0);
              return g;
            }
          },
          {
            graph_description::node_1_1_node,
            report_line("Erase node 1"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(1);
              return g;
            }
          },
          {
            graph_description::node_1_1_node,
            report_line("Erase node 2"),
            [](graph_to_test g) -> graph_to_test {
              g.erase_node(2);
              return g;
            }
          },
          {
            graph_description::node_2_2_1_1_node_2pos3_node_1,
            report_line("Swap {1,2}"),
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(1,2);
              return g;
            }
          },
          {
            graph_description::node_2_2_1_1_node_2pos3_node_1,
            report_line("Swap {2,1}"),
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(2,1);
              return g;
            }
          }
        }, // end 'node_1_1_2_2_node_2_node_1'
        {  // begin node_2_2_1_1_node_2_node_1
          {
            graph_description::node_1_1_2_2_node_2_node_1,
            report_line("Swap {1,2}"),
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(1,2);
              return g;
            }
          },
          {
            graph_description::node_1_1_2_2_node_2_node_1,
            report_line("Swap {2,1}"),
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(2,1);
              return g;
            }
          }
        }, // end node_2_2_1_1_node_2_node_1
        {  // begin 'node_3_1_node_2_node_node'
          {
             graph_description::node_1_node_node,
             report_line("Erase node 0"),
             [](graph_to_test g) -> graph_to_test {
               g.erase_node(0);
               return g;
             }
          }
        }, // end 'node_3_1_node_2_node_node'
        {  // begin 'node_1_node_node_node_2'
          {
            graph_description::node_2_node_node_node_1,
            report_line("Swap {2,1}"),
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(2,1);
              return g;
            }
          },
          {
            graph_description::node_2_node_node_node_1,
            report_line("Swap {0,3}"),
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(0,3);
              return g;
            }
          }
        }, // end 'node_1_node_node_node_2'
        {  // begin 'node_2_node_node_node_1'
          {
            graph_description::node_1_node_node_node_2,
            report_line("Swap {2,1}"),
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(2,1);
              return g;
            }
          },
          {
            graph_description::node_1_node_node_node_2,
            report_line("Swap {0,3}"),
            [](graph_to_test g) -> graph_to_test {
              g.swap_nodes(0,3);
              return g;
            }
          }
        }, // end 'node_2_node_node_node_1'
      },
      {
        //  'empty'
        make_and_check(report_line(""), {}),

        //  'node'
        make_and_check(report_line(""), {{}}),

        //  'node_0'
        make_and_check(report_line(""), {{edge_t{0, 1}, edge_t{0, 0}}}),

        //  'node_0_0'
        make_and_check(report_line(""), {{edge_t{0, 1}, edge_t{0, 0}, edge_t{0, 3}, edge_t{0, 2}}}),

        // 'node_0_0_interleaved'
        make_and_check(report_line(""), {{edge_t{0, 2}, edge_t{0, 3}, edge_t{0, 0}, edge_t{0, 1}}}),

        // 'node_0_0_0_interleaved'
        make_and_check(report_line(""), {{edge_t{0, 1}, edge_t{0, 0}, edge_t{0, 4}, edge_t{0, 5}, edge_t{0, 2}, edge_t{0, 3}}}),

        //  'node_node'
        make_and_check(report_line(""), {{}, {}}),

        //  'node_1_node'
        make_and_check(report_line(""), {{edge_t{1, 0}}, {edge_t{0, 0}}}),

        //  'node_0_1_node'
        make_and_check(report_line(""), {{edge_t{0, 1}, edge_t{0, 0}, edge_t{1, 0}}, {edge_t{0, 2}}}),

        //  'node_0_node'
        make_and_check(report_line(""), {{edge_t{0, 1}, edge_t{0, 0}}, {}}),

        //  'node_node_1'
        make_and_check(report_line(""), {{}, {edge_t{1, 1}, edge_t{1, 0}}}),

        // 'node_1_1_interleaved'
        make_and_check(report_line(""), {{}, {edge_t{1, 2}, edge_t{1, 3}, edge_t{1, 0}, edge_t{1, 1}}}),

        // 'node_1_1_1_interleaved'
        make_and_check(report_line(""), {{}, {edge_t{1, 1}, edge_t{1, 0}, edge_t{1, 4}, edge_t{1, 5}, edge_t{1, 2}, edge_t{1, 3}}}),

        // 'node_1_1_node'
        make_and_check(report_line(""), {{edge_t{1, 0}, edge_t{1, 1}}, {edge_t{0, 0}, edge_t{0, 1}}}),

        // 'node_1pos1_node_0pos0'
        make_and_check(report_line(""), {{edge_t{1, 1}, edge_t{1, 0}}, {edge_t{0, 1}, edge_t{0, 0}}}),

        //  'node_node_node'
        make_and_check(report_line(""), {{}, {}, {}}),

        //  'node_1_node_node'
        make_and_check(report_line(""), {{edge_t{1, 0}}, {edge_t{0, 0}}, {}}),

        //  'node_node_node_1'
        make_and_check(report_line(""), {{}, {edge_t{2, 0}}, {edge_t{1, 0}}}),

        // 'node_1_node_2_node'
        make_and_check(report_line(""), {{edge_t{1, 0}}, {edge_t{0, 0}, edge_t{2, 0}}, {edge_t{1, 1}}}),

        // 'node_1_node_2_node_0'
        make_and_check(report_line(""), {{edge_t{1, 0}, edge_t{2, 1}},
                                         {edge_t{0, 0}, edge_t{2, 0}},
                                         {edge_t{1, 1}, edge_t{0, 1}}}),

        // 'node_node_1_node'
        make_and_check(report_line(""), {{}, {edge_t{1, 1}, edge_t{1, 0}}, {}}),

        // 'node_1_node_1_node'
        make_and_check(report_line(""), {{edge_t{1, 0}}, {edge_t{0, 0}, edge_t{1, 2}, edge_t{1, 1}}, {}}),

        // 'node_1_node_1_0_node'
        make_and_check(report_line(""), {{edge_t{1, 0}, edge_t{1, 3}},
                                         {edge_t{0, 0}, edge_t{1, 2}, edge_t{1, 1}, edge_t{0, 1}},
                                         {}}),

        // 'node_1_node_1_0_2_node'
        make_and_check(report_line(""), {{edge_t{1, 0}, edge_t{1, 3}},
                                         {edge_t{0, 0}, edge_t{1, 2}, edge_t{1, 1}, edge_t{0, 1}, edge_t{2, 0}},
                                         {edge_t{1, 4}}}),

        // 'node_1_node_1_2_node'
        make_and_check(report_line(""), {{edge_t{1, 0}},
                                         {edge_t{0, 0}, edge_t{1, 2}, edge_t{1, 1}, edge_t{2, 0}},
                                         {edge_t{1, 3}}}),

        // 'node_2_node_node_2'
        make_and_check(report_line(""), {{edge_t{2, 0}}, {}, {edge_t{0, 0}, edge_t{2, 2}, edge_t{2, 1}}}),

        // 'node_1_1_2_2_node_2_node'
        make_and_check(report_line(""), {{edge_t{1, 0}, edge_t{1, 2}, edge_t{2, 0}, edge_t{2, 2}},
                                         {edge_t{0, 0}, edge_t{2, 1}, edge_t{0, 1}},
                                         {edge_t{0, 2}, edge_t{1, 1}, edge_t{0, 3}}}),

        // 'node_1_1_2_2_node_2_node_1'
        make_and_check(report_line(""), {{edge_t{1, 0}, edge_t{1, 3}, edge_t{2, 0}, edge_t{2, 3}},
                                         {edge_t{0, 0}, edge_t{2, 2}, edge_t{2, 1}, edge_t{0, 1}},
                                         {edge_t{0, 2}, edge_t{1, 2}, edge_t{1, 1}, edge_t{0, 3}}}),

        // node_2_2_1_1_node_2_node_1
        make_and_check(report_line(""), {{edge_t{2, 0}, edge_t{2, 3}, edge_t{1, 0}, edge_t{1, 3}},
                                         {edge_t{0, 2}, edge_t{2, 2}, edge_t{2, 1}, edge_t{0, 3}},
                                         {edge_t{0, 0}, edge_t{1, 2}, edge_t{1, 1}, edge_t{0, 1}}}),

        // 'node_3_1_node_2_node_node'
        make_and_check(report_line(""), {{edge_t{3, 0},edge_t{1, 0}},
                                         {edge_t{0, 1}, edge_t{2, 0}},
                                         {edge_t{1, 1}},
                                         {edge_t{0, 0}}}),

        // 'node_1_node_node_node_2'
        make_and_check(report_line(""), {{edge_t{1, 0}}, {edge_t{0, 0}}, {edge_t{3, 0}}, {edge_t{2, 0}}}),

        // 'node_2_node_node_node_1'
        make_and_check(report_line(""), {{edge_t{2, 0}}, {edge_t{3, 0}}, {edge_t{0, 0}}, {edge_t{1, 0}}})
      }
    };

    auto checker{
        [this](std::string_view description, const graph_to_test& obtained, const graph_to_test& prediction, const graph_to_test& parent, std::size_t host, std::size_t target) {
          check(equality, description, obtained, prediction);
          if(host != target) check_semantics(description, prediction, parent);
        }
    };

    transition_checker<graph_to_test>::check(report_line(""), trg, checker);
  }
}