////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

export module sequoia.maths.graph:StaticGraphDetails;

import std;

import :Edge;
import :EdgesAndNodesUtilities;
import :GraphDetails;
export import sequoia.core.data_structures;
export import sequoia.core.meta;
export import sequoia.core.object;

/** \file
    \brief Implementation details for static graphs.
 */

export namespace data_structures
{
}

export namespace sequoia::maths::graph_impl
{
  template<std::size_t MaxValue>
  struct narrowest_unsigned_holding
  {
    template<class T>
    using holds = std::bool_constant<(std::numeric_limits<T>::max() >= MaxValue)>;

    using candidates = std::tuple<unsigned char, unsigned short, unsigned int, std::size_t>;

    using type = std::tuple_element_t<meta::find_if_v<candidates, holds>, candidates>;
  };

  template<std::size_t MaxValue>
  using narrowest_unsigned_holding_t = narrowest_unsigned_holding<MaxValue>::type;

  /** \brief The index type shared by a static graph's node indices and its edge-storage offsets.

      A node index runs up to `Order - 1`; a partition offset into the edge storage runs up to `NumEdges`,
      which also bounds an embedded edge's complementary index.
   */
  template<std::size_t Order, std::size_t NumEdges>
  using static_edge_index_type = narrowest_unsigned_holding_t<std::ranges::max(Order ? Order - 1 : Order, NumEdges)>;
}
