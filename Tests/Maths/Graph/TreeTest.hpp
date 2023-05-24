////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "TreeTestingUtilities.hpp"

namespace sequoia::testing
{
  class tree_test final : public regular_test
  {
  public:
    using regular_test::regular_test;

    [[nodiscard]]
    std::filesystem::path source_file() const;

    void run_tests();
  private:

    template<maths::directed_flavour Directedness, maths::tree_link_direction TreeLinkDir>
    void test_tree(maths::directed_flavour_constant<Directedness>, maths::tree_link_direction_constant<TreeLinkDir>);

    template<maths::directed_flavour Directedness, maths::tree_link_direction TreeLinkDir>
    void test_tree_unweighted_nodes(maths::directed_flavour_constant<Directedness>, maths::tree_link_direction_constant<TreeLinkDir>);
  };
}
