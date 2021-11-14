////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "sequoia/TestFramework/RegularTestCore.hpp"
#include "sequoia/Maths/Graph/DynamicTree.hpp"

namespace sequoia::testing
{
  template<maths::directed_flavour Directedness, maths::tree_link_direction TreeLinkDir, class EdgeWeight, class NodeWeight, class EdgeWeightCreator, class NodeWeightCreator, class EdgeStorageTraits, class NodeWeightStorageTraits >
  struct detailed_equality_checker<sequoia::maths::tree<Directedness, TreeLinkDir, EdgeWeight, NodeWeight, EdgeWeightCreator, NodeWeightCreator, EdgeStorageTraits, NodeWeightStorageTraits>>
  {
    using type = sequoia::maths::tree<Directedness, TreeLinkDir, EdgeWeight, NodeWeight, EdgeWeightCreator, NodeWeightCreator, EdgeStorageTraits, NodeWeightStorageTraits>;

    template<test_mode Mode>
    static void check(test_logger<Mode>& logger, const type& actual, const type& prediction)
    {
      // e.g. check_equality("Description", logger, actual.method(), prediction.method());
    }
  };

  template<maths::directed_flavour Directedness, maths::tree_link_direction TreeLinkDir, class EdgeWeight, class NodeWeight, class EdgeWeightCreator, class NodeWeightCreator, class EdgeStorageTraits, class NodeWeightStorageTraits>
  struct equivalence_checker<sequoia::maths::tree<Directedness, TreeLinkDir, EdgeWeight, NodeWeight, EdgeWeightCreator, NodeWeightCreator, EdgeStorageTraits, NodeWeightStorageTraits>>
  {
    using type = sequoia::maths::tree<Directedness, TreeLinkDir, EdgeWeight, NodeWeight, EdgeWeightCreator, NodeWeightCreator, EdgeStorageTraits, NodeWeightStorageTraits>;

    template<test_mode Mode>
    static void check(test_logger<Mode>& logger, const type& actual, const maths::tree_initializer<NodeWeight>& prediction)
    {
      // e.g. check_equality("Description", logger, actual.method(), prediction);
    }
  };
}
