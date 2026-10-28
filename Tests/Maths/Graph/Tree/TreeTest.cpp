////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TreeTest.hpp"

#include "sequoia/Maths/Graph/DynamicTree.hpp"
#include "sequoia/TestFramework/StateTransitionUtilities.hpp"

#include <algorithm>
#include <ranges>
#include <vector>

namespace sequoia::testing
{
  using namespace maths;

  namespace
  {
    template<class Tree>
    inline constexpr bool forest_beneath_callable_v{
      requires(const Tree& tree) { forest_beneath(tree, 0); }
    };

    /** \brief Orders node 0 first, and the other nodes by descending index. */
    inline constexpr auto root_first_then_descending_index{
      [](std::size_t i, std::size_t j) { return (i == 0) ? (j != 0) : ((j != 0) && (i > j)); }
    };

    /** \brief Enough nodes that an unstable sort may reorder nodes it treats
               as equivalent.
     */
    constexpr int nodes_in_star{64};

    /** \brief Orders node 0 and the odd nodes, as equivalent, before the
               other even nodes.
     */
    inline constexpr auto root_and_odd_nodes_first{
      [](auto i, auto j) {
        auto isOtherEven{[](auto n) { return (n != 0) && (n % 2 == 0); }};
        return !isOtherEven(i) && isOtherEven(j);
      }
    };

    /** \brief Returns a tree whose root has the first of `weights`, with a
               child for each of the rest, in order.

        \pre `weights` is not empty.
     */
    template<maths::dynamic_tree Tree>
    [[nodiscard]]
    Tree star_tree(const std::vector<int>& weights)
    {
      Tree tree{{weights.front()}};
      for(const auto weight : weights | std::views::drop(1))
      {
        tree.add_node(0, weight);
      }

      return tree;
    }

    /** \brief Returns the weights `0` to `nodes_in_star - 1`, each the index
               of its node in the tree `star_tree` builds from them.
     */
    [[nodiscard]]
    std::vector<int> star_weights()
    {
      return std::views::iota(0, nodes_in_star) | std::ranges::to<std::vector>();
    }

    /** \brief Returns `star_weights()`, stably sorted by
               `root_and_odd_nodes_first`.
     */
    [[nodiscard]]
    std::vector<int> stably_sorted_star_weights()
    {
      auto weights{star_weights()};
      std::ranges::stable_sort(weights, root_and_odd_nodes_first);
      return weights;
    }

    inline constexpr auto by_ascending_target{
      [](const auto& l, const auto& r) { return l.target_node() < r.target_node(); }
    };

    inline constexpr auto by_descending_target{
      [](const auto& l, const auto& r) { return l.target_node() > r.target_node(); }
    };

    template<class Tree>
    inline constexpr bool sort_all_nodes_callable_v{
      requires(Tree& tree) { tree.sort_nodes(std::ranges::less{}); }
    };
  }

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

    test_root_fixed_at_zero<directed_tree<tree_link_direction::forward, null_weight, int>>();
    test_root_fixed_at_zero<directed_tree<tree_link_direction::backward, null_weight, int>>();
    test_root_fixed_at_zero<directed_tree<tree_link_direction::symmetric, null_weight, int>>();
    test_root_fixed_at_zero<undirected_tree<tree_link_direction::symmetric, null_weight, int>>();

    test_sort_all_nodes<directed_tree<tree_link_direction::forward, null_weight, int>>();
    test_sort_all_nodes<directed_tree<tree_link_direction::backward, null_weight, int>>();

    test_prune_of_absent_node<directed_tree<tree_link_direction::forward, null_weight, int>>();
    test_prune_of_absent_node<directed_tree<tree_link_direction::backward, null_weight, int>>();
    test_prune_of_absent_node<directed_tree<tree_link_direction::symmetric, null_weight, int>>();
    test_prune_of_absent_node<undirected_tree<tree_link_direction::symmetric, null_weight, int>>();

