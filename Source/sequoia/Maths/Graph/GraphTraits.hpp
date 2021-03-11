////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Traits and Concepts for graphs.

 */

namespace sequoia::maths
{
  template<class N>
  concept network = requires(const N& n) {
    typename N::edge_type;
    typename N::edge_weight_type;
    typename N::edge_index_type;
    typename N::edge_init_type;
    typename N::size_type;

    n.size();
    n.order();
    n.cbegin_edges(0);
    n.cend_edges(0);
  };

  template<class G>
  concept dynamic_nodes =  requires(G& g) {
     g.add_node();
  };

  template<class G>
  concept static_nodes = (!dynamic_nodes<G>);

  template<class G>
  concept dynamic_network = network<G> && dynamic_nodes<G>;

  template<class G>
  concept static_network = network<G> && static_nodes<G>;
}
