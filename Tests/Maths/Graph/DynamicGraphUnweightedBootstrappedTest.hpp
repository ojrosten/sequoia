////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "DynamicGraphTestingUtilities.hpp"

namespace sequoia::testing
{
  class unweighted_graph_bootstrapped_test final : public regular_test
  {
  public:
    using regular_test::regular_test;

    [[nodiscard]]
    std::string_view source_file() const noexcept final;
  private:
    template <class, class, class>
    friend class graph_test_helper;

    void run_tests() final;

    /*template
    <
      maths::graph_flavour GraphFlavour,
      class EdgeWeight,
      class NodeWeight,
      class EdgeWeightCreator,
      class NodeWeightCreator,
      class EdgeStorageTraits,
      class NodeWeightStorageTraits
    >*/
    void execute_operations();
  };
}