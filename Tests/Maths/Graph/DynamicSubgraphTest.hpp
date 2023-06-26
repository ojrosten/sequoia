////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "DynamicGraphTestingUtilities.hpp"

namespace sequoia::testing
{
  class test_subgraph final : public regular_test
  {
  public:
    using regular_test::regular_test;

    [[nodiscard]]
    std::filesystem::path source_file() const;

    void run_tests();
  private:
    template <class, class, concrete_test>
    friend class graph_test_helper;


    template
    <
      maths::graph_flavour GraphFlavour,
      class EdgeWeight,
      class NodeWeight,
      class EdgeWeightCreator,
      class NodeWeightCreator,
      class EdgeStorageTraits,
      class NodeWeightStorageTraits
    >
    void execute_operations();
  };
}
