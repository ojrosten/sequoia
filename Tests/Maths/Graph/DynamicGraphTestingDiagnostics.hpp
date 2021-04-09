////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "DynamicGraphTestingUtilities.hpp"

namespace sequoia::testing
{
  class test_graph_false_positives final : public graph_false_positive_test
  {
  public:
    using graph_false_positive_test::graph_false_positive_test;

    [[nodiscard]]
    std::string_view source_file() const noexcept final;
  private:
    template <class, class, class>
    friend class graph_test_helper;

    void run_tests() final;

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
