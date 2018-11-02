#pragma once

#include "DynamicGraph.hpp"

namespace sequoia::maths
{
  template<class G, class Pred>
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