    test_forest_ranges();
    test_sort_nodes_constraints();
    test_forest_beneath_constraints();
    test_adaptors_at_absent_nodes();
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
              t.sort_edges(t.cbegin_edges(0), t.cend_edges(0), by_ascending_target);
              return t;
            }
          },
          edge_t{4, report("Insert node"), [](tree_t t) -> tree_t {
              t.insert_node(1, 0, -7);
              t.stable_sort_edges(t.cbegin_edges(0), t.cend_edges(0), by_ascending_target);
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
          edge_t{2, report("Prune right branch"), [](tree_t t) { t.prune(2); return t; }},
          edge_t{2, report("Prune a branch whose leaf has the lower index"), [](tree_t t) {
              t.sort_nodes(1, t.order(), std::ranges::greater{});
              t.prune(2);
              return t;
            }
          },
          edge_t{2, report("Prune a branch whose first edge goes to its leaf"), [](tree_t t) {
              t.sort_edges(t.cbegin_edges(2), t.cend_edges(2), by_descending_target);
              t.prune(2);
              return t;
            }
          }
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

  template<maths::dynamic_tree Tree>
  void tree_test::test_root_fixed_at_zero()
  {
    using tree_t      = Tree;
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
          edge_t{1, report("Insert the first node at 0"), [](tree_t t) {
              t.insert_node(0, tree_t::npos, 42);
              return t;
            }
          },
          edge_t{0, report("Sort an empty range of an empty tree"), [](tree_t t) {
              t.sort_nodes(0, 0, std::ranges::greater{});
              return t;
            }
          }
        }, // end node 0 edges
        {
          edge_t{1, report("Insert a node at 0 of a non-empty tree"), [this](tree_t t) {
              check_exception_thrown<std::out_of_range>("Insert a node at 0 of a non-empty tree",
                                                        [&t](){ t.insert_node(0, 1, -7); });
              return t;
            }
          },
          edge_t{2, report("Insert a node at 1"), [](tree_t t) { t.insert_node(1, 0, -7); return t; }}
        }, // end node 1 edges
        {
          edge_t{2, report("Swap the root with another node"), [this](tree_t t) {
              check_exception_thrown<std::out_of_range>("Swap the root with another node",
                                                        [&t](){ t.swap_nodes(0, 1); });
              return t;
            }
          },
          edge_t{2, report("Swap another node with the root"), [this](tree_t t) {
              check_exception_thrown<std::out_of_range>("Swap another node with the root",
                                                        [&t](){ t.swap_nodes(1, 0); });
              return t;
            }
          },
          edge_t{2, report("Swap the root with itself"), [](tree_t t) { t.swap_nodes(0, 0); return t; }},
          edge_t{2, report("Sort a range holding only the root"), [](tree_t t) {
              t.sort_nodes(0, 1, std::ranges::greater{});
              return t;
            }
          },
          edge_t{3, report("Insert a node at 2"), [](tree_t t) { t.insert_node(2, 0, 6); return t; }}
        }, // end node 2 edges
        {
          edge_t{3, report("Sort a range of nodes so as to move the root"), [this](tree_t t) {
              check_exception_thrown<std::out_of_range>("Sort a range of nodes so as to move the root",
                                                        [&t](){ t.sort_nodes(0, 3, std::ranges::greater{}); });
              return t;
            }
          },
          edge_t{3, report("Sort a range whose end precedes its start"), [](tree_t t) {
              t.sort_nodes(2, 1, std::ranges::greater{});
              return t;
            }
          },
          edge_t{4, report("Sort a range of nodes from the root, keeping it first"), [](tree_t t) {
              t.sort_nodes(0, 3, root_first_then_descending_index);
              t.sort_edges(t.cbegin_edges(0), t.cend_edges(0), by_ascending_target);
              return t;
            }
          }
        }, // end node 3 edges
        {
        }, // end node 4 edges
        {
          edge_t{6, report("Sort with some nodes equivalent to the root"), [](tree_t t) {
              t.sort_nodes(0, t.order(), root_and_odd_nodes_first);
              t.sort_edges(t.cbegin_edges(0), t.cend_edges(0), by_ascending_target);
              return t;
            }
          }
        }, // end node 5 edges
        {
        } // end node 6 edges
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
        // -7  6
        //  \ /
        //   42
        {report(""), initCheckFn, initializer{42, {{-7}, {6}}}},
        //  6  -7
        //  \ /
        //   42
        {report(""), initCheckFn, initializer{42, {{6}, {-7}}}},
        // 0 with children 1 to nodes_in_star - 1: an input, so not checked
        star_tree<tree_t>(star_weights()),
        // 0 with its odd children before its even ones
        star_tree<tree_t>(stably_sorted_star_weights())
      } // end nodes
    };

    auto checkerFn{
        [this](std::string_view description, const tree_t& obtained, const tree_t& prediction) {
          check(equality, description, obtained, prediction);
        }
    };

    transition_checker_t::check(report(""), g, checkerFn);
  }

  template<maths::dynamic_tree Tree>
    requires (Tree::link_dir != maths::tree_link_direction::symmetric)
  void tree_test::test_sort_all_nodes()
  {
    using tree_t      = Tree;
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
          edge_t{0, report("Sort all the nodes so as to move the root"), [this](tree_t t) {
              check_exception_thrown<std::out_of_range>("Sort all the nodes so as to move the root",
                                                        [&t](){ t.sort_nodes(std::ranges::greater{}); });
              return t;
            }
          },
          edge_t{1, report("Sort all the nodes, keeping the root first"), [](tree_t t) {
              t.sort_nodes(root_first_then_descending_index);
              t.sort_edges(t.cbegin_edges(0), t.cend_edges(0), by_ascending_target);
              return t;
            }
          }
        }, // end node 0 edges
        {
        } // end node 1 edges
      }, // end edges
      {
        // -7  6
        //  \ /
        //   42
        {report(""), initCheckFn, initializer{42, {{-7}, {6}}}},
        //  6  -7
        //  \ /
        //   42
        {report(""), initCheckFn, initializer{42, {{6}, {-7}}}}
      } // end nodes
    };

    auto checkerFn{
        [this](std::string_view description, const tree_t& obtained, const tree_t& prediction) {
          check(equality, description, obtained, prediction);
        }
    };

    transition_checker_t::check(report(""), g, checkerFn);
  }

  template<maths::dynamic_tree Tree>
  void tree_test::test_prune_of_absent_node()
  {
    // 42 with child -7
    Tree tree{{42, {{-7}}}};

    check_exception_thrown<std::out_of_range>("Prune a node the tree lacks", [&tree](){ tree.prune(2); });
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
    check_exception_thrown<std::out_of_range>("Subtrees beneath a node the tree lacks",
                                              [&tree](){ return forest_beneath(tree, 4); });

    const std::vector<tree_t> forest{tree, tree_t{{-1}}};
    check(equality, "A forest's trees, each at its root", rootWeights(forest_of(forest)), std::vector<int>{42, -1});
  }

  void tree_test::test_sort_nodes_constraints()
  {
    STATIC_CHECK( sort_all_nodes_callable_v<directed_tree<tree_link_direction::forward,     null_weight, int>>);
    STATIC_CHECK( sort_all_nodes_callable_v<directed_tree<tree_link_direction::backward,    null_weight, int>>);
    STATIC_CHECK(!sort_all_nodes_callable_v<directed_tree<tree_link_direction::symmetric,   null_weight, int>>);
    STATIC_CHECK(!sort_all_nodes_callable_v<undirected_tree<tree_link_direction::symmetric, null_weight, int>>);
  }

  void tree_test::test_forest_beneath_constraints()
  {
    STATIC_CHECK( forest_beneath_callable_v<directed_tree<tree_link_direction::forward,     null_weight, int>>);
    STATIC_CHECK(!forest_beneath_callable_v<directed_tree<tree_link_direction::backward,    null_weight, int>>);
    STATIC_CHECK(!forest_beneath_callable_v<directed_tree<tree_link_direction::symmetric,   null_weight, int>>);
    STATIC_CHECK(!forest_beneath_callable_v<undirected_tree<tree_link_direction::symmetric, null_weight, int>>);
  }

  void tree_test::test_adaptors_at_absent_nodes()
  {
    using tree_t = directed_tree<tree_link_direction::forward, null_weight, int>;

    const std::vector<tree_t> forest{tree_t{}};
    const auto adaptorOfEmptyTree{*forest_of(forest).begin()};

    check("An empty tree's adaptor is false", !adaptorOfEmptyTree);
    check_exception_thrown<std::out_of_range>("root_weight of an empty tree's adaptor",
                                              [&adaptorOfEmptyTree](){ return root_weight(adaptorOfEmptyTree); });
    check_exception_thrown<std::out_of_range>("root_weight_iter of an empty tree's adaptor",
                                              [&adaptorOfEmptyTree](){ return root_weight_iter(adaptorOfEmptyTree); });

    // 42 with children -7 and 6
    tree_t tree{{42, {{-7}, {6}}}};
    const tree_adaptor<tree_t> rootAdaptor{tree, 0};
    tree_adaptor<tree_t> leafAdaptor{tree, 2};
    tree.prune(0);

    check_exception_thrown<std::out_of_range>("root_weight of an adaptor at a node its tree has lost",
                                              [&rootAdaptor](){ return root_weight(rootAdaptor); });

    // At node 0, the node storage would refuse the mutation itself
    check_exception_thrown<std::out_of_range>("mutate_root_weight of an adaptor beyond the end of its tree",
                                              [&leafAdaptor](){ mutate_root_weight(leafAdaptor, [](int& w){ ++w; }); });
  }
}
