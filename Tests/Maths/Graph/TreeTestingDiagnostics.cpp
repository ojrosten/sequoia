////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TreeTestingDiagnostics.hpp"

namespace sequoia::testing
{
  using namespace maths;

  [[nodiscard]]
  std::string_view tree_false_positive_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void tree_false_positive_test::run_tests()
  {
    test_forward_link_tree();
    test_backward_link_tree();
    test_symmetric_link_tree();
    test_undirected_tree(); 
  }

  void tree_false_positive_test::test_forward_link_tree()
  {
    using tree = tree<directed_flavour::directed, tree_link_direction::forward, null_weight, int>;
    using initializer = tree_initializer<int>;

    tree x{}, y{{1}}, z{{1, {{2}}}}, w{{1, {{2, {{4}, {5}}}, {3}}}};

    check_equivalence(LINE("Empty vs non-empty"), x, initializer{1, {}});
    check_equivalence(LINE("Incorrect weight"), y, initializer{0, {}});
    check_equivalence(LINE("Too many children"), z, initializer{1, {}});
    check_equivalence(LINE("Incorrect child weight"), z, initializer{1, {{3}}});
    check_equivalence(LINE("Too few children"), z, initializer{1, {{2}, {3}}});

    check_equivalence(LINE("Too many grand children"), w, initializer{1, {{2, {{4}}}, {3}}});
    check_equivalence(LINE("Incorrect grand child weight"), w, initializer{1, {{2, {{3}, {4}}}, {3}}});
    check_equivalence(LINE("Too few grand children"), w, initializer{1, {{2, {{4}, {5}, {6}}}, {3}}});

    check_equality(LINE(""), x, y);
    check_equality(LINE(""), y, z);
  }

  void tree_false_positive_test::test_backward_link_tree()
  {

  }

  void tree_false_positive_test::test_symmetric_link_tree()
  {
    using tree = tree<directed_flavour::directed, tree_link_direction::symmetric, null_weight, int>;
    using initializer = tree_initializer<int>;

    tree x{}, y{{1}}, z{{1, {{2}}}};
    tree w{{1, {{2, {{4}, {5}}}, {3}}}};

    check_equivalence(LINE("Empty vs non-empty"), x, initializer{1, {}});
    /*check_equivalence(LINE("Incorrect weight"), y, initializer{0, {}});
    check_equivalence(LINE("Too many children"), z, initializer{1, {}});
    check_equivalence(LINE("Incorrect child weight"), z, initializer{1, {{3}}});
    check_equivalence(LINE("Too few children"), z, initializer{1, {{2}, {3}}});

    check_equivalence(LINE("Too many grand children"), w, initializer{1, {{2, {{4}}}, {3}}});
    check_equivalence(LINE("Incorrect grand child weight"), w, initializer{1, {{2, {{3}, {4}}}, {3}}});
    check_equivalence(LINE("Too few grand children"), w, initializer{1, {{2, {{4}, {5}, {6}}}, {3}}});

    check_equality(LINE(""), x, y);
    check_equality(LINE(""), y, z);*/
  }

  void tree_false_positive_test::test_undirected_tree()
  {

  }
}
