////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "TreeTest.hpp"

#include "sequoia/Maths/Graph/DynamicTree.hpp"
#include "sequoia/TestFramework/StateTransitionUtilities.hpp"

namespace sequoia::testing
{
  using namespace maths;

  [[nodiscard]]
  std::filesystem::path tree_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void tree_test::run_tests()
  {
    test_tree<directed_tree<tree_link_direction::forward, null_weight, int>>();
    test_tree<directed_tree<tree_link_direction::backward, null_weight, int>>();
    test_tree<directed_tree<tree_link_direction::symmetric, null_weight, int>>();
    test_tree<undirected_tree<tree_link_direction::symmetric, null_weight, int>>();

    test_tree_unweighted_nodes<directed_tree<tree_link_direction::forward, null_weight, null_weight>>();
    test_tree_unweighted_nodes<directed_tree<tree_link_direction::backward, null_weight, null_weight>>();
    test_tree_unweighted_nodes<directed_tree<tree_link_direction::symmetric, null_weight, null_weight>>();
    test_tree_unweighted_nodes<undirected_tree<tree_link_direction::symmetric, null_weight, null_weight>>();
  }

  template<maths::dynamic_tree Tree>
  void tree_test::test_tree()
  {
    using tree_type = Tree;
    using initializer = tree_initializer<int>;

    auto initCheckFn{
      [this](std::string_view message, const tree_type& t, initializer i) {
        check(equivalence, message, t, i);
      }
    };

    using transition_checker_type = transition_checker<tree_type>;
    using tree_state_graph        = typename transition_checker_type::transition_graph;
    using edge_t                  = typename transition_checker_type::edge;

    tree_state_graph g{
      {
        {
          edge_t{1, report("Add node to empty tree"), [](tree_type t) { t.add_node(tree_type::npos, 42); return t; }}
        }, // end node 0 edges
        {
          edge_t{2, report("Add second node"), [](tree_type t) { t.add_node(0, -7); return t; }},
          edge_t{3, report("Add second node"), [](tree_type t) { t.add_node(0, 6); return t; }}
        }, // end node 1 edges
        {
          edge_t{4, report("Add third node"),    [](tree_type t) { t.add_node(0, 6); return t; }},
          edge_t{1, report("Prune single node"), [](tree_type t) { t.prune(1); return t; }},
          edge_t{0, report("Prune both nodes"),  [](tree_type t) { t.prune(0); return t; }}
        }, // end node 2 edges
        {
          edge_t{4, report("Insert node"), [](tree_type t) {
              t.insert_node(1, 0, -7);
              t.sort_edges(t.cbegin_edges(0), t.cend_edges(0), [](const auto& l, const auto& r) { return l.target_node() < r.target_node(); });
              return t;
            }
          },
          edge_t{4, report("Insert node"), [](tree_type t) {
              t.insert_node(1, 0, -7);
              t.stable_sort_edges(t.cbegin_edges(0), t.cend_edges(0), [](const auto& l, const auto& r) { return l.target_node() < r.target_node(); });
              return t;
            }
          }
        }, // end node 3 edges
        {
          edge_t{0, report("Prune three nodes"),   [](tree_type t) { t.prune(0); return t; }},
          edge_t{2, report("Prune right node"),    [](tree_type t) { t.prune(2); return t; }},
          edge_t{3, report("Prune left node"),     [](tree_type t) { t.prune(1); return t; }},
          edge_t{5, report("Add to right branch"), [](tree_type t) { t.add_node(2, 3); return t; }}
        }, // end node 4 edges
        {
          edge_t{0, report("Prune four nodes"),   [](tree_type t) { t.prune(0); return t; }},
          edge_t{2, report("Prune right branch"), [](tree_type t) { t.prune(2); return t; }}
        } // end node 5 edges
      }, // end edges
      {
        // empty
        tree_type{},
        // 42
        {report(""), initCheckFn, initializer{42}},
        // -7
        //  \
        //   42
        {report(""), initCheckFn, initializer{42, {{-7}}}},
        //  6
        //  \
        //   42
        {report(""), initCheckFn, initializer{42, {{6}}}},
        // -7  6
        //  \ /
        //   42
        {report(""), initCheckFn, initializer{42, {{-7}, {6}}}},
        //       3
        //      /
        // -7  6
        //  \ /
        //   42
        {report(""), initCheckFn, initializer{42, {{-7}, {6, {{3}}}}}}
      } // end nodes
    };

    auto checker{
        [this](std::string_view description, const tree_type& obtained, const tree_type& prediction, const tree_type& parent) {
          check(equality, description, obtained, prediction);
          check_semantics(description, prediction, parent);
        }
    };

    transition_checker_type::check(report(""), g, checker);
  }

  template<maths::dynamic_tree Tree>
  void tree_test::test_tree_unweighted_nodes()
  {
    using tree_type   = Tree;
    using initializer = tree_initializer<null_weight>;

    auto initCheckFn{
      [this](std::string_view message, const tree_type& t, initializer i) {
        check(equivalence, message, t, i);
      }
    };

    using transition_checker_type = transition_checker<tree_type>;
    using tree_state_graph        = typename transition_checker_type::transition_graph;
    using edge_t                  = typename transition_checker_type::edge;

    tree_state_graph g{
      {
        {
          edge_t{1, report("Add node to empty tree"), [](tree_type t) { t.add_node(tree_type::npos); return t; }}
        }, // end node 0 edges
        {
          edge_t{2, report("Add second node"), [](tree_type t) { t.add_node(0); return t; }},
        }, // end node 1 edges
        {
          edge_t{3, report("Add third node"),    [](tree_type t) { t.add_node(0); return t; }},
          edge_t{1, report("Prune single node"), [](tree_type t) { t.prune(1); return t; }},
          edge_t{0, report("Prune both nodes"),  [](tree_type t) { t.prune(0); return t; }}
        }, // end node 2 edges
        {
          edge_t{0, report("Prune three nodes"),   [](tree_type t) { t.prune(0); return t; }},
          edge_t{2, report("Prune right node"),    [](tree_type t) { t.prune(2); return t; }},
          edge_t{4, report("Add to right branch"), [](tree_type t) { t.add_node(2); return t; }}
        }, // end node 3 edges
        {
          edge_t{0, report("Prune four nodes"),   [](tree_type t) { t.prune(0); return t; }},
          edge_t{2, report("Prune right branch"), [](tree_type t) { t.prune(2); return t; }}
        } // end node 4 edges
      }, // end edges
      {
        // empty
        tree_type{},
        // x
        {report(""), initCheckFn, initializer{}},
        // x
        //  \
        //   x
        {report(""), initCheckFn, initializer{{{}}}},
        // x   x
        //  \ /
        //   42
        {report(""), initCheckFn, initializer{{{}, {}}}},
        //       x
        //      /
        // x   x
        //  \ /
        //   42
        {report(""), initCheckFn, initializer{{{}, {{{}}}}}}
      } // end nodes
    };

    auto checker{
        [this](std::string_view description, const tree_type& obtained, const tree_type& prediction, const tree_type& parent) {
          check(equality, description, obtained, prediction);
          check_semantics(description, prediction, parent);
        }
    };

    transition_checker_type::check(report(""), g, checker);
  }
}
