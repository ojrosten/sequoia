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
  }

  template<maths::directed_flavour Directedness, maths::tree_link_direction TreeLinkDir>
  void tree_test::test_tree(maths::directed_flavour_constant<Directedness>, maths::tree_link_direction_constant<TreeLinkDir>)
  {
    using tree_type = tree<Directedness, TreeLinkDir, null_weight, int>;
    using initializer = tree_initializer<int>;

    tree_type x{}, y{{1}}, z{{1, {{2}}}};

    check(equivalence, LINE(""), y, initializer{1});
    check(equivalence, LINE(""), z, initializer{1, {{2}}});

    using tree_state_graph = transition_checker<tree_type>::transition_graph;
    using edge_t = transition_checker<tree_type>::edge;

    tree_state_graph g{
      {
        {
          edge_t{1, "Add node to empty tree", [](tree_type t) { t.add_node(tree_type::npos, 42); return t; }}
        }, // end node 0 edges
        {
          edge_t{2, "Add second node", [](tree_type t) { t.add_node(0, -7); return t; }}
        }, // end node 1 edges
        {} // end node 2 edges
      }, // end edges
      {
        tree_type{},
        tree_type{{42}},
        tree_type{{42, {{-7}}}}
      } // end nodes
    };

    auto checker{
        [this](std::string_view description, const tree_type& obtained, const tree_type& prediction, const tree_type& parent) {
          check(equality, description, obtained, prediction);
          check_semantics(description, prediction, parent);
        }
    };

    transition_checker<tree_type>::check(LINE(""), g, checker);
  }
}
