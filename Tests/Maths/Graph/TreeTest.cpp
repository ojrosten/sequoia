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
  std::string_view tree_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void tree_test::run_tests()
  {
    test_tree(directed_type{},   forward_tree_type{});
    test_tree(directed_type{},   backward_tree_type{});
    test_tree(directed_type{},   symmetric_tree_type{});
    test_tree(undirected_type{}, symmetric_tree_type{});

    test_tree_unweighted_nodes(directed_type{}, forward_tree_type{});
    test_tree_unweighted_nodes(directed_type{}, backward_tree_type{});
    test_tree_unweighted_nodes(directed_type{}, symmetric_tree_type{});
    test_tree_unweighted_nodes(undirected_type{}, symmetric_tree_type{});
  }

  template<maths::directed_flavour Directedness, maths::tree_link_direction TreeLinkDir>
  void tree_test::test_tree(maths::directed_flavour_constant<Directedness>, maths::tree_link_direction_constant<TreeLinkDir>)
  {
    using tree_type = tree<Directedness, TreeLinkDir, null_weight, int>;
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
          edge_t{1, LINE("Add node to empty tree"), [](tree_type t) { t.add_node(tree_type::npos, 42); return t; }}
        }, // end node 0 edges
        {
          edge_t{2, LINE("Add second node"), [](tree_type t) { t.add_node(0, -7); return t; }},
          edge_t{3, LINE("Add second node"), [](tree_type t) { t.add_node(0, 6); return t; }}
        }, // end node 1 edges
        {
          edge_t{4, LINE("Add third node"),    [](tree_type t) { t.add_node(0, 6); return t; }},
          edge_t{1, LINE("Prune single node"), [](tree_type t) { t.prune(1); return t; }},
          edge_t{0, LINE("Prune both nodes"),  [](tree_type t) { t.prune(0); return t; }}
        }, // end node 2 edges
        {
          edge_t{4, LINE("Insert node"), [](tree_type t) {
              t.insert_node(1, 0, -7);
              t.sort_edges(t.cbegin_edges(0), t.cend_edges(0), [](const auto& l, const auto& r) { return l.target_node() < r.target_node(); });
              return t;
            }
          }
        }, // end node 3 edges
        {
          edge_t{0, LINE("Prune three nodes"),   [](tree_type t) { t.prune(0); return t; }},
          edge_t{2, LINE("Prune right node"),    [](tree_type t) { t.prune(2); return t; }},
          edge_t{3, LINE("Prune left node"),     [](tree_type t) { t.prune(1); return t; }},
          edge_t{5, LINE("Add to right branch"), [](tree_type t) { t.add_node(2, 3); return t; }}
        }, // end node 4 edges
        {
          edge_t{0, LINE("Prune four nodes"),   [](tree_type t) { t.prune(0); return t; }},
          edge_t{2, LINE("Prune right branch"), [](tree_type t) { t.prune(2); return t; }}
        } // end node 5 edges
      }, // end edges
      {
        // empty
        tree_type{},
        // 42
        {LINE(""), initCheckFn, initializer{42}},
        // -7
        //  \
        //   42
        {LINE(""), initCheckFn, initializer{42, {{-7}}}},
        //  6
        //  \
        //   42
        {LINE(""), initCheckFn, initializer{42, {{6}}}},
        // -7  6
        //  \ /
        //   42
        {LINE(""), initCheckFn, initializer{42, {{-7}, {6}}}},
        //       3
        //      /
        // -7  6
        //  \ /
        //   42
        {LINE(""), initCheckFn, initializer{42, {{-7}, {6, {{3}}}}}}
      } // end nodes
    };

    auto checker{
        [this](std::string_view description, const tree_type& obtained, const tree_type& prediction, const tree_type& parent) {
          check(equality, description, obtained, prediction);
          check_semantics(description, prediction, parent);
        }
    };

    transition_checker_type::check(LINE(""), g, checker);
  }

  template<maths::directed_flavour Directedness, maths::tree_link_direction TreeLinkDir>
  void tree_test::test_tree_unweighted_nodes(maths::directed_flavour_constant<Directedness>, maths::tree_link_direction_constant<TreeLinkDir>)
  {
    using tree_type   = tree<Directedness, TreeLinkDir, null_weight, null_weight>;
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
          edge_t{1, LINE("Add node to empty tree"), [](tree_type t) { t.add_node(tree_type::npos); return t; }}
        }, // end node 0 edges
        {
          edge_t{2, LINE("Add second node"), [](tree_type t) { t.add_node(0); return t; }},
        }, // end node 1 edges
        {
          edge_t{3, LINE("Add third node"),    [](tree_type t) { t.add_node(0); return t; }},
          edge_t{1, LINE("Prune single node"), [](tree_type t) { t.prune(1); return t; }},
          edge_t{0, LINE("Prune both nodes"),  [](tree_type t) { t.prune(0); return t; }}
        }, // end node 2 edges
        {
          edge_t{0, LINE("Prune three nodes"),   [](tree_type t) { t.prune(0); return t; }},
          edge_t{2, LINE("Prune right node"),    [](tree_type t) { t.prune(2); return t; }},
          edge_t{4, LINE("Add to right branch"), [](tree_type t) { t.add_node(2); return t; }}
        }, // end node 3 edges
        {
          edge_t{0, LINE("Prune four nodes"),   [](tree_type t) { t.prune(0); return t; }},
          edge_t{2, LINE("Prune right branch"), [](tree_type t) { t.prune(2); return t; }}
        } // end node 4 edges
      }, // end edges
      {
        // empty
        tree_type{},
        // x
        {LINE(""), initCheckFn, initializer{}},
        // x
        //  \
        //   x
        {LINE(""), initCheckFn, initializer{{{}}}},
        // x   x
        //  \ /
        //   42
        {LINE(""), initCheckFn, initializer{{{}, {}}}},
        //       x
        //      /
        // x   x
        //  \ /
        //   42
        {LINE(""), initCheckFn, initializer{{{}, {{{}}}}}}
      } // end nodes
    };

    auto checker{
        [this](std::string_view description, const tree_type& obtained, const tree_type& prediction, const tree_type& parent) {
          check(equality, description, obtained, prediction);
          check_semantics(description, prediction, parent);
        }
    };

    transition_checker_type::check(LINE(""), g, checker);

    using tree_type = tree<Directedness, TreeLinkDir, null_weight, null_weight>;
    using initializer = tree_initializer<null_weight>;

    tree_type x{}, y{{}}, z{{{{}}}};

    check(equivalence, LINE(""), y, initializer{});
    check(equivalence, LINE(""), z, initializer{{{}}});

    check_semantics(LINE(""), x, y);
    check_semantics(LINE(""), y, z);
  }
}
