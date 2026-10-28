////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include "TreeTestingUtilities.hpp"

namespace sequoia::testing
{
  class tree_test final : public regular_test
  {
  public:
    using regular_test::regular_test;

    [[nodiscard]]
    static std::filesystem::path source_file();

    void run_tests();
  private:

    template<maths::dynamic_tree Tree>
    void test_tree();

    template<maths::dynamic_tree Tree>
    void test_tree_unweighted_nodes();

    template<maths::dynamic_tree Tree>
    void test_root_fixed_at_zero();

    template<maths::dynamic_tree Tree>
      requires (Tree::link_dir != maths::tree_link_direction::symmetric)
    void test_sort_all_nodes();

    template<maths::dynamic_tree Tree>
    void test_prune_of_absent_node();

    void test_forest_ranges();

    void test_sort_nodes_constraints();

    void test_forest_beneath_constraints();

    void test_adaptors_at_absent_nodes();
  };
}
