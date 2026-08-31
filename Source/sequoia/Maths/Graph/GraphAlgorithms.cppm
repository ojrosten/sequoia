////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

module;

#include "sequoia/PlatformSpecific/Macros.hpp"

#include <algorithm>
#include <array>
#include <concepts>
#include <execution>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <numeric>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

export module sequoia.maths.graph:GraphAlgorithms;

import :Connectivity;
import :DynamicGraph;
import :Edge;
import :EdgesAndNodesUtilities;
import :GraphDetails;
import :GraphErrors;
import :GraphPrimitive;
import :GraphTraits;
import :NodeStorage;
export import sequoia.algorithms;
export import sequoia.core.container_utilities;
export import sequoia.core.data_structures;
export import sequoia.core.meta;
export import sequoia.core.object;
export import sequoia.maths.sequences;
export import sequoia.platform_specific;

/** \file
    \brief A collection of graph algorithms that fall outside the specific groupings.

 */


export namespace sequoia::maths
{
  template<class G, class Pred>
  [[nodiscard]]
  G sub_graph(const G& g, Pred nodePred)
  {
    G subGraph{g};

    std::size_t pos{};
    while(pos < subGraph.order())
    {
      if(!nodePred(*(subGraph.cbegin_node_weights() + pos)))
      {
        subGraph.erase_node(pos);
      }
      else
      {
        ++pos;
      }
    }

    return subGraph;
  }
}
