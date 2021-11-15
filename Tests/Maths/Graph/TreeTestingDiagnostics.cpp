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

  namespace
  {
    [[nodiscard]]
    std::string to_string(tree_link_direction dir)
    {
      switch(dir)
      {
      case tree_link_direction::forward:
        return "forward";
      case tree_link_direction::backward:
        return "backward";
      case tree_link_direction::symmetric:
        return "symmetric";
      }

      throw std::logic_error{"Unrecognized option for enum class maths::tree_link_direction"};
    }

    template<tree_link_direction dir>
    [[nodiscard]]
    std::string message(std::string_view m)
    {
      return std::string{"Tree Link Dir: "}.append(to_string(dir)).append(" - ").append(m);
    }
  }

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
    test_tree(forward_tree_type{});
  }

  void tree_false_positive_test::test_backward_link_tree()
  {

  }

  void tree_false_positive_test::test_symmetric_link_tree()
  {
    test_tree(symmetric_tree_type{});
  }

  void tree_false_positive_test::test_undirected_tree()
  {

  }

  template<maths::tree_link_direction TreeLinkDir>
  void tree_false_positive_test::test_tree(maths::tree_link_direction_constant<TreeLinkDir>)
  {
    using tree = tree<directed_flavour::directed, TreeLinkDir, null_weight, int>;
    using initializer = tree_initializer<int>;

    tree x{}, y{{1}}, z{{1, {{2}}}};
    tree w{{1, {{2, {{4}, {5}}}, {3}}}};

    check_equivalence(LINE(message<TreeLinkDir>("Empty vs non-empty")), x, initializer{1, {}});
    check_equivalence(LINE(message<TreeLinkDir>("Incorrect weight")), y, initializer{0, {}});
    check_equivalence(LINE(message<TreeLinkDir>("Too many children")), z, initializer{1, {}});
    check_equivalence(LINE(message<TreeLinkDir>("Incorrect child weight")), z, initializer{1, {{3}}});
    check_equivalence(LINE(message<TreeLinkDir>("Too few children")), z, initializer{1, {{2}, {3}}});

    check_equivalence(LINE(message<TreeLinkDir>("Too many grand children")), w, initializer{1, {{2, {{4}}}, {3}}});
    check_equivalence(LINE(message<TreeLinkDir>("Incorrect grand child weight")), w, initializer{1, {{2, {{3}, {4}}}, {3}}});
    check_equivalence(LINE(message<TreeLinkDir>("Too few grand children")), w, initializer{1, {{2, {{4}, {5}, {6}}}, {3}}});

    check_equality(LINE(""), x, y);
    check_equality(LINE(""), y, z);
  }
}
