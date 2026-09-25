////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TreeTest.hpp"

#include "sequoia/Maths/Graph/DynamicTree.hpp"
#include "sequoia/TestFramework/StateTransitionUtilities.hpp"

#include <ranges>

namespace sequoia::testing
{
  using namespace maths;

  [[nodiscard]]
  std::filesystem::path tree_test::source_file()
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

    test_forest_ranges();
  }

  template<maths::dynamic_tree Tree>
  void tree_test::test_tree()
  {
    using tree_t = Tree;
    using initializer = tree_initializer<int>;

    auto initCheckFn{
      [this](std::string_view message, const tree_t& t, initializer i) {
        check(equivalence, message, t, i);
      }
    };

    using transition_checker_t = transition_checker<tree_t>;
    using tree_state_graph     = transition_checker_t::transition_graph;
    using edge_t               = transition_checker_t::edge;

    tree_state_graph g{
      {
        {
          edge_t{1, report("Add node to empty tree"), [](tree_t t) { t.add_node(tree_t::npos, 42); return t; }}
        }, // end node 0 edges
        {
          edge_t{2, report("Add second node"), [](tree_t t) { t.add_node(0, -7); return t; }},
          edge_t{3, report("Add second node"), [](tree_t t) { t.add_node(0, 6); return t; }}
        }, // end node 1 edges
        {
          edge_t{4, report("Add third node"),    [](tree_t t) { t.add_node(0, 6); return t; }},
          edge_t{1, report("Prune single node"), [](tree_t t) { t.prune(1); return t; }},
          edge_t{0, report("Prune both nodes"),  [](tree_t t) { t.prune(0); return t; }}
        }, // end node 2 edges
        {
          edge_t{4, report("Insert node"), [](tree_t t) -> tree_t {
              t.insert_node(1, 0, -7);
              t.sort_edges(t.cbegin_edges(0), t.cend_edges(0), [](const auto& l, const auto& r) { return l.target_node() < r.target_node(); });
              return t;
            }
          },
          edge_t{4, report("Insert node"), [](tree_t t) -> tree_t {
              t.insert_node(1, 0, -7);
              t.stable_sort_edges(t.cbegin_edges(0), t.cend_edges(0), [](const auto& l, const auto& r) { return l.target_node() < r.target_node(); });
              return t;
            }
          }
        }, // end node 3 edges
        {
          edge_t{0, report("Prune three nodes"),   [](tree_t t) { t.prune(0); return t; }},
          edge_t{2, report("Prune right node"),    [](tree_t t) { t.prune(2); return t; }},
          edge_t{3, report("Prune left node"),     [](tree_t t) { t.prune(1); return t; }},
          edge_t{5, report("Add to right branch"), [](tree_t t) { t.add_node(2, 3); return t; }}
        }, // end node 4 edges
        {
          edge_t{0, report("Prune four nodes"),   [](tree_t t) { t.prune(0); return t; }},
          edge_t{2, report("Prune right branch"), [](tree_t t) { t.prune(2); return t; }}
        } // end node 5 edges
      }, // end edges
      {
        // empty
        tree_t{},
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

    auto checkerFn{
        [this](std::string_view description, const tree_t& obtained, const tree_t& prediction, const tree_t& parent) {
          check(equality, description, obtained, prediction);
          check_semantics(description, prediction, parent);
        }
    };

    transition_checker_t::check(report(""), g, checkerFn);
  }

  template<maths::dynamic_tree Tree>
  void tree_test::test_tree_unweighted_nodes()
  {
    using tree_t      = Tree;
    using initializer = tree_initializer<null_weight>;

    auto initCheckFn{
      [this](std::string_view message, const tree_t& t, initializer i) {
        check(equivalence, message, t, i);
      }
    };

    using transition_checker_t = transition_checker<tree_t>;
    using tree_state_graph     = transition_checker_t::transition_graph;
    using edge_t               = transition_checker_t::edge;

    tree_state_graph g{
      {
        {
          edge_t{1, report("Add node to empty tree"), [](tree_t t) { t.add_node(tree_t::npos); return t; }}
        }, // end node 0 edges
        {
          edge_t{2, report("Add second node"), [](tree_t t) { t.add_node(0); return t; }},
        }, // end node 1 edges
        {
          edge_t{3, report("Add third node"),    [](tree_t t) { t.add_node(0); return t; }},
          edge_t{1, report("Prune single node"), [](tree_t t) { t.prune(1); return t; }},
          edge_t{0, report("Prune both nodes"),  [](tree_t t) { t.prune(0); return t; }}
        }, // end node 2 edges
        {
          edge_t{0, report("Prune three nodes"),   [](tree_t t) { t.prune(0); return t; }},
          edge_t{2, report("Prune right node"),    [](tree_t t) { t.prune(2); return t; }},
          edge_t{4, report("Add to right branch"), [](tree_t t) { t.add_node(2); return t; }}
        }, // end node 3 edges
        {
          edge_t{0, report("Prune four nodes"),   [](tree_t t) { t.prune(0); return t; }},
          edge_t{2, report("Prune right branch"), [](tree_t t) { t.prune(2); return t; }}
        } // end node 4 edges
      }, // end edges
      {
        // empty
        tree_t{},
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

    auto checkerFn{
        [this](std::string_view description, const tree_t& obtained, const tree_t& prediction, const tree_t& parent) {
          check(equality, description, obtained, prediction);
          check_semantics(description, prediction, parent);
        }
    };

    transition_checker_t::check(report(""), g, checkerFn);
  }

  void tree_test::test_forest_ranges()
  {
    using tree_t = directed_tree<tree_link_direction::forward, null_weight, int>;

    // 42 with children -7 and 6, the latter with child 3
    const tree_t tree{{42, {{-7}, {6, {{3}}}}}};

    auto rootWeights{
      [](const auto& forest) {
        return forest | std::views::transform([](const auto& adaptor){ return root_weight(adaptor); }) | std::ranges::to<std::vector>();
      }
    };

    check(equality, "Subtrees beneath the root",       rootWeights(forest_beneath(tree, 0)), std::vector<int>{-7, 6});
    check(equality, "Subtrees beneath an inner node",  rootWeights(forest_beneath(tree, 2)), std::vector<int>{3});
    check(equality, "Subtrees beneath a leaf",         rootWeights(forest_beneath(tree, 1)), std::vector<int>{});

    const std::vector<tree_t> forest{tree, tree_t{{-1}}};
    check(equality, "A forest's trees, each at its root", rootWeights(forest_of(forest)), std::vector<int>{42, -1});
  }
}
