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
  concept network = std::equality_comparable<N> && requires(const N& n) {
    typename N::edge_type;
    typename N::edge_weight_type;
    typename N::edge_index_type;
    typename N::edge_init_type;
    typename N::size_type;
    typename N::const_edge_iterator;
    typename N::const_reverse_edge_iterator;

    { n.size() }          -> std::same_as<typename N::size_type>;
    { n.order() }         -> std::same_as<typename N::size_type>;
    { n.cbegin_edges(0) } -> std::same_as<typename N::const_edge_iterator>;
    { n.cend_edges(0) }   -> std::same_as<typename N::const_edge_iterator>;
  };

  // TO DO: replace with constexpr bool once MSVC supports this
  template<class G>
  concept dynamic_nodes =    requires(std::remove_const_t<G>& g) { g.add_node(); } 
                          || requires(std::remove_const_t<G> & g) { g.add_node(0); };

  // TO DO: replace with constexpr bool once MSVC supports this
  template<class G>
  concept static_nodes = (!dynamic_nodes<G>);

  template<class G>
  concept dynamic_network = network<G> && dynamic_nodes<G>;

  template<class G>
  concept static_network = network<G> && static_nodes<G>;

  template<class T>
  concept dynamic_tree = dynamic_network<T> && requires {
    T::link_dir;
  };

}
