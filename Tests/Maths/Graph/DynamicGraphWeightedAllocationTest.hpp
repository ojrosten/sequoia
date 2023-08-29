////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "DynamicGraphAllocationTestingUtilities.hpp"

namespace sequoia::testing
{
  class weighted_graph_allocation_test final : public regular_allocation_test
  {
  public:
    using regular_allocation_test::regular_allocation_test;

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
      class EdgeStorageConfig,
      class NodeWeightStorage
    >
    void execute_operations();

    template<class Graph>
    void contiguous_memory();

    template<class Graph>
    void bucketed_memory();
  };
}
