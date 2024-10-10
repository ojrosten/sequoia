////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "TreeTestingDiagnostics.hpp"

#include "sequoia/Maths/Graph/DynamicTree.hpp"

namespace sequoia::testing
{
  using namespace maths;

  [[nodiscard]]
  std::filesystem::path tree_false_positive_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void tree_false_positive_test::run_tests()
  {
    test_tree<directed_tree<tree_link_direction::forward, null_weight, int>>();
    test_tree<directed_tree<tree_link_direction::backward, null_weight, int>>();
    test_tree<directed_tree<tree_link_direction::symmetric, null_weight, int>>();
    test_tree<undirected_tree<tree_link_direction::symmetric, null_weight, int>>();
  }

  template<maths::dynamic_tree Tree>
  void tree_false_positive_test::test_tree()
  {
    using tree_type = Tree;
    using node_weight_type = typename  Tree::node_weight_type;
    using initializer = tree_initializer<node_weight_type>;

    tree_type x{}, y{{1}}, z{{1, {{2}}}}, w{{1, {{2, {{4}, {5}}}, {3}}}};

    check(equivalence, "Empty vs non-empty", x, initializer{1, {}});
    check(equivalence, "Incorrect weight", y, initializer{0, {}});
    check(equivalence, "Too many children", z, initializer{1, {}});
    check(equivalence, "Incorrect child weight", z, initializer{1, {{3}}});
    check(equivalence, "Too few children", z, initializer{1, {{2}, {3}}});

    check(equivalence, "Too many grand children", w, initializer{1, {{2, {{4}}}, {3}}});
    check(equivalence, "Incorrect grand child weight", w, initializer{1, {{2, {{3}, {4}}}, {3}}});
    check(equivalence, "Too few grand children", w, initializer{1, {{2, {{4}, {5}, {6}}}, {3}}});

    check(equality, "", x, y);
    check(equality, "", y, z);
  }
}
