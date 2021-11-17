////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TreeTest.hpp"

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
    tree<Directedness, TreeLinkDir, null_weight, int> x{}, y{{1}};

    check_equivalence(LINE(""), y, tree_initializer<int>{{1}});

    check_semantics(LINE(""), x, y);
  }
}
