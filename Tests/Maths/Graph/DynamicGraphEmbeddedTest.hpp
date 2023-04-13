////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "DynamicGraphTestingUtilities.hpp"

namespace sequoia
{
  namespace testing
  {
    class test_edge_insertion final : public regular_test
    {
    public:
      using regular_test::regular_test;

      [[nodiscard]]
      std::filesystem::path source_file() const noexcept final;
    private:
      template<class, class, class>
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

      template<class Graph>
      void edge_insertions();

      template<class Graph>
      void weighted_edge_insertions();
    };
  }
}
