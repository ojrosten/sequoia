////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file GraphAlgorithms.hpp
    \brief A collection of graph algorithms that fall outside the specific groupings. 

 */

#include "sequoia/Maths/Graph/DynamicGraph.hpp"

namespace sequoia::maths
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
